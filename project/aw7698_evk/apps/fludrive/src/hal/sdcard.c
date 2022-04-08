
/* hal includes */
#include "hal.h"
#include "memory_attribute.h"
#include "hal_sd.h"
#include "task_def.h"

#include "ff.h"
#include "sdcard.h"
#include "lwip_server.h"
#include "cJSON.h"
/*hal pinmux define*/
#include "hal_pinmux_define.h"


#define MAX_TX_BUFFER (64*1024)
#define BLOCK_NUMBER  (16)
TickType_t xTimeNow,last_time;

int8_t ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN shared_buffer[MAX_TX_BUFFER];

log_create_module(sd_card, PRINT_LEVEL_INFO);
FIL fdst;                        /* file target which must be a global variable name if it would be accessed with global scope. */
FATFS fatfs;                     /* fs target which must be a global variable name if it would be accessed with global scope. */
static hal_sd_config_t sd_cfg = {HAL_SD_BUS_WIDTH_4, 45000};

static SdCardState state = None;
static uint16_t offsetWrite = 0;

static int8_t process_read_request(int8_t sk, const int8_t* filename, uint16_t fileLen);
static int8_t process_completed_request();
static int8_t process_list_file(int8_t sck);
static int8_t process_delete_file(int8_t sk,int8_t* filename);

int8_t endfile[] = "msg_end";
//uint32_t offsetWriteFile = 0;

void dispose(const int8_t* mem)
{
    if(mem != NULL){
        free(mem);       
    }
    f_close(&fdst);
}
static void cb_handle_rw_file(void *args)
{
    SWrtMessage* reqMsg;
    //xSemaphoreTake(g_SignalRequest, portMAX_DELAY);
    for(;;)
    {
        //xSemaphoreTake(g_SignalRequest, portMAX_DELAY);        
        if( xQueueReceive( g_RWQueue,&( reqMsg ),( TickType_t ) 100 ) == pdPASS) {
            LOG_I(sd_card, "get opcode message =  %d ", reqMsg->opcode);

            switch (reqMsg->opcode)
            {
                case ReadFileRQ:
                    LOG_I(sd_card, "process read file");
                    process_read_request(reqMsg->client_socket, reqMsg->data, reqMsg->length);
                    free(reqMsg);
                    g_packetType = Header;
                    break;    
                case CompletedRQ:
                    process_completed_request();
                    free(reqMsg);
                    g_packetType = Header;
                    break;
                case ListFileRQ:
                    LOG_I(sd_card, "process list file");
                    process_list_file(reqMsg->client_socket);
                    vTaskDelay(1000);
                    lwip_write(reqMsg->client_socket, endfile, sizeof(endfile));
                    if(reqMsg->data != NULL)
                        free(reqMsg->data);
                    free(reqMsg);
                    g_packetType = Header;
                    break;
                case DeletelFileRQ:
                    LOG_I(sd_card, "process delete file");
                    process_delete_file(reqMsg->client_socket,reqMsg->data);
                    //vTaskDelay(1000);
                    lwip_write(reqMsg->client_socket, endfile, sizeof(endfile));
                    if(reqMsg->data != NULL)
                        free(reqMsg->data);
                    free(reqMsg);
                    g_packetType = Header;
                    break;
                default:
                    break;
            }
        }       
    }
}

static int8_t process_delete_file(int8_t sk,int8_t* filename)
{
    f_unlink(filename);
    LOG_I(sd_card,"delete file %s ",filename);
    return 0;
}
static int8_t handle_create_file(int8_t sk,int8_t* filename)
{
    if(sdcard_create(filename) != 0){ // 1: create file
    LOG_I(sd_card,"create file error");
    return -1; 
    }
    return 0;
}
static int8_t handle_rename_file(int8_t sk, int8_t* old_name, int8_t new_name){
    int8_t endfile[] = "msg_end";
    if(f_rename(old_name,new_name) !=0){
     LOG_I(sd_card,"rename file fail");
    return -1;    
    }
    lwip_write(sk, endfile, sizeof(endfile));
    return 0;
}

