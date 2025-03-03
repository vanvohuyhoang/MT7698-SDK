/* Copyright Statement:
 *
 * (C) 2018  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#include <stdint.h>
#include <stdio.h>

#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "hal_gpt.h"

#if LWIP_SOCKET
#include "lwip/sockets.h"
#endif
#include "mbedtls/debug.h"

#define DEBUG_LEVEL 3
#define CLK32_TICK_TO_MS (32)

#if defined(MQTT_TASK)
#include "task_def.h"
#endif

#include "MQTTFreeRTOS.h"

log_create_module(MQTT_CLIENT, PRINT_LEVEL_WARNING);

int mqtt_read(Network *n, unsigned char *buffer, int len, int timeout_ms);
int mqtt_write(Network *n, unsigned char *buffer, int len, int timeout_ms);
void mqtt_disconnect(Network *n);

static unsigned int mqtt_current_time_ms(void)
{
    unsigned int current_ms = 0;
    uint32_t count = 0;
    uint64_t count_temp = 0;
    hal_gpt_status_t ret_status;

    ret_status = hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &count);
    if (HAL_GPT_STATUS_OK != ret_status) {
        MQTT_DBG("[%s:%d]get count error, ret_status = %d \n", __FUNCTION__, __LINE__, ret_status);
    }

    count_temp = (uint64_t)count * 1000;
    current_ms = (uint32_t)(count_temp / 32768);
    return current_ms;
}

// MQTT Timer Porting
char TimerIsExpired(Timer *timer)
{
    unsigned int cur_time = 0;
    cur_time = mqtt_current_time_ms();
    if (timer->end_time < cur_time || timer->end_time == cur_time) {
        MQTT_DBG("MQTT expired enter");
        return 1;
    } else {
        MQTT_DBG("MQTT not expired");
        return 0;
    }
}

void TimerCountdownMS(Timer *timer, unsigned int timeout)
{
    timer->end_time = mqtt_current_time_ms() + timeout;
}

void TimerCountdown(Timer *timer, unsigned int timeout)
{
    timer->end_time = mqtt_current_time_ms() + (timeout * 1000);
}

int TimerLeftMS(Timer *timer)
{
    unsigned int cur_time = 0;
    cur_time = mqtt_current_time_ms();
    if (timer->end_time < cur_time || timer->end_time == cur_time) {
        return 0;
    } else {
        return timer->end_time - cur_time;
    }
}

void TimerInit(Timer *timer)
{
    timer->end_time = 0;
}


int mqtt_read(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    int rc = 0;
    int recvlen = 0;
    int ret = -1;
    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    FD_SET(n->my_socket, &fdset);

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    ret = select(n->my_socket + 1, &fdset, NULL, NULL, &tv);
    MQTT_INFO("mqtt read timer=%d ms ret=%d", timeout_ms, ret);
    if (ret < 0) {
        MQTT_ERR("mqtt read(select) fail ret=%d", ret);
        return -1;
    } else if (ret == 0) {
        MQTT_DBG("mqtt read(select) timeout");
        return -2;
    } else if (ret == 1) {
        do {
            MQTT_DBG("mqtt read recv len = %d, recvlen = %d", len, recvlen);
            rc = recv(n->my_socket, buffer + recvlen, len - recvlen, 0);
            if (rc > 0) {
                recvlen += rc;
                MQTT_DBG("mqtt read ret=%d, rc = %d, recvlen = %d", ret, rc, recvlen);
            } else {
                MQTT_ERR("mqtt read fail: ret=%d, rc = %d, recvlen = %d", ret, rc, recvlen);
                if (n->on_disconnect_callback) {
                    n->on_disconnect_callback(n);
                }
                return -3;
            }
        } while (recvlen < len);
    }
    return recvlen;
}


int mqtt_write(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    int rc = 0;
    int ret = -1;
    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    FD_SET(n->my_socket, &fdset);


    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    MQTT_INFO("mqtt write timer=%d ms", timeout_ms);
    ret = select(n->my_socket + 1, NULL, &fdset, NULL, &tv);

    if (ret < 0) {
        MQTT_ERR("mqtt write fail");
        return -1;
    } else if (ret == 0) {
        MQTT_WARN("mqtt write timeout");
        return -2;
    } else if (ret == 1) {
        // hex_dump("MQTT_write", buffer, len);
        rc = write(n->my_socket, buffer, len);
    }
    return rc;

}

void mqtt_disconnect(Network *n)
{
    close(n->my_socket);
}

void NewNetwork(Network *n)
{
    memset(n, 0, sizeof(Network));
    n->my_socket = -1;
    n->mqttread = mqtt_read;
    n->mqttwrite = mqtt_write;
    n->disconnect = mqtt_disconnect;
}

int ConnectNetwork(Network *n, char *addr,  char *port)
{
    int type = SOCK_STREAM;
    struct sockaddr_in address;
    int rc = -1;
    sa_family_t family = AF_INET;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

    if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0) {
        struct addrinfo *res = result;

        /* prefer ip4 addresses */
        while (res) {
            if (res->ai_family == AF_INET) {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET) {
            address.sin_port = htons(atoi(port));
            address.sin_family = family = AF_INET;
            address.sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;
        } else {
            rc = -1;
        }
        freeaddrinfo(result);
    }

    /* create client socket */
    if (rc == 0) {
        int opval = 1;
        n->my_socket = socket(family, type, 0);
        if (n->my_socket < 0) {
            MQTT_ERR("mqtt socket create fail");
            return -1;
        }
        /* connect remote servers*/
        rc = connect(n->my_socket, (struct sockaddr *)&address, sizeof(address));
        if (rc < 0) {
            close(n->my_socket);
            MQTT_ERR("mqtt socket connect fail:rc=%d,socket = %d", rc, n->my_socket);
            return -2;
        }
        setsockopt(n->my_socket , IPPROTO_TCP, TCP_NODELAY, &opval, sizeof(opval));
    }

    return rc;
}

