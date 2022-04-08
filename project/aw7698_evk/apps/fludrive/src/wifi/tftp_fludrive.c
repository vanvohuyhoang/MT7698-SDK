#include <stdio.h>

#include "lwip/apps/tftp_client.h"
#include "lwip/apps/tftp_server.h"
#include "tftp_fludrive.h"
#include "sdcard.h"

#include <string.h>

#ifdef  TFTP_FLUDRIVEV1
/* Define a base directory for TFTP access
 * ATTENTION: This code does NOT check for sandboxing,
 * i.e. '..' in paths is not checked! */
#ifndef LWIP_TFTP_FLUDRIVER_BASE_DIR
#define LWIP_TFTP_FLUDRIVER_BASE_DIR ""
#endif

/* Define this to a file to get via tftp client */
#ifndef LWIP_TFTP_FLUDRIVER_CLIENT_FILENAME
#define LWIP_TFTP_FLUDRIVER_CLIENT_FILENAME "test.bin"
#endif

/* Define this to a server IP string */
#ifndef LWIP_TFTP_FLUDRIVER_CLIENT_REMOTEIP
#define LWIP_TFTP_FLUDRIVER_CLIENT_REMOTEIP "192.168.0.1"
#endif

FIL file;                        /* file target which must be a global variable name if it would be accessed with global scope. */


static char full_filename[256];
log_create_module(ftp_server, PRINT_LEVEL_INFO);

static void *
tftp_open_file(const char* fname, u8_t is_write)
{
  snprintf(full_filename, sizeof(full_filename), "%s%s", LWIP_TFTP_FLUDRIVER_BASE_DIR, fname);
  full_filename[sizeof(full_filename)-1] = 0;
  
  LOG_I(ftp_server, "tftp_open_file %s \n\r", full_filename);

  if (is_write) {
    f_open(&file, full_filename, FA_OPEN_EXISTING | FA_WRITE);
    return (void*)&file;
  } else {
    f_open(&file, full_filename, FA_OPEN_EXISTING | FA_READ);
    return (void*)&file;
  }
  // if (is_write) {
  //   return (void*)fopen(full_filename, "wb");
  // } else {
  //   return (void*)fopen(full_filename, "rb");
  // }
}

static void*
tftp_open(const char* fname, const char* mode, u8_t is_write)
{
  LWIP_UNUSED_ARG(mode);
  return tftp_open_file(fname, is_write);
}

static void
tftp_close(void* handle)
{
  f_close(&file);
  //fclose((FILE*)handle);
}

static int
tftp_read(void* handle, void* buf, int bytes)
{
  LOG_I(ftp_server, "tftp_read =  %d \n\r", bytes);
  uint32_t length_read;
  int ret = f_read((FIL*)&handle, buf, bytes, &length_read);
  //int ret = fread(buf, 1, bytes, (FILE*)handle);
  if (ret <= 0) {
    return -1;
  }
  return ret;
}

static int
tftp_write(void* handle, struct pbuf* p)
{
  LOG_I(ftp_server, "tftp_write \n\r");
  uint32_t length_write;

  while (p != NULL) {
    //if (fwrite(p->payload, 1, p->len, (FILE*)handle) != (size_t)p->len) {
    if (f_write((FIL*)&handle, p->payload, p->len,&length_write ) != (size_t)p->len) {
      return -1;
    }
    p = p->next;
  }

  return 0;
}

/* For TFTP client only */
static void
tftp_error(void* handle, int err, const char* msg, int size)
{
  LOG_I(ftp_server, "tftp_error \n\r");
  char message[100];

  LWIP_UNUSED_ARG(handle);

  memset(message, 0, sizeof(message));
  MEMCPY(message, msg, LWIP_MIN(sizeof(message)-1, (size_t)size));

  printf("TFTP error: %d (%s)", err, message);
}

static const struct tftp_context tftp = {
  tftp_open,
  tftp_close,
  tftp_read,
  tftp_write,
  tftp_error
};

void
tftp_fludriver_init_server(void)
{
  tftp_init_server(&tftp);
}

void
tftp_fludriver_init_client(void)
{
  void *f;
  err_t err;
  ip_addr_t srv;
  int ret = ipaddr_aton(LWIP_TFTP_FLUDRIVER_CLIENT_REMOTEIP, &srv);
  LWIP_ASSERT("ipaddr_aton failed", ret == 1);

  err = tftp_init_client(&tftp);
  LWIP_ASSERT("tftp_init_client failed", err == ERR_OK);

  f = tftp_open_file(LWIP_TFTP_FLUDRIVER_CLIENT_FILENAME, 1);
  LWIP_ASSERT("failed to create file", f != NULL);

  err = tftp_get(f, &srv, TFTP_PORT, LWIP_TFTP_FLUDRIVER_CLIENT_FILENAME, TFTP_MODE_OCTET);
  LWIP_ASSERT("tftp_get failed", err == ERR_OK);
}

#endif /* LWIP_UDP */