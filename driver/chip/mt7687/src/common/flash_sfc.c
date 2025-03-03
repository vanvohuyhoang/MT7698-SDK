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

/****************************************************************************
    Module Name:
    Flash

    Abstract:
    Flash related access function.

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
***************************************************************************/
#include "hal_flash.h"

#ifdef HAL_FLASH_MODULE_ENABLED
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "type_def.h"
#include "mt7687.h"

#include "flash_sfc.h"
#include "spi_flash.h"
#include "nvic.h"
#include "hal_cache.h"
#include "hal_flash.h"
#include "hal_gpt.h"
#include "hal_log.h"
#include "nvic.h"
#include "hal_nvic_internal.h"
#include "memory_attribute.h"
#include "bsp_flash_config.h"

#define PAGE_BUFFER_SIZE      (64)
#define SF_DAL_FLAG_BUSY()    (NOR_FLASH_BUSY)
#define SF_DAL_FLAG_SUS()     (NOR_FLASH_SUSPENDED)
#define ust_get_duration(a,b) ((a>b)?(b+(0xFFFFFFFF-a)+0x1):(b-a))

#define CACHE_LINE_ALIGNMENT_MASK (0xFFFFFFE0)
#define CACHE_LINE_SIZE           (0x20)
#define FLASH_BLOCK_SIZE          (0x10000)
#ifndef HAL_FLASH_BASE_ADDRESS
#define HAL_FLASH_BASE_ADDRESS    (0x10000000)
#endif
SF_DRV_STATE sf_drvier_status = SF_DRV_NOT_READY;
#define FLASH_DADA_MAX_LEN  (128)
ATTR_RWDATA_IN_TCM bool NOR_FLASH_BUSY = false;
ATTR_RWDATA_IN_TCM bool NOR_FLASH_SUSPENDED = false;

uint32_t gucFlashSFCMode;
//static int sf_Mutex = 1;
extern SF_TYPT support_flash_id;
extern int gd_write_sr2_1;

uint32_t suspend_time;
uint32_t resume_suspend_on_ready;
uint32_t resume_suspend_on_checkready;

ATTR_TEXT_IN_TCM uint32_t gpt_get_current_time(void)
{
    uint32_t counter = 0;
    hal_gpt_status_t ret;
    ret = hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &counter);
    if (ret != HAL_GPT_STATUS_OK) {
        assert(0);
    }
    return counter;
}

ATTR_TEXT_IN_TCM void SF_DAL_FLAG_BUSY_SET(void)
{
    NOR_FLASH_BUSY = true;
}
ATTR_TEXT_IN_TCM void SF_DAL_FLAG_BUSY_CLR(void)
{
    NOR_FLASH_BUSY = false;
}
ATTR_TEXT_IN_TCM void SF_DAL_FLAG_SUS_SET(void)
{
    NOR_FLASH_SUSPENDED = true;
}
ATTR_TEXT_IN_TCM void SF_DAL_FLAG_SUS_CLR(void)
{
    NOR_FLASH_SUSPENDED = false;
}

int32_t get_sf_lock(void)
{
#if 0
    uint32_t savedMask;
    savedMask = save_and_set_interrupt_mask();
    if (sf_Mutex == 1) {
        sf_Mutex--;
        restore_interrupt_mask(savedMask);
        return 0;
    } else if (sf_Mutex == 0) {
        restore_interrupt_mask(savedMask);
        return -2;
    } else {
        restore_interrupt_mask(savedMask);
        assert(0);
        return -1;
    }
#endif
    return 0;
}

void free_sf_lock(void)
{
#if 0
    uint32_t savedMask;
    savedMask = save_and_set_interrupt_mask();
    if (sf_Mutex == 0) {
        sf_Mutex++;
        restore_interrupt_mask(savedMask);
    } else {
        restore_interrupt_mask(savedMask);
        assert(0);
    }
#endif
}

void retrieve_sf_lock(void)
{
#if 0
    int32_t Result;
    Result = get_sf_lock();
    return Result;
#endif
}

/*****************************************************************
Description : relieve FDM synchronization lock.
Input       :
Output      : None
******************************************************************/
void release_sf_lock(void)
{
#if 0
    free_sf_lock();
#endif
}

