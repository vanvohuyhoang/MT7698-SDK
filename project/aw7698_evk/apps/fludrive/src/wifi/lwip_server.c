/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "lwip_server.h"
#include "cJSON.h"
#include "sdcard.h"
#include "memory_attribute.h"
#include "sdcard.h"

#define HEADER_BUFFER (1 * 512)
#define REQ_JSON       1
#define WRITE_FILE     0
#define SIZE_RX_BUFFER (24 * 1024)
log_create_module(lwip_server, PRINT_LEVEL_INFO);


int8_t ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN buffer[SIZE_RX_BUFFER];

QueueHandle_t g_RWQueue = NULL;

static int8_t decode_json_data(int sckClient, const int8_t *data, int length);

static void cb_run_lwip_task(void *args);
static void cb_socket_client(void *args);
static int8_t handle_rename_file(int8_t sk, int8_t* old_name, int8_t new_name);

static int8_t write_raw_data(int8_t sk,const int8_t* data, int32_t length);
static int8_t push_message(const SWrtMessage* msg);

static int8_t write_direct_data(int8_t sk, uint16_t  wlen);

PacketType g_packetType = Header;
SWrtMessage requestMsg;
SemaphoreHandle_t g_SignalRequest = NULL;

volatile uint32_t offset_WriteFile = 0;
uint32_t fileSize = 0;

void create_lwip_task(){

    SWrtMessage writeMsg;

    g_SignalRequest = xSemaphoreCreateBinary();
    if(g_SignalRequest == NULL)
    {
        LOG_I(lwip_server, "create signal failure");
        return -2;
    }
    g_RWQueue = xQueueCreate(10 ,sizeof( &writeMsg ));    
    
    if(g_RWQueue == NULL)
    {
        LOG_I(lwip_server, "create write/reading queue failure");
        return -2;
    }
    /* Create a mutex type semaphore. */
    BaseType_t xReturned = xTaskCreate(cb_run_lwip_task, 
                            USER_ENTRY_TASK_NAME,
                            USER_ENTRY_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                            NULL,
                            USER_ENTRY_TASK_PRIO,
                            NULL);
    if( xReturned != pdPASS )
    {
        /* The task was created.  Use the task's handle to delete the task. */
        LOG_I(lwip_server, "Create task  lwip  failure \n\r");
    }     
}
static void cb_run_lwip_task(void *args){
    //lwip_net_ready();
    LOG_I(lwip_server, "Started LWIP server \n\r");
    int s;
    int c;
    int ret;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(SOCK_TCP_SRV_PORT);// | SOCK_UDP_SRV_PORT);
    addr.sin_addr.s_addr = lwip_htonl(IPADDR_ANY);

    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0){
        LOG_I(lwip_server, "TCP server create failed");
        return;
    }

    ret = lwip_bind(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0){
        LOG_I(lwip_server, "TCP server bind failed");
        return;
    }

    ret = lwip_listen(s, 0);
    if (ret < 0)
    {
        LOG_I(lwip_server, "TCP server listen failed");
        return;
    }
    while (1)
    {
        //vTaskDelay(1000 / portTICK_RATE_MS); // release CPU
        socklen_t sockaddr_len = sizeof(addr);
        LOG_I(lwip_server, "Server is waiting....");
        c = lwip_accept(s, (struct sockaddr *)&addr, &sockaddr_len);
        if(c < 0){
            LOG_I(lwip_server, "Socket server accept error");
            break;
        }
        // create new task for handling private client;
        BaseType_t xReturned = xTaskCreate(cb_socket_client,
                            SOCKET_CLIENT_TASK_NAME,
                            SOCKET_CLIENT_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                            (void*) &c, 
                            SOCKET_CLIENT_TASK_PRIO,
                            NULL);
        if(xReturned != pdPASS){
            LOG_I(lwip_server, "create new task for client failed \r\n");
        } 
    }
}

static void cb_socket_client(void *args){
    int c = *((int *)args);
    LOG_I(lwip_server, "Started handle socket client = %d", c);
    int16_t rlen;
    LOG_I(lwip_server, "TCP server waiting for data...");
    while ((rlen = lwip_read(c, buffer, SIZE_RX_BUFFER)) >0)
    {
        LOG_I(lwip_server, "received number of byte = %d \r\n", rlen);

        if(g_packetType == Header)
        {
            if (decode_json_data(c, buffer, rlen)  < 0)
            {
                LOG_I(lwip_server, "JSON format invalid  = NULL \r\n");
                g_packetType = Header;
            }
        } else{
            write_direct_data(c, rlen);
        }      
    }
    offset_WriteFile = 0;
    int ret = lwip_close(c);
    if(g_packetType != Header){
    sdcard_close();
    }
    g_packetType = Header;
    LOG_I(lwip_server, "close lwip ret %d \r\n",ret); 
    vTaskDelete(NULL);
}

