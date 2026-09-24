/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-03-17 14:15:07
 * @LastEditTime: 2026-09-22 17:44:26
 * @FilePath: /kk_frame/library/keyboard/childs/keyboard_cn.cc
 * @Description:
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "keyboard_cn.h"

#if ENABLED(KEYBOARD_PINYIN)
#include <pinyinime.h>
#include <utils/textutils.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

/// @brief 字库文件可用性校验
/// @note 必须在 im_open_decoder() 之前调用，原因（实测）：
///       1. 字库不可用时引擎不报错，仍返回非空句柄，但其后 im_close_decoder() 会段错误；
///       2. 字库字长与当前平台不符时，im_open_decoder() 内部会 bad_alloc 直接 abort。
///       校验不通过就不打开（句柄保持空），这样既不会崩，也能在上层重试。
/// @note 字库格式随平台字长变化（头部计数按 sizeof(size_t) 写入），
///       所以"文件存在"并不代表可用 —— 这正是 64 位下崩溃的根因
/// @param path 字库路径
/// @param reason 校验失败原因（输出）
/// @return 是否可用
static bool checkPinyinDict(const std::string& path, std::string& reason) {
    static constexpr size_t HEAD_SIZE = 16;        // 覆盖 64 位下的文件头
    static constexpr size_t COUNT_MAX = 1u << 24;  // 头部计数上限，真实字库远小于此

    struct stat st;
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        reason = "文件不存在";
        return false;
    }
    if ((size_t)st.st_size < HEAD_SIZE) {
        reason = "文件过小或已损坏";
        return false;
    }

    unsigned char head[HEAD_SIZE] = { 0 };
    FILE* fp = ::fopen(path.c_str(), "rb");
    if (!fp) {
        reason = "无法打开";
        return false;
    }
    const size_t readSize = ::fread(head, 1, HEAD_SIZE, fp);
    ::fclose(fp);
    if (readSize != HEAD_SIZE) {
        reason = "读取失败";
        return false;
    }

    // 文件头为 2 个 size_t 计数（其后的载荷与架构无关，字节级一致）：
    // 64 位下第 1 个计数占 8 字节、其高 4 字节必为 0；
    // 32 位字库的第 5~8 字节则是第 2 个计数（不会为 0），据此识别字长
    uint32_t secondWord = 0;
    ::memcpy(&secondWord, head + 4, sizeof(secondWord));
    if (sizeof(size_t) == 8 && secondWord != 0) {
        reason = "字库由 32 位程序生成，与当前 64 位程序不兼容";
        return false;
    }

    // 按本机字长解析头部计数：字长不一致时取值会异常巨大，作为兜底拦截
    size_t counts[2] = { 0, 0 };
    ::memcpy(counts, head, sizeof(counts));
    if (!counts[0] || !counts[1] || counts[0] > COUNT_MAX || counts[1] > COUNT_MAX) {
        reason = "内容与当前架构不匹配或已损坏";
        return false;
    }

    return true;
}

/// @brief 依据引擎给出的音节起始位置，把拼音拼成带分隔符的显示串（nihao -> ni'hao）
/// @param raw 不含分隔符的拼音原串
/// @param splStart 音节起始位置数组（由 im_get_spl_start_pos 提供）
/// @param splCount 音节数量（splStart 的元素个数为 splCount + 1）
static std::string formatPinyin(const std::string& raw, const uint16_t* splStart, int splCount) {
    if (!splStart || splCount <= 0)return raw;

    std::string display;
    size_t begin = 0;
    for (int i = 0; i <= splCount; i++) {
        const size_t end = (splStart[i] < raw.size()) ? splStart[i] : raw.size();
        if (end <= begin)continue;
        if (!display.empty())display.push_back('\'');
        display.append(raw, begin, end - begin);
        begin = end;
    }
    // 引擎未消费的尾部原样保留，避免显示丢字符
    if (begin < raw.size())display.append(raw, begin, raw.size() - begin);
    return display.empty() ? raw : display;
}
#endif


Keyboard_CN::Keyboard_CN(CKeyBoard* parent) :Keyboard_EN(parent, "@keyboard:layout/keyboard_cn") {
    mScanBuffer.assign(CANDIDATE_BUFF_SIZE, 0);

    setKeyStr(DISPLAY_TYPE_DEFAULT, {
        "q","w","e","r","t","y","u","i","o","p",
        "a","s","d","f","g","h","j","k","l",
        "z","x","c","v","b","n","m",
        "，","空格","。",
        "?123","符号","中文","",""
        });
    setKeyStr(DISPLAY_TYPE_UPPER, {
        "Q","W","E","R","T","Y","U","I","O","P",
        "A","S","D","F","G","H","J","K","L",
        "Z","X","C","V","B","N","M",
        ",","空格",".",
        "?123","符号","中文","",""
        });
    copyKeyStr(DISPLAY_TYPE_UPPER, DISPLAY_TYPE_UPPER_PLUS);
    setKeyStr(DISPLAY_TYPE_NUMBER, {
        "1","2","3","4","5","6","7","8","9","0",
        "！","？","：","；","（","）","—","、","=",
        "￥","%","@","#","&","*","+",
        "，","空格","。",
        "拼音","符号","中文","",""
        });
    setKeyStr(DISPLAY_TYPE_MORE, {
        "「","」","『","』","【","】","《","》","〈","〉",
        "〔","〕","〖","〗","※","☆","★","○","●",
        "～","·","—","…","￥","€","£",
        "？","空格","。",
        "拼音","123","中文","",""
        });

    LOGI("Keyboard_CN::Keyboard_CN() Created");
}

