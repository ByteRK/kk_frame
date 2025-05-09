/*
 * @Author: hanakami
 * @Date: 2025-05-09 09:54:05
 * @email: hanakami@163.com
 * @LastEditTime: 2025-05-09 09:54:25
 * @FilePath: /hana_frame/src/buzzer/music_player.cc
 * @Description: 
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

#include "music_player.h"
#include "config_mgr.h"
#include <pthread.h>
#include <ghc/filesystem.hpp>

enum MusicStatus {
    MUSIC_STATUS_NULL = 0,
    MUSIC_STATUS_PLAY,
    MUSIC_STATUS_OVER,
};

////////////////////////////////////////////////////////////////////////////
MusicPlayer* MusicPlayer::ins() {
    static MusicPlayer ss;
    return &ss;
}

MusicPlayer::MusicPlayer() {
    mbeepStatus = 0; 
    mbeepPeriod = 0; 
    mPlayStatus = MUSIC_STATUS_OVER;
    cdroid::Looper::getMainLooper()->addEventHandler(this);
}

MusicPlayer::~MusicPlayer() {
    cdroid::Looper::getMainLooper()->removeEventHandler(this);
}

int MusicPlayer::init() {
    mPlayStatus = MUSIC_STATUS_OVER;
    return 0;
}

int MusicPlayer::play(const std::string& pathFile) {

    return 0;
}

void MusicPlayer::onStart() {
    pthread_t thid;
    pthread_create(&thid, NULL, MusicPlayer::onThreadPlay, this);
    mPlayStatus = MUSIC_STATUS_PLAY;
}

void* MusicPlayer::onThreadPlay(void* lpvoid) {
    MusicPlayer* pthis = (MusicPlayer*)lpvoid;
    pthis->onPlay();
    return 0;
}

void MusicPlayer::onPlay() {
    mPlayStatus = MUSIC_STATUS_OVER;
}

int MusicPlayer::checkEvents() {
    int64_t now_time_tick = SystemClock::uptimeMillis();
    if (soundCombosWait.size() > 0) {
        if (mPlayStatus == MUSIC_STATUS_OVER) {
            mMusicBeat = 0;
            mPlayStatus = MUSIC_STATUS_PLAY;
            mMusicTimeStart = SystemClock::uptimeMillis();
            mMusicSingleTime = SystemClock::uptimeMillis();
            if (soundCombosWait.size() > 0 && 
                soundCombosWait[0].frequencies.size() > mMusicBeat) {
                auto it = frequencyMap.find(soundCombosWait[0].frequencies[mMusicBeat]);
                if (it != frequencyMap.end()) {
                    setBeep(true,it->second);
                }
            }
        } else {
            // if(now_time_tick - mMusicSingleTime >= soundCombosWait[0].durations[mMusicBeat]/3){
            //     setBeep(false);
                if(now_time_tick - mMusicSingleTime >= soundCombosWait[0].durations[mMusicBeat]){
                    mMusicBeat++;
                    mMusicSingleTime = now_time_tick;
                    if(mMusicBeat > soundCombosWait[0].frequencies.size()-1){
                        return 1;
                    }else{
                        auto it = frequencyMap.find(soundCombosWait[0].frequencies[mMusicBeat]);
                        if (it != frequencyMap.end()) {
                            setBeep(true,it->second);
                        }
                    } 
                }
            // }

        }
    }
    return 0;
}

int MusicPlayer::handleEvents() {
    // soundCombosWait.erase(soundCombosWait.begin());
    setBeep(false);
    LOGE("handleEvents");
    soundCombosWait.clear();
    mMusicBeat = 0;
    mPlayStatus = MUSIC_STATUS_OVER;
    return 1;
}

void MusicPlayer::executeCommand(const char* command) {
    std::cout << "Executing command: " << command << std::endl;
    int result = system(command);
    if (result != 0) {
        std::cerr << "Error executing command: " << command << " (code: " << result << ")" << std::endl;
    }
}

void MusicPlayer::setBeep(bool beep,int period){
    if(mbeepStatus == beep && mbeepPeriod == period){
        return;  // 无需重复设置
    }
    
    mbeepStatus = beep;
    mbeepPeriod = period;
    if(period == 0){
        char cmd0[100];
        snprintf(cmd0, sizeof(cmd0), "echo %d > /sys/class/pwm/pwmchip0/pwm3/enable", 0);
        executeCommand(cmd0);
        return;
    }
    mbeepStatus = beep;
    mbeepPeriod = period;
#ifndef CDROID_X64
    char cmd[100];
    int mPwm = 1000000000/period ;
    snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/pwm/pwmchip0/pwm3/period", mPwm);
    executeCommand(cmd);

    snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/pwm/pwmchip0/pwm3/enable", beep);
    executeCommand(cmd);
    
#else
    LOGI("蜂鸣器响");
#endif
}


void MusicPlayer::powerUp()
{
    LOGE("[g_misic] 开机");
    soundCombosWait.push_back(soundCombos[0]);
    return;
}

void MusicPlayer::powerOn()
{
    LOGE("[g_misic] 开机");
    soundCombosWait.push_back(soundCombos[1]);
    return;
}

void MusicPlayer::powerOff() {
    LOGE("[g_misic] 工作关机");
    soundCombosWait.push_back(soundCombos[2]);
    return;
}

void MusicPlayer::btnValid() {
    LOGE("[g_misic] 有效按键音");
    soundCombosWait.push_back(soundCombos[5]);
    return;
}

void MusicPlayer::btnInvalid() {
    LOGE("[g_misic] 无效按键音");
    soundCombosWait.push_back(soundCombos[6]);
    return;
}

void MusicPlayer::operationValid()
{
    LOGE("[g_misic] 操作成功音");
    soundCombosWait.push_back(soundCombos[12]);
    return;
}
void MusicPlayer::touchValid()
{
    LOGE("[g_misic] 触摸成功音");
    soundCombosWait.push_back(soundCombos[9]);
    return;
}

void MusicPlayer::errorValid()
{
    LOGE("[g_misic] 错误成功音");
    soundCombosWait.push_back(soundCombos[15]);
    return;
}
void MusicPlayer::completeValid()
{
    LOGE("[g_misic] 程序完成音");
    soundCombosWait.push_back(soundCombos[10]);
    return;
}

void MusicPlayer::tipValid(){
    LOGE("[g_misic] 一般情况提示音");
    soundCombosWait.push_back(soundCombos[15]);
    return;
}

void MusicPlayer::tip2Valid(){
    LOGE("[g_misic] 紧急提醒音");
    soundCombosWait.push_back(soundCombos[16]);
    return;
}

void MusicPlayer::alarmValid(){
    LOGE("[g_misic] 闹钟");
    soundCombosWait.push_back(soundCombos[17]);
    return;
}
void MusicPlayer::lockValid(){
    LOGE("[g_misic] 锁定");
    soundCombosWait.push_back(soundCombos[4]);
    return;
}

void MusicPlayer::unlockValid(){
    LOGE("[g_misic] 解锁");
    soundCombosWait.push_back(soundCombos[3]);
    return;
}
