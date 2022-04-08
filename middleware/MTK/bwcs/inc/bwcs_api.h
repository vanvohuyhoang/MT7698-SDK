/* Copyright Statement:
 *
 * (C) 2019  Airoha Technology Corp. All rights reserved.
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

#ifndef __BWCS_API_H__
#define __BWCS_API_H__

/**@defgroup BWCS_ENUM Enumeration
* @{
*/
/** @brief This enumeration defines the PTA mode.
* @note The choice of #pta_mode_t must be based on the Wi-Fi and BT chip's PTA capability. For Airoha Wi-Fi and BT chips, the PTA pins must
* be set in the EPT file which can refer to the hardware reference design.
*/
typedef enum
{
    PTA_MODE_1_WIRE,              /**<  1-wire PTA mode. It is not supported.*/
    PTA_MODE_1_WIRE_EXTENDED,     /**<  1-wire extended PTA mode. It is not supported.*/
    PTA_MODE_3_WIRE,              /**<  3-wire mode. */
} pta_mode_t;

/** @brief This enumeration defines the coexistence mode.
*/
typedef enum
{
    PTA_CM_MODE_TDD,                  /**<  Time Division Duplex (TDD) is a scheduled coexistence mode.*/
    PTA_CM_MODE_FDD,                  /**<  Frequency Division Duplex (FDD) is an unscheduled coexistence mode. It is not supported. */
} pta_cm_mode_t;

/** @brief This enumeration defines the antenna mode in coexistence.
*/
typedef enum
{
    PTA_ANT_MODE_SINGLE,             /**<  Single antenna mode; Wi-Fi and BT share the same antenna. Single antenna mode is only used in TDD.*/
    PTA_ANT_MODE_DUAL,               /**<  Dual antenna mode; Wi-Fi and BT have an independent antenna. Dual antenna mode is used in both TDD and FDD.*/
} pta_antenna_mode_t;

/**
* @}
*/

/**@defgroup BWCS_STRUCT Structure
* @{
*/
/** @brief This enumeration defines the BWCS intial parameters.
*/
typedef struct
{
    pta_mode_t pta_mode;             /**<  The initial parameters for PTA mode. For more details, please refer to #pta_mode_t.*/
    pta_cm_mode_t pta_cm_mode;       /**<  The initial parameters for coexistence mode. For more details, please refer to #pta_cm_mode_t.*/
    pta_antenna_mode_t antenna_mode; /**<  The initial parameters for antenna mode. For more details, please refer to #pta_antenna_mode_t.*/
} bwcs_init_t;
/**
* @}
*/

/**
* @brief This function initializes the BWCS module.
*
* @param[in] bwcs_init_param is the BWCS intial parameters on which the function operates. For more details, please refer to #bwcs_init_t.
*
* @return  >=0 the operation completed successfully; <0 the operation failed.
*
*@note BWCS must be enabled with Wi-Fi and BLE coexistence mode so that it can schedule the use of the antenna.
* This function can be called before a scheduled task starts or in any other application tasks. To enable coexistence mode, the developper must also
* set the correct hardware PTA and antenna select pins, and set the related EPT configuration base on the hardware reference design.
*/
int32_t bwcs_init(bwcs_init_t bwcs_init_param);

/**
* @brief This function deinitializes the BWCS module.
*
* @return  >=0 the operation completed successfully; <0 the operation failed.
*
*@note BWCS can be deinitialized if there is no Wi-Fi and/or BLE in coexistence mode.
* This function can be called in any application tasks after #bwcs_init().
*/
int32_t bwcs_deinit(void);

#endif

