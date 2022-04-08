//tftp
#include <string.h>
#include "lwip/apps/tftp_client.h"
#include "lwip/apps/tftp_server.h"
#include "tftp_fludrive.h"
#include "sdcard.h"

log_create_module(ftp_fludrivev2, PRINT_LEVEL_INFO);
typedef struct
{
	uint8_t isOpenOK;
	uint8_t type;//0:GCode 1: Cache GCode 2: Firmware
	uint8_t write;
	char name[64];
}TFTP_Handler;
TFTP_Handler tftp_Handler;

//extern FIL *file = &fdst;                        /* file target which must be a global variable name if it would be accessed with global scope. */
FIL fdst2;   
FIL *file = &fdst2; 
void* TFTP_Open(const char* fname, const char* mode, u8_t write)
{
	uint8_t res;
	char name[64];
	tftp_Handler.write=write;
	
	tftp_Handler.type=0;
	strcpy(name,"");
	strcat(name,fname);
	
	if(write){
		res=f_open(file,name,FA_CREATE_ALWAYS|FA_WRITE);
	} else{
		res=f_open(file,name,FA_OPEN_EXISTING|FA_READ);
	}
	tftp_Handler.isOpenOK=res;
	return &tftp_Handler;
}

void TFTP_Close(void* handle)
{
	uint8_t res=0;
	 
	if(((TFTP_Handler*)handle)->isOpenOK)
		return;
	f_close(file);
	LOG_I(ftp_fludrivev2, "Transfer file completed!\r\n");
}

int TFTP_Read(void* handle, void* buf, int bytes)
{
	uint8_t res;
	int br;

	res=((TFTP_Handler*)handle)->isOpenOK;
	if(res)return -1;
	res = f_read(file,(uint8_t*)buf,bytes,&br);
	LOG_I(ftp_fludrivev2,"Read File:%d  Len:%d  br:%d\r\n",res,bytes,br);
	return br;
}

int TFTP_Write(void* handle, struct pbuf* p)
{
	uint8_t res;
	int bw;
	res=((TFTP_Handler*)handle)->isOpenOK;
	if(res)return -1;
	res = f_write(file,p->payload,p->len,&bw);
 
	LOG_I(ftp_fludrivev2,"Write File:%d  Len:%d  bw:%d\r\n",res,p->len,bw);
	return 0;
}

static const struct tftp_context tftp = {
  TFTP_Open,
  TFTP_Close,
  TFTP_Read,
  TFTP_Write
};


void
tftp_fludriver_init_server(void)
{
  tftp_init_server(&tftp);
}
