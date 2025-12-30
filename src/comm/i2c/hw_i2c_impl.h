/**********************************************************
 * @Author: Nijkhe
 * @Date: 2023-08-11 05:44:32
 * @LastEditTime: 2023-08-11 05:44:33
 * @LastEditors: Nijkhe
 * @Description:
 * @Version:
 * @FilePath: /hw_i2c_read/hw_i2c_impl.h
 * @Talk is cheap. Show me the code!
 ***********************************************************/

#ifndef HW_I2C_IMPL_H
#define HW_I2C_IMPL_H

#include <stdint.h>

int  hw_i2c_init(void);
void hw_i2c_release(void);

int8_t hw_i2c_write(uint8_t address, const uint8_t *data, uint16_t count);
int8_t hw_i2c_read(uint8_t address, uint8_t reg_addr, uint8_t *data, uint16_t count);

void hw_i2c_sleep_usec(uint32_t useconds);

#endif /* HW_I2C_IMPL_H */
