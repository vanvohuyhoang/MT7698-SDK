/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "FreeRTOS.h"
#include "task.h"
#include "wifi_api.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/etharp.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/ip4_addr.h"
#include "ethernetif.h"

#include "dhcpd.h"
#include "nvdm.h"
#include "task_def.h"


/* Define 0 to disable logging, define 1 to enable logging. */
#ifndef MTK_DEBUG_LEVEL_NONE
#define DHCPD_DEBUG 1
#else
#define DHCPD_DEBUG 0
#endif

/* The following content is used inside the DHCPD module. */
#if DHCPD_DEBUG
#define DHCPD_PRINTF(x, ...) LOG_I(dhcpd, x, ##__VA_ARGS__)
#define DHCPD_WARN(x, ...) LOG_W(dhcpd, x, ##__VA_ARGS__)
#define DHCPD_ERR(x, ...) LOG_E(dhcpd, x, ##__VA_ARGS__)
#else
#define DHCPD_PRINTF(x, ...)
#define DHCPD_WARN(x, ...)
#define DHCPD_ERR(x, ...)
#endif


/*
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
   +---------------+---------------+---------------+---------------+
   |                            xid (4)                            |
   +-------------------------------+-------------------------------+
   |           secs (2)            |           flags (2)           |
   +-------------------------------+-------------------------------+
   |                          ciaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          yiaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          siaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          giaddr  (4)                          |
   +---------------------------------------------------------------+
   |                          chaddr  (16)                         |
   +---------------------------------------------------------------+
   |                                                               |
   |                          sname   (64)                         |
   +---------------------------------------------------------------+
   |                                                               |
   |                          file    (128)                        |
   +---------------------------------------------------------------+
   |                                                               |
   |                          options (variable)                   |
   +---------------------------------------------------------------+

                  Figure 1:  Format of a DHCP message
 */

typedef struct {
    unsigned char  op;                /* Message op code / Message type */
    unsigned char  htype;            /* Hardware address type (see ARP section in "Assigned Numbers" RFC; e.g., '1' = 10mb Ethernet.) */
    unsigned char  hlen;              /* hardware address length (e.g.  '6' for 10mb Ethernet) */
    unsigned char  hops;              /* Optionally used by relay agents when booting via a relay agent. */
    unsigned int   xid;              /* Transaction ID */
    unsigned short secs;            /* seconds elapsed since client began address acquisition or renewal process */
    unsigned short flags;            /* Flags, BROADCAST flag */
    unsigned char  ciaddr[4];         /* Client IP address */
    unsigned char  yiaddr[4];         /* 'your' (client) IP address */
    unsigned char  siaddr[4];         /* IP address of next server to use in bootstrap */
    unsigned char  giaddr[4];          /* Relay agent IP address */
    unsigned char  chaddr[16];         /* Client hardware address */
    unsigned char  sname[64];        /* Optional server host name */
    unsigned char  file[128];        /* Optional parameters field */
    unsigned char  magic[4];        /* Magic Cookie (Vendor), 63,82,53,63*/
    unsigned char  options[308];    /* options(variable) content*/
} dhcpd_message_t;

typedef struct dhcpd_alloc_info {
    struct dhcpd_alloc_info *next;
    unsigned char mac[6];
    struct ip4_addr ip_addr;
} dhcpd_alloc_info_t;


#define DHCPD_DOMAIN_NAME    "example.org"

#define DHCPD_OP_REQ              1
#define DHCPD_OP_REPLY            2

#define DHCPD_SERVER_PORT 67
#define DHCPD_CLIENT_PORT 68


/* DHCP Options */
#define DHCPD_OPT_NETMASK           1
#define DHCPD_OPT_ROUTER            3
#define DHCPD_OPT_DNSSERVER         6
#define DHCPD_OPT_DOMAINNAME        15
#define DHCPD_OPT_BROADCAST_ADDR    28
#define DHCPD_OPT_REQUESTED_IP      50
#define DHCPD_OPT_LEASE_TIME        51
#define DHCPD_OPT_MESSAGE_TYPE      53
#define DHCPD_OPT_SERVER_IDENTIFIER 54
#define DHCPD_OPT_MESSAGE           56
#define DHCPD_OPT_T1                58
#define DHCPD_OPT_T2                59
#define DHCPD_OPT_END               255

#define DHCPD_DISCOVER    1
#define DHCPD_OFFER       2
#define DHCPD_REQUEST     3
#define DHCPD_DECLINE     4        //Not using in this release.
#define DHCPD_ACK         5
#define DHCPD_NAK         6
#define DHCPD_RELEASE     7
#define DHCPD_INFORM      8        //Not using in this release.

#define PROFILE_BUF_LEN (64)    //buffer length to get NVRAM

/* These addresses are all in network order. */
static struct ip4_addr dhcpd_primary_dns;
static struct ip4_addr dhcpd_secondary_dns;
static struct ip4_addr dhcpd_ip_pool_start;
static struct ip4_addr dhcpd_ip_pool_end;
static struct ip4_addr dhcpd_last_alloc_ip;
static struct ip4_addr dhcpd_server_address;    /* IP address of dhcp server. */
static struct ip4_addr dhcpd_server_netmask;
static struct ip4_addr dhcpd_server_gw;            /* IP address of gateway. Usually, it's router's IP. */

static xTaskHandle dhcpd_task_handle = 0;
static int dhcpd_running = 0;
static int dhcpd_socket = -1;

static xSemaphoreHandle dhcpd_mutex;

static unsigned char dhcpd_send_ack; /* always initialize to 1 in dhcpd_do_request() */
static dhcpd_message_t *dhcpd_msg;

static dhcpd_alloc_info_t *dhcpd_alloc_infos_in_use;
#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
static dhcpd_alloc_info_t *dhcpd_alloc_infos_unuse;
#endif

const char dhcpd_opt_magic_tmp[]        = {0x63, 0x82, 0x53, 0x63};
const char dhcpd_opt_subnet_tmp[]       = {DHCPD_OPT_NETMASK, 4, 255, 255, 255, 0};
const char dhcpd_opt_offer_tmp[]        = {DHCPD_OPT_MESSAGE_TYPE, 1, DHCPD_OFFER};
const char dhcpd_opt_ack_tmp[]          = {DHCPD_OPT_MESSAGE_TYPE, 1, DHCPD_ACK};
const char dhcpd_opt_nak_tmp[]          = {DHCPD_OPT_MESSAGE_TYPE, 1, DHCPD_NAK};
const char dhcpd_opt_msg_tmp[]          = {DHCPD_OPT_MESSAGE, 13, 'w', 'r', 'o', 'n', 'g', ' ', 'n', 'e', 't', 'w', 'o', 'r', 'k'};
const unsigned char dhcpd_opt_end_tmp[] = {DHCPD_OPT_END};
const unsigned char dhcpd_zeros[6]      = {0, 0, 0, 0, 0, 0};

