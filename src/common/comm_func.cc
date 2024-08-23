
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

#include "hv_defualt_config.h"

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

std::string getDateTime() {
    char           buffer[128];
    struct timeval _val;

    time_t     now;
    struct tm* tm_now;
    char       datetime[128];

    gettimeofday(&_val, NULL);
    now = _val.tv_sec;
    tm_now = localtime(&now);
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_now);

    snprintf(buffer, sizeof(buffer), "%s.%03d.%03d", datetime, _val.tv_usec / 1000, _val.tv_usec % 1000);

    return std::string(buffer);
}

std::string getDateTime(long int now, bool fmt) {
    struct tm* tm_now;
    tm_now = localtime(&now);
    char       datetime[128];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", tm_now);
    return datetime;
}

std::string getDate(long int now, bool fmt, bool language) {
    struct tm* tm_now;
    tm_now = localtime(&now);
    char       datetime[128];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d", tm_now);
    return datetime;
}

std::string getTime(long int now, bool fmt, bool language) {
    struct tm* tm_now;
    tm_now = localtime(&now);
    char       datetime[128];
    if (fmt) {
        snprintf(datetime, sizeof(datetime), "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
    } else {
        int PM = (tm_now->tm_hour / 12);
        if (PM) {
            tm_now->tm_hour = tm_now->tm_hour > 0 ? tm_now->tm_hour -= 12 : 12;
        }
        if (language) {
            snprintf(datetime, sizeof(datetime), "%s %02d:%02d", PM ? "下午" : "上午", tm_now->tm_hour, tm_now->tm_min);
        } else {
            snprintf(datetime, sizeof(datetime), "%s %02d:%02d", PM ? "PM" : "AM", tm_now->tm_hour, tm_now->tm_min);
        }

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
    if (hour >= 0) cur.tm_hour = hour;
    if (min >= 0) cur.tm_min = min;
    cur.tm_sec = sec;

    tv.tv_sec = mktime(&cur);
    tv.tv_usec = 0;
#ifndef DEBUG
    settimeofday(&tv, NULL);
#endif
}

void timeSet(const int64_t& time_sec) {
    struct timeval set_tv;
    set_tv.tv_sec = time_sec;
    set_tv.tv_usec = 0;
#ifndef DEBUG
    settimeofday(&set_tv, NULL);
#endif
}

int wordLen(const char* buffer) {
    int         word = 0;
    const char* pos = buffer;

    while (*pos) {
        if (*pos > 0 && *pos <= 127) {
            // ascii字符
            pos++;
            word++;
        } else {
            // utf-8字符
            pos += 3;
            word++;
        }
    }
    return word;
}

std::string getWord(const char* buffer, int count) {
    std::string words;
    int         wordCount = 0;
    const char* pos = buffer;
    while (*pos && wordCount < count) {
        if (*pos > 0 && *pos <= 127) {
            wordCount++;
            words.append(1, *(pos++));
        } else {
            wordCount++;
            words.append(1, *(pos++));
            words.append(1, *(pos++));
            words.append(1, *(pos++));
        }
    }
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

void setBrightness(int value) {
#ifndef CDROID_X64
    value = 100 - value;
    // /sys/class/pwm/pwmchip0/pwmN/duty_cycle 数值就为pwm占空比，范围[1-99]
    char path_duty_cycle[] = "/sys/class/pwm/pwmchip0/pwm2/duty_cycle";
    if (access(path_duty_cycle, F_OK)) { return; }

    int pwm = (value) * 99 / HV_SYS_CONFIG_MAX_SCREEN_BRIGHTNESS;
    if (true) {
        if (pwm > 99) {
            pwm = (HV_SYS_CONFIG_MAX_SCREEN_BRIGHTNESS - HV_SYS_CONFIG_SCREEN_BRIGHTNESS) * 99 / HV_SYS_CONFIG_MAX_SCREEN_BRIGHTNESS;   // 暗
        }
        if (pwm < 1) {
            pwm = 1;   // 亮
        }
    }

    char shell[128];
    snprintf(shell, sizeof(shell), "echo %d > %s", (100 - pwm) * 1000, path_duty_cycle);
    system(shell);
    LOG(VERBOSE) << shell;
#else
    LOGI("设置屏幕亮度");
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

uint8_t crc8_maxim(uint8_t* ptr, uint32_t len) {
    const static u_char crc8_table[256] = {
        0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
        157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
        35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
        190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
        70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
        219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
        101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
        248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
        140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
        17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
        175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
        50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
        202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
        87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
        233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
        116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
    };
    u_char crc = 0x00;
    while (len--) {
        crc = crc8_table[(crc ^ *ptr++) & 0xff];
    }
    LOGV("CRC-8/MAXIM-DOW : 0x%x", crc);
    return crc;
}

uint16_t calculateCRC16XModem(const uint8_t* data, size_t length) {
    uint16_t crc = 0x0000; // 初始 CRC 寄存器值
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8; // 将数据字节与 CRC 寄存器高位异或
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021; // CRC 寄存器左移一位并与多项式异或
            } else {
                crc <<= 1; // CRC 寄存器左移一位
            }
        }
    }
    return crc;
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
