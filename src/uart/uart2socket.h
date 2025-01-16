
#ifndef __uart2socket_h__
#define __uart2socket_h__

#define UART_PORT_LEN 16

typedef enum {
    main_cmd_uart = 1,
    main_cmd_data,
} main_cmd;

typedef enum {
    sub_cmd_uart_open_req = 1,
    sub_cmd_uart_open_rsp,    
} sub_cmd_uart;

typedef enum {
    sub_cmd_data_trans = 1,
    sub_cmd_heart_ask,
    sub_cmd_heart_ack,
} sub_cmd_data;

#pragma pack(1)
typedef struct tagUartHeader {
    uint16_t size;
    uint8_t  mcmd;
    uint8_t  scmd;
} UartHeader, UARTHEADER, *LPUARTHEADER;

typedef struct tagUartOpenReq {
    char  serialPort[UART_PORT_LEN]; // /dev/ttySxx
    int   speed;          // 串口速度
    uint8_t flow_ctrl;      // 数据流控制
    uint8_t databits;       // 数据位   取值为 7 或者8
    uint8_t stopbits;       // 停止位   取值为 1 或者2
    char  parity;         // 效验类型 取值为N,E,O,,S
} UartOpenReq;

typedef struct {
    int result;
}UartOpenRsp;

typedef struct tagDataTrans {
    uint8_t data[1];
}DataTrans;
#pragma pack()

#endif