static void dhcpd_prepare(void *param);

#if DHCPD_DEBUG
log_create_module(dhcpd, PRINT_LEVEL_INFO);
#endif

#if 0
static void dhcpd_hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen)
{
    unsigned char *pt;
    int x;

    pt = pSrcBufVA;
    printf("%s: %p, len = %d\n\r", str, pSrcBufVA, SrcBufLen);
    for (x = 0; x < SrcBufLen; x++) {
        if (x % 16 == 0) {
            printf("0x%04x : ", x);
        }
        printf("%02x ", ((unsigned char)pt[x]));
        if (x % 16 == 15) {
            printf("\n\r");
        }
    }
    printf("\n\r");
}
#endif

static void dhcpd_print_ip_allocation_status(void)
{
    dhcpd_alloc_info_t *alloc_info = dhcpd_alloc_infos_in_use;

    while (alloc_info) {
        DHCPD_PRINTF("[%02X:%02X:%02X:%02X:%02X:%02X][%d.%d.%d.%d]",
                     (alloc_info->mac)[0], (alloc_info->mac)[1],
                     (alloc_info->mac)[2], (alloc_info->mac)[3],
                     (alloc_info->mac)[4], (alloc_info->mac)[5],
                     ip4_addr1(&(alloc_info->ip_addr)),
                     ip4_addr2(&(alloc_info->ip_addr)),
                     ip4_addr3(&(alloc_info->ip_addr)),
                     ip4_addr4(&(alloc_info->ip_addr)));
        alloc_info = alloc_info->next;
    }
}


static void dhcpd_mutex_new(void)
{
    if (dhcpd_mutex == NULL) {
        dhcpd_mutex = xSemaphoreCreateMutex();
    }

    if (dhcpd_mutex == NULL) {
        DHCPD_PRINTF("Mutex create failed.");
    }
}

static void dhcpd_mutex_lock(void)
{
    while (xSemaphoreTake(dhcpd_mutex, portMAX_DELAY) != pdPASS);
}


static void dhcpd_mutex_unlock(void)
{
    xSemaphoreGive(dhcpd_mutex);
}


#if 0
static void dhcpd_mutex_free(void)
{
    vQueueDelete(dhcpd_mutex);
}
#endif


static void dhcpd_log_ip(char *intro, struct ip4_addr *ip)
{
    if (!ip) {
        return;
    }

    DHCPD_PRINTF("[%s]%s", intro, inet_ntoa(*ip));
}


int dhcpd_insert_alloc_info_into_list(dhcpd_alloc_info_t *alloc_info, dhcpd_alloc_info_t **alloc_info_list)
{
    dhcpd_alloc_info_t *tmp_alloc_info = NULL;

    // DHCPD_PRINTF("dhcpd_insert_alloc_info_into_list()");

    if (!alloc_info || !alloc_info_list) {
        return -1;
    }

#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
    DHCPD_PRINTF("Insert into %s", alloc_info_list == &dhcpd_alloc_infos_in_use ? "in use list" : \
                    (*alloc_info_list == dhcpd_alloc_infos_unuse ? "unsue list" : "unknown list"));
#endif

    if (NULL == *alloc_info_list) {
        *alloc_info_list = alloc_info;
    } else {
        tmp_alloc_info = *alloc_info_list;
        while (tmp_alloc_info->next) {
            tmp_alloc_info = tmp_alloc_info->next;
        }

        tmp_alloc_info->next = alloc_info;
    }

    // Add <mac address - IP> in ARP list
#if ETHARP_SUPPORT_STATIC_ENTRIES
    if (alloc_info_list == &dhcpd_alloc_infos_in_use) {
        etharp_add_static_entry(&alloc_info->ip_addr, (struct eth_addr *)alloc_info->mac);
    }
#endif

    return 0;
}


int dhcpd_remove_alloc_info_from_list(dhcpd_alloc_info_t *alloc_info,
                                              dhcpd_alloc_info_t *pre_alloc_info,
                                              dhcpd_alloc_info_t **alloc_info_list)
{
    if (!alloc_info || (!pre_alloc_info && !alloc_info_list) || (alloc_info_list && !(*alloc_info_list))) {
        DHCPD_PRINTF("dhcpd_remove_alloc_info_from_list() failed. ");
        return -1;
    }

    if (pre_alloc_info) {
        pre_alloc_info->next = alloc_info->next;
    } else if (alloc_info_list) {
        *alloc_info_list = alloc_info->next;
    }

    alloc_info->next = NULL;
    // Add <mac address - IP> in ARP list
#if ETHARP_SUPPORT_STATIC_ENTRIES
    if (alloc_info_list == &dhcpd_alloc_infos_in_use) {
        etharp_remove_static_entry(&alloc_info->ip_addr);
    }
#endif
    DHCPD_PRINTF("dhcpd_remove_alloc_info_from_list() succeed. ");
    return 0;
}


/* pre_alloc_info [OUT] The node before the target node. If the target node is the head of the list,
 * pre_alloc_info will be NULL. */
dhcpd_alloc_info_t *dhcpd_find_alloc_info_by_mac(unsigned char mac[6],
                                                         dhcpd_alloc_info_t *alloc_info_list,
                                                         dhcpd_alloc_info_t **pre_alloc_info)
{
    dhcpd_alloc_info_t *alloc_info = NULL, *previous_alloc_info = NULL;

    // DHCPD_PRINTF("dhcpd_find_alloc_info_by_mac()");

    if (!mac || !alloc_info_list) {
        return NULL;
    }

#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
    DHCPD_PRINTF("Try to find in %s", alloc_info_list == dhcpd_alloc_infos_in_use ? "in-use list" : \
                    (alloc_info_list == dhcpd_alloc_infos_unuse ? "unsue list" : "unknown list"));
#endif

    alloc_info = alloc_info_list;

    while (alloc_info) {
        if (memcmp(mac, alloc_info->mac, 6) == 0) {
            DHCPD_PRINTF("Find the alloc_info node");
            /* Find the alloc_info node. */
            if (pre_alloc_info) {
                *pre_alloc_info = previous_alloc_info;
            }
            return alloc_info;
        }

        previous_alloc_info = alloc_info;
        alloc_info = alloc_info->next;
    }

    /* Failed to find the alloc_info node. */
    DHCPD_PRINTF("Failed to find the alloc_info node");
    return NULL;
}


