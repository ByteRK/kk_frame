/**********************************************************
 * @Author: Nijkhe
 * @Date: 2023-08-11 05:44:05
 * @LastEditTime: 2023-08-11 05:44:05
 * @LastEditors: Nijkhe
 * @Description:
 * @Version:
 * @FilePath: /hw_i2c_read/hw_i2c_impl.c
 * @Talk is cheap. Show me the code!
 ***********************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> /*标准函数库定义*/
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include "hw_i2c_impl.h"

#define I2C_DEVICE_PATH "/dev/i2c-1"

#define I2C_READ_FAILED -1
#define I2C_WRITE_FAILED -1

static int i2c_device_fd = -1;
static uint8_t i2c_address = 0;

int hw_i2c_init(void)
{
    i2c_device_fd = open(I2C_DEVICE_PATH, O_RDWR);
    if (i2c_device_fd == -1){
        return -1;
    }
    return i2c_device_fd;
}

void hw_i2c_release(void)
{
    if (i2c_device_fd >= 0)
        close(i2c_device_fd);
}

int8_t hw_i2c_write(uint8_t address, const uint8_t *data, uint16_t count)
{
    if (i2c_address != address)
    {
        ioctl(i2c_device_fd, I2C_SLAVE_FORCE, address);
        i2c_address = address;
    }

    if (write(i2c_device_fd, data, count) != count)
    {
        return I2C_WRITE_FAILED;
    }
    return 0;
}

int8_t hw_i2c_read(uint8_t address, uint8_t reg_addr, uint8_t *data, uint16_t count)
{
    if (i2c_address != address)
    {
        ioctl(i2c_device_fd, I2C_SLAVE_FORCE, address);
        i2c_address = address;
    }

    if (write(i2c_device_fd, &reg_addr, 1) != 1)
    {
        return I2C_WRITE_FAILED;
    }
    // if (read(i2c_device_fd, data, count) != count)
    // {
    //     return I2C_READ_FAILED;
    // }
    return read(i2c_device_fd, data, count);
    
    return 0;
}

void hw_i2c_sleep_usec(uint32_t useconds)
{
    usleep(useconds);
}
