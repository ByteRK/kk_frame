
#include "R.h"
#include <comm_func.h>
#include "cdroid_keyboard.h"

#ifdef ENABLE_PINYIN2HZ
#include <core/inputmethodmanager.h>
#ifdef CDROID_RUNNING
#include <pinyin/pinyinime.h>
#endif
#endif

#ifdef CDROID_X64
    #define PINYIN_DATA_FILE "dict_pinyin.dat"
    #define USERDICT_FILE "userdict"
#else
    #define PINYIN_DATA_FILE "dict_pinyin_arm.dat"
    #define USERDICT_FILE "userdict"
#endif

using namespace kaidu_ms7;
using namespace cdroid;

#define ENTERTEXT_COLOR 0x210000
#define DESCRIPTION_COLOR 0x4C260000

#define ENTERTEXT_SIZE 24
#define DESCRIPTION_SIZE 24

GooglePinyin *gObjPinyin = nullptr;
std::set<int> gLetterKeys;
//////////////////////////////////////////////////////////////////////////

DECLARE_WIDGET(CdroidKeyBoard)
CdroidKeyBoard::CdroidKeyBoard(int w, int h) : RelativeLayout(w, h) {
    mLoadType         = LT_NULL;
    mWordCount        = 0;
    mCompleteListener = nullptr;
    mDescription = "";
    mEnterText = "";
    mMaxCountWork = 10;
#ifdef ENABLE_PINYIN2HZ
    gObjPinyin = (GooglePinyin *)InputMethodManager::getInstance().getInputMethod("GooglePinyin26");
#endif
}

CdroidKeyBoard::CdroidKeyBoard(Context *ctx, const AttributeSet &attr) : RelativeLayout(ctx, attr) {
    ViewGroup *vg;

    mLoadType         = LT_NULL;
    mWordCount        = 0;
    mCompleteListener = nullptr;

    LayoutInflater::from(ctx)->inflate("@layout/keyboard", this);

    mMaxCountWork = attr.getInt("maxCountWork", 10);

    setVisibility(View::GONE);
#ifdef ENABLE_PINYIN2HZ
    gObjPinyin = (GooglePinyin *)InputMethodManager::getInstance().getInputMethod("GooglePinyin26");
#endif
    mEnterText = "";
    mDescription = "";
    mText      = getView<EditText>(R::id::world_enter);
    mOk        = getView<Button>(R::id::confirm_button);
    mZhPingyin = getView<TextView>(R::id::txt_zh_pingyin);

    vg       = getView<ViewGroup>(R::id::kb_show_container);
    mWorldVG = __dc(ViewGroup, vg->findViewById(R::id::kb_choose_container));
    mWord    = __dc(TextView, mWorldVG->findViewById(R::id::key_world));
    mWord2   = __dc(TextView, mWorldVG->findViewById(R::id::key_world2));
    mWord3   = __dc(TextView, mWorldVG->findViewById(R::id::key_world3));
    mWord4   = __dc(TextView, mWorldVG->findViewById(R::id::key_world4));
    mWord5   = __dc(TextView, mWorldVG->findViewById(R::id::key_world5));
    mWord6   = __dc(TextView, mWorldVG->findViewById(R::id::key_world6));
    mWord7   = __dc(TextView, mWorldVG->findViewById(R::id::key_world7));
    mWord8   = __dc(TextView, mWorldVG->findViewById(R::id::key_world8));
    mWord9   = __dc(TextView, mWorldVG->findViewById(R::id::key_world9));
    mWord10   = __dc(TextView, mWorldVG->findViewById(R::id::key_world10));
    mHide    = vg->findViewById(R::id::btn_hide);
    mPrePage = vg->findViewById(R::id::btn_pre_page);

    mKeyQ = getView<Button>(R::id::key_q);
    mKeyW = getView<Button>(R::id::key_w);
    mKeyE = getView<Button>(R::id::key_e);
    mKeyR = getView<Button>(R::id::key_r);
    mKeyT = getView<Button>(R::id::key_t);
    mKeyY = getView<Button>(R::id::key_y);
    mKeyU = getView<Button>(R::id::key_u);
    mKeyI = getView<Button>(R::id::key_i);
    mKeyO = getView<Button>(R::id::key_o);
    mKeyP = getView<Button>(R::id::key_p);

    vg                 = getView<ViewGroup>(R::id::keyboard_row2);
    mRow2VG            = vg;
    // mLetterRow2Padding = 0;
    mKeyA              = __dc(Button, vg->findViewById(R::id::key_a));
    mKeyS              = __dc(Button, vg->findViewById(R::id::key_s));
    mKeyD              = __dc(Button, vg->findViewById(R::id::key_d));
    mKeyF              = __dc(Button, vg->findViewById(R::id::key_f));
    mKeyG              = __dc(Button, vg->findViewById(R::id::key_g));
    mKeyH              = __dc(Button, vg->findViewById(R::id::key_h));
    mKeyJ              = __dc(Button, vg->findViewById(R::id::key_j));
    mKeyK              = __dc(Button, vg->findViewById(R::id::key_k));
    mKeyL              = __dc(Button, vg->findViewById(R::id::key_l));
    // mKeyLR             = __dc(Button, vg->findViewById(R::id::key_l_right));

    mKeyCase = __dc(Button, findViewById(R::id::key_case));

    mKeyZ      = getView<Button>(R::id::key_z);
    mKeyX      = getView<Button>(R::id::key_x);
    mKeyC      = getView<Button>(R::id::key_c);
    mKeyV      = getView<Button>(R::id::key_v);
    mKeyB      = getView<Button>(R::id::key_b);
    mKeyN      = getView<Button>(R::id::key_n);
    mKeyM      = getView<Button>(R::id::key_m);
    mBackSpace = getView<Button>(R::id::key_backspace);

    mNumber = getView<Button>(R::id::key_number);
    mZhEn   = getView<Button>(R::id::key_zh_en);
    mComma  = getView<Button>(R::id::key_douhao);
    mSpace  = getView<Button>(R::id::key_space);
    mPeriod = getView<Button>(R::id::key_juhao);
    mWrap   = getView<Button>(R::id::key_enter);

    // 初始化
    initValue();

    // 监听
    initListener();

    cdroid::LayoutParams *layoutParam = mNumber->getLayoutParams();
    mNumber->setText(mENPageValue[mPageType][R::id::key_number]);
    mNumberWidth = layoutParam->width;

    cdroid::LayoutParams *zhLayoutParam = mZhEn->getLayoutParams();
    mZhEnWidth                          = zhLayoutParam->width;

    LOG(DEBUG) << "save width. NumberWidth=" << mNumberWidth << " ZhEnWidth=" << mZhEnWidth;    
}