/* Both ip_addr1 and ip_addr2 are in network order.
  * Return: 1   ip_addr1 > ip_addr2
  *            0   ip_addr1 == ip_addr2
  *            -2  ip_addr1 < ip_addr2
  *            -1 Input parameter error
  */
int dhcpd_ip_cmp(struct ip4_addr *ip_addr1, struct ip4_addr *ip_addr2)
{
    uint8_t i = 0, *ip1 = NULL, *ip2 = NULL;
    int res = 0;

    if (!ip_addr1 || !ip_addr2) {
        return -1;
    }

    ip1 = (uint8_t *)(&(ip_addr1->addr));
    ip2 = (uint8_t *)(&(ip_addr2->addr));

    for (i = 0; i < 4; i++) {
        res = ip1[i] - ip2[i];

        if (0 == res) {
            continue;
        }

        return res > 0 ? 1 : -2;
    }

    return 0;
}


/* ip_addr is in network order. */
int dhcpd_is_ip_allocable(struct ip4_addr *ip_addr)
{
    dhcpd_alloc_info_t *alloc_info = NULL;

    if (!ip_addr || !dhcpd_task_handle) {
        /* settings maybe hasn't been set yet */
        return -1;
    }

    if (0 < dhcpd_ip_cmp(&dhcpd_ip_pool_start, ip_addr) ||
        0 > dhcpd_ip_cmp(&dhcpd_ip_pool_end, ip_addr)) {
        /* Out of range */
        return -2;
    }

    /* For start and end both match newmask, addresses which are not out of range match the netmask for sure. */

    if (0 == ip4_addr4(ip_addr)) {
        return -3;
    }

    if (255 == ip4_addr4(ip_addr)) {
        return -4;
    }

    if (ip4_addr_cmp(ip_addr, &dhcpd_server_gw)) {
        return -5;
    }

    /* Check in-use list */
    alloc_info = dhcpd_alloc_infos_in_use;
    while (alloc_info) {
        if (0 == dhcpd_ip_cmp(&(alloc_info->ip_addr), ip_addr)) {
            /* IP has been allocated in in-use list. */
            return -6;
        }

        alloc_info = alloc_info->next;
    }

#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
    /* Check unuse list */
    alloc_info = dhcpd_alloc_infos_unuse;
    while (alloc_info) {
        if (0 == dhcpd_ip_cmp(&(alloc_info->ip_addr), ip_addr)) {
            /* IP has been allocated in unuse list. */
            return -7;
        }

        alloc_info = alloc_info->next;
    }
#endif

    return 0;
}


/* new_ip is in network order.
  * Return: 0 Brand new IP
  *            1 Reuse IP in unuse list
  */
int dhcpd_alloc_new_ip(struct ip4_addr *new_ip)
{
    struct ip4_addr tmp_ip = { 0 };
    int ret = -1, from_start = 0;
#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
    dhcpd_alloc_info_t *alloc_info = NULL;
#endif

    if (!new_ip) {
        return -1;
    }

    if (ip4_addr_isany_val(dhcpd_last_alloc_ip)) {
        /* First allocation */
        memcpy(&(new_ip->addr), &(dhcpd_ip_pool_start.addr), 4);
        DHCPD_PRINTF("dhcpd_last_alloc_ip is 0s.");
        dhcpd_log_ip("New IP", new_ip);
        return 0;
    }

    /* dhcpd_last_alloc_ip may be available. */
    tmp_ip.addr = lwip_ntohl(dhcpd_last_alloc_ip.addr);
    new_ip->addr = lwip_htonl(tmp_ip.addr);

    while (0 > (ret = dhcpd_is_ip_allocable(new_ip))) {
        DHCPD_PRINTF("Is ip allocable ret:%d", ret);
        /* Out of range. Go back to the start. */
        if (-2 == ret) {
            if (from_start) {
                DHCPD_PRINTF("NOT FOUND: Out of range twice.");
                break;
            }

            from_start = 1;
            tmp_ip.addr = lwip_ntohl(dhcpd_ip_pool_start.addr);
        } else {
            tmp_ip.addr++;
        }

        new_ip->addr = lwip_htonl(tmp_ip.addr);

        if (0 == dhcpd_ip_cmp(new_ip, &dhcpd_last_alloc_ip)) {
            DHCPD_PRINTF("NOT FOUND: Complete one cycle.");
            break;
        }
    }

#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
    if (0 > ret) {
        alloc_info = dhcpd_alloc_infos_unuse;
        dhcpd_remove_alloc_info_from_list(alloc_info, NULL, &dhcpd_alloc_infos_unuse);
        if (alloc_info) {
            DHCPD_PRINTF("Free oldest node in unuse list to release ip it occupied for new assignment req.");
            memcpy(&alloc_info->ip_addr, new_ip, sizeof(struct ip4_addr));
            vPortFree(alloc_info);
            ret = 1;
        }
    }
#endif

    if (0 <= ret) {
        dhcpd_log_ip("New IP", new_ip);
    }

    return ret;
}


static int dhcpd_opt_dns_tlv_int(char *dest, struct ip4_addr *dns1, struct ip4_addr *dns2)
{
    char *opt_len = NULL;
    char *buf_start = dest;


    if ((!dns1 && !dns2) || !dest) {
        return 0;
    }

    *dest++ = DHCPD_OPT_DNSSERVER;
    opt_len = dest++;
    //DHCPD_PRINTF("opt_len: %x, dest:%x", opt_len, dest);
    if (dns1) {
        *dest++ = ip4_addr1(dns1);
        *dest++ = ip4_addr2(dns1);
        *dest++ = ip4_addr3(dns1);
        *dest++ = ip4_addr4(dns1);
    }

    if (dns2 && !ip4_addr_isany_val(*dns2)) {
        *dest++ = ip4_addr1(dns2);
        *dest++ = ip4_addr2(dns2);
        *dest++ = ip4_addr3(dns2);
        *dest++ = ip4_addr4(dns2);
    }

    *opt_len = dest - opt_len - 1;

    if (buf_start) {
        DHCPD_PRINTF("opt type[%d]%d, %d.%d.%d.%d, %d.%d.%d.%d",
                buf_start[0], buf_start[1],
                buf_start[2], buf_start[3], buf_start[4], buf_start[5],
                buf_start[6], buf_start[7], buf_start[8], buf_start[9]);
    }

    return *opt_len + 2;
}


