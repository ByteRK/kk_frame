#include "sw_i2c_gpio.h"

static int scl_dir_fd;
static int scl_val_fd;
static int sda_dir_fd;
static int sda_val_fd;

static int open_or_exit(const char *path, int flags)
{
    int fd = open(path, flags);
    if (fd < 0)
    {
        perror(NULL);
        fprintf(stderr, "Error opening %s (mode %d)\n", path, flags);
        exit(-1);
    }
    return fd;
}

static void rev_or_exit(int fd)
{
    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        perror("Error seeking gpio");
        exit(-1);
    }
}

static void write_or_exit(int fd, const char *buf)
{
    size_t len = strlen(buf);
    ssize_t w = write(fd, buf, len);

    if (w < 0 || (size_t)w != len)
    {
        perror("Error writing");
        exit(-1);
    }
}

static void gpio_export(const char *path, const char *export_pin)
{
    int fd;

    // If not exist then create it
    if (access(path, F_OK) == -1)
    {
        fd = open_or_exit(GPIO_EXPORT_PATH, O_WRONLY);
        write_or_exit(fd, export_pin);
        close(fd);
    }
}

static void gpio_unexport(const char *path, const char *unexport_pin)
{
    int fd;

    // If exist then delete it
    if (access(path, F_OK) == 0)
    {
        fd = open_or_exit(GPIO_UNEXPORT_PATH, O_WRONLY);
        write_or_exit(fd, unexport_pin);
        close(fd);
    }
}

static void gpio_set_value(int fd, int value)
{
    char buf[] = {'0', '\0'};

    buf[0] += value;
    rev_or_exit(fd);
    write_or_exit(fd, buf);
}

static void gpio_set_direction(int fd, const char *dir)
{
    rev_or_exit(fd);
    write_or_exit(fd, dir);
}

static uint8_t gpio_get_value(int fd)
{
    char c;

    rev_or_exit(fd);
    if (read(fd, &c, 1) != 1)
    {
        perror("Error reading GPIO value");
        exit(-1);
    }
    return c == '1';
}

void sw_i2c_init_pins(void)
{
    gpio_export(GPIO_SCL_DIR, GPIO_PIN_SCL_STR);
    gpio_export(GPIO_SDA_DIR, GPIO_PIN_SDA_STR);

    scl_dir_fd = open_or_exit(GPIO_SCL_DIRECTION, O_WRONLY);
    scl_val_fd = open_or_exit(GPIO_SCL_VALUE, O_RDWR);
    sda_dir_fd = open_or_exit(GPIO_SDA_DIRECTION, O_WRONLY);
    sda_val_fd = open_or_exit(GPIO_SDA_VALUE, O_RDWR);
}

void sw_i2c_release_pins(void)
{
    gpio_unexport(GPIO_UNEXPORT_PATH, GPIO_PIN_SCL_STR);
}

void sw_i2c_SDA_in(void)
{
    gpio_set_direction(sda_dir_fd, GPIO_DIRECTION_IN);
}

void sw_i2c_SDA_out(void)
{
    gpio_set_direction(sda_dir_fd, GPIO_DIRECTION_OUT);
    gpio_set_value(sda_val_fd, GPIO_LOW);
}

uint8_t sw_i2c_SDA_read(void)
{
    return gpio_get_value(sda_val_fd);
}

void sw_i2c_SCL_in(void)
{
    gpio_set_direction(scl_dir_fd, GPIO_DIRECTION_IN);
}

void sw_i2c_SCL_out(void)
{
    gpio_set_direction(scl_dir_fd, GPIO_DIRECTION_OUT);
    gpio_set_value(scl_val_fd, GPIO_LOW);
}

uint8_t sw_i2c_SCL_read(void)
{
    return gpio_get_value(scl_val_fd);
}

void sw_i2c_sleep_usec(uint32_t useconds)
{
    usleep(useconds);
}
