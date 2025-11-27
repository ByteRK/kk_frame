
#include <arpa/inet.h>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <locale>
#include <netdb.h>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ghc/filesystem.hpp>
#include <iconv.h>
#include <regex> // 正则表达式

#include <cdlog.h>
#include "comm_func.h"

#include "series_info.h"

#include <sys/ioctl.h>

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

int getMaxDay(int year, int month) {
    if (month < 1 || month > 12) return 0;
#if 0 // 标准库方式
    tm tm_struct = {};                // 初始化 tm 结构体
    tm_struct.tm_year = year - 1900;  // tm_year 是从 1900 开始的年份偏移
    tm_struct.tm_mon = month;         // 设置为目标月份的下一个月（例如：3月 -> 4月）
    tm_struct.tm_mday = 0;            // 设置为下个月的“第0天”，即上个月的最后一天
    tm_struct.tm_hour = 0;            // 必须设置时间字段，避免未定义行为
    mktime(&tm_struct);               // 标准化时间，自动调整日期
    return tm_struct.tm_mday;         // 返回计算后的天数
#else // 手动计算方式
    static const int days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2) {
        // 判断闰年条件：能被4整除但不能被100整除，或能被400整除
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29; // 闰年2月有29天
        } else {
            return 28; // 平年2月有28天
        }
    } else {
        return days[month - 1];       // 月份从1开始，数组索引从0开始
    }
#endif
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
    // 数据有效性检查
    if (year == 0 && month == 0 && day == 0 && hour == 0 && min == 0 && sec == 0) {
        return;
    }

    // 使用 std::chrono::system_clock 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm cur = *std::localtime(&currentTime);

    // 更新时间信息 (仅当参数有效时更新)
    if (year > 0) cur.tm_year = year - 1900;              // Year - 1900.
    if (month > 0 && month <= 12) cur.tm_mon = month - 1; // [0-11]
    if (day > 0) cur.tm_mday = day;                       // [1-31]
    if (hour >= 0) cur.tm_hour = hour;                    // [0-23]
    if (min >= 0) cur.tm_min = min;                       // [0-59]
    if (sec >= 0) cur.tm_sec = sec;                       // [0-60]

    // 处理日期溢出
    std::time_t newTime = std::mktime(&cur);
    if (newTime == -1) {
        LOGE("Invalid date/time provided.");
        return;
    }

    // 设置系统时间
    timeSet(newTime);
}

void timeSet(const int64_t& time_sec) {
#ifndef CDROID_X64
    struct timeval set_tv;
    set_tv.tv_sec = time_sec;
    set_tv.tv_usec = 0;
    settimeofday(&set_tv, NULL);
    LOGE("set time %s", getDateTime("%Y-%m-%d %H:%M:%S").c_str());
#ifdef CDROID_SIGMA
    sysCommand("hwclock --systohc");
#endif
#else
    LOGE("set time %s", getDateTime(time_sec, "%Y-%m-%d %H:%M:%S").c_str());
#endif
}