void dhcpd_release_alloc_info_lists(void)
{
    dhcpd_alloc_info_t *alloc_info = NULL, *tmp_alloc_info = NULL;

    /* Release in-use list */
    alloc_info = dhcpd_alloc_infos_in_use;
    while (alloc_info) {
        tmp_alloc_info = alloc_info->next;
#if ETHARP_SUPPORT_STATIC_ENTRIES
        etharp_remove_static_entry(&alloc_info->ip_addr);
#endif
        vPortFree(alloc_info);
        alloc_info = tmp_alloc_info;
    }
    dhcpd_alloc_infos_in_use = NULL;

#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
    /* Release unuse list */
    alloc_info = dhcpd_alloc_infos_unuse;
    while (alloc_info) {
        tmp_alloc_info = alloc_info->next;
        vPortFree(alloc_info);
        alloc_info = tmp_alloc_info;
    }
    dhcpd_alloc_infos_unuse = NULL;
#endif
}


void dhcpd_memzero_settings(void)
{
    memset(&dhcpd_server_address, 0, sizeof(dhcpd_server_address));
    memset(&dhcpd_server_netmask, 0, sizeof(dhcpd_server_netmask));
    memset(&dhcpd_server_gw, 0, sizeof(dhcpd_server_gw));
    memset(&dhcpd_primary_dns, 0, sizeof(dhcpd_primary_dns));
    memset(&dhcpd_secondary_dns, 0, sizeof(dhcpd_secondary_dns));
    memset(&dhcpd_ip_pool_start, 0, sizeof(dhcpd_ip_pool_start));
    memset(&dhcpd_ip_pool_end, 0, sizeof(dhcpd_ip_pool_end));
}

int dhcpd_set_settings(char *ip_str_settings, char *ip_str_default, struct ip4_addr *ip_addr)
{
    if (!ip_addr || !(ip_str_settings || ip_str_default)) {
        return -1;
    }

    if (ip_str_settings) {
        /* inet_aton() will change IP from text format to 32bytes format in network order (big endian).  */
        if (!inet_aton(ip_str_settings, ip_addr)) {
            DHCPD_PRINTF("Input setting is invalid.");
            return -2;
        }
    } else {
        if (!inet_aton(ip_str_default, ip_addr)) {
            DHCPD_PRINTF("Default setting is invalid.");
            return -3;
        }
    }

    return 0;
}


int dhcpd_start(dhcpd_settings_t *dhcpd_settings)
{
    DHCPD_PRINTF("dhcpd_start [%d][%d]", (int)dhcpd_task_handle, dhcpd_running);

    dhcpd_mutex_new();
    dhcpd_mutex_lock();

    if (dhcpd_running == 0 && dhcpd_task_handle == 0) {

        DHCPD_PRINTF("DHCPD preparing");

        dhcpd_memzero_settings();

        if (dhcpd_settings) {
            if (0 > dhcpd_set_settings(dhcpd_settings->dhcpd_server_address, DHPCD_DEFAULT_SERVER_IP, &dhcpd_server_address) ||
                0 > dhcpd_set_settings(dhcpd_settings->dhcpd_gateway, DHPCD_DEFAULT_GATEWAY, &dhcpd_server_gw) ||
                0 > dhcpd_set_settings(dhcpd_settings->dhcpd_netmask, DHPCD_DEFAULT_NETMASK, &dhcpd_server_netmask) ||
                0 > dhcpd_set_settings(dhcpd_settings->dhcpd_primary_dns, DHPCD_DEFAULT_PRIMARY_DNS, &dhcpd_primary_dns) ||
                0 > dhcpd_set_settings(dhcpd_settings->dhcpd_secondary_dns, DHPCD_DEFAULT_SECONDARY_DNS, &dhcpd_secondary_dns) ||
                0 > dhcpd_set_settings(dhcpd_settings->dhcpd_ip_pool_start, DHPCD_DEFAULT_IP_POOL_START, &dhcpd_ip_pool_start) ||
                0 > dhcpd_set_settings(dhcpd_settings->dhcpd_ip_pool_end, DHPCD_DEFAULT_IP_POOL_END, &dhcpd_ip_pool_end)) {
                dhcpd_memzero_settings();
                return -2;
            }

            if (!ip4_addr_netmask_valid(dhcpd_server_netmask.addr) ||
                !ip4_addr_netcmp(&dhcpd_server_gw, &dhcpd_ip_pool_start, &dhcpd_server_netmask) ||
                !ip4_addr_netcmp(&dhcpd_ip_pool_start, &dhcpd_ip_pool_end, &dhcpd_server_netmask) ||
                0 < dhcpd_ip_cmp(&dhcpd_ip_pool_start, &dhcpd_ip_pool_end)) {

                dhcpd_log_ip("Server IP", &dhcpd_server_address);
                dhcpd_log_ip("Netmask", &dhcpd_server_netmask);
                dhcpd_log_ip("Gateway", &dhcpd_server_gw);
                dhcpd_log_ip("DNS1", &dhcpd_primary_dns);
                dhcpd_log_ip("DNS2", &dhcpd_secondary_dns);
                dhcpd_log_ip("Start IP", &dhcpd_ip_pool_start);
                dhcpd_log_ip("End IP", &dhcpd_ip_pool_end);

                dhcpd_memzero_settings();
                DHCPD_PRINTF("Something wrong with settings.");
                return -4;
            }
        } else {
            if (0 > dhcpd_set_settings(NULL, DHPCD_DEFAULT_SERVER_IP, &dhcpd_server_address) ||
                0 > dhcpd_set_settings(NULL, DHPCD_DEFAULT_GATEWAY, &dhcpd_server_gw) ||
                0 > dhcpd_set_settings(NULL, DHPCD_DEFAULT_NETMASK, &dhcpd_server_netmask) ||
                0 > dhcpd_set_settings(NULL, DHPCD_DEFAULT_PRIMARY_DNS, &dhcpd_primary_dns) ||
                0 > dhcpd_set_settings(NULL, DHPCD_DEFAULT_SECONDARY_DNS, &dhcpd_secondary_dns) ||
                0 > dhcpd_set_settings(NULL, DHPCD_DEFAULT_IP_POOL_START, &dhcpd_ip_pool_start) ||
                0 > dhcpd_set_settings(NULL, DHPCD_DEFAULT_IP_POOL_END, &dhcpd_ip_pool_end)) {
                dhcpd_memzero_settings();
                return -3;
            }
        }

        memset(&dhcpd_last_alloc_ip, 0, sizeof(dhcpd_last_alloc_ip));

        if (dhcpd_alloc_infos_in_use || dhcpd_alloc_infos_unuse) {
            dhcpd_release_alloc_info_lists();
        }

        if (pdFALSE == xTaskCreate(dhcpd_prepare, DHCPD_TASK_NAME, DHCPD_TASK_STACKSIZE/sizeof(portSTACK_TYPE), NULL, TASK_PRIORITY_NORMAL, &dhcpd_task_handle)) {
            DHCPD_PRINTF("DHCPD canot start task");;
        }
    } else {
        DHCPD_PRINTF("DHCPD no need to start.");
    }

    dhcpd_mutex_unlock();
    return 0;
}


