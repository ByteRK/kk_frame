
#include <arpa/inet.h>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <locale>
#include <netdb.h>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ghc/filesystem.hpp>

#include <cdlog.h>
#include "comm_func.h"

#include "series_config.h"

bool isNumric(const char* str_nums) {
    for (const char* p = str_nums; *p; p++) {
        if (*p < '0' || *p > '9') { return false; }
    }

    // 待完善，支持小数、负数。。。

    return true;
}

std::string getTimeStr() {
    char           buff[128];
    std::time_t    t = std::time(NULL);
    struct std::tm cur = *std::localtime(&t);

    snprintf(buff, sizeof(buff), "%02d:%02d", cur.tm_hour, cur.tm_min);

    return std::string(buff);
}

int64_t getTimeSec() {
    struct timeval _val;
    gettimeofday(&_val, NULL);
    return _val.tv_sec;
}

int64_t getTimeMSec() {
    struct timeval _val;
    gettimeofday(&_val, NULL);
    return (int64_t)_val.tv_sec * 1000 + _val.tv_usec / 1000;
}

std::string getDateTime(const char* fmt) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    return getDateTime(now_c, fmt);
}

std::string getDateTime(long int time_sec, const char* fmt) {
    std::tm* time_tm = std::localtime(&time_sec);
    char       datetime[128];
    strftime(datetime, sizeof(datetime), fmt, time_tm);
    return datetime;
}

std::string getDateTimeAP(const char* fmt_am, const char* fmt_pm) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    return getDateTimeAP(now_c, fmt_am, fmt_pm);
}

std::string getDateTimeAP(long int time_sec, const char* fmt_am, const char* fmt_pm) {
    std::tm* time_tm = std::localtime(&time_sec);
    char       datetime[128];
    if (time_tm->tm_hour >= 12) {
        // 屏蔽，通过格式化小时参数为%I进行自动计算
        // time_tm->tm_hour = time_tm->tm_hour > 0 ? time_tm->tm_hour -= 12 : 12;
        strftime(datetime, sizeof(datetime), fmt_pm, time_tm);
    } else {
        strftime(datetime, sizeof(datetime), fmt_am, time_tm);
    }
    return datetime;
}

int getMaxDay(int year, int mon) {
    std::tm timeinfo = { 0 };
    timeinfo.tm_year = year - 1900; // 年份从1900开始
    timeinfo.tm_mon = mon;

    std::time_t time = std::mktime(&timeinfo);

    return std::localtime(&time)->tm_mday;
}

int64_t getZeroTime() {
    time_t     t = time(NULL);
    struct tm* tm = localtime(&t);
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    return mktime(tm);
}

bool isToday(int64_t sec) {
    int64_t zeroSec = getZeroTime();
    return zeroSec <= sec && sec < zeroSec + DAY_SECONDS;
}

std::string fillLength(int num, int len, std::string pre /* = "0"*/) {
    std::string retString;
    std::string numString = std::to_string(num);
    while (numString.length() + retString.length() < len) { retString.append(pre); }
    retString += numString;
    return retString;
}

std::string fillLength(std::string& str, int len, char end /* = ' '*/) {
    std::string ret = str;
    if (ret.size() < len) ret.append(len - ret.size(), end);
    return ret;
}

void hexdump(const char* label, unsigned char* buf, int len, int width /* = 16*/) {
    int  i, clen;
    char line_buf[512];

    if (width <= 0 || width > 64) { width = 16; }

    LOG(DEBUG) << label << " hex dump:";
    clen = 0;
    for (i = 0; i < len; i++) {
        clen += snprintf(line_buf + clen, sizeof(line_buf) - clen, "%02X ", buf[i]);
        if ((i + 1) % width == 0) {
            LOG(DEBUG) << "  " << line_buf;
            clen = 0;
        }
    }
    if (clen > 0) { LOG(DEBUG) << "  " << line_buf; }
}

std::string hexstr(unsigned char* buf, int len, int width/* = 32*/) {
    std::ostringstream oss;
    for (int i = 0; i < len; i++) {
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)buf[i] << " ";
        if ((i + 1) % width == 0) {
            oss << '\n';
        }
    }
    return oss.str();
}

std::string& toUpper(std::string& letter) {
    int         i;
    std::string old = letter;
    letter = "";
    for (i = 0; i < old.size(); i++) {
        if (old[i] >= 'a' && old[i] <= 'z') {
            letter.append(1, old[i] - 32);
        } else {
            letter.append(1, old[i]);
        }
    }
    return letter;
}

