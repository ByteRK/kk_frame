/*
 * wpa_supplicant/hostapd control interface library
 * Copyright (c) 2004-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#define CONFIG_CTRL_IFACE
#define CONFIG_CTRL_IFACE_UNIX
//#define CONFIG_CTRL_IFACE_UDP
//#define CONFIG_CTRL_IFACE_NAMED_PIPE

#ifdef CONFIG_CTRL_IFACE

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifndef _WIN32_WCE
#ifndef CONFIG_TI_COMPILER
#include <signal.h>
#include <sys/types.h>
#endif /* CONFIG_TI_COMPILER */
#include <errno.h>
#endif /* _WIN32_WCE */
#include <ctype.h>
#include <time.h>
#ifndef CONFIG_TI_COMPILER
#ifndef _MSC_VER
#include <unistd.h>
#endif /* _MSC_VER */
#endif /* CONFIG_TI_COMPILER */

#ifndef CONFIG_NATIVE_WINDOWS
#ifndef CONFIG_TI_COMPILER
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef __vxworks
#include <sys/uio.h>
#include <sys/time.h>
#endif /* __vxworks */
#endif /* CONFIG_TI_COMPILER */
#endif /* CONFIG_NATIVE_WINDOWS */


#ifdef CONFIG_CTRL_IFACE_UNIX
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#endif /* CONFIG_CTRL_IFACE_UNIX */
#ifdef CONFIG_CTRL_IFACE_UDP_REMOTE
#include <netdb.h>
#endif /* CONFIG_CTRL_IFACE_UDP_REMOTE */

#include <dirent.h>

#if defined(ANDROID) && !defined(PURE_LINUX)
#include <cutils/sockets.h>
#include "private/android_filesystem_config.h"
#endif /* ANDROID */

#include "wpa_ctrl.h"

struct wpa_time {
	time_t sec;
	time_t usec;
};


#if defined(CONFIG_CTRL_IFACE_UNIX) || defined(CONFIG_CTRL_IFACE_UDP)
#define CTRL_IFACE_SOCKET
#endif /* CONFIG_CTRL_IFACE_UNIX || CONFIG_CTRL_IFACE_UDP */

const char *gpWorkPath = NULL;
char gWorkPath[256]={0};
/**
 * struct wpa_ctrl - Internal structure for control interface library
 *
 * This structure is used by the wpa_supplicant/hostapd control interface
 * library to store internal data. Programs using the library should not touch
 * this data directly. They can only use the pointer to the data structure as
 * an identifier for the control interface connection and use this as one of
 * the arguments for most of the control interface library functions.
 */
struct wpa_ctrl {
#ifdef CONFIG_CTRL_IFACE_UDP
	int s;
	struct sockaddr_in local;
	struct sockaddr_in dest;
	char *cookie;
	char *remote_ifname;
	char *remote_ip;
#endif /* CONFIG_CTRL_IFACE_UDP */
#ifdef CONFIG_CTRL_IFACE_UNIX
	int s;
	struct sockaddr_un local;
	struct sockaddr_un dest;
#endif /* CONFIG_CTRL_IFACE_UNIX */
#ifdef CONFIG_CTRL_IFACE_NAMED_PIPE
	HANDLE pipe;
#endif /* CONFIG_CTRL_IFACE_NAMED_PIPE */
};


#ifdef CONFIG_CTRL_IFACE_UNIX

#ifndef CONFIG_CTRL_IFACE_CLIENT_DIR
#define CONFIG_CTRL_IFACE_CLIENT_DIR "/tmp"
#endif /* CONFIG_CTRL_IFACE_CLIENT_DIR */

#ifndef CONFIG_CTRL_IFACE_CLIENT_PREFIX
#define CONFIG_CTRL_IFACE_CLIENT_PREFIX "wpa_ctrl_"
#endif /* CONFIG_CTRL_IFACE_CLIENT_PREFIX */

#ifndef PATH_MAX
#define PATH_MAX 266
#endif


int wpa_get_time(struct wpa_time *t)
{
	int res;
	struct timeval tv;
	res = gettimeofday(&tv, NULL);
	t->sec = tv.tv_sec;
	t->usec = tv.tv_usec;
	return res;
}