void CdroidKeyBoard::initValue() {
    mPageType          = KB_PT_SMALL;
    // mLetterRow2Padding = 0;
    mHideLetter        = false;
    mZhPage            = false;

    mENPageValue.resize(KB_PT_COUNT);
    mZHPageValue.resize(KB_PT_COUNT);

    mENPageValue[KB_PT_SMALL][R::id::key_q]       = "q";
    mENPageValue[KB_PT_SMALL][R::id::key_w]       = "w";
    mENPageValue[KB_PT_SMALL][R::id::key_e]       = "e";
    mENPageValue[KB_PT_SMALL][R::id::key_r]       = "r";
    mENPageValue[KB_PT_SMALL][R::id::key_t]       = "t";
    mENPageValue[KB_PT_SMALL][R::id::key_y]       = "y";
    mENPageValue[KB_PT_SMALL][R::id::key_u]       = "u";
    mENPageValue[KB_PT_SMALL][R::id::key_i]       = "i";
    mENPageValue[KB_PT_SMALL][R::id::key_o]       = "o";
    mENPageValue[KB_PT_SMALL][R::id::key_p]       = "p";
    mENPageValue[KB_PT_SMALL][R::id::key_a]       = "a";
    mENPageValue[KB_PT_SMALL][R::id::key_s]       = "s";
    mENPageValue[KB_PT_SMALL][R::id::key_d]       = "d";
    mENPageValue[KB_PT_SMALL][R::id::key_f]       = "f";
    mENPageValue[KB_PT_SMALL][R::id::key_g]       = "g";
    mENPageValue[KB_PT_SMALL][R::id::key_h]       = "h";
    mENPageValue[KB_PT_SMALL][R::id::key_j]       = "j";
    mENPageValue[KB_PT_SMALL][R::id::key_k]       = "k";
    mENPageValue[KB_PT_SMALL][R::id::key_l]       = "l";
    // mENPageValue[KB_PT_SMALL][R::id::key_l_right] = "";
    mENPageValue[KB_PT_SMALL][R::id::key_z]       = "z";
    mENPageValue[KB_PT_SMALL][R::id::key_x]       = "x";
    mENPageValue[KB_PT_SMALL][R::id::key_c]       = "c";
    mENPageValue[KB_PT_SMALL][R::id::key_v]       = "v";
    mENPageValue[KB_PT_SMALL][R::id::key_b]       = "b";
    mENPageValue[KB_PT_SMALL][R::id::key_n]       = "n";
    mENPageValue[KB_PT_SMALL][R::id::key_m]       = "m";
    mENPageValue[KB_PT_SMALL][R::id::key_douhao]  = ".";
    mENPageValue[KB_PT_SMALL][R::id::key_juhao]   = "?";
    mENPageValue[KB_PT_SMALL][R::id::key_case]    = "小写";
    mENPageValue[KB_PT_SMALL][R::id::key_number]  = "?123";

    mENPageValue[KB_PT_BIG][R::id::key_q]       = "Q";
    mENPageValue[KB_PT_BIG][R::id::key_w]       = "W";
    mENPageValue[KB_PT_BIG][R::id::key_e]       = "E";
    mENPageValue[KB_PT_BIG][R::id::key_r]       = "R";
    mENPageValue[KB_PT_BIG][R::id::key_t]       = "T";
    mENPageValue[KB_PT_BIG][R::id::key_y]       = "Y";
    mENPageValue[KB_PT_BIG][R::id::key_u]       = "U";
    mENPageValue[KB_PT_BIG][R::id::key_i]       = "I";
    mENPageValue[KB_PT_BIG][R::id::key_o]       = "O";
    mENPageValue[KB_PT_BIG][R::id::key_p]       = "P";
    mENPageValue[KB_PT_BIG][R::id::key_a]       = "A";
    mENPageValue[KB_PT_BIG][R::id::key_s]       = "S";
    mENPageValue[KB_PT_BIG][R::id::key_d]       = "D";
    mENPageValue[KB_PT_BIG][R::id::key_f]       = "F";
    mENPageValue[KB_PT_BIG][R::id::key_g]       = "G";
    mENPageValue[KB_PT_BIG][R::id::key_h]       = "H";
    mENPageValue[KB_PT_BIG][R::id::key_j]       = "J";
    mENPageValue[KB_PT_BIG][R::id::key_k]       = "K";
    mENPageValue[KB_PT_BIG][R::id::key_l]       = "L";
    // mENPageValue[KB_PT_BIG][R::id::key_l_right] = "";
    mENPageValue[KB_PT_BIG][R::id::key_z]       = "Z";
    mENPageValue[KB_PT_BIG][R::id::key_x]       = "X";
    mENPageValue[KB_PT_BIG][R::id::key_c]       = "C";
    mENPageValue[KB_PT_BIG][R::id::key_v]       = "V";
    mENPageValue[KB_PT_BIG][R::id::key_b]       = "B";
    mENPageValue[KB_PT_BIG][R::id::key_n]       = "N";
    mENPageValue[KB_PT_BIG][R::id::key_m]       = "M";
    mENPageValue[KB_PT_BIG][R::id::key_douhao]  = ".";
    mENPageValue[KB_PT_BIG][R::id::key_juhao]   = "?";
    mENPageValue[KB_PT_BIG][R::id::key_case]    = "大写";
    mENPageValue[KB_PT_BIG][R::id::key_number]  = "?123";

    mENPageValue[KB_PT_NUMBER][R::id::key_q]       = "1";
    mENPageValue[KB_PT_NUMBER][R::id::key_w]       = "2";
    mENPageValue[KB_PT_NUMBER][R::id::key_e]       = "3";
    mENPageValue[KB_PT_NUMBER][R::id::key_r]       = "4";
    mENPageValue[KB_PT_NUMBER][R::id::key_t]       = "5";
    mENPageValue[KB_PT_NUMBER][R::id::key_y]       = "6";
    mENPageValue[KB_PT_NUMBER][R::id::key_u]       = "7";
    mENPageValue[KB_PT_NUMBER][R::id::key_i]       = "8";
    mENPageValue[KB_PT_NUMBER][R::id::key_o]       = "9";
    mENPageValue[KB_PT_NUMBER][R::id::key_p]       = "0";
    mENPageValue[KB_PT_NUMBER][R::id::key_a]       = "-";
    mENPageValue[KB_PT_NUMBER][R::id::key_s]       = "/";
    mENPageValue[KB_PT_NUMBER][R::id::key_d]       = ":";
    mENPageValue[KB_PT_NUMBER][R::id::key_f]       = ";";
    mENPageValue[KB_PT_NUMBER][R::id::key_g]       = "(";
    mENPageValue[KB_PT_NUMBER][R::id::key_h]       = ")";
    mENPageValue[KB_PT_NUMBER][R::id::key_j]       = "_";
    mENPageValue[KB_PT_NUMBER][R::id::key_k]       = "$";
    mENPageValue[KB_PT_NUMBER][R::id::key_l]       = "&";
    // mENPageValue[KB_PT_NUMBER][R::id::key_l_right] = "\"";
    mENPageValue[KB_PT_NUMBER][R::id::key_z]       = "~";
    mENPageValue[KB_PT_NUMBER][R::id::key_x]       = ",";
    mENPageValue[KB_PT_NUMBER][R::id::key_c]       = "…";
    mENPageValue[KB_PT_NUMBER][R::id::key_v]       = "@";
    mENPageValue[KB_PT_NUMBER][R::id::key_b]       = "!";
    mENPageValue[KB_PT_NUMBER][R::id::key_n]       = "'";
    mENPageValue[KB_PT_NUMBER][R::id::key_m]       = "\"";
    mENPageValue[KB_PT_NUMBER][R::id::key_douhao]  = ".";
    mENPageValue[KB_PT_NUMBER][R::id::key_juhao]   = "?";
    mENPageValue[KB_PT_NUMBER][R::id::key_case]    = "更多";
    mENPageValue[KB_PT_NUMBER][R::id::key_number]  = "返回";

    mENPageValue[KB_PT_MORE][R::id::key_q]       = "[";
    mENPageValue[KB_PT_MORE][R::id::key_w]       = "]";
    mENPageValue[KB_PT_MORE][R::id::key_e]       = "{";
    mENPageValue[KB_PT_MORE][R::id::key_r]       = "}";
    mENPageValue[KB_PT_MORE][R::id::key_t]       = "#";
    mENPageValue[KB_PT_MORE][R::id::key_y]       = "%";
    mENPageValue[KB_PT_MORE][R::id::key_u]       = "^";
    mENPageValue[KB_PT_MORE][R::id::key_i]       = "*";
    mENPageValue[KB_PT_MORE][R::id::key_o]       = "+";
    mENPageValue[KB_PT_MORE][R::id::key_p]       = "=";
    mENPageValue[KB_PT_MORE][R::id::key_a]       = "ˇ";
    mENPageValue[KB_PT_MORE][R::id::key_s]       = "/";
    mENPageValue[KB_PT_MORE][R::id::key_d]       = "\\";
    mENPageValue[KB_PT_MORE][R::id::key_f]       = "<";
    mENPageValue[KB_PT_MORE][R::id::key_g]       = ">";
    mENPageValue[KB_PT_MORE][R::id::key_h]       = "￥";
    mENPageValue[KB_PT_MORE][R::id::key_j]       = "€";
    mENPageValue[KB_PT_MORE][R::id::key_k]       = "£";
    mENPageValue[KB_PT_MORE][R::id::key_l]       = "₤";
    // mENPageValue[KB_PT_MORE][R::id::key_l_right] = "·";
    mENPageValue[KB_PT_MORE][R::id::key_z]       = "~";
    mENPageValue[KB_PT_MORE][R::id::key_x]       = ",";
    mENPageValue[KB_PT_MORE][R::id::key_c]       = "·";
    mENPageValue[KB_PT_MORE][R::id::key_v]       = "@";
    mENPageValue[KB_PT_MORE][R::id::key_b]       = "!";
    mENPageValue[KB_PT_MORE][R::id::key_n]       = "`";
    mENPageValue[KB_PT_MORE][R::id::key_m]       = "\"";
    mENPageValue[KB_PT_MORE][R::id::key_douhao]  = ".";
    mENPageValue[KB_PT_MORE][R::id::key_juhao]   = "?";
    mENPageValue[KB_PT_MORE][R::id::key_case]    = "123";
    mENPageValue[KB_PT_MORE][R::id::key_number]  = "返回";

    mZHPageValue[KB_PT_SMALL][R::id::key_q]       = "q";
    mZHPageValue[KB_PT_SMALL][R::id::key_w]       = "w";
    mZHPageValue[KB_PT_SMALL][R::id::key_e]       = "e";
    mZHPageValue[KB_PT_SMALL][R::id::key_r]       = "r";
    mZHPageValue[KB_PT_SMALL][R::id::key_t]       = "t";
    mZHPageValue[KB_PT_SMALL][R::id::key_y]       = "y";
    mZHPageValue[KB_PT_SMALL][R::id::key_u]       = "u";
    mZHPageValue[KB_PT_SMALL][R::id::key_i]       = "i";
    mZHPageValue[KB_PT_SMALL][R::id::key_o]       = "o";
    mZHPageValue[KB_PT_SMALL][R::id::key_p]       = "p";
    mZHPageValue[KB_PT_SMALL][R::id::key_a]       = "a";
    mZHPageValue[KB_PT_SMALL][R::id::key_s]       = "s";
    mZHPageValue[KB_PT_SMALL][R::id::key_d]       = "d";
    mZHPageValue[KB_PT_SMALL][R::id::key_f]       = "f";
    mZHPageValue[KB_PT_SMALL][R::id::key_g]       = "g";
    mZHPageValue[KB_PT_SMALL][R::id::key_h]       = "h";
    mZHPageValue[KB_PT_SMALL][R::id::key_j]       = "j";
    mZHPageValue[KB_PT_SMALL][R::id::key_k]       = "k";
    mZHPageValue[KB_PT_SMALL][R::id::key_l]       = "l";
    // mZHPageValue[KB_PT_SMALL][R::id::key_l_right] = "";
    mZHPageValue[KB_PT_SMALL][R::id::key_z]       = "z";
    mZHPageValue[KB_PT_SMALL][R::id::key_x]       = "x";
    mZHPageValue[KB_PT_SMALL][R::id::key_c]       = "c";
    mZHPageValue[KB_PT_SMALL][R::id::key_v]       = "v";
    mZHPageValue[KB_PT_SMALL][R::id::key_b]       = "b";
    mZHPageValue[KB_PT_SMALL][R::id::key_n]       = "n";
    mZHPageValue[KB_PT_SMALL][R::id::key_m]       = "m";
    mZHPageValue[KB_PT_SMALL][R::id::key_douhao]  = "，";
    mZHPageValue[KB_PT_SMALL][R::id::key_juhao]   = "。";
    mZHPageValue[KB_PT_SMALL][R::id::key_case]    = "小写";
    mZHPageValue[KB_PT_SMALL][R::id::key_number]  = "?123";

    mZHPageValue[KB_PT_BIG][R::id::key_q]       = "Q";
    mZHPageValue[KB_PT_BIG][R::id::key_w]       = "W";
    mZHPageValue[KB_PT_BIG][R::id::key_e]       = "E";
    mZHPageValue[KB_PT_BIG][R::id::key_r]       = "R";
    mZHPageValue[KB_PT_BIG][R::id::key_t]       = "T";
    mZHPageValue[KB_PT_BIG][R::id::key_y]       = "Y";
    mZHPageValue[KB_PT_BIG][R::id::key_u]       = "U";
    mZHPageValue[KB_PT_BIG][R::id::key_i]       = "I";
    mZHPageValue[KB_PT_BIG][R::id::key_o]       = "O";
    mZHPageValue[KB_PT_BIG][R::id::key_p]       = "P";
    mZHPageValue[KB_PT_BIG][R::id::key_a]       = "A";
    mZHPageValue[KB_PT_BIG][R::id::key_s]       = "S";
    mZHPageValue[KB_PT_BIG][R::id::key_d]       = "D";
    mZHPageValue[KB_PT_BIG][R::id::key_f]       = "F";
    mZHPageValue[KB_PT_BIG][R::id::key_g]       = "G";
    mZHPageValue[KB_PT_BIG][R::id::key_h]       = "H";
    mZHPageValue[KB_PT_BIG][R::id::key_j]       = "J";
    mZHPageValue[KB_PT_BIG][R::id::key_k]       = "K";
    mZHPageValue[KB_PT_BIG][R::id::key_l]       = "L";
    // mZHPageValue[KB_PT_BIG][R::id::key_l_right] = "";
    mZHPageValue[KB_PT_BIG][R::id::key_z]       = "Z";
    mZHPageValue[KB_PT_BIG][R::id::key_x]       = "X";
    mZHPageValue[KB_PT_BIG][R::id::key_c]       = "C";
    mZHPageValue[KB_PT_BIG][R::id::key_v]       = "V";
    mZHPageValue[KB_PT_BIG][R::id::key_b]       = "B";
    mZHPageValue[KB_PT_BIG][R::id::key_n]       = "N";
    mZHPageValue[KB_PT_BIG][R::id::key_m]       = "M";
    mZHPageValue[KB_PT_BIG][R::id::key_douhao]  = "，";
    mZHPageValue[KB_PT_BIG][R::id::key_juhao]   = "。";
    mZHPageValue[KB_PT_BIG][R::id::key_case]    = "大写";
    mZHPageValue[KB_PT_BIG][R::id::key_number]  = "?123";

    mZHPageValue[KB_PT_NUMBER][R::id::key_q]       = "1";
    mZHPageValue[KB_PT_NUMBER][R::id::key_w]       = "2";
    mZHPageValue[KB_PT_NUMBER][R::id::key_e]       = "3";
    mZHPageValue[KB_PT_NUMBER][R::id::key_r]       = "4";
    mZHPageValue[KB_PT_NUMBER][R::id::key_t]       = "5";
    mZHPageValue[KB_PT_NUMBER][R::id::key_y]       = "6";
    mZHPageValue[KB_PT_NUMBER][R::id::key_u]       = "7";
    mZHPageValue[KB_PT_NUMBER][R::id::key_i]       = "8";
    mZHPageValue[KB_PT_NUMBER][R::id::key_o]       = "9";
    mZHPageValue[KB_PT_NUMBER][R::id::key_p]       = "0";
    mZHPageValue[KB_PT_NUMBER][R::id::key_a]       = "-";
    mZHPageValue[KB_PT_NUMBER][R::id::key_s]       = "/";
    mZHPageValue[KB_PT_NUMBER][R::id::key_d]       = "：";
    mZHPageValue[KB_PT_NUMBER][R::id::key_f]       = "；";
    mZHPageValue[KB_PT_NUMBER][R::id::key_g]       = "（";
    mZHPageValue[KB_PT_NUMBER][R::id::key_h]       = "）";
    mZHPageValue[KB_PT_NUMBER][R::id::key_j]       = "@";
    mZHPageValue[KB_PT_NUMBER][R::id::key_k]       = "“";
    mZHPageValue[KB_PT_NUMBER][R::id::key_l]       = "”";
    // mZHPageValue[KB_PT_NUMBER][R::id::key_l_right] = "”";
    mZHPageValue[KB_PT_NUMBER][R::id::key_z]       = "…";
    mZHPageValue[KB_PT_NUMBER][R::id::key_x]       = "～";
    mZHPageValue[KB_PT_NUMBER][R::id::key_c]       = "、";
    mZHPageValue[KB_PT_NUMBER][R::id::key_v]       = "？";
    mZHPageValue[KB_PT_NUMBER][R::id::key_b]       = "！";
    mZHPageValue[KB_PT_NUMBER][R::id::key_n]       = ".";
    mZHPageValue[KB_PT_NUMBER][R::id::key_m]       = "—";
    mZHPageValue[KB_PT_NUMBER][R::id::key_douhao]  = "，";
    mZHPageValue[KB_PT_NUMBER][R::id::key_juhao]   = "。";
    mZHPageValue[KB_PT_NUMBER][R::id::key_case]    = "更多";
    mZHPageValue[KB_PT_NUMBER][R::id::key_number]  = "返回";

    mZHPageValue[KB_PT_MORE][R::id::key_q]       = "【";
    mZHPageValue[KB_PT_MORE][R::id::key_w]       = "】";
    mZHPageValue[KB_PT_MORE][R::id::key_e]       = "｛";
    mZHPageValue[KB_PT_MORE][R::id::key_r]       = "｝";
    mZHPageValue[KB_PT_MORE][R::id::key_t]       = "#";
    mZHPageValue[KB_PT_MORE][R::id::key_y]       = "%";
    mZHPageValue[KB_PT_MORE][R::id::key_u]       = "^";
    mZHPageValue[KB_PT_MORE][R::id::key_i]       = "*";
    mZHPageValue[KB_PT_MORE][R::id::key_o]       = "+";
    mZHPageValue[KB_PT_MORE][R::id::key_p]       = "=";
    mZHPageValue[KB_PT_MORE][R::id::key_a]       = "_";
    mZHPageValue[KB_PT_MORE][R::id::key_s]       = "\\";
    mZHPageValue[KB_PT_MORE][R::id::key_d]       = "|";
    mZHPageValue[KB_PT_MORE][R::id::key_f]       = "《";
    mZHPageValue[KB_PT_MORE][R::id::key_g]       = "》";
    mZHPageValue[KB_PT_MORE][R::id::key_h]       = "￥";
    mZHPageValue[KB_PT_MORE][R::id::key_j]       = "$";
    mZHPageValue[KB_PT_MORE][R::id::key_k]       = "&";
    mZHPageValue[KB_PT_MORE][R::id::key_l]       = "·";
    // mZHPageValue[KB_PT_MORE][R::id::key_l_right] = "’";
    mZHPageValue[KB_PT_MORE][R::id::key_z]       = "…";
    mZHPageValue[KB_PT_MORE][R::id::key_x]       = "～";
    mZHPageValue[KB_PT_MORE][R::id::key_c]       = "｀";
    mZHPageValue[KB_PT_MORE][R::id::key_v]       = "？";
    mZHPageValue[KB_PT_MORE][R::id::key_b]       = "！";
    mZHPageValue[KB_PT_MORE][R::id::key_n]       = ".";
    mZHPageValue[KB_PT_MORE][R::id::key_m]       = "’";
    mZHPageValue[KB_PT_MORE][R::id::key_douhao]  = "，";
    mZHPageValue[KB_PT_MORE][R::id::key_juhao]   = "。";
    mZHPageValue[KB_PT_MORE][R::id::key_case]    = "123";
    mZHPageValue[KB_PT_MORE][R::id::key_number]  = "返回";

    if (gLetterKeys.empty()) {
       if(gObjPinyin == nullptr) gObjPinyin = new GooglePinyin("");
        gObjPinyin->load_dicts(PINYIN_DATA_FILE, USERDICT_FILE);

#ifdef CDROID_RUNNING
        ime_pinyin::im_enable_ym_as_szm(true);
#endif

        gLetterKeys.insert(R::id::key_a);
        gLetterKeys.insert(R::id::key_b);
        gLetterKeys.insert(R::id::key_c);
        gLetterKeys.insert(R::id::key_d);
        gLetterKeys.insert(R::id::key_e);
        gLetterKeys.insert(R::id::key_f);
        gLetterKeys.insert(R::id::key_g);
        gLetterKeys.insert(R::id::key_h);
        gLetterKeys.insert(R::id::key_i);
        gLetterKeys.insert(R::id::key_j);
        gLetterKeys.insert(R::id::key_k);
        gLetterKeys.insert(R::id::key_l);
        gLetterKeys.insert(R::id::key_m);
        gLetterKeys.insert(R::id::key_n);
        gLetterKeys.insert(R::id::key_o);
        gLetterKeys.insert(R::id::key_p);
        gLetterKeys.insert(R::id::key_q);
        gLetterKeys.insert(R::id::key_r);
        gLetterKeys.insert(R::id::key_s);
        gLetterKeys.insert(R::id::key_t);
        gLetterKeys.insert(R::id::key_u);
        gLetterKeys.insert(R::id::key_v);
        gLetterKeys.insert(R::id::key_w);
        gLetterKeys.insert(R::id::key_x);
        gLetterKeys.insert(R::id::key_y);
        gLetterKeys.insert(R::id::key_z);

        LOG(DEBUG) << "insert pinyin keys. count=" << gLetterKeys.size();
    }
}