std::string& toLower(std::string& letter) {
    int         i;
    std::string old = letter;
    letter = "";
    for (i = 0; i < old.size(); i++) {
        if (old[i] >= 'A' && old[i] <= 'Z') {
            letter.append(1, old[i] + 32);
        } else {
            letter.append(1, old[i]);
        }
    }
    return letter;
}

int stringSplit(const std::string& str, std::vector<std::string>& out, char ch /* = ',' */) {
    int    offset = 0;
    size_t pos = str.find(ch, offset);
    while (pos != std::string::npos) {
        if (offset < pos) out.push_back(str.substr(offset, pos - offset));
        offset = pos + 1;
        pos = str.find(ch, offset);
    }
    if (offset < str.size()) { out.push_back(str.substr(offset)); }

    return out.size();
}

void timeSet(int year, int month, int day, int hour, int min, int sec) {
    struct timeval tv;
    std::time_t    t;
    struct std::tm cur;

    if (year == 0 && month == 0 && day == 0 && hour == 0 && min == 0 && sec == 0) { return; }

    t = std::time(NULL);
    cur = *std::localtime(&t);

    if (year > 0) cur.tm_year = year - 1900;
    if (month > 0) cur.tm_mon = month - 1;
    if (day > 0) cur.tm_mday = day;
    if (hour > 0) cur.tm_hour = hour;
    if (min > 0) cur.tm_min = min;
    cur.tm_sec = sec;

    tv.tv_sec = mktime(&cur);
    tv.tv_usec = 0;
#ifndef PRODUCT_X64
    settimeofday(&tv, NULL);
#endif
}

void timeSet(const int64_t& time_sec) {
    struct timeval set_tv;
    set_tv.tv_sec = time_sec;
    set_tv.tv_usec = 0;
#ifndef PRODUCT_X64
    settimeofday(&set_tv, NULL);
#ifdef CDROID_SIGMA
    sysCommand("hwclock --systohc");
#endif
#endif
}

std::string getDayOnWeek(int day) {
    static const std::string weekList[7] = { "星期日","星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
    return weekList[day % 7];
}

int wordLen(const char* buffer) {
    int word = 0;
    const char* pos = buffer;

    while (*pos) {
        if ((*pos & 0x80) == 0) {
            // ASCII字符 (1字节)
            pos++;
        } else if ((*pos & 0xE0) == 0xC0) {
            // 2字节字符
            pos += 2;
        } else if ((*pos & 0xF0) == 0xE0) {
            // 3字节字符
            pos += 3;
        } else if ((*pos & 0xF8) == 0xF0) {
            // 4字节字符
            pos += 4;
        } else {
            // 无效字节，直接跳过
            pos++;
        }
        word++;
    }
    return word;
}

std::string getWord(const char* buffer, int count) {
    std::string words;
    int         wordCount = 0;
    const char* pos = buffer;
    while (*pos && wordCount < count) {
        if (*pos & 0x80) {
            // 多字节字符
            int numBytes = 0;

            if ((*pos & 0xE0) == 0xC0) {
                // 2字节字符
                numBytes = 1;
            } else if ((*pos & 0xF0) == 0xE0) {
                // 3字节字符
                numBytes = 2;
            } else if ((*pos & 0xF8) == 0xF0) {
                // 4字节字符
                numBytes = 3;
            }

            for (int i = 0; i < numBytes + 1; ++i) {
                words.push_back(*(pos++));
            }

        } else {
            words.push_back(*(pos++));
        }
        wordCount++;
    }
    LOGV(" words = (%s) To (%s)", buffer, words.c_str());
    return words;
}

std::string decLastWord(const char* buffer) {
    std::string words;
    const char* pos = buffer;
    while (*pos) {
        if (*pos & 0x80) {
            // 多字节字符
            int numBytes = 0;

            if ((*pos & 0xE0) == 0xC0) {
                // 2字节字符
                numBytes = 1;
            } else if ((*pos & 0xF0) == 0xE0) {
                // 3字节字符
                numBytes = 2;
            } else if ((*pos & 0xF8) == 0xF0) {
                // 4字节字符
                numBytes = 3;
            }

            if (!*(pos + numBytes + 1)) break;
            for (int i = 0; i < numBytes + 1; ++i) {
                words.push_back(*(pos++));
            }

        } else {
            if (!*(pos + 1)) break;
            words.push_back(*(pos++));
        }
    }
    LOGV(" words = (%s) To (%s)", buffer, words.c_str());
    return words;
}

std::string keepSpecifiedWord(const char* buffer, int count, bool en2cn, std::string spec) {
    int nowWordCount = 0;
    const char* cpos = buffer;

    // 计算当前字符数量
    while (*cpos) {
        if ((*cpos & 0x80) == 0) {
            // ASCII字符 (1字节)
            cpos++;
            nowWordCount++;
        } else if ((*cpos & 0xE0) == 0xC0) {
            // 2字节字符
            cpos += 2;
            nowWordCount++;
            if (en2cn) nowWordCount++;
        } else if ((*cpos & 0xF0) == 0xE0) {
            // 3字节字符
            cpos += 3;
            nowWordCount++;
            if (en2cn) nowWordCount++;
        } else if ((*cpos & 0xF8) == 0xF0) {
            // 4字节字符
            cpos += 4;
            nowWordCount++;
            if (en2cn) nowWordCount++;
        }
    }

    // 如果当前字符数量小于等于目标数量，直接返回原字符串
    if (nowWordCount <= count) return std::string(buffer);

    std::string words;
    int wordCount = 0;
    const char* pos = buffer;

    // 提取字符直到达到目标数量
    while (*pos) {
        if ((*pos & 0x80) == 0) {
            // ASCII字符 (1字节)
            words.push_back(*(pos++));
            wordCount++;
        } else if ((*pos & 0xE0) == 0xC0) {
            // 2字节字符
            words.append(pos, pos + 2);
            pos += 2;
            wordCount++;
            if (en2cn) wordCount++;
        } else if ((*pos & 0xF0) == 0xE0) {
            // 3字节字符
            words.append(pos, pos + 3);
            pos += 3;
            wordCount++;
            if (en2cn) wordCount++;
        } else if ((*pos & 0xF8) == 0xF0) {
            // 4字节字符
            words.append(pos, pos + 4);
            pos += 4;
            wordCount++;
            if (en2cn) wordCount++;
        }

        // 达到目标字符数量，停止
        if (wordCount >= count) break;
    }

    // 添加指定的后缀
    words += spec;

    // 打印调试信息
    std::cout << "words = (" << buffer << ") To (" << words << ")" << std::endl;

    return words;
}

std::string sysCommand(const std::string& cmd) {
    std::string result;
    char        buffer[128]; // 存储输出数据的缓冲区

    // 执行系统命令，并读取输出数据
    FILE* fp = popen(cmd.c_str(), "r");
    if (fp == NULL) {
        perror("Failed to execute the command.\n");
        return "";
    }

    // 逐行读取输出数据并打印
    bzero(buffer, sizeof(buffer));
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        result.append(buffer);
        bzero(buffer, sizeof(buffer));
    }

    // 关闭文件指针
    pclose(fp);

    return result;
}