void wap_sleep(time_t sec, time_t usec)
{
	if (sec)
		sleep(sec);
	if (usec)
		usleep(usec);
}

struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path)
{
	struct wpa_ctrl *ctrl;
	static int counter = 0;
	int ret;
	size_t res;
	int tries = 0;
	int flags;

	ctrl = (struct wpa_ctrl *)(malloc(sizeof(*ctrl)));
	if (ctrl == NULL)
		return NULL;
	memset(ctrl, 0, sizeof(*ctrl));

	ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (ctrl->s < 0) {
		free(ctrl);
		return NULL;
	}

	ctrl->local.sun_family = AF_UNIX;
	counter++;
try_again:
	ret = snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
			  CONFIG_CTRL_IFACE_CLIENT_DIR "/"
			  CONFIG_CTRL_IFACE_CLIENT_PREFIX "%d-%d",
			  (int) getpid(), counter);
	if (ret < 0 || (size_t) ret >= sizeof(ctrl->local.sun_path)) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}
	tries++;
	if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
		    sizeof(ctrl->local)) < 0) {
		if (errno == EADDRINUSE && tries < 2) {
			/*
			 * getpid() returns unique identifier for this instance
			 * of wpa_ctrl, so the existing socket file must have
			 * been left by unclean termination of an earlier run.
			 * Remove the file and try again.
			 */
			unlink(ctrl->local.sun_path);
			goto try_again;
		}
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}

#if defined(ANDROID) && !defined(PURE_LINUX)
	chmod(ctrl->local.sun_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	chown(ctrl->local.sun_path, AID_SYSTEM, AID_WIFI);
	/*
	 * If the ctrl_path isn't an absolute pathname, assume that
	 * it's the name of a socket in the Android reserved namespace.
	 * Otherwise, it's a normal UNIX domain socket appearing in the
	 * filesystem.
	 */
	if (ctrl_path != NULL && *ctrl_path != '/') {
		char buf[21];
		snprintf(buf, sizeof(buf), "wpa_%s", ctrl_path);
		if (socket_local_client_connect(
			    ctrl->s, buf,
			    ANDROID_SOCKET_NAMESPACE_RESERVED,
			    SOCK_DGRAM) < 0) {
			close(ctrl->s);
			unlink(ctrl->local.sun_path);
			free(ctrl);
			return NULL;
		}
		return ctrl;
	}
#endif /* ANDROID */

	ctrl->dest.sun_family = AF_UNIX;
	res = strlen(strncpy(ctrl->dest.sun_path, ctrl_path,
			 sizeof(ctrl->dest.sun_path)));

	if (res >= sizeof(ctrl->dest.sun_path)) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}

	if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
		    sizeof(ctrl->dest)) < 0) {
        perror("conenct");
		close(ctrl->s);
		unlink(ctrl->local.sun_path);
		free(ctrl);
		return NULL;
	}

	/*
	 * Make socket non-blocking so that we don't hang forever if
	 * target dies unexpectedly.
	 */
	flags = fcntl(ctrl->s, F_GETFL);
	if (flags >= 0) {
		flags |= O_NONBLOCK;
		if (fcntl(ctrl->s, F_SETFL, flags) < 0) {
			perror("fcntl(ctrl->s, O_NONBLOCK)");
			/* Not fatal, continue on.*/
		}
	}

	return ctrl;
}


void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
	if (ctrl == NULL)
		return;
	unlink(ctrl->local.sun_path);
	if (ctrl->s >= 0)
		close(ctrl->s);
	free(ctrl);
}

/**
 * wpa_ctrl_cleanup() - Delete any local UNIX domain socket files that
 * may be left over from clients that were previously connected to
 * wpa_supplicant. This keeps these files from being orphaned in the
 * event of crashes that prevented them from being removed as part
 * of the normal orderly shutdown.
 */