static void sfc_pad_config(void)
{
#if (PRODUCT_VERSION == 7687)
    uint32_t pad_io_setting = 0;
#define TOP_PAD_CLT0 (0x8102188)
    pad_io_setting = *(volatile uint32_t *)TOP_PAD_CLT0;
    pad_io_setting |= 0x00007E00;    //bit9 - bit14 used by sip flash
    *(volatile uint32_t *)TOP_PAD_CLT0 = pad_io_setting;
#elif (PRODUCT_VERSION == 7697)
#define TOP_PAD_CLT0 (0x81021080)
    uint32_t pad_io_setting = 0;
    /* bit4   SPI_DATA0_EXT
           bit5   SPI_DATA1_EXT
           bit7   SPI_CS_EXT
           bit24  SPI_DATA2_EXT
           bit25  SPI_DATA4_EXT
           bit26  SPI_CLK_EXT
        */
    pad_io_setting = *(volatile uint32_t *)TOP_PAD_CLT0;
    pad_io_setting |= 0x070000B0;
    *(volatile uint32_t *)TOP_PAD_CLT0 = pad_io_setting;
#endif
}
ATTR_TEXT_IN_TCM int32_t flash_sfc_config(uint8_t mode)
{
    INT32 ret = 0;
    sfc_pad_config();
    gpt_get_current_time();   // init gpt one time to avoid put more GPT code in RAM
    if (customer_flash_register() != NULL) {
        //configured external flash
        support_flash_id = SF_TYPE_CUSTOMER;
    }
    flash_check_device();

    return ret;
}

ATTR_TEXT_IN_TCM int32_t hal_flash_direct_read(void *absolute_address, uint8_t *buffer, uint32_t length)
{
    int32_t result = 0;

    retrieve_sf_lock();
    if (flash_wait_ready(2)) {
        /*staus should not be busy at here*/
        return (result = -7);
    }

    /*trigger resume if last P/E not finished */
    do {
        result = SF_DAL_CheckDeviceReady(0, 0);
    } while (-121 == result);

    memcpy(buffer, absolute_address, length);
    release_sf_lock();
    return result;
}

ATTR_TEXT_IN_TCM int32_t flash_sfc_read(uint32_t address, uint32_t length, uint8_t *buffer)
{
    INT32 ret = 0;
    UINT32 u4Redidual = length;
    UINT32 u4ReadLen = 0;

    retrieve_sf_lock();
    if (flash_wait_ready(2)) {
        /*staus should not be busy at here*/
        return (ret = -7);
    }

    /*trigger resume if last P/E not finished */
    do {
        ret = SF_DAL_CheckDeviceReady(0, 0);
    } while (-121 == ret);

    while (FLASH_DADA_MAX_LEN <= u4Redidual) {
        u4ReadLen = FLASH_DADA_MAX_LEN;
        if (FLASH_MODE_SPI == gucFlashSFCMode) {
            flash_read(buffer, address, u4ReadLen);
        } else if (FLASH_MODE_QPI == gucFlashSFCMode) {
            flash_fast_read(buffer, address, u4ReadLen, 1 /* dummy_cycle, 4bit * 2cycle == 1byte */);
        }

        buffer = buffer + u4ReadLen;
        address = address + u4ReadLen;
        u4Redidual = u4Redidual - u4ReadLen;
    }

    if (FLASH_MODE_SPI == gucFlashSFCMode) {
        flash_read(buffer, address, u4Redidual);
    } else if (FLASH_MODE_QPI == gucFlashSFCMode) {
        flash_fast_read(buffer, address, u4Redidual, 1 /* dummy_cycle, 4bit * 2cycle == 1byte */);
    }
    sf_drvier_status = SF_DRV_READY;
    release_sf_lock();
    return ret;
}


int32_t flash_sfc_write(uint32_t address, uint32_t length, const uint8_t *buffer)
{
    INT32 ret = 0;

    retrieve_sf_lock();


    ret = flash_write(buffer, address, length);
    sf_drvier_status = SF_DRV_READY;
#ifdef HAL_CACHE_MODULE_ENABLED
    if (hal_cache_is_cacheable(address + HAL_FLASH_BASE_ADDRESS)) {
        uint32_t addr;
        for (addr = ((address + HAL_FLASH_BASE_ADDRESS)&CACHE_LINE_ALIGNMENT_MASK); addr <= (((address + HAL_FLASH_BASE_ADDRESS) + length + CACHE_LINE_SIZE) & CACHE_LINE_ALIGNMENT_MASK); addr += CACHE_LINE_SIZE) {
            hal_cache_invalidate_one_cache_line(addr);
        }
    }
#endif
    release_sf_lock();
    return ret;
}

