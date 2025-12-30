/**********************************************************
 * @Author: Nijkhe
 * @Date: 2023-08-11 05:45:59
 * @LastEditTime: 2023-08-11 05:46:00
 * @LastEditors: Nijkhe
 * @Description:
 * @Version:
 * @FilePath: /hw_i2c_read/i2c_mcu.h
 * @Talk is cheap. Show me the code!
 ***********************************************************/

#ifndef __i2c_mcu_h__
#define __i2c_mcu_h__

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "hw_i2c_impl.h"

extern "C" {
#include "sw_i2c.h"
}

// typedef enum mcu_i2c_addr
// {
//     TEST_I2C_ADDR = 0xA6,
//     MCU_I2C_ADDR_WRITE = 0x88,
//     MCU_I2C_ADDR_READ = 0x89,
// } mcu_i2c_addr_t;

// 硬件I2C读
int i2c_mcu_read_hw(uint8_t addr, uint8_t reg_addr, uint8_t* str, uint32_t len);
// 硬件I2C写
int i2c_mcu_write_hw(uint8_t addr, uint8_t reg_addr, uint8_t* data, uint32_t len);

// 软件I2C读
int i2c_mcu_read_sw(uint8_t addr, uint8_t* str, uint32_t len);
// 软件I2C写
int i2c_mcu_write_sw(uint8_t addr, uint8_t* data, uint32_t len);

#endif /* I2C_MCU */