std::string getHostIp(const std::string& host) {
    std::string     ip_addr;
    const char* hostname = host.c_str(); // 要获取IP地址的主机名
    struct addrinfo hints, * result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;   // 支持IPv4和IPv6
    hints.ai_socktype = SOCK_STREAM; // 使用TCP协议

    // 解析主机名
    int status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return "";
    }

    // 遍历结果列表并打印IP地址
    struct addrinfo* p = result;
    while (p != NULL) {
        void* addr;
        if (p->ai_family == AF_INET) { // IPv4地址
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6地址
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        char ip[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, addr, ip, sizeof(ip));
        ip_addr = ip;
        break;

        p = p->ai_next;
    }

    // 释放内存
    freeaddrinfo(result);

    return ip_addr;
}

char* stristr(const char* str1, const char* str2) {
    char* cp = (char*)str1;
    char* s1, * s2;

    if (!*str2) return ((char*)str1);

    while (*cp) {
        s1 = cp;
        s2 = (char*)str2;

        while (*s1 && *s2 && toupper(*s1) == toupper(*s2)) { s1++, s2++; }

        if (!*s2) return (cp);

        cp++;
    }

    return (NULL);
}

/* 编写函数：
 * string& replace_all (string& src, const string& old_value, const string& new_value);
 * 参数：源字符串src    被替换的子串old_value    替换的子串new_value
 *
 * 功能：将 源串src 中 子串old_value 全部被替换为 new_value
 */
std::string& replace_all(std::string& src, const std::string& old_value, const std::string& new_value) {
    // 每次重新定位起始位置，防止上轮替换后的字符串形成新的old_value
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
        if ((pos = src.find(old_value, pos)) != std::string::npos) {
            src.replace(pos, old_value.length(), new_value);
        } else break;
    }
    return src;
}


// 强信号：Quality > 75% 且 Signal level > -50 dBm
// 中等信号：Quality > 50% 且 Signal level > -60 dBm
// 弱信号：Quality > 25% 且 Signal level > -70 dBm
// 非常弱信号：Quality < 25% 或 Signal level < -70 dBm