void wpa_ctrl_cleanup(void)
{
	DIR *dir;
	struct dirent entry;
	struct dirent *result;
	size_t dirnamelen;
	int prefixlen = strlen(CONFIG_CTRL_IFACE_CLIENT_PREFIX);
	size_t maxcopy;
	char pathname[PATH_MAX];
	char *namep;

	if ((dir = opendir(CONFIG_CTRL_IFACE_CLIENT_DIR)) == NULL)
		return;

	dirnamelen = (size_t) snprintf(pathname, sizeof(pathname), "%s/",
					  CONFIG_CTRL_IFACE_CLIENT_DIR);
	if (dirnamelen >= sizeof(pathname)) {
		closedir(dir);
		return;
	}
	namep = pathname + dirnamelen;
	maxcopy = PATH_MAX - dirnamelen;
	while (readdir_r(dir, &entry, &result) == 0 && result != NULL) {
		if (strncmp(entry.d_name, CONFIG_CTRL_IFACE_CLIENT_PREFIX,
			       prefixlen) == 0) {
			uint16_t len =strlen(strncpy(namep, entry.d_name, maxcopy));
			if (len < maxcopy)
				unlink(pathname);
		}
	}
	closedir(dir);
}

#else /* CONFIG_CTRL_IFACE_UNIX */

#ifdef ANDROID
void wpa_ctrl_cleanup(void)
{
}
#endif /* ANDROID */

#endif /* CONFIG_CTRL_IFACE_UNIX */


#ifdef CONFIG_CTRL_IFACE_UDP

struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path)
{
	struct wpa_ctrl *ctrl;
	char buf[128];
	size_t len;
#ifdef CONFIG_CTRL_IFACE_UDP_REMOTE
	struct hostent *h;
#endif /* CONFIG_CTRL_IFACE_UDP_REMOTE */

	ctrl = malloc(sizeof(*ctrl));
	if (ctrl == NULL)
		return NULL;
	memset(ctrl, 0, sizeof(*ctrl));

	ctrl->s = socket(PF_INET, SOCK_DGRAM, 0);
	if (ctrl->s < 0) {
		perror("socket");
		free(ctrl);
		return NULL;
	}

	ctrl->local.sin_family = AF_INET;
#ifdef CONFIG_CTRL_IFACE_UDP_REMOTE
	ctrl->local.sin_addr.s_addr = INADDR_ANY;
#else /* CONFIG_CTRL_IFACE_UDP_REMOTE */
	ctrl->local.sin_addr.s_addr = htonl((127 << 24) | 1);
#endif /* CONFIG_CTRL_IFACE_UDP_REMOTE */
	if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
		 sizeof(ctrl->local)) < 0) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}

	ctrl->dest.sin_family = AF_INET;
	ctrl->dest.sin_addr.s_addr = htonl((127 << 24) | 1);
	ctrl->dest.sin_port = htons(WPA_CTRL_IFACE_PORT);

#ifdef CONFIG_CTRL_IFACE_UDP_REMOTE
	if (ctrl_path) {
		char *port, *name;
		int port_id;

		name = strdup(ctrl_path);
		if (name == NULL) {
			close(ctrl->s);
			free(ctrl);
			return NULL;
		}
		port = strchr(name, ':');

		if (port) {
			port_id = atoi(&port[1]);
			port[0] = '\0';
		} else
			port_id = WPA_CTRL_IFACE_PORT;

		h = gethostbyname(name);
		ctrl->remote_ip = strdup(name);
		free(name);
		if (h == NULL) {
			perror("gethostbyname");
			close(ctrl->s);
			free(ctrl->remote_ip);
			free(ctrl);
			return NULL;
		}
		ctrl->dest.sin_port = htons(port_id);
		memcpy(h->h_addr, (char *) &ctrl->dest.sin_addr.s_addr,
			  h->h_length);
	} else
		ctrl->remote_ip = strdup("localhost");
#endif /* CONFIG_CTRL_IFACE_UDP_REMOTE */

	if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
		    sizeof(ctrl->dest)) < 0) {
		perror("connect");
		close(ctrl->s);
		free(ctrl->remote_ip);
		free(ctrl);
		return NULL;
	}

	len = sizeof(buf) - 1;
	if (wpa_ctrl_request(ctrl, "GET_COOKIE", 10, buf, &len, NULL) == 0) {
		buf[len] = '\0';
		ctrl->cookie = strdup(buf);
	}

	if (wpa_ctrl_request(ctrl, "IFNAME", 6, buf, &len, NULL) == 0) {
		buf[len] = '\0';
		ctrl->remote_ifname = strdup(buf);
	}

	return ctrl;
}