static int8_t process_list_file(int8_t sck)
{
    cJSON *root;
    cJSON *file_infos = NULL;
    cJSON *file_info = NULL;
    char *out;
    root  = cJSON_CreateObject();   
    FRESULT res;
    DIR dir;
    static FILINFO fno;

    cJSON_AddStringToObject(root, "ID", "1");
    file_infos = cJSON_CreateArray();
    file_infos = cJSON_AddArrayToObject(root, "File info");
    res = f_opendir(&dir,"");                       /* Open the directory */
    //lwip_write(sck, "[", 1;
    if (res == FR_OK) {
        for (;;) {             
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            //LOG_I(lwip_server, "f_readdir %d fno.fname[0] %d \n",res,fno.fname[0] );
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if ((fno.fattrib & AM_DIR) == 0) {                    /* It is a directory */
                file_info = cJSON_CreateObject();
                cJSON_AddNumberToObject(file_info, "ID", sck);
                cJSON_AddItemToObject(file_info,  "File name", cJSON_CreateString(fno.fname));
                cJSON_AddNumberToObject(file_info, "File size", fno.fsize);
                cJSON_AddItemToArray(file_infos, file_info);
                // list each file
               /* out = cJSON_Print(file_info); 
                //sprintf(out,"{%s}",out); 
                if (res != FR_OK || fno.fname[0] == 0) {
                    lwip_write(sck, out, strlen(out));
                    lwip_write(sck, "]", 1);
                    cJSON_free(out);
                } 
                else{     
                if(out!= NULL){
                    lwip_write(sck, out, strlen(out));
                    lwip_write(sck, ",", 1);
                    cJSON_free(out);
                }
                else{
                    LOG_I(sd_card, "cJSON_Print fail");
                }
                }
                */
            }
        }
        f_closedir(&dir);
        
        // list all file
        out = cJSON_Print(file_infos);        
        if(out!= NULL){
            lwip_write(sck, out, strlen(out));
            cJSON_free(out);
        }
        else
        {
            LOG_I(sd_card, "cJSON_Print fail");
        }
        
        cJSON_Delete(root);
    }
    return 0;
}

static int8_t 
process_read_request(int8_t sk, const int8_t* filename, uint16_t fileLen)
{
    FRESULT res;
    res = f_open(&fdst, filename, FA_OPEN_EXISTING | FA_READ);
    if (FR_OK != res) {
        LOG_I(sd_card,"4-Failed to open the file,res=%d",res);
        dispose(filename);
        return -1;
    }
    uint32_t fileSize  = sdcard_getsize();
    LOG_I(sd_card,"file size = %d ",fileSize);
    state = Reading;
    uint32_t offset = 0;
    uint32_t rb;
    
    xTimeNow = xTaskGetTickCount();
    while(offset < fileSize)
    {
        // set offset 
        rb = 0;
        res = f_lseek(&fdst, offset);
        if (FR_OK != res) {
            LOG_I( sd_card, "5-Failed to seek the file,res=%d\r\n",res);
            res = f_close(&fdst);
            return -1;
        }
        // read from offset  
        res = f_read(&fdst, shared_buffer, MAX_TX_BUFFER, &rb);
        //LOG_I(sd_card,"Finish read");
        if (FR_OK != res) {
            LOG_I(sd_card,"Failed to read the file,res=%d",res);
            res = f_close(&fdst);
            return -1;
        }
        // send to app
        lwip_write(sk, shared_buffer, rb);
        offset += rb;
        LOG_I( sd_card, "offset=%d , rb= %d , filesize = %d \r\n",offset, rb, fileSize);
    }
        last_time = xTaskGetTickCount();
        int16_t time_lost = (last_time-xTimeNow) / 1000 / portTICK_PERIOD_MS;
        LOG_I(lwip_server, "Transfer file:  %s file size %d time lost: %d speed %d   ",filename,fileSize,time_lost,fileSize/time_lost);
        vTaskDelay(1000);
        dispose(filename);
        lwip_write(sk, endfile, sizeof(endfile));
    //lwip_close(sk);
    return 0;
}


static int8_t 
process_completed_request()
{
    FRESULT res;
    res = f_close(&fdst);
    if (FR_OK != res) {
        LOG_I(sd_card,  "5-Failed to close the file,res=%d\r\n",res);
        return -1;
    }
    return  0;
}