// 计算 wifi 信号的大小
int calculation_signal(float Quality, int SignalLevel) {
    if (Quality > 0.75 && SignalLevel > -50)
        return 4;
    else if (Quality > 0.5 && SignalLevel > -60)
        return 3;
    else if (Quality > 0.25 && SignalLevel > -70)
        return 2;
    else if (Quality < 0.25 || SignalLevel < -70)
        return 1;
    else
        return 1;
}

void setBrightness(uint8_t value, bool swap) {
#ifndef CDROID_X64
    value = value % 101; // 限制在0-100之间
    char pwm_path[128];
    snprintf(pwm_path, 128, "%s%s", "/sys/class/pwm/pwmchip0/", SYS_SCREEN_BEIGHTNESS_PWM);
    if (access(pwm_path, F_OK)) { return; }

    if (value) {
        if (swap)value = 100 - value; // 反转亮度值
        if (value == 0) value = 1;    // 亮度值不能为0

        char setEnable[128];
        snprintf(setEnable, 128, "echo 1 > %s%s", pwm_path, "/enable");
        system(setEnable);
        LOG(VERBOSE) << setEnable;

        char setDutyCycle[128];
        int pwm = (value * (SYS_SCREEN_BRIGHTNESS_MAX - SYS_SCREEN_BRIGHTNESS_MIN)) / 100; // 转换pwm值
        snprintf(setDutyCycle, 128, "echo %d > %s%s", pwm, pwm_path, "/duty_cycle");
        system(setDutyCycle);
        LOG(VERBOSE) << setDutyCycle;
    } else {
        char shell[128];
        snprintf(shell, sizeof(shell), "echo 0 > %s%s", pwm_path, "/enable");
        system(shell);
        LOG(VERBOSE) << shell;
    }
#else
    LOGI("设置屏幕亮度 %d", value);
#endif
}

bool readLoaclFile(const char* filename, std::string& content) {
    ghc::filesystem::path  file_path(filename);
    std::string tmp;
    content.swap(tmp);

    // 文件不存在或者是一个目录
    if (!ghc::filesystem::exists(file_path) || ghc::filesystem::is_directory(file_path))
        return false;

    std::ifstream  pfile;
    pfile.open(filename, std::ios::binary);
    if (pfile.is_open()) {
        // 获取文件大小
        pfile.seekg(0, std::ios::end);
        std::streampos fileSize = pfile.tellg();
        pfile.seekg(0, std::ios::beg);
        pfile.clear();

        char data[1024];
        do {
            pfile.read(data, sizeof(data));
            size_t extracted = pfile.gcount();
            content.append(data, extracted);
        } while (!pfile.eof());
        pfile.close();
        return true;
    }
    return false;
}

std::vector<int> splitVersionString(const std::string& version) {
    std::vector<int> result;
    std::stringstream ss(version);
    std::string token;

    while (std::getline(ss, token, '.')) {
        result.push_back(std::stoi(token));
    }

    return result;
}

int compareVersionNumbers(const std::string& version1, const std::string& version2) {
    std::vector<int> v1 = splitVersionString(version1);
    std::vector<int> v2 = splitVersionString(version2);

    int minLength = std::min(v1.size(), v2.size());

    for (int i = 0; i < minLength; ++i) {
        if (v1[i] < v2[i]) {
            return -1; // version1 < version2
        } else if (v1[i] > v2[i]) {
            return 1; // version1 > version2
        }
    }

    if (v1.size() < v2.size()) {
        return -1; // version1 < version2
    } else if (v1.size() > v2.size()) {
        return 1; // version1 > version2
    }

    return 0; // version1 == version2
}

// 拆分版本号字符串为数字部分
std::vector<int> splitVersion(const std::string& version) {
    std::vector<int> result;
    std::istringstream iss(version);
    std::string part;
    while (std::getline(iss, part, '.')) {
        result.push_back(std::stoi(part));
    }
    return result;
}

// 检查网络版本号 是否大于 当前的版本号
int checkVersion(const std::string& version, const std::string& localVersion) {
    std::vector<int> otaVersion = splitVersion(version);
    std::vector<int> localOtaVersion = splitVersion(localVersion);

    // 比较每个部分
    for (size_t i = 0; i < std::min(otaVersion.size(), localOtaVersion.size()); ++i) {
        if (otaVersion[i] < localOtaVersion[i]) {
            return -1;
        } else if (otaVersion[i] > localOtaVersion[i]) {
            return 1;
        }
    }

    // 版本号的前缀部分相同，比较长度确定大小关系
    if (otaVersion.size() < localOtaVersion.size()) {
        return -1;
    } else if (otaVersion.size() > localOtaVersion.size()) {
        return 1;
    }

    return 0;
}