Keyboard_CN::~Keyboard_CN() {
    // 本对象同时是候选列表的 Adapter，必须先解绑，否则 RecyclerView 会持有已释放的 Adapter
    if (mCandidateList)mCandidateList->setAdapter(nullptr);
    if (mPinyin)mPinyin->setOnClickListener(nullptr);
    pinyinClose();
}

CKeyBoard::KeyBoardType Keyboard_CN::getType() {
    return CKeyBoard::KB_TYPE_CN;
}

void Keyboard_CN::init() {
    Keyboard_EN::init();
    mCandidateBoxes = __dc(ViewGroup, mRootView->findViewById(LibRid::candidate_box));
    mPinyin = __dc(TextView, mRootView->findViewById(LibRid::pinyin));
    mCandidateList = __dc(RecyclerView, mRootView->findViewById(LibRid::candidate_list));
    FailFast(!mCandidateBoxes || !mPinyin || !mCandidateList,
        "Keyboard_CN init failed, check @keyboard:layout/keyboard_cn and the ids in R.h");

    mCandidateBoxes->setEnabled(false);

    mPinyin->setOnClickListener([this](View &v) {
        // 点击拼音串：上屏的是原串（不含分隔符），与用户实际键入内容一致
        mParent->appendText(mPinyinRaw);
        clearCandidate();
    });

    mCandidateList->setLayoutManager(new LinearLayoutManager(mRootView->getContext(), LinearLayoutManager::HORIZONTAL, false));
    mCandidateList->setAdapter(this);
}

void Keyboard_CN::onShow() {
    Keyboard_EN::onShow();
    updateParentBtn("确定", "取消");
    pinyinOpen();
    clearCandidate();
}

void Keyboard_CN::onHide() {
    Keyboard_EN::onHide();
    clearCandidate();
}

void Keyboard_CN::onKeyClick(int key) {
    if (mDisplayType != DISPLAY_TYPE_DEFAULT || (!isLetterKey(key) && key != idxBackspace())) {
        // 非拼音，不接管
        clearCandidate();
        Keyboard_EN::onKeyClick(key);
        return;
    }

    if (key == idxBackspace()) {
        mPinyinRaw.size() ? pinyinDel() : Keyboard_EN::onKeyClick(key);
    } else {
        pinyinAdd(keyStr(key));
    }
}

void Keyboard_CN::onBackspaceLongPress() {
    // 拼音未上屏时先清拼音，否则清除光标前的内容
    if (!mPinyinRaw.empty())clearCandidate();
    else Keyboard_EN::onBackspaceLongPress();
}

void Keyboard_CN::onTextCleared() {
    // 内容被清空，同时丢弃未上屏的拼音与候选
    clearCandidate();
}

int Keyboard_CN::getItemCount() {
    return mCandidateListData.size();
}

RecyclerView::ViewHolder* Keyboard_CN::onCreateViewHolder(ViewGroup* parent, int viewType) {
    TextView* item = __dc(TextView, LayoutInflater::from(parent->getContext())->inflate("@keyboard:layout/keyboard_cn_candidate", parent, false));
    item->setOnClickListener([this](View &v) {
        mParent->appendText(__dc(TextView, &v)->getText());
        clearCandidate();
    });
    return new RecyclerView::ViewHolder(item);
}

void Keyboard_CN::onBindViewHolder(RecyclerView::ViewHolder& holder, int position) {
    TextView* item = __dc(TextView, holder.itemView);
    item->setText(position < mCandidateListData.size() ? mCandidateListData[position] : "OUT_OF_RANGE");
}