static int32_t dhcpd_wifi_api_rx_event_handler(wifi_event_t evt,
                                               uint8_t      *payload,
                                               uint32_t     len)
{
    dhcpd_alloc_info_t *alloc_info = NULL, *pre_alloc_info = NULL;

    /* search for existing */
    DHCPD_PRINTF("wifi notifi");
    DHCPD_PRINTF("paylodlen=%d evt=%d,(%02X:%02X:%02X:%02X:%02X:%02X)", (int)len, evt, payload[0], payload[1], payload[2], payload[3], payload[4], payload[5]);
    if (evt != WIFI_EVENT_IOT_DISCONNECTED || dhcpd_running != 1) {
        return 0;
    }

    dhcpd_mutex_lock();

    alloc_info = dhcpd_find_alloc_info_by_mac(payload, dhcpd_alloc_infos_in_use, &pre_alloc_info);
    if (alloc_info) {
        //dhcpd_release_alloc_info(alloc_info);
        dhcpd_remove_alloc_info_from_list(alloc_info, pre_alloc_info, &dhcpd_alloc_infos_in_use);
#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
        dhcpd_insert_alloc_info_into_list(alloc_info, &dhcpd_alloc_infos_unuse);
#else
        vPortFree(alloc_info);
#endif

        dhcpd_print_ip_allocation_status();
        dhcpd_mutex_unlock();
        return 1;
    }

    dhcpd_mutex_unlock();
    return 0;
}


/* Return: 1 in in-use list
  *            2 in unuse list
  *            0 other
  */
static int dhcpd_lease_address(unsigned char mac[], unsigned char ip[])
{
    dhcpd_alloc_info_t *alloc_info = NULL, *pre_alloc_info = NULL;

    alloc_info = dhcpd_find_alloc_info_by_mac(mac, dhcpd_alloc_infos_in_use, NULL);
    if (alloc_info) {
        DHCPD_PRINTF("Assigned, Old Client");
        /* Both are big endian. */
        memcpy(ip, &(alloc_info->ip_addr.addr), 4);
        dhcpd_print_ip_allocation_status();
        return 1;
    } else {
#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
        alloc_info = dhcpd_find_alloc_info_by_mac(mac, dhcpd_alloc_infos_unuse, &pre_alloc_info);
        if (alloc_info) {
            DHCPD_PRINTF("Unassigned, Old Client");
            dhcpd_remove_alloc_info_from_list(alloc_info, pre_alloc_info, &dhcpd_alloc_infos_unuse);
            dhcpd_insert_alloc_info_into_list(alloc_info, &dhcpd_alloc_infos_in_use);
            memcpy(ip, &(alloc_info->ip_addr.addr), 4);
            dhcpd_print_ip_allocation_status();
            return 2;
        } else
#endif
        {
            DHCPD_PRINTF("Unassigned, New Client");
            alloc_info = (dhcpd_alloc_info_t *)pvPortMalloc(sizeof(dhcpd_alloc_info_t));
            if (!alloc_info) {
#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
                /* Assign the oldest unuse node to the new client. */
                alloc_info = dhcpd_alloc_infos_unuse;
                dhcpd_remove_alloc_info_from_list(alloc_info, NULL, &dhcpd_alloc_infos_unuse);
                DHCPD_PRINTF("Try to reuse oldest node in unuse list for new client when memory is not enough. 0x%x", (unsigned int)alloc_info);
                if (!alloc_info)
#endif
                {
                    DHCPD_PRINTF("Not enough memory for new allocation.");
                    return -1;
                }
            }

            memset(alloc_info, 0, sizeof(dhcpd_alloc_info_t));

            if (0 <= dhcpd_alloc_new_ip(&(alloc_info->ip_addr))) {
                memcpy(alloc_info->mac, mac, 6);
                memcpy(ip, &(alloc_info->ip_addr.addr), 4);
                memcpy(&dhcpd_last_alloc_ip.addr, &alloc_info->ip_addr.addr, 4);
                //dhcpd_insert_alloc_info(alloc_info);
                dhcpd_insert_alloc_info_into_list(alloc_info, &dhcpd_alloc_infos_in_use);
                dhcpd_print_ip_allocation_status();
                return 0;
            } else {
                // dhcpd_release_alloc_info(alloc_info);
                vPortFree(alloc_info);
                return -1;
            }
        }
    }

}