char * wpa_ctrl_get_remote_ifname(struct wpa_ctrl *ctrl)
{
#define WPA_CTRL_MAX_PS_NAME 100
	static char ps[WPA_CTRL_MAX_PS_NAME] = {};
	snprintf(ps, WPA_CTRL_MAX_PS_NAME, "%s/%s",
		    ctrl->remote_ip, ctrl->remote_ifname);
	return ps;
}


void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
	close(ctrl->s);
	free(ctrl->cookie);
	free(ctrl->remote_ifname);
	free(ctrl->remote_ip);
	free(ctrl);
}

#endif /* CONFIG_CTRL_IFACE_UDP */


#ifdef CTRL_IFACE_SOCKET
int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
		     char *reply, size_t *reply_len,
		     void (*msg_cb)(char *msg, size_t len))
{
	struct timeval tv;
	struct wpa_time started_at;
	int res;
	fd_set rfds;
	const char *_cmd;
	char *cmd_buf = NULL;
	size_t _cmd_len;

#ifdef CONFIG_CTRL_IFACE_UDP
	if (ctrl->cookie) {
		char *pos;
		_cmd_len = strlen(ctrl->cookie) + 1 + cmd_len;
		cmd_buf = malloc(_cmd_len);
		if (cmd_buf == NULL)
			return -1;
		_cmd = cmd_buf;
		pos = cmd_buf;
		strncpy(pos, ctrl->cookie, _cmd_len);
		pos += strlen(ctrl->cookie);
		*pos++ = ' ';
		memcpy(pos, cmd, cmd_len);
	} else
#endif /* CONFIG_CTRL_IFACE_UDP */
	{
		_cmd = cmd;
		_cmd_len = cmd_len;
	}

	errno = 0;
	started_at.sec = 0;
	started_at.usec = 0;
retry_send:
	if (send(ctrl->s, _cmd, _cmd_len, 0) < 0) {
		if (errno == EAGAIN || errno == EBUSY || errno == EWOULDBLOCK)
		{
			/*
			 * Must be a non-blocking socket... Try for a bit
			 * longer before giving up.
			 */
			if (started_at.sec == 0)
				wpa_get_time(&started_at);
			else {
				struct wpa_time n;
				wpa_get_time(&n);
				/* Try for a few seconds. */
				if (n.sec > started_at.sec + 5)
					goto send_err;
			}
			wap_sleep(1, 0);
			goto retry_send;
		}
	send_err:
		free(cmd_buf);
		return -1;
	}
	free(cmd_buf);

	for (;;) {
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(ctrl->s, &rfds);
		res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
		if (res < 0)
			return res;
		if (FD_ISSET(ctrl->s, &rfds)) {
			res = recv(ctrl->s, reply, *reply_len, 0);
			if (res < 0)
				return res;
			if (res > 0 && reply[0] == '<') {
				/* This is an unsolicited message from
				 * wpa_supplicant, not the reply to the
				 * request. Use msg_cb to report this to the
				 * caller. */
				if (msg_cb) {
					/* Make sure the message is nul
					 * terminated. */
					if ((size_t) res == *reply_len)
						res = (*reply_len) - 1;
					reply[res] = '\0';
					msg_cb(reply, res);
				}
				continue;
			}
			*reply_len = res;
			break;
		} else {
			return -2;
		}
	}
	return 0;
}
#endif /* CTRL_IFACE_SOCKET */


static int wpa_ctrl_attach_helper(struct wpa_ctrl *ctrl, int attach)
{
	char buf[10];
	int ret;
	size_t len = 10;

	ret = wpa_ctrl_request(ctrl, attach ? "ATTACH" : "DETACH", 6,
			       buf, &len, NULL);
	if (ret < 0)
		return ret;
	if (len == 3 && memcmp(buf, "OK\n", 3) == 0)
		return 0;
	return -1;
}


int wpa_ctrl_attach(struct wpa_ctrl *ctrl)
{
	return wpa_ctrl_attach_helper(ctrl, 1);
}


int wpa_ctrl_detach(struct wpa_ctrl *ctrl)
{
	return wpa_ctrl_attach_helper(ctrl, 0);
}


#ifdef CTRL_IFACE_SOCKET