void Keyboard_CN::pinyinOpen() {
    if (mPinyinhandle) return;
#if ENABLED(KEYBOARD_PINYIN)
    // 字库放在程序运行目录下的 pinyin/ 中，见 library/keyboard/pinyin/README.md
    const std::string sysDict = KEYBOARD_PINYIN_DIR KEYBOARD_PINYIN_SYS_DICT;
    const std::string usrDict = KEYBOARD_PINYIN_DIR KEYBOARD_PINYIN_USR_DICT;

    // 先校验再打开：字库字长与当前平台不符时，im_open_decoder() 内部会 bad_alloc 直接 abort；
    // 文件缺失时提前返回可让句柄保持为空，待字库就位后可再次尝试
    std::string reason;
    if (!checkPinyinDict(sysDict, reason)) {
        LOGE("pinyin dict unusable: [%s] (%s), chinese input disabled", sysDict.c_str(), reason.c_str());
        return;
    }

    if (access(usrDict.c_str(), F_OK) != 0)
        LOGW("pinyin usr dict not exist: [%s], user learning disabled", usrDict.c_str());

    mPinyinhandle = ime_pinyin::im_open_decoder(sysDict.c_str(), usrDict.c_str());
    if (!mPinyinhandle) {
        LOGE("pinyin open failed: [%s]", sysDict.c_str());
        return;
    }

    // 打开后探针：字库内容损坏/被截断时引擎照样不报错，但其后 im_close_decoder() 会段错误。
    // 用两次正常字库必有候选的检索判定引擎是否真的初始化成功（未初始化时一律返回 0）
    if (!ime_pinyin::im_search(mPinyinhandle, "a", 1)
        && !ime_pinyin::im_search(mPinyinhandle, "ni", 2)) {
        // 此处不能调用 im_close_decoder()（未初始化时必崩），也无法安全释放，只能保留句柄；
        // 保留句柄同时阻止了反复重试导致的重复泄漏
        LOGE("pinyin dict load failed: [%s], chinese input disabled", sysDict.c_str());
        return;
    }

    mPinyinUsable = true;
    LOGI("pinyinOpen: %p", mPinyinhandle);
#else
    LOGE("please enable pinyin support");
#endif
}

void Keyboard_CN::pinyinClose() {
    if (!mPinyinUsable) return;  // 未初始化成功时调用 im_close_decoder() 会段错误
#if ENABLED(KEYBOARD_PINYIN)
    ime_pinyin::im_close_decoder(mPinyinhandle);
    mPinyinhandle = nullptr;
    mPinyinUsable = false;
#endif
}

void Keyboard_CN::pinyinAdd(const std::string& pinyin) {
    if (pinyin.empty())return;
    mPinyinRaw.push_back(pinyin[0]);
#if ENABLED(KEYBOARD_PINYIN)
    if (!mPinyinUsable) {
        pinyinShowRaw();
        return;
    }
    if (mPinyinRaw.size() == 1) {  // 新词，先落盘用户词并重置检索
        ime_pinyin::im_flush_cache(mPinyinhandle);
        ime_pinyin::im_reset_search(mPinyinhandle);
    }
    pinyinSearch(mPinyinRaw);
#else
    pinyinShowRaw();
    LOGE("please enable pinyin support");
#endif
}

void Keyboard_CN::pinyinDel() {
    if (mPinyinRaw.empty()) {
        clearCandidate();
        return;
    }
    mPinyinRaw.pop_back();
    if (mPinyinRaw.empty()) {
        clearCandidate();
        return;
    }
#if ENABLED(KEYBOARD_PINYIN)
    pinyinSearch(mPinyinRaw);
#else
    pinyinShowRaw();
    LOGE("please enable pinyin support");
#endif
}

void Keyboard_CN::pinyinSearch(const std::string& pinyin) {
#if ENABLED(KEYBOARD_PINYIN)
    if (!mPinyinUsable)return;

    size_t resSize = ime_pinyin::im_search(mPinyinhandle, pinyin.c_str(), pinyin.length());
    size_t needResSize = resSize > CANDIDATE_MAX_SIZE ? CANDIDATE_MAX_SIZE : resSize;
    LOGD("search [%s] -> %d/%d", pinyin.c_str(), (int)needResSize, (int)resSize);

    // 用引擎给出的音节起始位置做分段显示（ni'hao），便于用户确认与编辑
    const uint16_t* splStart = nullptr;
    const int splCount = (int)ime_pinyin::im_get_spl_start_pos(mPinyinhandle, splStart);
    mPinyin->setText(formatPinyin(pinyin, splStart, splCount));

    mCandidateListData.clear();

    for (size_t i = 0; i < needResSize; i++) {
        uint16_t* scan = ime_pinyin::im_get_candidate(mPinyinhandle, i, mScanBuffer.data(), CANDIDATE_BUFF_SIZE);
        if (!scan)continue;  // 引擎失败时返回 NULL
        std::string u8s = cdroid::TextUtils::utf16_utf8(scan, ime_pinyin::utf16_strlen(scan));
        if (u8s.size())mCandidateListData.push_back(u8s);
    }
    notifyDataSetChanged();
#else
    LOGE("please enable pinyin support: %s", pinyin.c_str());
#endif
}

void Keyboard_CN::pinyinShowRaw() {
    mPinyin->setText(mPinyinRaw);
    mCandidateListData.clear();
    if (!mPinyinRaw.empty())mCandidateListData.push_back(mPinyinRaw);
    notifyDataSetChanged();
}

void Keyboard_CN::clearCandidate() {
    mPinyinRaw.clear();
    mPinyin->setText("");
    if (mCandidateListData.size()) {
        mCandidateListData.clear();
        notifyDataSetChanged();
    }
}