static int dhcpd_send_response(int type)
{
    struct sockaddr_in dest_addr;
    int ret = 0;
    ip4_addr_t temp_addr;

    dest_addr.sin_port   = htons(DHCPD_CLIENT_PORT);
    dest_addr.sin_family = AF_INET;
    temp_addr.addr = INADDR_BROADCAST;
    if ((type == DHCPD_ACK &&
                dhcpd_msg->yiaddr[0]==dhcpd_msg->ciaddr[0] &&
                dhcpd_msg->yiaddr[1]==dhcpd_msg->ciaddr[1] &&
                dhcpd_msg->yiaddr[2]==dhcpd_msg->ciaddr[2] &&
                dhcpd_msg->yiaddr[3]==dhcpd_msg->ciaddr[3])
#if ETHARP_SUPPORT_STATIC_ENTRIES
            || (!dhcpd_msg->flags && (type == DHCPD_ACK || type == DHCPD_OFFER))
#endif
        ) { //unicast
        IP4_ADDR(&temp_addr, dhcpd_msg->yiaddr[0], dhcpd_msg->yiaddr[1], dhcpd_msg->yiaddr[2], dhcpd_msg->yiaddr[3]);
        DHCPD_PRINTF("send to dest ip");
    }
    dhcpd_msg->flags = 0;
    DHCPD_PRINTF("sendto [%d][0x%08x]", sizeof(dhcpd_msg->ciaddr), (unsigned int)temp_addr.addr);
    dest_addr.sin_addr.s_addr = temp_addr.addr;
    //dhcpd_hex_dump("-----dhcpd_msg ready-----\n", (unsigned char *)dhcpd_msg, sizeof(dhcpd_message_t));
    ret = sendto(dhcpd_socket, (char *)dhcpd_msg, sizeof(dhcpd_message_t), 0 , (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    DHCPD_PRINTF("sendto ret=[%d] type[%d] [%02X:%02X:%02X:%02X:%02X:%02X][%d.%d.%d.%d]", ret, type,
                 dhcpd_msg->chaddr[0], dhcpd_msg->chaddr[1], dhcpd_msg->chaddr[2],
                 dhcpd_msg->chaddr[3], dhcpd_msg->chaddr[4], dhcpd_msg->chaddr[5],
                 dhcpd_msg->yiaddr[0], dhcpd_msg->yiaddr[1], dhcpd_msg->yiaddr[2], dhcpd_msg->yiaddr[3]);

    return ret;
}

static int dhcpd_opt_tlv_int(char *dest, char type, char v1, char v2, char v3, char v4)
{
    *dest++ = type;
    *dest++ = 4;
    *dest++ = v1;
    *dest++ = v2;
    *dest++ = v3;
    *dest++ = v4;
    DHCPD_PRINTF("opt type[%d]:%d.%d.%d.%d", type, v1, v2, v3, v4);

    return 6;
}

static int dhcpd_do_discover(struct sockaddr *source_addr)
{
    int ret = 0;
    struct ip4_addr ip_addr;
    char *option_ptr;
    unsigned int lease_time = DHCPD_DEFAULT_LEASE_TIME;
    unsigned char *lease_ptr = (unsigned char *)&lease_time;

    ip_addr.addr = *((unsigned int *) &dhcpd_server_address);

    if (dhcpd_lease_address(dhcpd_msg->chaddr , dhcpd_msg->yiaddr) != -1) {
        DHCPD_PRINTF("do discover:%d.%d.%d.%d", dhcpd_msg->yiaddr[0], dhcpd_msg->yiaddr[1], dhcpd_msg->yiaddr[2], dhcpd_msg->yiaddr[3]);

        dhcpd_msg->op = DHCPD_OP_REPLY;
        dhcpd_msg->secs = 0;

        memset(dhcpd_msg->options, 0, sizeof(dhcpd_msg->options));
        memcpy(dhcpd_msg->magic, dhcpd_opt_magic_tmp, 4);
        //dhcpd_hex_dump("-----1 dhcpd_msg--------\n", (unsigned char *)dhcpd_msg, sizeof(dhcpd_message_t));
        // printf("\n\n");

        option_ptr = (char *)dhcpd_msg->options;
        memcpy(option_ptr, dhcpd_opt_offer_tmp, sizeof(dhcpd_opt_offer_tmp));
        option_ptr += sizeof(dhcpd_opt_offer_tmp);
        //dhcpd_hex_dump("-----2 dhcpd_msg--------\n", (unsigned char *)dhcpd_msg, sizeof(dhcpd_message_t));
        //printf("\n\n");
        //dhcpd_hex_dump("dhcpd_opt_subnet_tmp", (unsigned char *)&dhcpd_opt_subnet_tmp, sizeof(dhcpd_opt_subnet_tmp));
        memcpy(option_ptr, dhcpd_opt_subnet_tmp, sizeof(dhcpd_opt_subnet_tmp));
        option_ptr += sizeof(dhcpd_opt_subnet_tmp);
        //dhcpd_hex_dump("-----3 dhcpd_msg--------\n", (unsigned char *)dhcpd_msg, sizeof(dhcpd_message_t));
        //printf("\n\n");

        option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_LEASE_TIME, *(lease_ptr + 3), *(lease_ptr + 2), *(lease_ptr + 1), *(lease_ptr));
        option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_SERVER_IDENTIFIER, ip4_addr1(&ip_addr), ip4_addr2(&ip_addr), ip4_addr3(&ip_addr), ip4_addr4(&ip_addr));

        *option_ptr++ = DHCPD_OPT_END;
        //dhcpd_hex_dump("-----4 dhcpd_msg--------\n", (unsigned char *)dhcpd_msg, sizeof(dhcpd_message_t));
        //printf("\n\n");

        ret = dhcpd_send_response(DHCPD_OFFER);

    } else {
        DHCPD_WARN("Can't handle discover, pool full");
    }

    return ret;
}