void CdroidKeyBoard::initListener() {
    mText->setTextWatcher(std::bind(&CdroidKeyBoard::onTextChanged, this, std::placeholders::_1));
    mKeyA->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyB->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyC->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyD->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyE->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyF->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyG->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyH->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyI->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyJ->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyK->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyL->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyM->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyN->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyO->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyP->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyQ->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyR->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyS->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyT->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyU->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyV->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyW->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyX->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyY->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyZ->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));

    mComma->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mPeriod->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mKeyCase->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mNumber->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mZhEn->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));

    mSpace->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mWrap->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));

    mHide->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mPrePage->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));
    mOk->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));

    mBackSpace->setOnClickListener(std::bind(&CdroidKeyBoard::onClick, this, std::placeholders::_1));

    mWord->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord2->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord3->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord4->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord5->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord6->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord7->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord8->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord9->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));
    mWord10->setOnClickListener(std::bind(&CdroidKeyBoard::onClickWord, this, std::placeholders::_1));

    mOnLayoutChangeListener = std::bind(&CdroidKeyBoard::onLayoutChanged, this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,
            std::placeholders::_4,std::placeholders::_5,std::placeholders::_6,std::placeholders::_7,std::placeholders::_8,std::placeholders::_9);
    mWord->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord2->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord3->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord4->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord5->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord6->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord7->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord8->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord9->addOnLayoutChangeListener(mOnLayoutChangeListener);
    mWord10->addOnLayoutChangeListener(mOnLayoutChangeListener);
}