int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
	int res;

	res = recv(ctrl->s, reply, *reply_len, 0);
	if (res < 0)
		return res;
	*reply_len = res;
	return 0;
}


int wpa_ctrl_pending(struct wpa_ctrl *ctrl)
{
	struct timeval tv;
	fd_set rfds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(ctrl->s, &rfds);
	select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
	return FD_ISSET(ctrl->s, &rfds);
}


int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl)
{
	return ctrl->s;
}

#endif /* CTRL_IFACE_SOCKET */


#ifdef CONFIG_CTRL_IFACE_NAMED_PIPE

#ifndef WPA_SUPPLICANT_NAMED_PIPE
#define WPA_SUPPLICANT_NAMED_PIPE "WpaSupplicant"
#endif
#define NAMED_PIPE_PREFIX TEXT("\\\\.\\pipe\\") TEXT(WPA_SUPPLICANT_NAMED_PIPE)

struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path)
{
	struct wpa_ctrl *ctrl;
	DWORD mode;
	TCHAR name[256];
	int i, ret;

	ctrl = malloc(sizeof(*ctrl));
	if (ctrl == NULL)
		return NULL;
	memset(ctrl, 0, sizeof(*ctrl));

#ifdef UNICODE
	if (ctrl_path == NULL)
		ret = _snwprintf(name, 256, NAMED_PIPE_PREFIX);
	else
		ret = _snwprintf(name, 256, NAMED_PIPE_PREFIX TEXT("-%S"),
				 ctrl_path);
#else /* UNICODE */
	if (ctrl_path == NULL)
		ret = snprintf(name, 256, NAMED_PIPE_PREFIX);
	else
		ret = snprintf(name, 256, NAMED_PIPE_PREFIX "-%s",
				  ctrl_path);
#endif /* UNICODE */
	if (ret < 0 || ret >= 256) {
		free(ctrl);
		return NULL;
	}

	for (i = 0; i < 10; i++) {
		ctrl->pipe = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0,
					NULL, OPEN_EXISTING, 0, NULL);
		/*
		 * Current named pipe server side in wpa_supplicant is
		 * re-opening the pipe for new clients only after the previous
		 * one is taken into use. This leaves a small window for race
		 * conditions when two connections are being opened at almost
		 * the same time. Retry if that was the case.
		 */
		if (ctrl->pipe != INVALID_HANDLE_VALUE ||
		    GetLastError() != ERROR_PIPE_BUSY)
			break;
		WaitNamedPipe(name, 1000);
	}
	if (ctrl->pipe == INVALID_HANDLE_VALUE) {
		free(ctrl);
		return NULL;
	}

	mode = PIPE_READMODE_MESSAGE;
	if (!SetNamedPipeHandleState(ctrl->pipe, &mode, NULL, NULL)) {
		CloseHandle(ctrl->pipe);
		free(ctrl);
		return NULL;
	}

	return ctrl;
}


void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
	CloseHandle(ctrl->pipe);
	free(ctrl);
}


int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
		     char *reply, size_t *reply_len,
		     void (*msg_cb)(char *msg, size_t len))
{
	DWORD written;
	DWORD readlen = *reply_len;

	if (!WriteFile(ctrl->pipe, cmd, cmd_len, &written, NULL))
		return -1;

	if (!ReadFile(ctrl->pipe, reply, *reply_len, &readlen, NULL))
		return -1;
	*reply_len = readlen;

	return 0;
}


int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
	DWORD len = *reply_len;
	if (!ReadFile(ctrl->pipe, reply, *reply_len, &len, NULL))
		return -1;
	*reply_len = len;
	return 0;
}


int wpa_ctrl_pending(struct wpa_ctrl *ctrl)
{
	DWORD left;

	if (!PeekNamedPipe(ctrl->pipe, NULL, 0, NULL, &left, NULL))
		return -1;
	return left ? 1 : 0;
}


int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl)
{
	return -1;
}

#endif /* CONFIG_CTRL_IFACE_NAMED_PIPE */


