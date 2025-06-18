/*
 * @Author: hanakami
 * @Date: 2025-05-08 17:08:00
 * @email: hanakami@163.com
 * @LastEditTime: 2025-06-18 08:23:45
 * @FilePath: /hana_frame/src/project/global_data.cc
 * @Description: 全局数据
 * Copyright (c) 2025 by hanakami, All Rights Reserved. 
 */

 #include "global_data.h"
 #include "this_func.h"
 #include "comm_func.h"
 #include "json_func.h"
 #include "base_data.h"
 #include "defualt_config.h"
 #include <unistd.h>
 
 static constexpr uint32_t GD_SAVE_CHECK_INITERVAL = 2000;       // 检查保存间隔[2s]
 static constexpr uint32_t GD_SAVE_BACKUP_INTERVAL = 1000 * 10;  // 备份间隔[10s]
 
 /// @brief 析构
 globalData::~globalData() {
     mLooper->removeMessages(this);
 }
 
 /// @brief 初始化
 void globalData::init() {
     mPowerOnTime = SystemClock::uptimeMillis();
 
     checkenv();
     loadFromFile();
 
     mNextBakTime = UINT64_MAX;
     mCheckSaveMsg.what = MSG_SAVE;
     mLooper = Looper::getMainLooper();
     mLooper->sendMessageDelayed(GD_SAVE_CHECK_INITERVAL, this, mCheckSaveMsg);
 }
 
 /// @brief 重置
 void globalData::reset() {
     std::string command = std::string("rm")\
         + " " + APP_FILE_FULL_PATH + " " + APP_FILE_BAK_PATH;
     std::system(command.c_str());
     init();
     LOGE("global_data factory reset.");
 }
 
 /// @brief 定时任务，用于保存修改后的配置
 /// @param message 
 void globalData::handleMessage(Message& message) {
     switch (message.what) {
     case MSG_SAVE:
         checkToSave();
         break;
     default:
         break;
     }
 }
 
 /// @brief 检查环境变量
 void globalData::checkenv() {
     // 设备模式
     if (getenv("DEV_MODE")) {
         uint8_t devMode = atoi(getenv("DEV_MODE"));
         if (devMode < DEVICE_MODE_MAX) {
             mDeviceMode = devMode;
         }
     }
     LOGW_IF(mDeviceMode, "DEVICE_MODE: %d", mDeviceMode);
 }
 
 /// @brief 载入本地文件
 /// @return 
 bool globalData::loadFromFile() {
     Json::Value appJson;
     std::string loadingPath = "";
     if (access(APP_FILE_FULL_PATH, F_OK) == 0) {
         loadingPath = APP_FILE_FULL_PATH;
     } else if (access(APP_FILE_BAK_PATH, F_OK) == 0) {
         loadingPath = APP_FILE_BAK_PATH;
     }
 
     if (loadingPath.empty() || !loadLocalJson(loadingPath, appJson)) {
         LOG(ERROR) << "[app] no local data file found. use default data";
         mHaveChange = true;
         return false;
     }
     LOG(INFO) << "[app] load local data. file=" << loadingPath;
 
     /**** 开始读取数据 ****/
     mCoffee = getJsonValue(appJson, "coffee", true);
     /**** 结束读取数据 ****/
     return true;
 }
 
 /// @brief 保存文件到本地
 /// @param isBak 是否为备份
 /// @return 
 bool globalData::saveToFile(bool isBak) {
     Json::Value appJson;
     /**** 开始写入数据 ****/
     appJson["coffee"] = mCoffee;
     /**** 结束写入数据 ****/
     return saveLocalJson(
         isBak ? APP_FILE_BAK_PATH : APP_FILE_FULL_PATH,
         appJson);;
 }
 
 /// @brief 检查是否需要保存
 void globalData::checkToSave() {
     uint64_t now = SystemClock::uptimeMillis();
     if (mHaveChange) {
         saveToFile();
 #ifndef CDROID_X64
         sync();
 #endif
         mHaveChange = false;
         mNextBakTime = now + GD_SAVE_BACKUP_INTERVAL;
         LOG(INFO) << "[app] save globalData. file=" << APP_FILE_FULL_PATH;
     }
     if (now >= mNextBakTime) {
         saveToFile(true);
 #ifndef CDROID_X64
         sync();
 #endif
         mNextBakTime = UINT64_MAX;
         LOG(INFO) << "[app] save globalData bak. file=" << APP_FILE_BAK_PATH;
     }
     mLooper->sendMessageDelayed(GD_SAVE_CHECK_INITERVAL, this, mCheckSaveMsg);
 }
 
 /// @brief 获取程序启动时间[可粗略计算程序运行时间]
 /// @return 
 uint64_t globalData::getPowerOnTime() {
     return mPowerOnTime;
 }