
#ifndef __comm_func_h__
#define __comm_func_h__

#include <iostream>
#include <stdio.h>
#include <vector>

#ifndef __rgba
#define __rgba(r, g, b, a)                                                                                             \
    (((unsigned char)(a) << 24) | ((unsigned char)(r) << 16) | ((unsigned char)(g) << 8) | (unsigned char)(b))
#endif
#ifndef __rgb
#define __rgb(r, g, b) __rgba(r, g, b, 0xFF)
#endif

#ifndef __max
#    define __max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef __min
#    define __min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#define __dc(T, P) dynamic_cast<T *>(P)
#define __del(P)                                                                                                       \
    do {                                                                                                               \
        if (P) {                                                                                                       \
            delete P;                                                                                                  \
            P = 0;                                                                                                     \
        }                                                                                                              \
    } while (false)
#define __count_array(A) sizeof(A) / sizeof(A[0])
#define __rand_value(A, B) ((A) + rand() % ((B) - (A) + 1))

#define DAY_SECONDS 86400
#define HOUR_SECONDS 3600
#define MIN_SECONDS 60

#define HV_POPEN(__cmd...)                                                                                             \
    do {                                                                                                               \
        char __result[256] = {0};                                                                                      \
        snprintf(__result, sizeof(__result), ##__cmd);                                                                 \
        system(__result);                                                                                              \
        printf("# %s\r\n", __result);                                                                                  \
    } while (0)

////////////////////////////////////////////////////////////////////////////////////////////////////
#define HV_STRING(arg...) #arg

// 是否全数字
bool isNumric(const char* str_nums);

// 时间字符串
std::string getTimeStr();

// 获取秒级时间戳
int64_t getTimeSec();

// 获取毫秒级时间戳
int64_t getTimeMSec();

// 字符串时间
std::string getDateTime();
std::string getDateTime(long int now, bool fmt = false);
std::string getDate(long int now, bool fmt = false, bool language = false);
std::string getTime(long int now, bool fmt = false, bool language = false);

//通过年 月，来设置 最大的日期（如 2月份只有29或28天）
int getMaxDay(int year,int mon);

// 获取0点时间戳
int64_t getZeroTime();

// 是否今天
bool isToday(int64_t sec);

// 长度不够，添加前缀
std::string fillLength(int num, int len, std::string pre = "0");
std::string fillLength(std::string& str, int len, char end = ' ');

// 打印二进制数据
void hexdump(const char* label, unsigned char* buf, int len, int width = 16);
std::string hexstr(unsigned char* buf, int len, int width = 32);

// 大小写转换
std::string& toUpper(std::string& letter);
std::string& toLower(std::string& letter);

// 分割字符串
int stringSplit(const std::string& str, std::vector<std::string>& out, char ch = ',');

// 设置时间
void timeSet(int year, int month, int day, int hour, int min, int sec);
void timeSet(const int64_t& time_sec);

// 获取星期几 [0-6]
std::string getDayOnWeek(int day);

// 获取字的长度
int         wordLen(const char* buffer);
std::string getWord(const char* buffer, int count);
std::string decLastWord(const char *buffer);    // 删除以后一个字符（可自适应 各种长度的符号）
std::string keepSpecifiedWord(const char *buffer, int count, bool en2cn, std::string spec = "");    // 保留指定长度的字符

// system结果
std::string sysCommand(const std::string& cmd);

// host -> ip
std::string getHostIp(const std::string& host);

// finds the first occurrence of string2 in string1
char* stristr(const char* str1, const char* str2);

/* 编写函数：
 * string& replace_all (string& src, const string& old_value, const string& new_value);
 * 参数：源字符串src    被替换的子串old_value    替换的子串new_value
 *
 * 功能：将 源串src 中 子串old_value 全部被替换为 new_value
 */
std::string& replace_all(std::string& src, const std::string& old_value, const std::string& new_value);

// 计算 wifi 信号的大小
int calculation_signal(float Quality, int SignalLevel);

/// @brief 设置屏幕亮度
/// @param value 亮度值 [1-99]
void setBrightness(int value);

/// @brief 读取本地文件
/// @param filename 
/// @param content 
bool readLoaclFile(const char* filename, std::string& content);

// 将版本号字符串按照 '.' 分割成数字部分
std::vector<int> splitVersionString(const std::string& version);

// 判断版本号大小(x.x.x.x)
//  1 version1 > version2
//  0 version1 == version2
// -1 version1 < version2
int compareVersionNumbers(const std::string& version1, const std::string& version2);

// 拆分版本号字符串为数字部分
std::vector<int> splitVersion(const std::string& version);

// 检查网络版本号 是否大于 当前的版本号
int checkVersion(const std::string &version,const std::string &localVersion);

#endif
