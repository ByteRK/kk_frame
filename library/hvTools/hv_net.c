#include "hv_net.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <net/route.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#define RESOLV_CONF "/etc/resolv.conf"
#define PROC_NET_ROUTE_PATH "/proc/net/route"
#define MULTICAST_ROUTING "224.0.0.0"

typedef union _NET_IPV4_U {
    unsigned int  int32;
    unsigned char u8Str[4];
} NET_IPV4_U;

int HV_NET_GetMacAddr(const char *pIface, char *pMac1, char *pMac2) {
    int           s32SockFd;
    unsigned char acTemp[IFHWADDRLEN+1] = {0};
    struct ifreq  stIfreq;

    if (NULL == pIface) { return -1; }

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error\n");
        return -1;
    }

    strncpy(stIfreq.ifr_name, pIface, IFNAMSIZ);

    if (ioctl(s32SockFd, SIOCGIFHWADDR, &stIfreq) < 0) {
        printf("%s: ioctl SIOCGIFHWADDR failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }
    close(s32SockFd);

    if (NULL != pMac1) { memcpy(pMac1, stIfreq.ifr_ifru.ifru_hwaddr.sa_data, IFHWADDRLEN); }

    if (NULL != pMac2) {
        memcpy(acTemp, stIfreq.ifr_ifru.ifru_hwaddr.sa_data, IFHWADDRLEN);

        sprintf(pMac2, "%02X:%02X:%02X:%02X:%02X:%02X", acTemp[0], acTemp[1], acTemp[2], acTemp[3], acTemp[4],
                acTemp[5]);
    }

    return 0;
}

int HV_NET_SetMacAddr(const char *pIface, char *pMac) {

    return 0;
}

int HV_NET_GetIpAddr(const char *pIface, char *pIP) {
    int                s32SockFd;
    struct sockaddr_in stSockAddr;
    struct ifreq       stIfreq;

    if (NULL == pIface || NULL == pIP) { return -1; }

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error. %s\n", pIface);
        return -1;
    }

    strncpy(stIfreq.ifr_name, pIface, IFNAMSIZ);
    stIfreq.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(s32SockFd, SIOCGIFADDR, &stIfreq) < 0) {
        printf("ioctl SIOCGIFADDR failed. %s\n", pIface);
        close(s32SockFd);
        return -1;
    }

    memcpy(&stSockAddr, &stIfreq.ifr_addr, sizeof(struct sockaddr_in));
    strcpy(pIP, inet_ntoa(stSockAddr.sin_addr));

    close(s32SockFd);

    return 0;
}