static int dhcpd_do_request(struct sockaddr *source_addr)
{
    int ret = 0, request_ip_opt_exist = 0;
    struct ip4_addr ip_addr;
    char *option_ptr;
    unsigned int lease_time = DHCPD_DEFAULT_LEASE_TIME;
    unsigned char *lease_ptr = (unsigned char *)&lease_time;
    unsigned int t1_time = DHCPD_DEFAULT_LEASE_TIME * 0.5;
    unsigned char *t1_ptr = (unsigned char *)&t1_time;
    unsigned int t2_time = DHCPD_DEFAULT_LEASE_TIME * 0.875;
    unsigned char *t2_ptr = (unsigned char *)&t2_time;
    unsigned char lease_ip[4];
    dhcpd_alloc_info_t *alloc_info = NULL, *pre_alloc_info = NULL;

    ip_addr.addr = *((unsigned int *) &dhcpd_server_address);
    dhcpd_send_ack = 1;

    ret = dhcpd_lease_address(dhcpd_msg->chaddr, lease_ip);
    if (ret != -1) {
        DHCPD_PRINTF("lease_ip:%d.%d.%d.%d", lease_ip[0], lease_ip[1], lease_ip[2], lease_ip[3]);
        dhcpd_msg->op = DHCPD_OP_REPLY;
        dhcpd_msg->secs = 0;
        option_ptr = (char *)dhcpd_msg->options;

        /* Handle Request special IP from Client */
        while (*option_ptr != DHCPD_OPT_END) {
            int len = option_ptr[1];
            //DHCPD_PRINTF("Request MSG option type:%d", *option_ptr);
            switch (*option_ptr) {
                case DHCPD_OPT_REQUESTED_IP:
                    request_ip_opt_exist = 1;
                    if (len == 4) {
                        DHCPD_PRINTF("Client req special IP");
                        memcpy(dhcpd_msg->yiaddr, option_ptr + 2, 4);
                        DHCPD_PRINTF("yiaddr:%d.%d.%d.%d", dhcpd_msg->yiaddr[0], dhcpd_msg->yiaddr[1], dhcpd_msg->yiaddr[2], dhcpd_msg->yiaddr[3]);
                    }
                    break;
                default:
                    break;
            }
            option_ptr += (len + 2);
        }
        /* Check if it is the REQUEST to extend the lease time at T1 or T2 */
        if (1 == ret &&
            !request_ip_opt_exist &&
            (dhcpd_msg->ciaddr[0] || dhcpd_msg->ciaddr[1] ||
             dhcpd_msg->ciaddr[2] || dhcpd_msg->ciaddr[3])) {
            /* This is the REQUEST to extend the lease time */
            DHCPD_PRINTF("This is the REQUEST to extend the lease time.\r\n");
        } else {
            /* Requested IP is not the same as the lease_ip */
            if (memcmp(dhcpd_msg->yiaddr, lease_ip, 4) != 0) {
                // Client receive NAK, and will restart dhcp from Discover.
                DHCPD_PRINTF("Can't handle request, reply NAK.");
                dhcpd_send_ack  = 0;
                memset(dhcpd_msg->yiaddr, 0, sizeof(dhcpd_msg->yiaddr));

                memcpy(dhcpd_msg->magic, dhcpd_opt_magic_tmp, 4);
                memset(dhcpd_msg->options, 0, sizeof(dhcpd_msg->options));

                option_ptr = (char *)dhcpd_msg->options;
                memcpy(option_ptr, dhcpd_opt_nak_tmp, sizeof(dhcpd_opt_nak_tmp));
                option_ptr += sizeof(dhcpd_opt_nak_tmp);
                memcpy(option_ptr, dhcpd_opt_msg_tmp, sizeof(dhcpd_opt_msg_tmp));
                option_ptr += sizeof(dhcpd_opt_msg_tmp);

                option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_SERVER_IDENTIFIER, ip4_addr1(&ip_addr), ip4_addr2(&ip_addr), ip4_addr3(&ip_addr), ip4_addr4(&ip_addr));

                /* Add end mark */
                *option_ptr++ = DHCPD_OPT_END;

                /* Release IP , due to NAK */
                alloc_info = dhcpd_find_alloc_info_by_mac(dhcpd_msg->chaddr,
                                                          dhcpd_alloc_infos_in_use,
                                                          &pre_alloc_info);
                if (alloc_info) {
                    //dhcpd_release_alloc_info(alloc_info);
                    dhcpd_remove_alloc_info_from_list(alloc_info, pre_alloc_info, &dhcpd_alloc_infos_in_use);
#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
                    if (2 == ret) {
                        /* alloc_info was in unuse list. */
                        dhcpd_insert_alloc_info_into_list(alloc_info, &dhcpd_alloc_infos_unuse);
                    } else
#endif
                    {
                        vPortFree(alloc_info);
                    }
                }

                ret = dhcpd_send_response(DHCPD_NAK);
            }
        }

        if (dhcpd_send_ack == 1) {
            DHCPD_PRINTF("Accept request, reply ACK.");

            memcpy(dhcpd_msg->yiaddr, lease_ip, 4);
            memcpy(dhcpd_msg->magic, dhcpd_opt_magic_tmp, 4);
            memset(dhcpd_msg->options, 0, sizeof(dhcpd_msg->options));
            option_ptr = (char *)dhcpd_msg->options;

            // ACK message type
            *option_ptr++ = DHCPD_OPT_MESSAGE_TYPE;
            *option_ptr++ = 1;
            *option_ptr++ = 5;

            // renewal time
            option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_T1, *(t1_ptr + 3), *(t1_ptr + 2), *(t1_ptr + 1), *(t1_ptr));//0, 0, 0x62, 0x70);

            // rebinding time
            option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_T2, *(t2_ptr + 3), *(t2_ptr + 2), *(t2_ptr + 1), *(t2_ptr));//0, 0, 0x62, 0x70);

            // lease time
            option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_LEASE_TIME, *(lease_ptr + 3), *(lease_ptr + 2), *(lease_ptr + 1), *(lease_ptr));

            // dhcp server identifier
            option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_SERVER_IDENTIFIER, ip4_addr1(&ip_addr), ip4_addr2(&ip_addr), ip4_addr3(&ip_addr), ip4_addr4(&ip_addr));

            // subnet mask
            ip_addr.addr = *((unsigned int *) &dhcpd_server_netmask);
            option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_NETMASK, ip4_addr1(&ip_addr), ip4_addr2(&ip_addr), ip4_addr3(&ip_addr), ip4_addr4(&ip_addr));

            // broadcast address
            option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_BROADCAST_ADDR, 0xff, 0xff, 0xff, 0xff);

#if 0
            // domain name
            *option_ptr++ = DHCPD_OPT_DOMAINNAME;
            int len = sprintf((char *)option_ptr + 1, DHCPD_DOMAIN_NAME);
            *option_ptr = (len + 1);
            option_ptr += (len + 2);
#endif

            // router(gateway)
            ip_addr.addr = *((unsigned int *) &dhcpd_server_gw);
            option_ptr += dhcpd_opt_tlv_int(option_ptr, DHCPD_OPT_ROUTER, ip4_addr1(&ip_addr), ip4_addr2(&ip_addr), ip4_addr3(&ip_addr), ip4_addr4(&ip_addr));

            // domain name server (DNS)
            ip_addr.addr = *((unsigned int *) &dhcpd_primary_dns);
            option_ptr += dhcpd_opt_dns_tlv_int(option_ptr, &dhcpd_primary_dns, &dhcpd_secondary_dns);

            // end mark
            *option_ptr++ = DHCPD_OPT_END;

            ret = dhcpd_send_response(DHCPD_ACK);
        }
    } else {
        DHCPD_WARN("Can't handle request, pool full");
    }

    return ret;
}


