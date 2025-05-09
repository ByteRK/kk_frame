/*
 * @Author: hanakami
 * @Date: 2025-05-09 09:54:05
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-09 09:54:19
 * @FilePath: /hana_frame/src/buzzer/music_player.h
 * @Description: 蜂鸣器播放类
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */
#ifndef __music_player_h__
#define __music_player_h__

#include <core/looper.h>
#include <common.h>


#define g_music MusicPlayer::ins()

struct SoundCombination {
    std::vector<std::string> frequencies; // 声音频率
    std::vector<int> durations;            // 持续时间（毫秒）
    // 构造函数
    SoundCombination(const std::vector<std::string>& f, const std::vector<int>& d)
        : frequencies(f), durations(d) {}
    // 计算蜂鸣器电源开通时间
    std::vector<int> calculatePowerOnTimes() const {
        std::vector<int> powerOnTimes;
        for (int duration : durations) {
            powerOnTimes.push_back(duration / 3); // 电源开通时间为时长的1/3
        }
        return powerOnTimes;
    }
};
class MusicPlayer : public cdroid::EventHandler {
public:
    static MusicPlayer* ins();
    int                 init();
    int                 play(const std::string& pathFile);

protected:
    virtual int checkEvents();
    virtual int handleEvents();

    static void* onThreadPlay(void* lpvoid);
    void         onStart();
    void         onPlay();
    void         executeCommand(const char *command);
    void         setBeep(bool beep, int period = 200);

public:
    void powerUp();           // 上电音         频率	M3	H3 时长ms	250	250
    void powerOn();           // 开机音         频率	H1	H1	M7	M6	H3	M6 时长	380	190	190	190	190	760     
    void powerOff();          // 关机音         频率	M6	M6	H3	M6	M7	H1 时长	380	190	190	190	190	760
    void btnValid();          // 有效点触   
    void btnInvalid();        // 无效点触    
    void operationValid();    // 操作成功
    void touchValid();        // 滑触操作

    void errorValid();

    void completeValid();

    void tipValid();

    void tip2Valid();

    void alarmValid();

    void lockValid();

    void unlockValid();

protected:
    MusicPlayer();
    ~MusicPlayer();

private:
    int                    mPlayStatus;
    bool                   mbeepStatus; 
    int                    mbeepPeriod;   
    std::string            mPathFile;
    std::vector<SoundCombination> soundCombosWait;

    int64_t          mMusicTimeStart; // 音乐播放时间
    int64_t          mMusicSingleTime; //单音播放时间
    int              mMusicBeat; //音乐节拍器
    std::map<std::string, int> frequencyMap = {
        {"L1", 523}, {"RL1", 554}, {"DM5", 1475}, {"M1", 1046},
        {"L2", 587}, {"RL2", 622}, {"DM6", 1661}, {"M2", 1175},
        {"L3", 659}, {"RL4", 740}, {"DM7", 1865}, {"M3", 1318},
        {"L4", 698}, {"RL5", 831}, {"M4", 1397},
        {"L5", 784}, {"RL6", 932}, {"M5", 1568},
        {"L6", 880}, {"M6", 1760},
        {"L7", 988}, {"M7", 1976},
        {"RM1", 1109}, {"H1", 2092}, {"RH1", 2216}, {"L0", 0},
        {"RM2", 1245}, {"H2", 2348}, {"RH2", 2488},
        {"RM4", 1480}, {"H3", 2636}, {"RH4", 2958},
        {"RM5", 1661}, {"H4", 2792}, {"RH5", 3322},
        {"RM6", 1865}, {"H5", 3136}, {"RH6", 3729},
        {"H6", 3520}, {"H7", 3952}
    };
    std::vector<SoundCombination> soundCombos = {
        // 上电音
        SoundCombination({"M3", "H3"}, {250, 250}),
        
        // 开机音
        SoundCombination({"H1", "H1", "M7", "M6", "H3", "M6"}, {380, 190, 190, 190, 190, 760}),
        
        // 关机音
        SoundCombination({"M6", "M6", "H3", "M6", "M7", "H1"}, {380, 190, 190, 190, 190, 760}),
        
        // 解锁
        SoundCombination({"RM5", "M7", "H3", "RH5"}, {80, 80, 80, 500}),
        
        // 锁定
        SoundCombination({"RH5", "H3", "M7", "RM5"}, {80, 80, 80, 500}),
        
        // 有效点触
        SoundCombination({"M7"}, {500}),
        
        // 无效点触
        SoundCombination({"M4", "M7"}, {120, 120}),
        
        // 程序启动音
        SoundCombination({"M7", "M3", "RH4", "H6"}, {130, 130, 130, 260}),
        
        // 暂停
        SoundCombination({"M3", "M5", "L0", "L0"}, {60, 60, 60, 60}),
        
        // 划控操作
        SoundCombination({"M3", "M4", "RM4", "M5", "RM5", "M6", "RM6", "M7", "H1", "RH1", "H2", "RH2", "H3"},
                         {250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250}),
        
        // 程序完成
        SoundCombination({"H1", "M7", "H1", "H3", "H5", "H4", "H3", "M6", "L0", "H2", "H1", "M2", "H3", "M4", "H5", "H4"},
                         {250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250}),
        
        // 新消息
        SoundCombination({"RH5", "M7", "RH4", "L0"}, {130, 130, 130, 130}),
        
        // 操作成功
        SoundCombination({"M7", "M3", "RM4", "RM5"}, {130, 130, 130, 130}),
        
        // 操作失败
        SoundCombination({"RH4", "H2", "M7", "RM5"}, {130, 130, 130, 130}),
        
        // 谨慎操作
        SoundCombination({"M2", "M1"}, {130, 130}),
        
        // 一般情况提醒音
        SoundCombination({"M3", "RM6", "L0", "L0", "M3", "RM6", "L0", "L0"},
                         {150, 300, 310, 310, 150, 300, 310, 310}),
        
        // 紧急提醒音
        SoundCombination({"M7", "M2", "M4", "L0", "M7", "M2", "M4", "L0", "M7", "M2", "M4", "L0", "M7", "M2", "M4", "L0"},
                         {150, 150, 150, 200, 150, 150, 150, 200, 150, 150, 150, 800, 150, 150, 150, 200}),
        
        // 闹钟
        SoundCombination({"M3", "RM1", "RM5", "L7", "M3", "RM1", "RM5", "L7", "M3", "RM1", "RM5", "L7", "RM4", "L0", "L0", "L0"},
                         {130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 130, 630, 130, 130, 130}),
        };
};


#endif