void CdroidKeyBoard::onClick(View &v) {
    onClickID(v.getId());
}

void CdroidKeyBoard::onClickID(int id) {
    LOG(DEBUG) << "keyboard click. id=" << id;

    if (id == R::id::key_case) {
        if (mPageType == KB_PT_SMALL || mPageType == KB_PT_BIG) {
            mLastTxt += mWord->getText();
            setText(mLastTxt);
            clearCandidate();
            mHzList.clear();
            mZhPingyin->setText("");
            mPageType = (mPageType == KB_PT_SMALL ? KB_PT_BIG : KB_PT_SMALL);
        } else {
            if (mEditTextType == EditText::TYPE_NUMBER && mPageType == KB_PT_NUMBER) { return; }
            mLastTxt += mWord->getText();
            setText(mLastTxt);
            clearCandidate();
            mHzList.clear();
            mZhPingyin->setText("");
            mPageType = (mPageType == KB_PT_NUMBER ? KB_PT_MORE : KB_PT_NUMBER);
        }
        trun2NextPage();
    } else if (id == R::id::key_zh_en) {
        if (mEditTextType == EditText::TYPE_PASSWORD) return;
            mLastTxt += mWord->getText();
            setText(mLastTxt);
            clearCandidate();
            mHzList.clear();
            mZhPingyin->setText("");
        mZhPage = !mZhPage;
        mZhEn->setText(mZhPage ? "中文" : "英文");
        if (mZhPage) {
            mComma->setText(mZHPageValue[mPageType][R::id::key_douhao]);
            mPeriod->setText(mZHPageValue[mPageType][R::id::key_juhao]);
        } else {
            mComma->setText(mENPageValue[mPageType][R::id::key_douhao]);
            mPeriod->setText(mENPageValue[mPageType][R::id::key_juhao]);
        }
    } else if (id == R::id::key_number) {
            mLastTxt += mWord->getText();
            setText(mLastTxt);
            clearCandidate();
            mHzList.clear();
            mZhPingyin->setText("");
        if (mEditTextType == EditText::TYPE_NUMBER && mPageType == KB_PT_NUMBER) { return; }

        cdroid::MarginLayoutParams *layoutParam = __dc(MarginLayoutParams, mNumber->getLayoutParams());

        mHideLetter = !mHideLetter;

        if (mHideLetter) {
            // 切换到数字输入
            mLastPageType = mPageType;
            mPageType     = KB_PT_NUMBER;
        } else {
            // 返回中英文输入
            mPageType = mLastPageType;
        }

        // 字母第2行内间距调整
        // if (mLetterRow2Padding == 0) { mLetterRow2Padding = mRow2VG->getPaddingLeft(); }
        // mRow2VG->setPadding(mHideLetter ? 0 : mLetterRow2Padding, mRow2VG->getPaddingTop(), mRow2VG->getPaddingRight(),
        //                     mRow2VG->getPaddingBottom());

        // 数字 / 返回 宽度调整
        mNumber->setText(mENPageValue[mPageType][R::id::key_number]);
        layoutParam->width = mHideLetter ? (mNumberWidth + layoutParam->rightMargin + mZhEnWidth) : mNumberWidth;
        mNumber->setLayoutParams(layoutParam);
        mZhEn->setVisibility(mHideLetter ? View::GONE : View::VISIBLE);

        trun2NextPage();
    } else if (id == R::id::btn_hide) {
        if (mHzList.size()) {
            int p, i;
            mHzCount = 0;
            clearCandidate();
            for (p = mHzPos + 1, i = 1; p < mHzList.size() && i <= mMaxCountWork; p++, i++) {
                switch (i) {
                case 1: setText(mHzList[p], mWord, 10, 16, 32); break;
                case 2: setText(mHzList[p], mWord2, 10, 16, 32); break;
                case 3: setText(mHzList[p], mWord3, 10, 16, 32); break;
                case 4: setText(mHzList[p], mWord4, 10, 16, 32); break;
                case 5: setText(mHzList[p], mWord5, 10, 16, 32); break;
                case 6: setText(mHzList[p], mWord6, 10, 16, 32); break;
                case 7: setText(mHzList[p], mWord7, 10, 16, 32); break;
                case 8: setText(mHzList[p], mWord8, 10, 16, 32); break;
                case 9: setText(mHzList[p], mWord9, 10, 16, 32); break;
                case 10: setText(mHzList[p], mWord10, 10, 16, 32); break;
                }
                mHzPos = p;
                mHzCount++;
            }
            if (mHzPos >= mMaxCountWork && mPrePage->getVisibility() == View::GONE) { mPrePage->setVisibility(View::VISIBLE); }
            if (mHzPos >= mHzList.size() - 1) {
                mHide->setEnabled(false); // 已经到尾页
            }
        } else {
            // setVisibility(View::GONE);
        }
    } else if (id == R::id::btn_pre_page) {
        // 上一页
        int p, i;
        p        = mHzPos - mHzCount - mMaxCountWork + 1;
        mHzCount = 0;
        for (i = 1; p >= 0 && p < mHzList.size() && i <= mMaxCountWork; p++, i++) {
            switch (i) {
            case 1: setText(mHzList[p], mWord, 10, 16, 32); break;
            case 2: setText(mHzList[p], mWord2, 10, 16, 32); break;
            case 3: setText(mHzList[p], mWord3, 10, 16, 32); break;
            case 4: setText(mHzList[p], mWord4, 10, 16, 32); break;
            case 5: setText(mHzList[p], mWord5, 10, 16, 32); break;
            case 6: setText(mHzList[p], mWord6, 10, 16, 32); break;
            case 7: setText(mHzList[p], mWord7, 10, 16, 32); break;
            case 8: setText(mHzList[p], mWord8, 10, 16, 32); break;
            case 9: setText(mHzList[p], mWord9, 10, 16, 32); break;
            case 10: setText(mHzList[p], mWord10, 10, 16, 32); break;
            }
            mHzPos = p;
            mHzCount++;
        }
        if (mHzPos < mMaxCountWork && mPrePage->getVisibility() == View::VISIBLE) { mPrePage->setVisibility(View::GONE); }
        if (!mHide->isEnabled()) { mHide->setEnabled(true); }
    } else if (id == R::id::confirm_button) {
        if (mCompleteListener) { mCompleteListener(mEnterText); }
        setVisibility(View::GONE);
    } else {
        switch (mEditTextType) {
        case EditText::TYPE_PASSWORD: enterPassword(id); break;
        case EditText::TYPE_IP:
        case EditText::TYPE_NUMBER: enterNumber(id); break;
        case EditText::TYPE_NONE:
        case EditText::TYPE_ANY:
        case EditText::TYPE_TEXT: enterText(id); break;
        default:
            // 英文
            enterEnglish(id);
            break;
        }
        LOGE("mEditTextType = %d",mEditTextType);
    }
}