static int dhcpd_task_loop(void *arg)
{
    int ret = -1;
    struct sockaddr_in addr;
    dhcpd_alloc_info_t *alloc_info = NULL, *pre_alloc_info = NULL;

    DHCPD_PRINTF("dhcpd task entry:%d", dhcpd_running);

    while (dhcpd_running == 1) {

        DHCPD_PRINTF("Wait for UDP");

        ret = recvfrom(dhcpd_socket, (char *)dhcpd_msg, sizeof(*dhcpd_msg),
                        0 , (struct sockaddr *)&addr, 0);
        DHCPD_PRINTF("recvfrom=[%d]", ret);

        if (ret > 0) {
            switch (dhcpd_msg->options[2]) {
                case DHCPD_DISCOVER:
                    DHCPD_PRINTF("Handle DISCOVER");
                    ret = dhcpd_do_discover((struct sockaddr *)&addr);
                    break;

                case DHCPD_REQUEST:
                    DHCPD_PRINTF("Handle REQUEST");
                    ret = dhcpd_do_request((struct sockaddr *)&addr);
                    break;

                case DHCPD_RELEASE:
                    DHCPD_PRINTF("DHCPD RELEASE");
                    alloc_info = dhcpd_find_alloc_info_by_mac(dhcpd_msg->chaddr,
                                                              dhcpd_alloc_infos_in_use,
                                                              &pre_alloc_info);
                    if (alloc_info) {
                        // dhcpd_release_alloc_info(alloc_info);
                        ret = dhcpd_remove_alloc_info_from_list(alloc_info, pre_alloc_info, &dhcpd_alloc_infos_in_use);
#ifdef DHCPD_SAVE_CLIENT_CONFIG_ON_LINE
                        ret = dhcpd_insert_alloc_info_into_list(alloc_info, &dhcpd_alloc_infos_unuse);
#else
                        vPortFree(alloc_info);
#endif
                    }
                    break;

                case DHCPD_DECLINE:
                default:
                    DHCPD_PRINTF("DECLINE Received. ignore message [%d]", dhcpd_msg->options[2]);
                    break;
            }
        }
    }
    return ret;
}


static void dhcpd_prepare(void *param)
{
    struct sockaddr_in dhcpd_addr = {0};
    struct ifreq iface;

    dhcpd_addr.sin_family      = AF_INET;
    dhcpd_addr.sin_port        = htons(DHCPD_SERVER_PORT);
    dhcpd_addr.sin_addr.s_addr = INADDR_ANY;
    if (dhcpd_socket != -1) {
        DHCPD_ERR("socket exist");
        dhcpd_task_handle = 0;
        vTaskDelete(NULL);
    }

    dhcpd_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (dhcpd_socket < 0) {
        DHCPD_ERR("socket error");
        dhcpd_task_handle = 0;
        vTaskDelete(NULL);
    }

    lwip_get_netif_name(NETIF_TYPE_AP, iface.ifr_name);

    if (0 > setsockopt(dhcpd_socket, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(struct ifreq))) {
        DHCPD_ERR("set sock option error");
        close(dhcpd_socket);
        dhcpd_socket = -1;
        dhcpd_task_handle = 0;
        vTaskDelete(NULL);
    }

    if (0 > bind(dhcpd_socket, (struct sockaddr *)&dhcpd_addr,  sizeof (dhcpd_addr))) {
        DHCPD_ERR("socket error");
        close(dhcpd_socket);
        dhcpd_socket = -1;
        dhcpd_task_handle = 0;
        vTaskDelete(NULL);
    }

    if (dhcpd_running == 0) {
        dhcpd_running = 1;

        if (dhcpd_msg) {
            memset(dhcpd_msg, 0, sizeof(dhcpd_message_t));
            DHCPD_PRINTF("DHCPD Warning: dhcpd_msg has been allocated.\n");
        } else {
            dhcpd_msg = pvPortMalloc(sizeof(dhcpd_message_t));

            if (NULL == dhcpd_msg) {
                DHCPD_PRINTF("DHCPD Err: Not enough memory for dhcpd_msg->\n");

                dhcpd_running = 0;

                close(dhcpd_socket);
                dhcpd_socket = -1;

                dhcpd_task_handle = 0;
                vTaskDelete(NULL);
            }

            memset(dhcpd_msg, 0, sizeof(dhcpd_message_t));
        }

        wifi_connection_register_event_notifier(WIFI_EVENT_IOT_DISCONNECTED,
                                                dhcpd_wifi_api_rx_event_handler);
        DHCPD_PRINTF("DHCPD started");

        dhcpd_log_ip("Server IP", &dhcpd_server_address);
        dhcpd_log_ip("Netmask", &dhcpd_server_netmask);
        dhcpd_log_ip("Gateway", &dhcpd_server_gw);
        dhcpd_log_ip("DNS1", &dhcpd_primary_dns);
        dhcpd_log_ip("DNS2", &dhcpd_secondary_dns);
        dhcpd_log_ip("Start IP", &dhcpd_ip_pool_start);
        dhcpd_log_ip("End IP", &dhcpd_ip_pool_end);

        dhcpd_task_loop(NULL);

        close(dhcpd_socket);
        dhcpd_socket = -1;

        if (dhcpd_msg) {
            vPortFree(dhcpd_msg);
            dhcpd_msg = NULL;
        }

        wifi_connection_unregister_event_notifier(WIFI_EVENT_IOT_DISCONNECTED, dhcpd_wifi_api_rx_event_handler);

        dhcpd_release_alloc_info_lists();
    }

    dhcpd_task_handle = 0;
    vTaskDelete(NULL);
}

void dhcpd_stop(void)
{
    DHCPD_PRINTF("dhcpd_stop [%d][%d]", (int)dhcpd_task_handle, dhcpd_running);
    dhcpd_mutex_lock();
    if (dhcpd_running == 1) {
        dhcpd_running = 0;
        vTaskDelete(dhcpd_task_handle);
        dhcpd_task_handle=0;
        wifi_connection_unregister_event_notifier(WIFI_EVENT_IOT_DISCONNECTED, dhcpd_wifi_api_rx_event_handler);
        close(dhcpd_socket);
        dhcpd_socket = -1;

        dhcpd_release_alloc_info_lists();

        if (dhcpd_msg) {
            vPortFree(dhcpd_msg);
            dhcpd_msg = NULL;
        }

        DHCPD_PRINTF("DHCPD stopped");
    } else {
        DHCPD_PRINTF("DHCPD no need to stop.");
    }
    dhcpd_mutex_unlock();
}
