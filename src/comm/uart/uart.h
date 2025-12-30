
#ifndef __uart_h__
#define __uart_h__

//串口相关的头文件
#include <errno.h>  /*错误号定义*/
#include <fcntl.h>  /*文件控制定义*/
#include <stdio.h>  /*标准输入输出定义*/
#include <stdlib.h> /*标准函数库定义*/
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h> /*PPSIX 终端控制定义*/
#include <unistd.h>  /*Unix 标准函数定义*/

#define TRUE 1
#define FALSE 0

int UART0_Open(const char *port);

int UART0_Init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);

int UART0_Recv(int fd, unsigned char *rcv_buf, int data_len);
int UART0_Send(int fd, unsigned char *send_buf, int data_len);

void UART0_Close(int fd);

#endif
