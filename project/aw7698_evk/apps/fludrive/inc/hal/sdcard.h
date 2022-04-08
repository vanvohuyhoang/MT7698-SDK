
#ifndef __SD_CARD_H__
#define __SD_CARD_H__

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "task_def.h"
#include "ff.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    None,
    Opening,
    Reading,
    Writing,
    Closed,
    Error
}SdCardState;

extern TickType_t xTimeNow;
extern TickType_t last_time;

void create_sdcard_task();
void create_read_file_task();

int8_t sdcard_init();
int8_t sdcard_create(const int8_t *filename);
int8_t sdcard_open(const int8_t *filename);
int32_t sdcard_write(const void* data, uint32_t length);
int32_t sdcard_read(void *buff, uint32_t length);
int32_t sdcard_read_dma(int8_t *buf, int32_t offset, uint32_t length);

int8_t sdcard_seek(uint32_t offset);
int8_t sdcard_close();
uint32_t sdcard_getsize();

int8_t sdcard_test();

// int8_t scan_files (
//     char* path        /* Start node to be scanned (***also used as work area***) */
// );
// new api 
// int16_t open_file_from_sd(FIL *fp, const TCHAR *path);
// void stop_read_from_sd(FIL *fp);
// int32_t sdcard_read(FIL*fp, void *buff, uint32_t length);
// int32_t sdcard_write(FIL*fp, void *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif //__SD_CARD_H__
