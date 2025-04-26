
// 输入法 CDROID版
// 键盘控件，放置于最外层控件组首个
#ifndef __keyboard_h__
#define __keyboard_h__

#include <set>
#include <widget/button.h>
#include <widget/edittext.h>
#include <view/viewgroup.h>
#include <widget/relativelayout.h>

// 小写按键
enum emKeyBoardPageType
{
    KB_PT_SMALL = 0,   // 小写
    KB_PT_BIG,         // 大写
    KB_PT_MORE,        // 更多
    KB_PT_NUMBER,      // 123
    KB_PT_COUNT,
};

class CdroidKeyBoard : public RelativeLayout
{
public:
    DECLARE_UIEVENT(void, OnCompleteListener, const std::string &editTxt);
    typedef enum {
        LT_NULL = 0,
        LT_NUMBER, // 数字
        LT_CN,     // 中文
        LT_EN,    // 英文
    }LoadType;
public:
    CdroidKeyBoard(int, int);
    CdroidKeyBoard(Context *, const AttributeSet &attr);

    // 加载后首次界面
    void setLoadType(LoadType lt);
    // 显示
    void showWindow();
    // 输入内容类型
    void setEditType(int editType=LT_CN);
    // 输入字长度
    void setWordCount(int count) {mWordCount = count;}

    std::string getEnterText();
    void setDescription(const std::string &txt);// 设置描述文本（输入框文本为空时 显示）
    void setEditText(const std::string &txt);   // 用于创建键盘时，输入框带默认文本
    void setText(const std::string &txt);       // 用于键盘修改 输入框 文本
    void setText(const std::string &txt, TextView *txtView, int maxLen = 40, int minSize = 16, int maxSize = 34);   // 修改不同控件的文本
    void decText(TextView *txtView);            // 删除最后一个文本
    
public:
    virtual void setOnCompleteListener(OnCompleteListener l);

protected:
    template<typename T = View>
    T *getView(int id)
    {
        return dynamic_cast<T *>(findViewById(id));
    }

    void initValue();
    // 设置监听
    void initListener();
    // 点击通知
    void onClick(View &v);
    void onClickID(int id);
    void onClickWord(View &v);
    void onLayoutChanged(View&v,int l,int t,int w,int h,int oldL,int oldT,int oldW,int oldH);
    // 转到下一页面
    void trun2NextPage();
    // 内容输入
    void enterPassword(int keyID);    
    void enterEnglish(int keyID);
    void enterNumber(int keyID);
    void enterText(int keyID);

    // 输入内容变化
    void onTextChanged(EditText & ls);

    std::string &decStr(std::string &txt);

    void setEnterText(const std::string &txt);
    // 清空候选区
    void clearCandidate();

protected:
    std::string mDescription;
    std::string mEnterText;
    EditText * mText;      // 确认的内容
    Button *   mOk;        // 确认按钮
    ViewGroup *mWorldVG;   // 候选区容器
    ViewGroup *mRow2VG;    // 第二行的容器
    TextView * mWord;       // 输入词
    TextView * mWord2;       // 输入词2
    TextView * mWord3;       // 输入词3
    TextView * mWord4;       // 输入词4
    TextView * mWord5;       // 输入词5
    TextView * mWord6;       // 输入词6
    TextView * mWord7;       // 输入词7
    TextView * mWord8;       // 输入词8
    TextView * mWord9;       // 输入词9
    TextView * mWord10;       // 输入词10
    View   *   mHide;        // 隐藏键盘,下一页
    View   *   mPrePage;     // 上一页
    Button *   mKeyQ;        // Q键
    Button *   mKeyW;        // W键
    Button *   mKeyE;        // E键
    Button *   mKeyR;        // R键
    Button *   mKeyT;        // T键
    Button *   mKeyY;        // Y键
    Button *   mKeyU;        // U键
    Button *   mKeyI;        // I键
    Button *   mKeyO;        // O键
    Button *   mKeyP;        // P键
    Button *   mKeyA;        // A键
    Button *   mKeyS;        // S键
    Button *   mKeyD;        // D键
    Button *   mKeyF;        // F键
    Button *   mKeyG;        // G键
    Button *   mKeyH;        // H键
    Button *   mKeyJ;        // J键
    Button *   mKeyK;        // K键
    Button *   mKeyL;        // L键
    Button *   mKeyLR;       // L键右边隐藏键
    Button *   mKeyCase;     // 大小写切换键
    Button *   mKeyZ;        // Z键
    Button *   mKeyX;        // X键
    Button *   mKeyC;        // C键
    Button *   mKeyV;        // V键
    Button *   mKeyB;        // B键
    Button *   mKeyN;        // N键
    Button *   mKeyM;        // M键
    Button *   mBackSpace;   // 退格键
    Button *   mNumber;      // 切换数字符号
    Button *   mZhEn;        // 中/英文切换键
    Button *   mComma;       // 逗号
    Button *   mSpace;       // 空格
    Button *   mPeriod;      // 句号
    Button *   mWrap;        // 换行
    TextView  *mZhPingyin;   // 中文拼音

    View::OnLayoutChangeListener mOnLayoutChangeListener;
protected:
    LoadType  mLoadType;
    int  mLastPageType;        // 上一次键盘类型
    int  mPageType;            // 小写按钮状态值
    bool mHideLetter;          // 字母隐藏
    // int  mLetterRow2Padding;   // 字母页行2行内间距
    int  mNumberWidth;         // 数字切换键宽度
    int  mZhEnWidth;           // 中英文切换键宽度
    bool mZhPage;              // 中文页面

    int  mEditTextType;        // 输入内容的类型 EditText::INPUTTYPE
    int  mMaxCountWork;        // 中文预选栏最大的数量  5-10

    // 语言 页面 按钮 值
    std::vector<std::map<int, std::string>> mENPageValue;   // 英文页面按钮值
    std::vector<std::map<int, std::string>> mZHPageValue;   // 中文页面按钮值

    OnCompleteListener mCompleteListener;

    std::string              mLastTxt; // 上一次的内容
    std::vector<std::string> mHzList;  // 汉字选择项
    int                      mHzPos;   // 当前尾部位置
    int                      mHzCount; // 当前显示的可选词数量
    int                      mWordCount;
};

#endif