ATTR_TEXT_IN_TCM int32_t flash_sfc_erase(uint32_t address, uint32_t type)
{
    INT32 ret = -1;

    retrieve_sf_lock();
    if (flash_wait_ready(2)) {
        /*staus should not be busy at here*/
        return (ret = -7);
    }

    /*trigger resume if last P/E not finished */
    do {
        ret = SF_DAL_CheckDeviceReady(0, 0);
    } while (-121 == ret);



    flash_write_enable();
    flash_unprotect();

    if (HAL_FLASH_BLOCK_4K == type) {
        if ((address & 0xFFF) != 0) {
            release_sf_lock();
            return (ret = -3);
        }
        ret = flash_erase_page(address);
    } else if (HAL_FLASH_BLOCK_32K == type) {
        if ((address & 0x7FFF) != 0) {
            release_sf_lock();
            return (ret = -3);
        }
        ret = flash_erase_sector_32k(address);
    } else if (HAL_FLASH_BLOCK_64K == type) {
        if ((address & 0xFFFF) != 0) {
            release_sf_lock();
            return (ret = -3);
        }
        ret = flash_erase_sector(address);
    }

#ifdef HAL_CACHE_MODULE_ENABLED
    /* Address should be alignment with cashe line size*/
    if (hal_cache_is_cacheable((address + HAL_FLASH_BASE_ADDRESS))) {
        uint32_t addr;
        for (addr = ((address + HAL_FLASH_BASE_ADDRESS)&CACHE_LINE_ALIGNMENT_MASK); addr <= (((address + HAL_FLASH_BASE_ADDRESS) + FLASH_BLOCK_SIZE + CACHE_LINE_SIZE) & CACHE_LINE_ALIGNMENT_MASK); addr += CACHE_LINE_SIZE) {
            hal_cache_invalidate_one_cache_line(addr);
        }
    }

#endif
    release_sf_lock();
    return ret;
}


ATTR_TEXT_IN_TCM void Flash_ReturnReady(void)
{
    uint8_t sr = 0;
    uint32_t savedMask = 0;

    if (sf_drvier_status == SF_DRV_NOT_READY) {
        //assert(0);
        //return;
    }

    // No Suspend Conditions
    // 1. For those deivces that do not support program-suspend (buffer length < 32 bytes).
    // 2. Serial Flash Unit Test: Erase/Program w/o suspend.
    // 3. NOR_NO_SUSPEND is defiend.
    if ((PAGE_BUFFER_SIZE < 16) && (sf_drvier_status >= SF_DRV_PROGRAMMING)) {
        while (1) {
            if (flash_read_sr(&sr) < 0) {
                assert(0);
            }
            if (0 == (sr & SR_WIP)) {
                break;
            }
        }
    }

    savedMask = save_and_set_interrupt_mask();
    if ((!SF_DAL_FLAG_SUS()) && SF_DAL_FLAG_BUSY()) {
        if (flash_read_sr(&sr) < 0) {
            //read SR failed
            assert(0);
        }

        // if flash is busy, suspend any on-going operations
        if (0 != (sr & FLASH_STATUS_BUSY)) {
            // 1. Issue suspend command
            flash_suspend_Winbond();
            // 2. wait for device ready
            while (1) {
                if (flash_read_sr(&sr) < 0) {
                    //read SR failed
                    assert(0);
                }
                if (0 == (sr & SR_WIP)) {
                    break;
                }
            }
            SF_DAL_FLAG_SUS_SET();

        } else {
            SF_DAL_FLAG_BUSY_CLR();
        }
    }
    restore_interrupt_mask(savedMask);
}

ATTR_TEXT_IN_TCM int32_t SF_DAL_CheckDeviceReady(void *MTDData, uint32_t BlockIndex)
{
    int32_t result;
    uint32_t savedMask = 0;
    uint8_t status_busy, status_suspend;
    uint8_t sr = 0;
    uint8_t sr1 = 0;

    /* ensure that the status check is atomic */
    savedMask = save_and_set_interrupt_mask();

    if (flash_read_sr(&sr) < 0) {
        assert(0);
    } else {
        status_busy = sr;
    }

    if (flash_read_sr2(&sr1) < 0) {
        assert(0);
    } else {
        status_suspend = sr1;
    }

    if (0 == (status_busy & FLASH_STATUS_BUSY)) {
        uint8_t check_status = (0x08 | 0x04); //defualt is MXIC

        if (support_flash_id == SF_TYPE_WINBOND || support_flash_id == SF_TYPE_GD) {
            //windbond is s15(0x80)
            check_status = 0x80;
            if (gd_write_sr2_1 == 0x31) {
                //GD25Q32CSIG  it's s15 & s10 bit
                check_status |= 0x84;
            }

        } else if (support_flash_id == SF_TYPE_MXIC) {
            //mxic is WSP & WSE of security status regist(0x04 and 0x08)
            check_status = (0x08 | 0x04);
        } else if (support_flash_id == SF_TYPE_MICRON) {
            //mxic is WSP & WSE of security status regist(0x04 and 0x08)
            check_status = 0x42;     //bit7: erase susspend      bit2: program suspend
        } else if (support_flash_id == SF_TYPE_CUSTOMER) {
            //customer flash
            check_status = customer_flash_suspend_bit();
        }

        // erase suspended or program suspended
        if ((0 != (status_suspend & check_status)) ||  // check suspend flags
                ((0 == check_status) && SF_DAL_FLAG_SUS())) { //devices that do not have suspend flags => check driver flag
            assert(SF_DAL_FLAG_BUSY());

            // issue resume command
            flash_resume_Winbond();
            SF_DAL_FLAG_SUS_CLR();
            result =  -121;  //FS_FLASH_ERASE_BUSY;
        } else { // flash is neither busy nor suspendeds
            SF_DAL_FLAG_BUSY_CLR();
            /********************************************//**
             * If an interrupt comes during program/erase, in Flash_ReturnReady(), the device may deny the
             * "Suspend Erase/Program" command because the device is near/already ready. However,
             * NOR_FLASH_SUSPENDED is still be set to true.
             *
             * In such case, after back to erase/program operation, CheckDeviceReady will reach here
             * because flash is not busy and not erase suspended (but with NOR_FLASH_SUSPENDED = true). If NOR_FLASH_SUSPENDED
             * is not set to false here, next time when an interrupt comes during erase/program
             * operation, Flash_ReturnReady() will be misleaded by wrong NOR_FLASH_SUSPENDED and return
             * to IRQ handler directly even if flash is still erasing/programing, leading to an execution
             * error!
             ***********************************************/
            SF_DAL_FLAG_SUS_CLR();
            result = 0;   //FS_NO_ERROR;
        }
    } else {
        result = -121;   //FS_FLASH_ERASE_BUSY;
    }

    restore_interrupt_mask(savedMask);
    return result;
}