/**
 * 执行一条命令
 * @cmd: 需要执行命令
 * @result: 用于存储命令执行后返回的数据
 * @len: 传入result缓冲区大小, 传出命令执行后实际存储到result的数据大小
 * @return: 0-成功， 其他失败
 * 可以执行哪些命令参考: wpa_supplicant源码包中 wpa_supplicant-2.9/wpa_supplicant/wpa_cli.c: wpa_cli_commands[]
 * 常用的命令有:
 *  "STATUS"
 *  "STATUS-DRIVER"
 *  "STATUS-VERBOSE"
 *  "SIGNAL_POLL"
 *  ""
 *
**/
int wpa_ctrl_run_cmd(const char *cmd, char *result, unsigned int *len)
{
	/*
	 * ctrl_path对应的文件路径必须在 /etc/wpa_supplicant.conf文件中的ctrl_interface指定的路径下的文件
	 *
	 *   # ls -l /var/run/wpa_supplicant/wlan0
     *   srwxrwx---    1 root     root             0 Oct 21 09:47 /var/run/wpa_supplicant/wlan0
	 */
    struct wpa_ctrl *wpa_ctrl=NULL;
    char path[256]={0};

	if(gpWorkPath)
	{
		snprintf(path, sizeof(path), "%s/wpa_supplicant/wlan0", gpWorkPath);
	}
	else
	{
        snprintf(path, sizeof(path), "/tmp/wpa_supplicant/wlan0");
	}

    wpa_ctrl = wpa_ctrl_open(path);
    if (!wpa_ctrl)
    {
        printf("wpa_ctrl_open %s failed\n", path);
        return -1;
    }

	size_t replay_len = *len;
	int ret = wpa_ctrl_request(wpa_ctrl, cmd, strlen(cmd), result, &replay_len, 0);
	*len = replay_len;
	result[*len] = 0;

	wpa_ctrl_close(wpa_ctrl);

	return ret;
}



static int result_get(char *str, char *key, char *val, int val_len)
{
	char *s;

	if (!(s = strstr(str, key))) {
		return -1;
	}

	if (!(s = strchr(s, '='))) {
		return -1;
	}

	s++;

	while (*s != '\n' && *s != '\0' && val_len > 1) {
		*val++ = *s++;
		val_len--;
	}
	*val ='\0';
	return 0;

}


/// @brief 获取wifi ap列表
/// @return 获取附近是否存在被链接的wifi
int wpa_ctrl_sta_get_near_ap_list(const char *apName,int *rssi)
{
	int ret  = 0;
	char  ack[1024];
	unsigned int len = sizeof(ack);
	char *scan_cmd = "SCAN";

	ret = wpa_ctrl_run_cmd(scan_cmd, ack, &len);
    if(!(ret == 0 && len > 0))
    {
        printf("Err: %s\r\n",scan_cmd);
        ret = -1;
        goto ERROR;
    }


	if (strncmp(ack, "OK", 2) != 0) {
		printf("Err: %s (FAIL)\n", scan_cmd);
		goto ERROR;
	}

	len = sizeof(ack);
	char *get_scan_cmd = "SCAN_RESULTS";
	ret = wpa_ctrl_run_cmd(get_scan_cmd,ack,&len);
    if(!(ret == 0 && len > 0))
    {
        printf("Err: %s\r\n",scan_cmd);
        ret = -1;
        goto ERROR;
    }


	// bssid / frequency / signal level / flags / ssid \n
	// 54:ef:33:34:95:91       2422    -23     [WPA2-PSK+PSK-SHA256-CCMP-preauth][ESS] HSNVR1969319678
	char *ptr = strtok(ack,"\r\n");
	while(ptr)
	{
		ptr = strtok(NULL,"\r\n");
		//LS_TRACE_DEBUG("ptr:%s(%s)",ptr,ApName);
		if(ptr && strstr(ptr,apName))
		{
			ret = 1;
			break;
		}
	}
ERROR:
	return ret;
}

/*
 * 查询WIFI已连接AP的信号状态
 * @result: 查询结果
 * @ret: 0: 成功, -1: 失败
 */