void CdroidKeyBoard::onClickWord(View &v) {
    TextView   *txtView = __dc(TextView, &v);
    std::string txt     = txtView->getText();
    if (txt.empty()) return;

    mLastTxt += txtView->getText();
    setText(mLastTxt);
    clearCandidate();
    mHzList.clear();
    mZhPingyin->setText("");
    mPrePage->setVisibility(View::GONE);
    gObjPinyin->close_search();
}

void CdroidKeyBoard::onLayoutChanged(View&v,int l,int t,int w,int h,int oldL,int oldT,int oldW,int oldH){
    if(l + w >= (mWorldVG->getRight() - mWorldVG->getPaddingRight())){
        v.setVisibility(View::INVISIBLE);
    }else{
        v.setVisibility(View::VISIBLE);
    }   
}

void CdroidKeyBoard::trun2NextPage() {
    std::vector<std::map<int, std::string>> *lanPtr;

    lanPtr = mZhPage ? (&mZHPageValue) : (&mENPageValue);

    for (auto &kv : lanPtr->at(mPageType)) {
        Button *v = __dc(Button, findViewById(kv.first));

        if (!v) continue;

        if (kv.second.empty()) {
            v->setVisibility(View::GONE);
            LOGE("v->id = %d  mZhPage = %d",v->getId(),mZhPage);
        } else {
            v->setText(kv.second);
            v->setVisibility(View::VISIBLE);
        }
    }
}