int HV_NET_SetIpAddr(const char *pIface, char *pIP) {
    int                s32SockFd;
    struct ifreq       stIfreq;
    struct sockaddr_in stSockAddr;

    if (NULL == pIface || NULL == pIP) { return -1; }

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error\n");
        return -1;
    }

    stSockAddr.sin_family = AF_INET;
    inet_aton(pIP, &stSockAddr.sin_addr);
    strncpy(stIfreq.ifr_name, pIface, IFNAMSIZ);
    memcpy((char *)&stIfreq.ifr_addr, (char *)&stSockAddr, sizeof(struct sockaddr_in));

    if (ioctl(s32SockFd, SIOCSIFADDR, &stIfreq) < 0) {
        printf("%s: ioctl SIOCSIFADDR failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }
    close(s32SockFd);

    return 0;
}

int HV_NET_GetNetmask(const char *pIface, char *pNetmask) {
    int                s32SockFd;
    struct ifreq       stIfreq;
    struct sockaddr_in stSockAddr;

    if (NULL == pIface || NULL == pNetmask) { return -1; }

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error\n");
        return -1;
    }

    strncpy(stIfreq.ifr_name, pIface, IFNAMSIZ);
    stIfreq.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(s32SockFd, SIOCGIFNETMASK, &stIfreq) < 0) {
        printf("%s: ioctl SIOCGIFNETMASK failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }

    memcpy(&stSockAddr, &stIfreq.ifr_addr, sizeof(stSockAddr));
    strcpy(pNetmask, inet_ntoa(stSockAddr.sin_addr));

    close(s32SockFd);

    return 0;
}

int HV_NET_SetNetmask(const char *pIface, char *pNetmask) {
    int                s32SockFd;
    struct ifreq       stIfreq;
    struct sockaddr_in stSockAddr;

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error\n");
        return -1;
    }

    stSockAddr.sin_family = AF_INET;
    inet_aton(pNetmask, &stSockAddr.sin_addr);
    strncpy(stIfreq.ifr_name, pIface, IFNAMSIZ);
    memcpy((char *)&stIfreq.ifr_addr, (char *)&stSockAddr, sizeof(struct sockaddr_in));

    if (ioctl(s32SockFd, SIOCSIFNETMASK, &stIfreq) < 0) {
        printf("%s: ioctl SIOCSIFNETMASK failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }
    close(s32SockFd);

    return 0;
}

int HV_NET_SearchGateway(const char *pIface, char *pBuf, char *pcGateway) {
    char          acIface[16];
    unsigned long u32Destination;
    unsigned long u32Gateway;
    int           s32Flags;

    sscanf(pBuf, "%s\t%08lX\t%08lX\t%8X\t", acIface, &u32Destination, &u32Gateway, &s32Flags);

    if (strncasecmp(acIface, pIface, strlen(pIface)) != 0) {
        // fprintf(stderr, "iface = %s, ifname = %s\n", acIface, pIface);
        return -1;
    }

    if ((s32Flags & (RTF_UP | RTF_GATEWAY)) == (RTF_UP | RTF_GATEWAY)) {
        NET_IPV4_U unGateway;
        unGateway.int32 = u32Gateway;
        sprintf(pcGateway, "%d.%d.%d.%d", unGateway.u8Str[0], unGateway.u8Str[1], unGateway.u8Str[2],
                unGateway.u8Str[3]);
        return 0;
    }

    return -1;
}

int HV_NET_DeleteGateway(const char *pIface, char *pcGateway) {
    int                s32SockFd;
    struct rtentry     stRtentry;
    struct sockaddr_in stSockAddr;

    /* Clean out the RTREQ structure. */
    memset((char *)&stRtentry, 0, sizeof(struct rtentry));

    /* Fill in the other fields. */
    char szIface[64] = {0};
    strcpy(szIface, pIface);
    stRtentry.rt_dev               = szIface;
    stRtentry.rt_flags             = (RTF_UP | RTF_GATEWAY);
    stRtentry.rt_dst.sa_family     = PF_INET;
    stRtentry.rt_genmask.sa_family = PF_INET;

    stSockAddr.sin_family = AF_INET;
    inet_aton(pcGateway, &stSockAddr.sin_addr);
    memcpy((char *)&stRtentry.rt_gateway, (char *)&stSockAddr, sizeof(struct sockaddr_in));

    /* Create a socket to the INET kernel. */
    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error");
        return -1;
    }

    /* Tell the kernel to accept this route. */
    if (ioctl(s32SockFd, SIOCDELRT, &stRtentry) < 0) {
        printf("%s: ioctl SIOCDELRT failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }

    close(s32SockFd);

    return (0);
}

int HV_NET_AddGateway(const char *pIface, char *pcGateway) {
    int                s32SockFd;
    struct rtentry     stRtentry;
    struct sockaddr_in stSockAddr;

    /* Clean out the RTREQ structure. */
    memset((char *)&stRtentry, 0, sizeof(struct rtentry));

    /* Fill in the other fields. */
    char szIface[64] = {0};
    strcpy(szIface, pIface);
    stRtentry.rt_dev               = szIface;
    stRtentry.rt_flags             = (RTF_UP | RTF_GATEWAY);
    stRtentry.rt_dst.sa_family     = PF_INET;
    stRtentry.rt_genmask.sa_family = PF_INET;

    stSockAddr.sin_family = AF_INET;
    inet_aton(pcGateway, &stSockAddr.sin_addr);
    memcpy((char *)&stRtentry.rt_gateway, (char *)&stSockAddr, sizeof(struct sockaddr_in));

    /* Create a socket to the INET kernel. */
    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error");
        return -1;
    }

    /* Tell the kernel to accept this route. */
    if (ioctl(s32SockFd, SIOCADDRT, &stRtentry) < 0) {
        printf("%s: ioctl SIOCADDRT failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }

    /* Close the socket. */
    close(s32SockFd);

    return 0;
}

int HV_NET_GetGateway(const char *pIface, char *pcGateway, int size) {
    char acBuff[132] = {0};
    char acTemp[16]  = {0};

    if (NULL == pIface || NULL == pcGateway) { return -1; }

    FILE *pFile = fopen(PROC_NET_ROUTE_PATH, "r");
    if (!pFile) { return -1; }

    fgets(acBuff, 130, pFile);
    while (fgets(acBuff, 130, pFile) != NULL) {
        if (HV_NET_SearchGateway(pIface, acBuff, acTemp) == 0) {
            fclose(pFile);
            pFile = NULL;
            strncpy(pcGateway, acTemp, size - 1);
            return 0;
        }
    }

    fclose(pFile);
    pFile = NULL;

    return -1;
}

int HV_NET_SetGateway(const char *pIface, char *pcGateway) {
    char acBuff[132] = {0};
    char acAddr[16]  = {0};

    if (NULL == pIface || NULL == pcGateway) { return -1; }

    FILE *pFile = fopen(PROC_NET_ROUTE_PATH, "r");
    if (NULL == pFile) { return -1; }

    fgets(acBuff, 130, pFile);
    while (fgets(acBuff, 130, pFile) != NULL) {
        if (HV_NET_SearchGateway(pIface, acBuff, acAddr) == 0) { HV_NET_DeleteGateway(pIface, acAddr); }
    }

    fclose(pFile);
    pFile = NULL;

    HV_NET_AddGateway(pIface, pcGateway);

    return 0;
}

int HV_NET_GetDNS(char *pcDNS) {
    FILE *pFile = NULL;

    pFile = fopen(RESOLV_CONF, "r");
    if (NULL != pFile) {
        if (fscanf(pFile, "nameserver %s\n", pcDNS) != EOF) {
            fclose(pFile);
            pFile = NULL;
        }
    }

    if (NULL != pFile) {
        fclose(pFile);
        pFile = NULL;
    }

    return 0;
}

int HV_NET_SetDNS(char *pcDNS) {
    char  acBuf[128] = {0};
    FILE *pFile      = NULL;

    pFile = fopen(RESOLV_CONF, "w");
    if (NULL != pFile) {
        memset(acBuf, 0, sizeof(acBuf));
        sprintf(acBuf,
                "nameserver %s\n"
                "nameserver %s\n",
                pcDNS, "8.8.8.8");

        if (fwrite(acBuf, 1, strlen(acBuf), pFile) <= 0) {
            perror("fwrite");
            fflush(pFile);
            fclose(pFile);
            pFile = NULL;
            return -1;
        }

        fflush(pFile);
        fclose(pFile);

        return 0;
    }

    printf("%s: file \"%s\" opened for writing error!\n", __FUNCTION__, RESOLV_CONF);

    return -1;
}

int HV_NET_GetDNS_ex(char *pcDNS1, char *pcDNS2) {
    char  acBuf[128] = {0};
    char *pcTemp     = NULL;
    char *pcMark     = NULL;
    FILE *pFile      = NULL;

    pFile = fopen(RESOLV_CONF, "r");
    if (NULL == pFile) { return -1; }

    if (fgets(acBuf, 128, pFile) != NULL) {
        pcTemp = strstr(acBuf, "nameserver");
        if (NULL != pcTemp) {
            pcMark = strchr(pcTemp, '\n');
            if (NULL != pcMark) {
                strncpy(pcDNS1, pcTemp + strlen("nameserver") + 1, pcMark - (pcTemp + strlen("nameserver") + 1));
            }
        }
    }

    if (fgets(acBuf, 128, pFile) != NULL) {
        pcTemp = strstr(acBuf, "nameserver");
        if (NULL != pcTemp) {
            pcMark = strchr(pcTemp, '\n');
            if (NULL != pcMark) {
                strncpy(pcDNS2, pcTemp + strlen("nameserver") + 1, pcMark - (pcTemp + strlen("nameserver") + 1));
            }
        }
    }

    if (NULL != pFile) {
        fclose(pFile);
        pFile = NULL;
    }

    return 0;
}

int HV_NET_SetDNS_ex(char *pcDomainDNS, char *pcWifiDNS) {
    char  acBuf[128] = {0};
    FILE *pFile      = NULL;

    pFile = fopen(RESOLV_CONF, "w");
    if (NULL != pFile) {
        memset(acBuf, 0, sizeof(acBuf));
        sprintf(acBuf, "nameserver %s\nnameserver %s\n", pcDomainDNS, pcWifiDNS);
        printf("%s: acBuf %s\n", __FUNCTION__, acBuf);
        printf("%s: dns1 %s, dns2 %s\n", __FUNCTION__, pcDomainDNS, pcWifiDNS);

        if (fwrite(acBuf, 1, strlen(acBuf), pFile) <= 0) {
            perror("fwrite");
            fflush(pFile);
            fclose(pFile);
            pFile = NULL;
            return -1;
        }

        fflush(pFile);
        fclose(pFile);

        return 0;
    }

    printf("%s: file \"%s\" opened for writing error!\n", __FUNCTION__, RESOLV_CONF);

    return -1;
}

int HV_NET_GetBrocastAddr(const char *pIface, char *pAddr) {
    int                s32SockFd;
    struct sockaddr_in stSockAddr;
    struct ifreq       stIfreq;

    if (NULL == pIface || NULL == pAddr) { return -1; }

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error\n");
        return -1;
    }

    strncpy(stIfreq.ifr_name, pIface, IFNAMSIZ);
    stIfreq.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(s32SockFd, SIOCGIFBRDADDR, &stIfreq) < 0) {
        printf("%s: ioctl SIOCGIFBRDADDR failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }

    memcpy(&stSockAddr, &stIfreq.ifr_addr, sizeof(struct sockaddr_in));
    strcpy(pAddr, inet_ntoa(stSockAddr.sin_addr));

    close(s32SockFd);

    return 0;
}

int HV_NET_GetPptpAddr(const char *pIface, char *pAddr) {
    int                s32SockFd;
    struct sockaddr_in stSockAddr;
    struct ifreq       stIfreq;

    if (NULL == pIface || NULL == pAddr) { return -1; }

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error\n");
        return -1;
    }

    strncpy(stIfreq.ifr_name, pIface, IFNAMSIZ);
    stIfreq.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(s32SockFd, SIOCGIFDSTADDR, &stIfreq) < 0) {
        printf("%s: ioctl SIOCGIFDSTADDR failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }

    memcpy(&stSockAddr, &stIfreq.ifr_addr, sizeof(struct sockaddr_in));
    strcpy(pAddr, inet_ntoa(stSockAddr.sin_addr));

    close(s32SockFd);

    return 0;
}

int HV_NET_SearchRoutingTable(const char *pIface, char *pBuf, in_addr_t *pstAddr) {
    char          acIface[16];
    unsigned long u32Destination;
    unsigned long u32Gateway;
    int           s32Flags;

    sscanf(pBuf, "%s\t%08lX\t%08lX\t%8X\t", acIface, &u32Destination, &u32Gateway, &s32Flags);

    if (strncasecmp(acIface, pIface, strlen(pIface)) != 0) {
        fprintf(stderr, "iface = %s,  ifname = %s\n", acIface, pIface);
        return -1;
    }

    *pstAddr = u32Destination;

    return 0;
}

int HV_NET_DetectRoutingAddr(const char *pIface, char *pRouting) {
    char      acBuff[132]       = {0};
    char      acRoutingAddr[32] = {0};
    in_addr_t stAddr;

    if (NULL == pIface) { return -1; }

    FILE *pFp = fopen(PROC_NET_ROUTE_PATH, "r");
    if (!pFp) { return -1; }

    fgets(acBuff, 130, pFp);
    while (fgets(acBuff, 130, pFp) != NULL) {
        if (0 == HV_NET_SearchRoutingTable(pIface, acBuff, &stAddr)) {
            NET_IPV4_U unIpv4Addr;
            unIpv4Addr.int32 = stAddr;
            sprintf(acRoutingAddr, "%d.%d.%d.%d", unIpv4Addr.u8Str[0], unIpv4Addr.u8Str[1], unIpv4Addr.u8Str[2],
                    unIpv4Addr.u8Str[3]);
            if (strstr(acRoutingAddr, pRouting)) {
                fclose(pFp);
                pFp = NULL;
                printf("[%s - %d] found %s routing addr : %s\n", __FUNCTION__, __LINE__, pIface, acRoutingAddr);
                return 0;
            }
        }
    }

    fclose(pFp);
    pFp = NULL;

    return -1;
}

int HV_NET_GetLinkStatus(const char *pIface) {
    int                  s32SockFd;
    struct ifreq         stIfreq;
    struct ethtool_value stEthData;

    stEthData.cmd  = ETHTOOL_GLINK;
    stEthData.data = 0;

    memset(&stIfreq, 0, sizeof(stIfreq));
    strncpy(stIfreq.ifr_name, pIface, sizeof(stIfreq.ifr_name) - 1);
    stIfreq.ifr_data = (char *)&stEthData;

    s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32SockFd < 0) {
        printf("socket error\n");
        return -1;
    }

    if (ioctl(s32SockFd, SIOCETHTOOL, &stIfreq) == -1) {
        printf("%s: ioctl SIOCETHTOOL failed\n", __FUNCTION__);
        close(s32SockFd);
        return -1;
    }

    close(s32SockFd);
    printf("%s LINK %s\n", pIface, stEthData.data > 0 ? "up" : "down");

    return stEthData.data;
}

int HV_NET_CheckNetStatus(int *ms) {
    int                fd;
    int                in_len = 0;
    struct timeval     tv_last, tv_now;
    struct sockaddr_in servaddr;
    // char buf[128];

    in_len = sizeof(struct sockaddr_in);
    fd     = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    // 设置套接字为非阻塞模式
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("223.6.6.6");
    server_addr.sin_port        = htons(53);

    gettimeofday(&tv_last, NULL);

    // 发起连接
    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno != EINPROGRESS) {
            // perror("connect error");
            close(fd);
            return 0;
        }
    }

    // 使用 select 监视套接字是否可写
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    struct timeval timeout;
    timeout.tv_sec  = 2;
    timeout.tv_usec = 0;

    int ready = select(fd + 1, NULL, &writefds, NULL, &timeout);
    if (ready == -1) {
        // perror("select error");
        close(fd);
        return 0;
    } else if (ready == 0) {
        // printf("Connection timeout\n");
        close(fd);
        return 0;
    }

    if (FD_ISSET(fd, &writefds)) {
        int       error = 0;
        socklen_t len   = sizeof(error);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
        if (error != 0) {
            // fprintf(stderr, "Connection error: %s\n", strerror(error));
            close(fd);
            return 0;
        }
    } else {
        // printf("Connection failed\n");
        close(fd);
        return 0;
    }

    // printf("Connection established\n");
    gettimeofday(&tv_now, NULL);
    if (ms) *ms = (tv_now.tv_sec - tv_last.tv_sec) * 1000 + (tv_now.tv_usec - tv_last.tv_usec) / 1000;

    close(fd);
    return 1;
}

/// @brief 网卡接口状态
/// @param nic_name
/// @return
int HV_NET_CheckNicStatus(const char *nic_name) {
    struct ifreq ifr;

    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == skfd) {
        printf("socket error\n");
        return -1;
    }
    strcpy(ifr.ifr_name, nic_name);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        close(skfd);
        return -1;
    }

    if (ifr.ifr_flags & IFF_RUNNING) {
        close(skfd);
        return 0;
    } else {
        close(skfd);
        return -1;
    }
}

/// @brief 网卡状态
/// @param nic_name
/// @return
int HV_NET_CheckNicStatusUp(const char *nic_name) {
    struct ifreq ifr;

    int skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == skfd) {
        printf("socket error\n");
        return -1;
    }
    strcpy(ifr.ifr_name, nic_name);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        close(skfd);
        return -1;
    }

    if (ifr.ifr_flags & IFF_UP) {
        close(skfd);
        return 0;
    } else {
        close(skfd);
        return -1;
    }
}