std::string getDayOnWeek(int day) {
    static const std::string weekList[7] = { "星期日","星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
    return weekList[day % 7];
}

int wordLen(const char* buffer, bool en2cn) {
    int word = 0;
    const char* pos = buffer;

    while (*pos) {
        if ((*pos & 0x80) == 0) {
            // ASCII字符 (1字节)
            pos++;
            word++;
        } else if ((*pos & 0xE0) == 0xC0) {
            // 2字节字符
            pos += 2;
            word++;
            if (en2cn) word++;
        } else if ((*pos & 0xF0) == 0xE0) {
            // 3字节字符
            pos += 3;
            word++;
            if (en2cn) word++;
        } else if ((*pos & 0xF8) == 0xF0) {
            // 4字节字符
            pos += 4;
            word++;
            if (en2cn) word++;
        } else {
            // 无效字节，直接跳过
            pos++;
        }
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
/// @brief 去除反斜杠
/// @param input 
/// @return 
std::string removeBackslashes(const std::string& input) {
    std::string output;
    for (char ch : input) {
        if (ch != '\\') { // 只保留不是反斜杠的字符
            output += ch;
        }
    }
    return output;
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

int calculation_signal(int SignalLevel) {
    if (SignalLevel > -60)
        return 4;
    else if (SignalLevel > -75)
        return 3;
    else if (SignalLevel > -90)
        return 2;
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

std::string ConvertEncoding(const std::string& input, const char* from_encoding, const char* to_encoding) {
    iconv_t cd = iconv_open(to_encoding, from_encoding);
    if (cd == (iconv_t)-1) {
        return "";
    }

    size_t in_bytes = input.size();
    size_t out_bytes = in_bytes * 4; // 预估输出缓冲区大小
    std::string output(out_bytes, 0);

    char* in_ptr = const_cast<char*>(input.data());
    char* out_ptr = &output[0];

    if (iconv(cd, &in_ptr, &in_bytes, &out_ptr, &out_bytes) == (size_t)-1) {
        iconv_close(cd);
        return "";
    }

    iconv_close(cd);
    output.resize(output.size() - out_bytes);
    return output;
}

std::string GBKToUTF8_Iconv(const char* gbk_str) {
    iconv_t cd = iconv_open("UTF-8", "GBK"); // 定义转换方向
    if (cd == (iconv_t)-1) return "";

    size_t in_len = strlen(gbk_str);
    size_t out_len = in_len * 4; // UTF-8 最多占用 4 字节/字符
    std::string utf8_str(out_len, 0);

    char* in_ptr = const_cast<char*>(gbk_str);
    char* out_ptr = &utf8_str[0];

    if (iconv(cd, &in_ptr, &in_len, &out_ptr, &out_len) == (size_t)-1) {
        iconv_close(cd);
        return "";
    }

    iconv_close(cd);
    utf8_str.resize(utf8_str.size() - out_len); // 调整实际长度
    return utf8_str;
}

std::string ParseHexEscapes(const std::string& input) {
    std::vector<char> bytes;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\\' && i + 3 < input.size() && input[i + 1] == 'x') {
            // 提取十六进制值（如 "e4"）
            std::string hex_str = input.substr(i + 2, 2);
            std::istringstream hex_stream(hex_str);
            int byte_value;
            if (hex_stream >> std::hex >> byte_value) {
                bytes.push_back(static_cast<char>(byte_value));
                i += 4; // 跳过已处理的 "\xHH"
            } else {
                // 解析失败，保留原字符
                bytes.push_back(input[i++]);
            }
        } else {
            bytes.push_back(input[i++]);
        }
    }
    return std::string(bytes.begin(), bytes.end());
}


bool macToChar(const std::string& mac, uint8_t* mac_char) {
    std::istringstream ss(mac);
    std::string item;
    for (int i = 0; i < 6; ++i) {
        if (std::getline(ss, item, ':')) {
            // 将十六进制字符串转换为 uint8_t
            mac_char[i] = static_cast<uint8_t>(std::stoi(item, nullptr, 16));
        } else {
            // 如果输入的 MAC 地址不够长，处理错误
            LOGE("Invalid MAC address format.");
            return false;
        }
    }
    return true;
}

bool isValidMacAddress(const std::string& mac) {
    // 使用正则表达式检查 MAC 地址格式
    std::regex macRegex(R"((([0-9A-Fa-f]{2}[-:]){5}([0-9A-Fa-f]{2}))|(([0-9A-Fa-f]{2}){6}))");
    return std::regex_match(mac, macRegex);
}

int getTpVersion() {
#ifdef CDROID_SIGMA
    int fd = open("/dev/techwin_ioctl", O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }

    int ret = ioctl(fd, 0xff);
    LOG(ERROR) << "getTpVersion: " << ret << std::endl;

    ::close(fd);
    return ret;
#else
    return 0;
#endif
}

std::string clearWhiteSpace(std::string& str) {
    auto it = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    str.erase(it.base(), str.end());
    return str;
}

bool checkFileEmpty(const std::string& filename) {
    // 使用 std::ifstream 打开文件
    std::ifstream file(filename);

    // 检查文件是否成功打开
    if (!file) {
        LOGE("File not found or cannot be opened.");
        return false; // 或者抛出异常
    }

    // 移动到文件末尾
    file.seekg(0, std::ios::end);
    // 检查文件大小
    return file.tellg() == 0;
}

bool checkFileExitAndNoEmpty(const char* filename) {
    // 检查文件能否成功打开
    std::ifstream file(filename);
    if (!file) {
        LOGE("File [%s] not found or cannot be opened.", filename);
        return false;
    }

    // 检查文件大小
    file.seekg(0, std::ios::end);
    if (file.tellg() == 0) {
        LOGE("File [%s] is empty.", filename);
        return false;
    }

    // 检查文件权限
    // struct stat buffer;
    // if (stat(filename, &buffer) != 0 || !(buffer.st_mode & S_IRUSR) || !(buffer.st_mode & S_IWUSR)) {
    //     LOGE("File [%s] does not have required permissions.", filename);
    //     return false;
    // }

    return true;
}