void CdroidKeyBoard::setOnCompleteListener(OnCompleteListener l) {
    mCompleteListener = l;
}

void CdroidKeyBoard::showWindow() {
    mLastTxt = "";
    setEnterText("");
    clearCandidate();

    mText->requestFocus();

    if (mEditTextType == EditText::TYPE_NUMBER) { onClickID(R::id::key_number); }

    mHzList.clear();
    mZhPingyin->setText("");
    mPrePage->setVisibility(View::GONE);
    gObjPinyin->close_search();

    setVisibility(View::VISIBLE);
}

void CdroidKeyBoard::setEditType(int editType) {
    mEditTextType = editType;
}

std::string CdroidKeyBoard::getEnterText(){
    return mEnterText;
}

void CdroidKeyBoard::setDescription(const std::string &txt){
    mDescription = txt;
}   

void CdroidKeyBoard::setEditText(const std::string &txt){
    int wordCount = wordLen(txt.c_str());
    mLastTxt = txt;
    if (mWordCount > 0 && wordCount > mWordCount) {
        mLastTxt = getWord(txt.c_str(), mWordCount);
        setText(mLastTxt, mText, 40, 16, 34);
    } else {
        setText(txt, mText, 40, 16, 34);
    }
}

void CdroidKeyBoard::setText(const std::string &txt) {
    int wordCount = wordLen(txt.c_str());
    if (mWordCount > 0 && wordCount > mWordCount) {
        mLastTxt = getWord(txt.c_str(), mWordCount);
        setText(mLastTxt, mText, 40, 16, 34);
    } else {
        setText(txt, mText, 40, 16, 34);
    }
    LOGI("txt.size() = %d  wordCount = %d/%d",txt.size(),wordCount,mWordCount);
}

