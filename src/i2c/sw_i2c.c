#include "sw_i2c.h"

#define DELAY_USEC (I2C_CLOCK_PERIOD_USEC / 2)

static int8_t wait_while_clock_stretching(void)
{
    /* Maximal timeout of 150ms (SCD30) in sleep polling cycles */
    uint32_t timeout_cycles = 150000 / I2C_CLOCK_PERIOD_USEC;

    while (--timeout_cycles)
    {
        if (sw_i2c_SCL_read())
            return NO_ERROR;
        sw_i2c_sleep_usec(I2C_CLOCK_PERIOD_USEC);
    }

    return STATUS_FAIL;
}

static int8_t sw_i2c_write_byte(uint8_t data)
{
    int8_t nack, i;
    for (i = 7; i >= 0; i--)
    {
        sw_i2c_SCL_out();
        if ((data >> i) & 0x01)
            sw_i2c_SDA_in();
        else
            sw_i2c_SDA_out();
        sw_i2c_sleep_usec(DELAY_USEC);
        sw_i2c_SCL_in();
        sw_i2c_sleep_usec(DELAY_USEC);
        if (wait_while_clock_stretching())
            return STATUS_FAIL;
    }
    sw_i2c_SCL_out();
    sw_i2c_SDA_in();
    sw_i2c_sleep_usec(DELAY_USEC);
    sw_i2c_SCL_in();
    if (wait_while_clock_stretching())
        return STATUS_FAIL;
    nack = (sw_i2c_SDA_read() != 0);
    sw_i2c_SCL_out();

    return nack;
}

static uint8_t sw_i2c_read_byte(uint8_t ack)
{
    int8_t i;
    uint8_t data = 0;
    sw_i2c_SDA_in();
    for (i = 7; i >= 0; i--)
    {
        sw_i2c_sleep_usec(DELAY_USEC);
        sw_i2c_SCL_in();
        if (wait_while_clock_stretching())
            return 0xFF; // return 0xFF on error
        data |= (sw_i2c_SDA_read() != 0) << i;
        sw_i2c_SCL_out();
    }
    if (ack)
        sw_i2c_SDA_out();
    else
        sw_i2c_SDA_in();
    sw_i2c_sleep_usec(DELAY_USEC);
    sw_i2c_SCL_in();
    sw_i2c_sleep_usec(DELAY_USEC);
    if (wait_while_clock_stretching())
        return 0xFF; // return 0xFF on error
    sw_i2c_SCL_out();
    sw_i2c_SDA_in();

    return data;
}

static int8_t i2c_start(void)
{
    sw_i2c_SCL_in();
    if (wait_while_clock_stretching())
        return STATUS_FAIL;

    sw_i2c_SDA_out();
    sw_i2c_sleep_usec(DELAY_USEC);
    sw_i2c_SCL_out();
    sw_i2c_sleep_usec(DELAY_USEC);
    return NO_ERROR;
}

static void i2c_stop(void)
{
    sw_i2c_SDA_out();
    sw_i2c_sleep_usec(DELAY_USEC);
    sw_i2c_SCL_in();
    sw_i2c_sleep_usec(DELAY_USEC);
    sw_i2c_SDA_in();
    sw_i2c_sleep_usec(DELAY_USEC);
}

int8_t sw_i2c_write(uint8_t address, const uint8_t *data,
                    uint16_t count)
{
    int8_t ret;
    uint16_t i;

    ret = i2c_start();
    if (ret != NO_ERROR)
        return ret;

    ret = sw_i2c_write_byte(address << 1);
    if (ret != NO_ERROR)
    {
        i2c_stop();
        return ret;
    }
    for (i = 0; i < count; i++)
    {
        ret = sw_i2c_write_byte(data[i]);
        if (ret != NO_ERROR)
        {
            i2c_stop();
            break;
        }
    }
    i2c_stop();
    return ret;
}

int8_t sw_i2c_read(uint8_t address, uint8_t *data, uint16_t count)
{
    int8_t ret;
    uint8_t send_ack;
    uint16_t i;

    ret = i2c_start();
    if (ret != NO_ERROR)
        return ret;

    ret = sw_i2c_write_byte((address << 1) | 1);
    if (ret != NO_ERROR)
    {
        i2c_stop();
        return ret;
    }
    for (i = 0; i < count; i++)
    {
        send_ack = i < (count - 1); /* last byte must be NACK'ed */
        data[i] = sw_i2c_read_byte(send_ack);
    }

    i2c_stop();
    return NO_ERROR;
}

void sw_i2c_init(void)
{
    sw_i2c_init_pins();
    sw_i2c_SCL_in();
    sw_i2c_SDA_in();
}

void sw_i2c_release(void)
{
    sw_i2c_SCL_in();
    sw_i2c_SDA_in();
    sw_i2c_release_pins();
}