int wpa_ctrl_sta_signal_poll(struct wifi_signal_poll_result_t *result)
{
	int ret;
	char val[512];
	char *ack = result->ack;
	unsigned int len = sizeof(result->ack);
	char *cmd = "SIGNAL_POLL";

	memset(result, 0x00, sizeof(struct wifi_signal_poll_result_t));

	ret = wpa_ctrl_run_cmd(cmd, ack, &len);
	if (ret < 0 || len == 0) {
		ret = -1;
		printf("Err: %s\r\n", cmd);
		goto ERROR;
	}

	if (strncmp(ack, "FAIL", 4) == 0) {
		printf("Err: %s (FAIL)\r\n", cmd);
		goto ERROR;
	}


	if (result_get(ack, "RSSI", val, sizeof(val)) == 0) {
		result->rssi = strtol(val, NULL, 10);
	}

	if (result_get(ack, "LINKSPEED", val, sizeof(val)) == 0) {
		result->linkspeed = strtol(val, NULL, 10);
	}

	if (result_get(ack, "FREQUENCY", val, sizeof(val)) == 0) {
		result->frequency = strtol(val, NULL, 10);
	}

	if (result_get(ack, "NOISE", val, sizeof(val)) == 0) {
		result->noise = strtol(val, NULL, 10);
	}
ERROR:
	return ret;
}



/// @brief 查询WIFi当前工作状态信息
/// @param result 查询结果
/// @return  查询成功, -1: 查询失败
int wpa_ctrl_sta_status(struct wifi_status_result_t *result)
{
	int ret;
	char val[512];
	char *ack = result->ack;
	unsigned int len = sizeof(result->ack);
	char *cmd = "STATUS";

	memset(result, 0x00, sizeof(struct wifi_status_result_t));

	ret = wpa_ctrl_run_cmd(cmd, ack, &len);
	if (ret < 0 || len == 0) {
		ret = -1;
		printf("Err: %s\r\n", cmd);
		goto ERROR;
	}

	if (strncmp(ack, "FAIL", 4) == 0) {
        ret = -1;
		printf("Err: %s (FAIL)\r\n", cmd);
		goto ERROR;
	}

	result_get(ack, "\nbssid", result->bssid, sizeof(result->bssid));

	if (result_get(ack, "freq", val, sizeof(val)) == 0) {
		result->freq = strtol(val, NULL, 10);
	}

	result_get(ack, "\nssid", result->ssid, sizeof(result->ssid));

	if (result_get(ack, "id", val, sizeof(val)) == 0) {
		result->id = strtol(val, NULL, 10);
	}

	result_get(ack, "mode", result->mode, sizeof(result->mode));
	result_get(ack, "pairwise_cipher", result->pairwise_cipher, sizeof(result->pairwise_cipher));
	result_get(ack, "group_cipher", result->group_cipher, sizeof(result->group_cipher));
	result_get(ack, "key_mgmt", result->key_mgmt, sizeof(result->key_mgmt));
	result_get(ack, "wpa_state", result->wpa_state, sizeof(result->wpa_state));
	result_get(ack, "ip_address", result->ip_address, sizeof(result->ip_address));
	result_get(ack, "address", result->address, sizeof(result->address));
	result_get(ack, "uuid", result->uuid, sizeof(result->uuid));

ERROR:
	return ret;
}


static int wpa_ctrl_RSSI = 0;


/*
 * 查询WIFI已连接AP的信号状态
 * @result: 查询结果,查询实际得状态
 * @ret: 信号值
 */
int wpa_ctrl_sta_signal_rssi()
{
	static unsigned int time;
	struct timespec stTime;
	clock_gettime(CLOCK_MONOTONIC,&stTime);
	if(stTime.tv_sec - time > 2 )
	{
		time = stTime.tv_sec;
		struct wifi_signal_poll_result_t result;
		wpa_ctrl_sta_signal_poll(&result);
		wpa_ctrl_RSSI=result.rssi;
		return result.rssi;
	}
	return wpa_ctrl_RSSI;
}


/*
 * 查询WIFI已连接AP的信号状态
 * @result: 查询结果,获取得缓存
 * @ret: 信号值
 */
int wpa_ctrl_sta_signal_rssi_fast()
{
	return wpa_ctrl_RSSI;
}


/// @brief 设置wifi运行路径
/// @param path
void wpa_ctrl_run_set_path(const char *path)
{
    if(path)
    {
        snprintf(gWorkPath, sizeof(gWorkPath), "%s", path);
        gpWorkPath = gWorkPath;
        printf("work path:%s\n", gpWorkPath);
    }
}

#endif /* CONFIG_CTRL_IFACE */