u32_t mqtt_avRandom()
{
    return (((u32_t)rand() << 16) + rand());
}

static int mqtt_ssl_random(void *p_rng, unsigned char *output, size_t output_len)
{
    uint32_t rnglen = output_len;
    uint8_t   rngoffset = 0;

    while (rnglen > 0) {
        *(output + rngoffset) = (unsigned char)mqtt_avRandom() ;
        rngoffset++;
        rnglen--;
    }
    return 0;
}

static void mqtt_ssl_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    MQTT_DBG("%s\n", str);
}

int mqtt_real_confirm(int verify_result)
{
#define VERIFY_ITEM(Result, Item, ErrMsg) \
    do { \
        if (((Result) & (Item)) != 0) { \
            MQTT_DBG(ErrMsg); \
        } \
    } while (0)

    MQTT_INFO("certificate verification result: 0x%02x", verify_result);
    VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_EXPIRED, "! fail ! server certificate has expired");
    VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_REVOKED, "! fail ! server certificate has been revoked");
    VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_CN_MISMATCH, "! fail ! CN mismatch");
    VERIFY_ITEM(verify_result, MBEDTLS_X509_BADCERT_NOT_TRUSTED, "! fail ! self-signed or not signed by a trusted CA");
    return 0;
}

static int ssl_parse_crt(mbedtls_x509_crt *crt)
{
    char buf[1024];
    mbedtls_x509_crt *local_crt = crt;
    int i = 0;
    while (local_crt) {
        MQTT_DBG("# %d\r\n", i);
        mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", local_crt);
        {
            char str[512];
            const char *start, *cur;
            start = buf;
            for (cur = buf; *cur != '\0'; cur++) {
                if (*cur == '\n') {
                    size_t len = cur - start + 1;
                    if (len > 511) {
                        len = 511;
                    }
                    memcpy(str, start, len);
                    str[len] = '\0';
                    start = cur + 1;
                    MQTT_DBG("%s", str);
                }
            }
        }
        MQTT_DBG("crt content:%d!\r\n", strlen(buf));
        local_crt = local_crt->next;
        i++;
    }
    return i;
}

