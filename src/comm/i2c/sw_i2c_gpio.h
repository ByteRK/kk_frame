#ifndef __SW_I2C_GPIO_H__
#define __SW_I2C_GPIO_H__

#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define GPIO_PIN_SCL 62
#define GPIO_PIN_SDA 63
#define GPIO_DIR "/sys/class/gpio/"

#define __str(x) #x
#define GPIO_PIN_STR(p) __str(p)
#define GPIO_PIN_SCL_STR GPIO_PIN_STR(GPIO_PIN_SCL)
#define GPIO_PIN_SDA_STR GPIO_PIN_STR(GPIO_PIN_SDA)
#define GPIO_ID "gpio"
#define GPIO(p) GPIO_ID __str(p)
#define GPIO_PATH(p) GPIO_DIR p
#define GPIO_EXPORT_PATH GPIO_PATH("export")
#define GPIO_UNEXPORT_PATH GPIO_PATH("unexport")
#define GPIO_SCL_DIR GPIO_PATH(GPIO(GPIO_PIN_SCL))
#define GPIO_SDA_DIR GPIO_PATH(GPIO(GPIO_PIN_SDA))
#define GPIO_SCL_PATH(d) GPIO_SCL_DIR d
#define GPIO_SDA_PATH(d) GPIO_SDA_DIR d
#define GPIO_SCL_DIRECTION GPIO_SCL_PATH("/direction")
#define GPIO_SDA_DIRECTION GPIO_SDA_PATH("/direction")
#define GPIO_SCL_VALUE GPIO_SCL_PATH("/value")
#define GPIO_SDA_VALUE GPIO_SDA_PATH("/value")

#define GPIO_DIRECTION_IN "in"
#define GPIO_DIRECTION_OUT "out"
#define GPIO_LOW 0

void sw_i2c_init_pins(void);
void sw_i2c_release_pins(void);

void sw_i2c_SDA_in(void);
void sw_i2c_SDA_out(void);
uint8_t sw_i2c_SDA_read(void);

void sw_i2c_SCL_in(void);
void sw_i2c_SCL_out(void);
uint8_t sw_i2c_SCL_read(void);

void sw_i2c_sleep_usec(uint32_t useconds);

#endif