ATTR_TEXT_IN_TCM int32_t SF_DAL_CheckReadyAndResume(void *MTDData, uint32_t addr, uint8_t data)
{
    uint32_t          savedMask;
    int32_t           result = 0;   //RESULT_FLASH_BUSY;    // default result is busy
    uint8_t           check_data;
    uint16_t          status_busy = 0;
    uint8_t           sr = 0;

    savedMask = save_and_set_interrupt_mask();

    // Read device status
    if (flash_read_sr(&sr) < 0) {
        assert(0);
    } else {
        status_busy = sr;
    }
    // Flash is suspended due to interrupt => Resume
    if (SF_DAL_FLAG_SUS()) {
        assert(SF_DAL_FLAG_BUSY());
        flash_resume_Winbond();
        SF_DAL_FLAG_SUS_CLR();
        restore_interrupt_mask(savedMask);
    }
    // Flash is not suspended and ready => Validate programmed data
    else  if (0 == (status_busy & FLASH_STATUS_BUSY)) {
        SF_DAL_FLAG_BUSY_CLR();
        // Compare last programmed byte
        check_data = *(volatile uint8_t *)addr;
        if (check_data == data) {
            result = 1;    //RESULT_FLASH_DONE;
        } else {
            result = -1;   //RESULT_FLASH_FAIL;
        }
    }
    restore_interrupt_mask(savedMask);

    return result;
}

#ifdef OTP_FEATURE_SUPPORT
ATTR_TEXT_IN_TCM int SF_DAL_OTPAccess(void *MTDData, int accesstype, uint32_t Offset, void *Buffer, uint32_t Length)
{
    switch (accesstype) {
        case OTP_READ:
            if (support_flash_id	== SF_TYPE_WINBOND) {
                return SF_OTPRead_WINBOND(Offset, Buffer, Length);
            } else if (support_flash_id == SF_TYPE_MXIC) {
                log_hal_info("Not support OTP!\r\n");
            }
            break;
        case OTP_WRITE:
            if (support_flash_id	== SF_TYPE_WINBOND) {
                return SF_OTPWrite_WINBOND(Offset, Buffer, Length);
            } else if (support_flash_id == SF_TYPE_MXIC) {
                log_hal_info("Not support OTP!\r\n");
            }
            break;
        case OTP_LOCK:
            if (support_flash_id	== SF_TYPE_WINBOND) {
                return SF_OTPLock_WINBOND();
            } else if (support_flash_id == SF_TYPE_MXIC) {
                log_hal_info("Not support OTP!\r\n");
            }
            break;
        default:
            break;
    }

    return -1;
}

ATTR_TEXT_IN_TCM int SF_DAL_OTPQueryLength(void *MTDData, uint32_t *Length)
{
    if (support_flash_id == SF_TYPE_WINBOND) {
        *Length = 768;
    } else if (support_flash_id	== SF_TYPE_MXIC) {
        log_hal_info("Not support OTP!\r\n");
    } else {
        log_hal_info("Invalid Flash!\r\n");
        return -1;
    }
    return -1;
}
#endif

#endif