void CdroidKeyBoard::setText(const std::string &txt, TextView *txtView, int maxLen /* = 40 */, int minSize /* = 16 */,
                            int maxSize /* = 34 */) {
    txtView->setTextSize(txt.size() >= maxLen ? (((maxSize - (int)(txt.size()-maxLen)) >= minSize?(maxSize - (int)(txt.size()-maxLen)):minSize)) : maxSize);
    if(txtView==mText){
        setEnterText(txt);
    }else{
        txtView->setText(txt);
        txtView->requestLayout();
    }
}

std::string &CdroidKeyBoard::decStr(std::string &txt) {
    unsigned char _last;

    if (txt.empty()) return txt;

    txt = decLastWord(txt.c_str());
    return txt;
}

void CdroidKeyBoard::setEnterText(const std::string& txt) {
    mEnterText = txt;
    if (mEnterText.empty() && !mDescription.empty()) {
        mText->setTextColor(DESCRIPTION_COLOR);
        mText->setTextSize(DESCRIPTION_SIZE);
        mText->setText(mDescription);
        mText->setCaretPos(0);
    } else {
        mText->setTextColor(ENTERTEXT_COLOR);
        mText->setTextSize(ENTERTEXT_SIZE);
        mText->setText(mEnterText + " ");
        mText->setCaretPos(wordLen(txt.c_str()));
    }
}

void CdroidKeyBoard::clearCandidate(){
    mWord->setText("");
    mWord2->setText("");
    mWord3->setText("");
    mWord4->setText("");
    mWord5->setText("");
    mWord6->setText("");
    mWord7->setText("");
    mWord8->setText("");
    mWord9->setText("");
    mWord10->setText("");
}

void CdroidKeyBoard::decText(TextView *txtView) {
    if (!txtView) return;
    std::string txt = txtView->getText();
    if(txtView->getId()==R::id::world_enter) txt = mEnterText;
    if (txt.empty()) return;

    if (txtView->getId() == R::id::world_enter) {
        std::string decText = decStr(txt);
        if(mWord->getText().empty())  mLastTxt = decText;
        setText(decText);
    } else {
        setText(decStr(txt), txtView, 15, 16, 32);
    }
}

void CdroidKeyBoard::enterPassword(int keyID) {
    switch (keyID) {
    case R::id::key_space: mLastTxt += " "; break;
    case R::id::key_backspace: decStr(mLastTxt); break;
    default:{
        auto keyValue = mENPageValue[mPageType].find(keyID);
        if(keyValue != mENPageValue[mPageType].end())   mLastTxt += keyValue->second;
        break;
    }
    }
    setText(mLastTxt);
}