int mqtt_ssl_client_init(mbedtls_ssl_context *ssl,
                         mbedtls_net_context *tcp_fd,
                         mbedtls_ssl_config *conf,
                         mbedtls_x509_crt *crt509_ca, const char *ca_crt, size_t ca_len,
                         mbedtls_x509_crt *crt509_cli, const char *cli_crt, size_t cli_len,
                         mbedtls_pk_context *pk_cli, const char *cli_key, size_t key_len,  const char *cli_pwd, size_t pwd_len
                        )
{
    int ret = -1;
    //verify_source_t *verify_source = &custom_config->verify_source;

    /*
     * 0. Initialize the RNG and the session data
     */
#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif
    mbedtls_net_init(tcp_fd);
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_x509_crt_init(crt509_ca);

    /*verify_source->trusted_ca_crt==NULL
     * 0. Initialize certificates
     */

    MQTT_INFO("  . Loading the CA root certificate ...");
    if (NULL != ca_crt) {
        if (0 != (ret = mbedtls_x509_crt_parse(crt509_ca, (const unsigned char *)ca_crt, ca_len))) {
            MQTT_ERR(" failed ! x509parse_crt returned -0x%04x", -ret);
            return ret;
        }
    }
    ssl_parse_crt(crt509_ca);
    MQTT_INFO(" ok (%d skipped)", ret);


    /* Setup Client Cert/Key */
#if defined(MBEDTLS_X509_CRT_PARSE_C)
#if defined(MBEDTLS_CERTS_C)
    mbedtls_x509_crt_init(crt509_cli);
    mbedtls_pk_init(pk_cli);
#endif
    if (cli_crt != NULL && cli_key != NULL) {
#if defined(MBEDTLS_CERTS_C)
        MQTT_INFO("start prepare client cert .\n");
        ret = mbedtls_x509_crt_parse(crt509_cli, (const unsigned char *) cli_crt, cli_len);
#else
        {
            ret = 1;
            MQTT_ERR("MBEDTLS_CERTS_C not defined.");
        }
#endif
        if (ret != 0) {
            MQTT_ERR(" failed!  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
            return ret;
        }

#if defined(MBEDTLS_CERTS_C)
        MQTT_INFO("start mbedtls_pk_parse_key[%s]", cli_pwd);
        ret = mbedtls_pk_parse_key(pk_cli,
                                   (const unsigned char *) cli_key, key_len,
                                   (const unsigned char *) cli_pwd, pwd_len);
#else
        {
            ret = 1;
            MQTT_ERR("MBEDTLS_CERTS_C not defined.");
        }
#endif

        if (ret != 0) {
            MQTT_ERR(" failed\n  !  mbedtls_pk_parse_key returned -0x%x\n\n", -ret);
            return ret;
        }
    }
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    return 0;
}


int mqtt_ssl_read_all(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    size_t readLen = 0;
    int ret = -1;

    MQTT_DBG("mqtt_ssl_read_all len=%d", len);
    MQTT_DBG("mqtt ssl read all timer=%d ms", timeout_ms);
    mbedtls_ssl_conf_read_timeout(&(n->conf), timeout_ms);
    while (readLen < len) {
        ret = mbedtls_ssl_read(&(n->ssl), (unsigned char *)(buffer + readLen), (len - readLen));
        MQTT_DBG("%s, mbedtls_ssl_read return:%d", __func__, ret);
        if (ret > 0) {
            readLen += ret;
        } else if (ret == 0 || ret == MBEDTLS_ERR_SSL_TIMEOUT) {
            MQTT_DBG("mqtt ssl read timeout");
            return -2; //eof
        } else {
            if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {//read already complete(if call mbedtls_ssl_read again, it will return 0(eof))
                MQTT_WARN("MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY");
                if (n->on_disconnect_callback) {
                    n->on_disconnect_callback(n);
                }
                return -3;
            }
            MQTT_ERR("mqtt_ssl_read_all fail, read ret = %d", ret);
            if (n->on_disconnect_callback) {
                n->on_disconnect_callback(n);
            }
            return -1; //Connnection error
        }
    }
    MQTT_DBG("mqtt_ssl_read_all readlen=%d", readLen);
    return readLen;
}

int mqtt_ssl_write_all(Network *n, unsigned char *buffer, int len, int timeout_ms)
{
    size_t writtenLen = 0;
    int ret = -1;
    MQTT_DBG("mqtt_ssl_write_all len=%d timeout_ms=%d", len, timeout_ms);

    while (writtenLen < len) {
        ret = mbedtls_ssl_write(&(n->ssl), (unsigned char *)(buffer + writtenLen), (len - writtenLen));
        if (ret > 0) {
            writtenLen += ret;
            continue;
        } else if (ret == 0) {
            MQTT_DBG("mqtt ssl write timeout");
            return writtenLen;
        } else {
            MQTT_DBG("mqtt ssl write fail");
            return -1;
        }
    }
    MQTT_DBG("mqtt ssl write len=%d", writtenLen);
    return writtenLen;
}

void mqtt_ssl_disconnect(Network *n)
{
    if (n == NULL || n->fd.fd == -1) {
        return;
    }
    mbedtls_ssl_close_notify(&(n->ssl));
    mbedtls_net_free(&(n->fd));
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_free(&(n->cacertl));
    if ((n->pkey).pk_info != NULL) {
        MQTT_INFO("mqtt need free client crt&key");
        mbedtls_x509_crt_free(&(n->clicert));
        mbedtls_pk_free(&(n->pkey));
    }
#endif
    mbedtls_ssl_free(&(n->ssl));
    mbedtls_ssl_config_free(&(n->conf));
    MQTT_DBG(" mqtt_ssl_disconnect\n");
}


int TLSConnectNetwork(Network *n, const char *addr, const char *port,
                      const char *ca_crt, size_t ca_crt_len,
                      const char *client_crt,   size_t client_crt_len,
                      const char *client_key,   size_t client_key_len,
                      const char *client_pwd, size_t client_pwd_len)
{
    int ret = -1;
    /*
     * 0. Init
     */
    if (0 != (ret = mqtt_ssl_client_init(&(n->ssl), &(n->fd), &(n->conf),
                                         &(n->cacertl), ca_crt, ca_crt_len,
                                         &(n->clicert), client_crt, client_crt_len,
                                         &(n->pkey), client_key, client_key_len, client_pwd, client_pwd_len))) {
        MQTT_ERR(" failed ! ssl_client_init returned -0x%04x", -ret);
        goto exit;
    }

    /*
     * 1. Start the connection
     */
    MQTT_INFO("  . Connecting to tcp/%s/%s...", addr, port);
    if (0 != (ret = mbedtls_net_connect(&(n->fd), addr, port, MBEDTLS_NET_PROTO_TCP))) {
        MQTT_ERR(" failed ! net_connect returned -0x%04x", -ret);
        goto exit;
    }
    MQTT_INFO(" ok\n");

    /*
     * 2. Setup stuff
     */
    MQTT_INFO("  . Setting up the SSL/TLS structure...");
    if ((ret = mbedtls_ssl_config_defaults(&(n->conf),
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        MQTT_ERR(" failed! mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }
    MQTT_INFO(" ok");

    /*
     * OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example
     */
    if (ca_crt != NULL) {
        mbedtls_ssl_conf_authmode(&(n->conf), MBEDTLS_SSL_VERIFY_OPTIONAL);
    } else {
        mbedtls_ssl_conf_authmode(&(n->conf), MBEDTLS_SSL_VERIFY_NONE);
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_ssl_conf_ca_chain(&(n->conf), &(n->cacertl), NULL);

    if ((ret = mbedtls_ssl_conf_own_cert(&(n->conf), &(n->clicert), &(n->pkey))) != 0) {
        MQTT_ERR(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        goto exit;
    }
#endif
    mbedtls_ssl_conf_rng(&(n->conf), mqtt_ssl_random, NULL);
    mbedtls_ssl_conf_dbg(&(n->conf), mqtt_ssl_debug, NULL);


    if ((ret = mbedtls_ssl_setup(&(n->ssl), &(n->conf))) != 0) {
        MQTT_ERR(" failed! mbedtls_ssl_setup returned %d", ret);
        goto exit;
    }
    mbedtls_ssl_set_hostname(&(n->ssl), addr);
    mbedtls_ssl_set_bio(&(n->ssl), &(n->fd), mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    /*
     * 4. Handshake
     */
    MQTT_INFO("  . Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&(n->ssl))) != 0) {
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            MQTT_ERR(" failed  ! mbedtls_ssl_handshake returned -0x%04x", -ret);
            goto exit;
        }
    }
    MQTT_INFO(" ok");
    /*
     * 5. Verify the server certificate
     */
    MQTT_DBG("  . Verifying peer X.509 certificate..");
    if (0 != (ret = mqtt_real_confirm(mbedtls_ssl_get_verify_result(&(n->ssl))))) {
        MQTT_ERR(" failed  ! verify result not confirmed.");
        goto exit;
    }
    ret = 0;

exit:
    if (ret < 0) {
        mqtt_ssl_disconnect(n);
    } else {
        n->my_socket = (int)((n->fd).fd);
        MQTT_INFO("my_socket=%d", n->my_socket);
        n->mqttread = mqtt_ssl_read_all;
        n->mqttwrite = mqtt_ssl_write_all;
        n->disconnect = mqtt_ssl_disconnect;
    }

    return ret;
}

#if defined(MQTT_TASK)
void MutexInit(Mutex *pxMutex)
{
    *pxMutex = xSemaphoreCreateMutex();
    if (*pxMutex == NULL) {
        MQTT_ERR("MQTT MutexInit Fail");
    }
}

void MutexLock(Mutex *pxMutex)
{
    while (xSemaphoreTake(*pxMutex, portMAX_DELAY) != pdPASS);
}

void MutexUnlock(Mutex *pxMutex)
{
    xSemaphoreGive(*pxMutex);
}

int ThreadStart(Thread *thread, void(*pxThread)(void *pvParameters), void *pvArg)
{
    TaskHandle_t xCreatedTask;
    sys_thread_t xReturn;
    portBASE_TYPE xResult = xTaskCreate(pxThread, MQTT_TASK_NAME,
                                        MQTT_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                                        pvArg, MQTT_TASK_PRIO, thread);
    return (xResult == pdPASS);
}

#endif