void create_sdcard_task()
{
    BaseType_t xReturn = xTaskCreate(cb_handle_rw_file, 
                                SD_CARD_THREAD_NAME, 
                                SD_CARD_THREAD_STACKSIZE / sizeof(portSTACK_TYPE), 
                                NULL, 
                                SD_CARD_THREAD_PRIO, NULL);
    if(xReturn != pdPASS){
         LOG_I(sd_card, "create new task for sd card failure \r\n");
         return ;
    }
}

int8_t sdcard_init()
{
    /* Mount the partition before it is accessed, and need only do it once. */
    FRESULT res; 
    res = f_mount(&fatfs,"", 1);
    if(res != FR_OK){
         printf( "1-Failed to mount the partition,res=%d\r\n",res);
    } else{
         printf(" FS ok !!!!!!\r\n");
    }
    return 0;
}

int8_t sdcard_create(const int8_t *filename)
{
    FRESULT res;   
    /* Create a new file for writing and reading. */
    res = f_open(&fdst, filename , FA_CREATE_ALWAYS| FA_WRITE | FA_READ);
    if (FR_OK != res) {
        printf( "3-Failed to create the file,res=%d\r\n",res);
        return -1;
    }
    return 0;
}

int8_t sdcard_open(const int8_t *filename)
{
    FRESULT res;
    res = f_open(&fdst, filename, FA_OPEN_EXISTING | FA_WRITE | FA_READ);
    if (FR_OK != res) {
        LOG_I(sd_card,"4-Failed to open the file,res=%d",res);
        return -1;
    }
    else{
        LOG_I(sd_card,"SD open file success %s ",filename);
    }
    return 0;
}

int32_t sdcard_write(const void* data, uint32_t length)
{
    FRESULT res;
    uint32_t length_written;
    /* Write the file */
    res = f_write(&fdst, data, length, &length_written);
    if (FR_OK != res) {
        printf( "5-Failed to write the file,res=%d\r\n",res);
        res = f_close(&fdst);
        return -1;
    }
    return length_written;
}

int32_t sdcard_read_dma(int8_t *buf,int32_t offset,  uint32_t length)
{
    
    /*read with dma mode*/
    uint32_t retry = 0;
    while (retry < 3) {
        if (HAL_SD_STATUS_OK != hal_sd_read_blocks_dma_blocking(HAL_SD_PORT_0, buf, offset, length)) {
            if (HAL_SD_STATUS_OK == hal_sd_init(HAL_SD_PORT_0, &sd_cfg)) {
                retry++;
            } else {
                printf("SD read (dma mode) failed!\r\n");
                break;
            }
        } else {
            break;
        }
    }

    if (3 <= retry) {
        printf("SD read (dma mode) failed!\r\n");
        return -1;
    }

    return length;

}

int32_t sdcard_read(void *buf, uint32_t length)
{
    FRESULT res;
    uint32_t length_read;
    /* Write the file */
    //LOG_I(sd_card,"Start read");
    res = f_read(&fdst, buf, length, &length_read);
    //LOG_I(sd_card,"Finish read");
    if (FR_OK != res) {
        LOG_I(sd_card,"Failed to read the file,res=%d",res);
        res = f_close(&fdst);
        return -1;
    }
    //else{
        //LOG_I(sd_card,"read file success,buf %s length_read=%d",buf,length_read);
    //}
    return length_read;
}

int8_t sdcard_seek(uint32_t offset)
{
    FRESULT res;
    /* Write the file */
    res = f_lseek(&fdst, offset);
    if (FR_OK != res) {
        printf( "5-Failed to seek the file,res=%d\r\n",res);
        res = f_close(&fdst);
        return -1;
    }
    return  0;
}

int8_t sdcard_close()
{
    FRESULT res;
    res = f_close(&fdst);
    if (FR_OK != res) {
        printf( "5-Failed to close the file,res=%d\r\n",res);
        return -1;
    }
    return  0;
}

uint32_t sdcard_getsize()
{  
    return f_size(&fdst);
}

// int16_t open_file_from_sd(FIL *fp, const TCHAR *path)
// {
//     FRESULT res;
//     res = f_mount(&fatfs, _T("0:"), 1);

//     if (!res) {
//         LOG_I(sd_card,"fmount ok \n");
//         res = f_open(fp, path, FA_OPEN_EXISTING | FA_READ);
//         if (!res) {
//             LOG_I(sd_card,"fopen ok \n");
//         } else {
//             LOG_I(sd_card,"fopen error, res = %d \n", res);
//             return -1;
//         }
//     } else {
//          LOG_I(sd_card,"fmount error \n");
//     }