void CdroidKeyBoard::enterEnglish(int keyID) {
    if (keyID == R::id::key_space) {
        // 首词输入并加空格
        mLastTxt += " ";
        setText(mLastTxt);
        clearCandidate();
        return;
    }

    if (keyID == R::id::key_backspace) {
        decText(mText);
        decText(mWord);
        decText(mWord2);
        decText(mWord3);
        decText(mWord4);
        decText(mWord5);
        decText(mWord6);
        decText(mWord7);
        decText(mWord8);
        decText(mWord9);
        decText(mWord10);
        return;
    }

    if (gLetterKeys.find(keyID) != gLetterKeys.end()) {
        std::string keyStr;
        
        std::string firstStr = mWord->getText();
        std::string editTextStr  = mText->getText();
        if((mPageType == KB_PT_NUMBER || mPageType == KB_PT_MORE) && mZhPage){
            auto keyValue = mZHPageValue[mPageType].find(keyID);
            if(keyValue != mZHPageValue[mPageType].end() && (editTextStr == mDescription || (mWordCount > 0 && wordLen(editTextStr.c_str()) < mWordCount+1)))
                keyStr = keyValue->second;
            else keyStr = "";
        }else{
            auto keyValue = mENPageValue[mPageType].find(keyID);
            if(keyValue != mENPageValue[mPageType].end() && (editTextStr == mDescription || (mWordCount > 0 && wordLen(editTextStr.c_str()) < mWordCount+1))) 
                keyStr = keyValue->second;
            else keyStr = "";
        }
        LOGV("mText = %s mLastTxt = %s",mText->getText().c_str(),mLastTxt.c_str());
        
        // std::string keyStr   = mENPageValue[mPageType][keyID];   

        setText(firstStr + keyStr, mWord, 16, 16, 32);

        // 首字母大写
        if (firstStr.empty()) {
            mWord2->setText(toUpper(keyStr));
        } else {
            setText(mWord2->getText() + keyStr, mWord2, 12, 16, 32);
        }

        // 全部大写
        setText(mWord3->getText() + toUpper(keyStr), mWord3, 12, 16, 32);

        setText(mLastTxt + mWord->getText());

        return;
    }

    // 符号
    mLastTxt += mWord->getText();
    if(mZhPage){
        auto keyValue = mZHPageValue[mPageType].find(keyID);
        if (keyValue != mZHPageValue[mPageType].end())   mLastTxt += keyValue->second;
    }else{
        auto keyValue = mENPageValue[mPageType].find(keyID);
        if (keyValue != mENPageValue[mPageType].end())   mLastTxt += keyValue->second;
    }

    // mLastTxt += mWord->getText() + mENPageValue[mPageType][keyID];
    setText(mLastTxt);
    clearCandidate();

    mHzList.clear();
    mZhPingyin->setText("");
    mPrePage->setVisibility(View::GONE);
    gObjPinyin->close_search();
}

void CdroidKeyBoard::enterNumber(int keyID) {
    switch (keyID) {
    case R::id::key_q:
    case R::id::key_w:
    case R::id::key_e:
    case R::id::key_r:
    case R::id::key_t:
    case R::id::key_y:
    case R::id::key_u:
    case R::id::key_i:
    case R::id::key_o:
    case R::id::key_p:{
        auto keyValue = mENPageValue[KB_PT_SMALL].find(keyID);
        if(keyValue != mENPageValue[KB_PT_SMALL].end()) setText(mLastTxt + keyValue->second);
        else                                            setText(mLastTxt);
        break; 
    } 
    case R::id::key_backspace: decText(mText); break;
    }
}

void CdroidKeyBoard::enterText(int keyID) {
    // 中文，按字母
    if (mZhPage && mPageType == KB_PT_SMALL) {
        if (keyID == R::id::key_backspace) {
            // 删除拼音
            std::string pinyin = mZhPingyin->getText();
            if (pinyin.size() > 0) {
                decStr(pinyin);
                mZhPingyin->setText(pinyin);
                if (pinyin.size() > 0) {
                    goto show_hanzi;
                } else {
                    clearCandidate();
                    mHzList.clear();
                    mPrePage->setVisibility(View::GONE);
                    gObjPinyin->close_search();
                }
            } else {
                decText(mText);
            }
        } else if (gLetterKeys.find(keyID) != gLetterKeys.end()) {
            if(mZhPingyin->getText().size() >= 14){
                LOGE("ping yin len over 16");
                return;
            };
            // 拼音
        show_hanzi:
            std::string pinyin = mZhPingyin->getText();
            auto keyValue = mENPageValue[KB_PT_SMALL].find(keyID);
            if(keyValue != mENPageValue[KB_PT_SMALL].end()){
                pinyin += keyValue->second;
            }   

            int hz_num = 0;
            mHzList.clear();
            mPrePage->setVisibility(View::GONE);
            if ((hz_num = gObjPinyin->search(pinyin, mHzList)) > 0) {
                setText(mHzList[0], mWord, 10, 16, 32);
                mHzPos = 0;
                if (hz_num > 1) {
                    setText(mHzList[1], mWord2, 10, 16, 32);
                    mHzPos = 1;
                }
                if (hz_num > 2) {
                    setText(mHzList[2], mWord3, 10, 16, 32);
                    mHzPos = 2;
                }
                if (hz_num > 3) {
                    setText(mHzList[3], mWord4, 10, 16, 32);
                    mHzPos = 3;
                }
                if (hz_num > 4) {
                    setText(mHzList[4], mWord5, 10, 16, 32);
                    mHzPos = 4;
                }
                if (hz_num > 5 && mMaxCountWork > 5) {
                    setText(mHzList[5], mWord6, 10, 16, 32);
                    mHzPos = 5;
                }
                if (hz_num > 6 && mMaxCountWork > 6) {
                    setText(mHzList[6], mWord7, 10, 16, 32);
                    mHzPos = 6;
                }
                if (hz_num > 7 && mMaxCountWork > 7) {
                    setText(mHzList[7], mWord8, 10, 16, 32);
                    mHzPos = 7;
                }
                if (hz_num > 8 && mMaxCountWork > 8) {
                    setText(mHzList[8], mWord9, 10, 16, 32);
                    mHzPos = 8;
                }
                if (hz_num > 9 && mMaxCountWork > 9) {
                    setText(mHzList[9], mWord10, 10, 16, 32);
                    mHzPos = 9;
                }
            }

            if (mZhPingyin->getVisibility() != View::VISIBLE) { mZhPingyin->setVisibility(View::VISIBLE); }
            mZhPingyin->setText(pinyin);
            if(hz_num <= mMaxCountWork)     mHide->setEnabled(false);
            else                            mHide->setEnabled(true);
            return;
        } else {
            // 符号：选择第1个直接输入
            mLastTxt += mWord->getText();
            auto keyValue = mZHPageValue[mPageType].find(keyID);
            if(keyValue != mZHPageValue[mPageType].end()){
                mLastTxt += keyValue->second;
            }  
            // mLastTxt += mWord->getText() + mZHPageValue[mPageType][keyID];
            setText(mLastTxt);
            clearCandidate();
            mHzList.clear();
            mZhPingyin->setText("");
            mPrePage->setVisibility(View::GONE);
            gObjPinyin->close_search();
        }
    } else {
        enterEnglish(keyID);
    }
}

void CdroidKeyBoard::onTextChanged(EditText& ls) {
}

void CdroidKeyBoard::setLoadType(LoadType lt) {
    if (mLoadType != LT_NULL)
        return;

    mLoadType = lt;
    switch (lt) {
        case LT_NUMBER: onClickID(R::id::key_number); break;
        case LT_CN: onClickID(R::id::key_zh_en); break;
        default: break; // 默认英文
    }
}
