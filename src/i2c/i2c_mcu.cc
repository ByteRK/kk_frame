/**********************************************************
 * @Author: Nijkhe
 * @Date: 2023-08-11 05:46:30
 * @LastEditTime: 2023-08-11 05:46:31
 * @LastEditors: Nijkhe
 * @Description:
 * @Version:
 * @FilePath: /hw_i2c_read/i2c_mcu.c
 * @Talk is cheap. Show me the code!
 ***********************************************************/

#include "i2c_mcu.h"
#include "hw_i2c_impl.h"

int i2c_mcu_read(uint8_t addr, uint8_t reg_addr, uint8_t* data, uint32_t len) {
    return hw_i2c_read(addr, reg_addr, data, len);
}

int i2c_mcu_write(uint8_t addr, uint8_t reg_addr, uint8_t* data, uint32_t len) {
    uint8_t* buf = (uint8_t*)malloc((len + 1) * sizeof(uint8_t));
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);
    int ret = hw_i2c_write(addr, buf, len + 1);
    free(buf);

    return ret;
}

int i2c_mcu_read2(uint8_t addr, uint8_t* str, uint32_t len) {
    return sw_i2c_read(addr, str, len);
}

int i2c_mcu_write2(uint8_t addr, uint8_t* str, uint32_t len) {
    return sw_i2c_write(addr, str, len);
}