int8_t write_direct_data(int8_t sk, uint16_t  wlen)
{
    int8_t endfile[] = "msg_end";
    FRESULT res;
    // set offset 
    //TickType_t time1 = xTaskGetTickCount();
    
    res = sdcard_seek(offset_WriteFile);
    // write data;
    uint32_t wb;
    /* Write the file */
    wb = sdcard_write(buffer, wlen);
    //TickType_t time2 = xTaskGetTickCount();
    offset_WriteFile += wb;
    LOG_I( sd_card, "write data with offset: =%d\r\n",offset_WriteFile);
    if(offset_WriteFile >= fileSize){
        lwip_write(sk, endfile, 8);
        g_packetType = Header;
        offset_WriteFile = 0;
        last_time = xTaskGetTickCount();
        int16_t time_lost = (last_time-xTimeNow) / 1000 / portTICK_PERIOD_MS;
        LOG_I(lwip_server, "completly receive file offetwrt %d time lost: %d speed %d   ",fileSize,time_lost,fileSize/time_lost);
        sdcard_close();
    }
    return 0;    
}
static 
int8_t push_message(const SWrtMessage* msg)
{
    //xSemaphoreGive( g_SignalRequest );
    if(g_RWQueue == NULL)
    {
        LOG_I(lwip_server, "Not initialize read/write queue");
        return -1;
    }
    if(uxQueueSpacesAvailable(g_RWQueue) > 0){
        if(xQueueSend( /* The handle of the queue. */
                    g_RWQueue,
                    ( void * ) &msg,
                    ( TickType_t ) 0 ) != pdPASS)
        { 
            LOG_I(lwip_server, "push message failure");
            return  -1;
        } else{
            LOG_I(lwip_server, "push message success");
            return 0;
        }        
    }else {
        taskYIELD();
        return 0; 
    }
    /* Send the address of xMessage to the queue created to hold 10    pointers. */
}

/* Start node to be scanned (***also used as work area***) */

static int8_t write_raw_data(int8_t sk, const int8_t* data, int32_t length)
{   
    SWrtMessage *msg = (SWrtMessage *)malloc(sizeof(SWrtMessage));
    if(msg == NULL)
    {
        LOG_I(lwip_server, "cann't alloc mem for raw message");
        return -1;
    }

    msg->data = (int8_t*)malloc(length*sizeof(int8_t));

    if(msg->data == NULL)
    {
        LOG_I(lwip_server, "cann't alloc mem for raw data");
        return -1;
    }

    memset(msg->data ,0, length);
    memcpy(msg->data , data, length);
    msg->client_socket = sk;
    msg->length = length;
    msg->opcode = WriteDataRQ;
    push_message(msg);

    return 0;
}

/**
  * @brief     Parse text to JSON, then render back in text, and print
  * @param[in] char *text:input string
  * @return    None
  */
static int8_t decode_json_data(int sckClient, const int8_t *data, int length)
{
    char *out;
    cJSON *json;
    cJSON *item;

    json = cJSON_Parse(data);
    if (!json)
    {
        LOG_I(lwip_server, "Error before: [%s]\n", cJSON_GetErrorPtr());
        return -1;
    }
    int8_t type = 0;
    int8_t id = 0;
    uint32_t dataLength = 0;
    int8_t content[255];
    out = cJSON_Print(json);
    LOG_I(lwip_server, "data decode json = %s",out);
    item = cJSON_GetObjectItem(json, "Type");
    if ( NULL != item ) {
        type =  item->valueint;
    }
    item = cJSON_GetObjectItem(json, "Content");
    if ( NULL != item ) {
        // content =  item->valuestring;
            strcpy (content, item->valuestring);
    }
    item = cJSON_GetObjectItem(json, "ID");
    if ( NULL != item ) {
        id =  item->valueint;
    }
    item = cJSON_GetObjectItem(json, "DataLength");
    if ( NULL != item ) {
        dataLength =  item->valueint;
        
    }
    cJSON_free(out);
    cJSON_Delete(json);

    SWrtMessage *msg = (SWrtMessage *)malloc(sizeof(SWrtMessage));
    if(msg == NULL)
    {
        LOG_I(lwip_server, "cann't alloc mem for header message");
        return -1;
    }

    if(type  == 1 ) // write file request
    {
        LOG_I(lwip_server, "process header write file fileSize: %d ",dataLength);
        
        offset_WriteFile = 0;

        if(sdcard_create(content) < 0)
        {
            LOG_E(lwip_server, "Create file failure");
            return -1;
        }
        fileSize = dataLength;
        int8_t msg_write[] = "msg_wrt";
        lwip_write(sckClient, msg_write, 8);
        xTimeNow = xTaskGetTickCount();       
        g_packetType = Data;

    } else {
        msg->data = (int8_t*)malloc(255);
        memset(msg->data ,0, 255);
        memcpy(msg->data , content, 255);
        msg->client_socket = sckClient;
        msg->length = dataLength;
        msg->opcode = type;
        //LOG_I(lwip_server, "dataLength %d dataLength %d", msg->length,dataLength);
        push_message(msg);

    }



    return 0;
}