//     if (res != FR_OK) {
//         return -1;
//     } else {
//         return 0;
//     }
// }
// void stop_read_from_sd()
// {
//     res = f_close(&fdst);
//     if (!res) {
//         LOG_I(sd_card,"fclose success \n");
//     } else {
//          LOG_I(sd_card,"fmount error \n");
//     }
// }
// int32_t sdcard_read(FIL*fp, void *buff, uint32_t length)
// {

// }
// int32_t sdcard_write(FIL*fp, void *data, uint32_t length)
// {

// }

// void sd_test()
// {
//     sdcard_open("1.txt");
    
//     char *data = (char *)malloc(512);
//     if(data == NULL){
//         LOG_I(sd_card,"Can't allocate memory");
//         return -2;
//     }
//     sdcard_read(data, 512);
//     LOG_I(sd_card,"data %s",data);
//     sdcard_close();
//     free(data);
// }
int8_t sdcard_test()
{
    /**
    * @brief       write one file and then read the file to verify whether the file is written successfully.
    * @param[in]   None.
    * @return      None.
    */
    BYTE buffer_src[1024];
    BYTE buffer_dst[1024];

    FRESULT res;                     /* fs status infor*/
    UINT temp_count, length_written, length_read;

    for (temp_count = 0; temp_count < 1000; temp_count++) {
        buffer_src[temp_count] = temp_count;
    }

    /* Mount the partition before it is accessed, and need only do it once. */
    // res = f_mount(&fatfs, _T("0:"), 1);
    // //res = f_mkfs(_T("0:"), FM_ANY, 0 , test_wrok, 1024);
 
    // if(res == FR_OK) {
    //     printf("1-make FAT32 FS ok !!!!!!\r\n");
    // }
	
    res = f_mount(&fatfs, _T("0:"), 1);
    if (FR_OK != res) {
        printf( "2-Failed to mount the partition,res=%d\r\n",res);
        return -1;
    }else{
         printf("1-make FAT32 FS ok !!!!!!\r\n");
    }

    /* Step0: Create a new file for writing and reading. */
    res = f_open(&fdst, _T("0:/Test_W.txt"), FA_CREATE_ALWAYS);
    if (FR_OK != res) {
        printf( "3-Failed to create the file,res=%d\r\n",res);
        return -1;
    }

    res = f_open(&fdst, _T("0:/Test_W.txt"), FA_OPEN_EXISTING | FA_WRITE | FA_READ);
    if (FR_OK != res) {;
        printf( "4-Failed to open the file,res=%d\r\n",res);
        return -1;
    }

    /* Step2: Write the file */
    res = f_write(&fdst, buffer_src, 1000, &length_written);
    if (FR_OK != res) {
        printf( "5-Failed to write the file,res=%d\r\n",res);
        res = f_close(&fdst);
        return -1;
    }

    /* Close the open file object, flush the cached information of the file back to the volume.*/
    res = f_close(&fdst);
    if (FR_OK != res) {
        printf( "6-Failed to close the file,res=%d\r\n",res);
        res = f_close(&fdst);
        return -1;
    }       
    
    res = f_open(&fdst, _T("0:/Test_W.txt"), FA_OPEN_EXISTING | FA_READ);
    if (FR_OK != res) {
        printf( "7-Failed to open the file,res=%d\r\n",res);
        res = f_close(&fdst);
        return -1;
    }

    /* Step3: Read the file and verify whether the data is the same as written. */
    res = f_read(&fdst, buffer_dst, 1000, &length_read);
    if (FR_OK != res) {
        printf( "8-Failed to read the file,res=%d\r\n",res);
        res = f_close(&fdst);
        return -1;
    }

    for (temp_count = 0; temp_count < 1000; temp_count++) {
        if (buffer_dst[temp_count] != buffer_src[temp_count]) {
            printf( "9-Failed: data read is different from the data written,temp_count=%d\r\n",temp_count);
            res = f_close(&fdst);
            return -1;
        }
    }
    res = f_close(&fdst);

    printf( "The file is written and read successfully!\r\n");
    return 0;
}

