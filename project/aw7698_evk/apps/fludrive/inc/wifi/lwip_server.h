/*

*/
// static int8_t decode_recv_data(int sckClient, const char *data, int length);

#ifndef __LWIP_SERVER_H__
#define __LWIP_SERVER_H__


#ifdef __cplusplus
extern "C" {
#endif

#define SOCK_TCP_SRV_PORT        6500
#define SOCK_UDP_SRV_PORT        6600
#define TRX_PACKET_COUNT         5

#include "semphr.h"

typedef enum { 
    Header,
    Data
}PacketType;

typedef enum {
    ReadFileRQ = 0,
    WriteFileRQ,
    ListFileRQ,
    DeletelFileRQ,
    WriteDataRQ,
    ReadDataRQ,
    CompletedRQ
} Opcode;

typedef struct Write_message{
    int8_t client_socket;
    Opcode opcode;
    int8_t *data;
    uint32_t length;
}SWrtMessage;

extern PacketType g_packetType;
extern SemaphoreHandle_t g_SignalRequest;
extern QueueHandle_t g_RWQueue;

extern void create_lwip_task();
extern void create_socket_task();

#ifdef __cplusplus
}
#endif

#endif //__LWIP_SERVER_H__