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
 
#include "hal_spi_slave.h"

#ifdef HAL_SPI_SLAVE_MODULE_ENABLED

#include "hal_spi_slave_internal.h"
#include "hal_clock.h"
#include "hal_log.h"
#include "hal_sleep_manager.h"
#include "hal_sleep_manager_internal.h"

static SPIS_REGISTER_T *const g_spi_slave_register[HAL_SPI_SLAVE_MAX] = {SPI_SLAVE_0};
static hal_spi_slave_fsm_status_t g_spi_slave_fsm[MAX_STATUS][MAX_OPERATION_CMD] = {
            /* POWER_OFF_CMD                             POWER_ON_CMD                         CONFIG_READ_CMD                       READ_CMD                               CONFIG_WRITE_CMD                      WRITE_CMD */
/*PWROFF_STA*/ {HAL_SPI_SLAVE_FSM_INVALID_OPERATION,     HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION, HAL_SPI_SLAVE_FSM_INVALID_OPERATION,  HAL_SPI_SLAVE_FSM_INVALID_OPERATION,   HAL_SPI_SLAVE_FSM_INVALID_OPERATION,  HAL_SPI_SLAVE_FSM_INVALID_OPERATION},
/*PWRON_STA */ {HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION,     HAL_SPI_SLAVE_FSM_INVALID_OPERATION, HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION,  HAL_SPI_SLAVE_FSM_INVALID_OPERATION,   HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION,  HAL_SPI_SLAVE_FSM_INVALID_OPERATION},
/*CR_STA    */ {HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CR, HAL_SPI_SLAVE_FSM_INVALID_OPERATION, HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CR, HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION,   HAL_SPI_SLAVE_FSM_ERROR_CW_AFTER_CR,  HAL_SPI_SLAVE_FSM_ERROR_WRITE_AFTER_CR},
/*CW_STA    */ {HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CW, HAL_SPI_SLAVE_FSM_INVALID_OPERATION, HAL_SPI_SLAVE_FSM_ERROR_CR_AFTER_CW,  HAL_SPI_SLAVE_FSM_ERROR_READ_AFTER_CW, HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CW, HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION}
};
#ifdef HAL_SLEEP_MANAGER_ENABLED
static uint32_t g_spi_slave_ctrl_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t g_spi_slave_ie_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t g_spi_slave_tmout_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t g_spi_slave_cmd_def0_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t g_spi_slave_cmd_def1_reg[HAL_SPI_SLAVE_MAX] = {0};
static uint32_t g_spi_slave_cmd_def2_reg[HAL_SPI_SLAVE_MAX] = {0};
static sleep_management_lock_request_t g_spi_slave_sleep_handle[HAL_SPI_SLAVE_MAX] = {SLEEP_LOCK_SPI_SLAVE};
#endif

uint8_t g_last2now_status[2] = {PWROFF_STA, PWROFF_STA};

static inline void update_fsm_status(hal_spi_slave_transaction_status_t *transaction_status, \
    hal_spi_slave_callback_event_t int_status, spi_slave_fsm_status_t fsm_status, spi_slave_operation_cmd_t current_command) {
   transaction_status->interrupt_status = int_status;
   spi_slave_update_status(fsm_status);
   transaction_status->fsm_status = g_spi_slave_fsm[g_last2now_status[0]][current_command];
}

typedef void (*spi_slave_int_callback_t)(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data);

static void spi_slave_poweron_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
#ifdef HAL_SLEEP_MANAGER_ENABLED
    /* after receive POWER-ON command, lock sleep */
    hal_sleep_manager_lock_sleep(g_spi_slave_sleep_handle[spi_port]);
#endif
    update_fsm_status(&status, HAL_SPI_SLAVE_EVENT_POWER_ON, PWRON_STA, POWER_ON_CMD);
    /* set slv_on bit here */
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.SW_ON = SPIS_STA_SLV_ON_MASK;
    user_callback(status, user_data);    
}

static void spi_slave_poweroff_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
    update_fsm_status(&status, HAL_SPI_SLAVE_EVENT_POWER_OFF, PWROFF_STA, POWER_OFF_CMD);
    /* clear slv_on bit here */
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.SW_ON &= (~SPIS_STA_SLV_ON_MASK);
#ifdef HAL_SLEEP_MANAGER_ENABLED
    /* after spis de-init done, unlock sleep */
    hal_sleep_manager_unlock_sleep(g_spi_slave_sleep_handle[spi_port]);
#endif
    user_callback(status, user_data);    
}

static void spi_slave_read_finish_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
    update_fsm_status(&status, HAL_SPI_SLAVE_EVENT_RD_FINISH, PWRON_STA, READ_CMD);
    /* clear TX_DMA_SW_READY bit here */
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.TXDMA_SW_RDY = 0;
    user_callback(status, user_data);    
}

static void spi_slave_write_finish_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
    update_fsm_status(&status, HAL_SPI_SLAVE_EVENT_WR_FINISH, PWRON_STA, WRITE_CMD);
    /* clear RX_DMA_SW_READY bit here */
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.RXDMA_SW_RDY = 0;
    user_callback(status, user_data);    
}

static void spi_slave_read_config_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
    update_fsm_status(&status, HAL_SPI_SLAVE_EVENT_CRD_FINISH, CR_STA, CONFIG_READ_CMD); 
    user_callback(status, user_data);    
}

static void spi_slave_write_config_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
    update_fsm_status(&status, HAL_SPI_SLAVE_EVENT_CWR_FINISH, CW_STA, CONFIG_WRITE_CMD);
    user_callback(status, user_data);    
}

static void spi_slave_error_callback(hal_spi_slave_port_t spi_port)
{
    spi_slave_update_status(PWRON_STA);
    /* clear TX/RX_DMA_SW_READY bit here */
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.TXDMA_SW_RDY = 0;
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.RXDMA_SW_RDY = 0;
}

static void spi_slave_read_error_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    hal_spi_slave_transaction_status_t status;
    
    spi_slave_error_callback(spi_port); 
    status.interrupt_status = HAL_SPI_SLAVE_EVENT_RD_ERR;
    user_callback(status, user_data);
}

static void spi_slave_write_error_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{  
    hal_spi_slave_transaction_status_t status;
    
    spi_slave_error_callback(spi_port); 
    status.interrupt_status = HAL_SPI_SLAVE_EVENT_WR_ERR;
    user_callback(status, user_data); 
}

static void spi_slave_timeout_error_callback(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{   
    hal_spi_slave_transaction_status_t status;
    
    spi_slave_error_callback(spi_port); 
    status.interrupt_status = HAL_SPI_SLAVE_EVENT_TIMEOUT_ERR;
    user_callback(status, user_data);     
}

static spi_slave_int_callback_t spi_slave_int_callback[] = {
    spi_slave_read_finish_callback,
    spi_slave_write_finish_callback,
    spi_slave_poweroff_callback,
    spi_slave_poweron_callback,
    spi_slave_read_config_callback,
    spi_slave_write_config_callback,
    spi_slave_read_error_callback,
    spi_slave_write_error_callback,
    spi_slave_timeout_error_callback,
};

void spi_slave_lisr(hal_spi_slave_port_t spi_port, hal_spi_slave_callback_t user_callback, void *user_data)
{
    uint32_t irq_status;
    uint32_t shift_h;
    uint32_t shift_l;
    uint32_t i;

    irq_status = ((g_spi_slave_register[spi_port]->INT) & SPIS_INT_MASK);
    
    /* regroup the priority of interrupts for subsequent processing. */
    shift_h = (irq_status & (SPIS_INT_RD_TRANS_FINISH_MASK | SPIS_INT_WR_TRANS_FINISH_MASK | SPIS_INT_POWER_ON_MASK | SPIS_INT_POWER_OFF_MASK)) >> 2;
    shift_l = (irq_status & (SPIS_INT_RD_CFG_FINISH_MASK | SPIS_INT_WR_CFG_FINISH_MASK)) << 4;
    irq_status = shift_h | shift_l | (irq_status & (SPIS_INT_RD_DATA_ERR_MASK | SPIS_INT_WR_DATA_ERR_MASK | SPIS_INT_TMOUT_ERR_MASK));
    
    /* because more than one interrupt may be raised at the same time, they must be processed one by one at a specify prority. */
    for (i = 0; irq_status; i++) {
        if (irq_status & (1 << i)) {
            spi_slave_int_callback[i](spi_port, user_callback, user_data);
            irq_status &= ~(1 << i);
        }
    }
}

void spi_slave_init(hal_spi_slave_port_t spi_port, const hal_spi_slave_config_t *spi_config)
{
    /* reset spi slave's status frist */
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.SW_RST = 1;
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.SW_RST = 0;

    /* user configure parameters */
    switch (spi_config->bit_order) {
        case HAL_SPI_SLAVE_LSB_FIRST:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL0 &= (~(SPIS_CTRL_TXMSBF_MASK | SPIS_CTRL_RXMSBF_MASK));
            break;
        case HAL_SPI_SLAVE_MSB_FIRST:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL0 |= (SPIS_CTRL_TXMSBF_MASK | SPIS_CTRL_RXMSBF_MASK);
            break;
    }

    switch (spi_config->phase) {
        case HAL_SPI_SLAVE_CLOCK_PHASE0:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL0 &= (~SPIS_CTRL_CPHA_MASK);
            break;
        case HAL_SPI_SLAVE_CLOCK_PHASE1:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL0 |= SPIS_CTRL_CPHA_MASK;
            break;
    }

    switch (spi_config->polarity) {
        case HAL_SPI_SLAVE_CLOCK_POLARITY0:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL0 &= (~SPIS_CTRL_CPOL_MASK);
            break;
        case HAL_SPI_SLAVE_CLOCK_POLARITY1:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL0 |= SPIS_CTRL_CPOL_MASK;
            break;
    }

    /* timeout threshold */
    g_spi_slave_register[spi_port]->TMOUT_THR = spi_config->timeout_threshold;

    /* enable all interrupt, set four-byte address and size, set sw decode bit */
    g_spi_slave_register[spi_port]->IE |= SPIS_IE_MASK;
    g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL0 |= SPIS_CTRL_SIZE_OF_ADDR_MASK;
    g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.CTRL1 = (SPIS_CTRL_DEC_ADDR_EN_MASK >> 8) | (SPIS_CTRL_SW_RDY_EN_MASK >> 8);
}

hal_spi_slave_status_t spi_slave_send(hal_spi_slave_port_t spi_port, const uint8_t *data, uint32_t size)
{
    uint32_t config_size = 0;

    /* return HAL_SPI_SLAVE_STATUS_ERROR if config_size isn't equal to size. */
    config_size = g_spi_slave_register[spi_port]->TRANS_LENGTH;
    if (config_size != size) {
        log_hal_error("[SPIS%d][send]:size error.\r\n", spi_port);
        return HAL_SPI_SLAVE_STATUS_ERROR;
    } else {
        /* set src_buffer_addr, buffer_size as size and tx_fifo_ready. */
        g_spi_slave_register[spi_port]->BUFFER_BASE_ADDR = (uint32_t)data;
        g_spi_slave_register[spi_port]->BUFFER_SIZE = size;
        g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.TXDMA_SW_RDY = 1;
    }

    return HAL_SPI_SLAVE_STATUS_OK;
}

hal_spi_slave_status_t spi_slave_query_config_info(hal_spi_slave_port_t spi_port, uint32_t *address, uint32_t *length)
{
    if ((g_spi_slave_register[spi_port]->STA_UNION.STA_CELLS.STA & 0xff) != (SPIS_STA_CFG_SUCCESS_MASK | SPIS_STA_SLV_ON_MASK)) {
        return HAL_SPI_SLAVE_STATUS_ERROR;
    }

    *address = g_spi_slave_register[spi_port]->TRANS_ADDR;
    *length = g_spi_slave_register[spi_port]->TRANS_LENGTH;

    return HAL_SPI_SLAVE_STATUS_OK;
}

hal_spi_slave_status_t spi_slave_receive(hal_spi_slave_port_t spi_port, uint8_t *buffer, uint32_t size)
{
    uint32_t config_size = 0;

    /* return HAL_SPI_SLAVE_STATUS_ERROR if config_size isn't equal to size */
    config_size = g_spi_slave_register[spi_port]->TRANS_LENGTH;
    if (config_size != size) {
        log_hal_error("[SPIS%d][receive]:size error.\r\n", spi_port);
        return HAL_SPI_SLAVE_STATUS_ERROR;
    } else {
        /* set src_buffer_addr, buffer_size as size and rx_fifo_ready. */
        g_spi_slave_register[spi_port]->BUFFER_BASE_ADDR = (uint32_t)buffer;
        g_spi_slave_register[spi_port]->BUFFER_SIZE = size;
        g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.RXDMA_SW_RDY = 1;
    }

    return HAL_SPI_SLAVE_STATUS_OK;
}

void spi_slave_set_early_miso(hal_spi_slave_port_t spi_port, hal_spi_slave_early_miso_t early_miso)
{
    switch (early_miso) {
        case HAL_SPI_SLAVE_EARLY_MISO_DISABLE:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.MISO_EARLY_TRANS = 0;
            break;
        case HAL_SPI_SLAVE_EARLY_MISO_ENABLE:
            g_spi_slave_register[spi_port]->CTRL_UNION.CTRL_CELLS.MISO_EARLY_TRANS = 1;
            break;
    }
}

void spi_slave_set_command(hal_spi_slave_port_t spi_port, hal_spi_slave_command_type_t command, uint8_t value)
{
    switch (command) {
        case HAL_SPI_SLAVE_CMD_WS:
            g_spi_slave_register[spi_port]->CMD_DEF0_UNION.CMD_DEF0_CELLS.CMD_WS = value;
            break;
        case HAL_SPI_SLAVE_CMD_RS:
            g_spi_slave_register[spi_port]->CMD_DEF0_UNION.CMD_DEF0_CELLS.CMD_RS = value;
            break;
        case HAL_SPI_SLAVE_CMD_WR:
            g_spi_slave_register[spi_port]->CMD_DEF1_UNION.CMD_DEF1_CELLS.CMD_WR = value;
            break;
        case HAL_SPI_SLAVE_CMD_RD:
            g_spi_slave_register[spi_port]->CMD_DEF1_UNION.CMD_DEF1_CELLS.CMD_RD = value;
            break;
        case HAL_SPI_SLAVE_CMD_POWEROFF:
            g_spi_slave_register[spi_port]->CMD_DEF0_UNION.CMD_DEF0_CELLS.CMD_PWOFF = value;
            break;
        case HAL_SPI_SLAVE_CMD_POWERON:
            g_spi_slave_register[spi_port]->CMD_DEF0_UNION.CMD_DEF0_CELLS.CMD_PWON = value;
            break;
        case HAL_SPI_SLAVE_CMD_CW:
            g_spi_slave_register[spi_port]->CMD_DEF1_UNION.CMD_DEF1_CELLS.CMD_CW = value;
            break;
        case HAL_SPI_SLAVE_CMD_CR:
            g_spi_slave_register[spi_port]->CMD_DEF1_UNION.CMD_DEF1_CELLS.CMD_CR = value;
            break;
        case HAL_SPI_SLAVE_CMD_CT:
            g_spi_slave_register[spi_port]->CMD_DEF2 = value;
            break;
    }
}

void spi_slave_reset_default(hal_spi_slave_port_t spi_port)
{
    uint32_t int_status;

    g_spi_slave_register[spi_port]->CTRL_UNION.CTRL = 0x00000100;
    int_status = g_spi_slave_register[spi_port]->INT;
    int_status = int_status;
    g_spi_slave_register[spi_port]->IE = 0x00000000;
    g_spi_slave_register[spi_port]->CMD_DEF0_UNION.CMD_DEF0 = 0x08060402;
    g_spi_slave_register[spi_port]->CMD_DEF1_UNION.CMD_DEF1 = 0x0e810c0a;
    g_spi_slave_register[spi_port]->CMD_DEF2 = 0x00000010;

    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.SW_ON = 0;
    g_spi_slave_register[spi_port]->TRIG_UNION.TRIG_CELLS.SW_RST = 1;
}

#ifdef HAL_SLEEP_MANAGER_ENABLED
void spi_slave_backup_register_callback(void *data)
{
    hal_spi_slave_port_t slave_port;

    for (slave_port = HAL_SPI_SLAVE_0; slave_port < HAL_SPI_SLAVE_MAX; slave_port++) {
        /* backup related spi_slave register values */
        g_spi_slave_ctrl_reg[slave_port] = g_spi_slave_register[slave_port]->CTRL_UNION.CTRL;
        g_spi_slave_ie_reg[slave_port] = g_spi_slave_register[slave_port]->IE;
        g_spi_slave_tmout_reg[slave_port] = g_spi_slave_register[slave_port]->TMOUT_THR;
        g_spi_slave_cmd_def0_reg[slave_port] = g_spi_slave_register[slave_port]->CMD_DEF0_UNION.CMD_DEF0;
        g_spi_slave_cmd_def1_reg[slave_port] = g_spi_slave_register[slave_port]->CMD_DEF1_UNION.CMD_DEF1;
        g_spi_slave_cmd_def2_reg[slave_port] = g_spi_slave_register[slave_port]->CMD_DEF2;
    }
}

void spi_slave_restore_register_callback(void *data)
{
    hal_spi_slave_port_t slave_port;

    for (slave_port = HAL_SPI_SLAVE_0; slave_port < HAL_SPI_SLAVE_MAX; slave_port++) {
        /* restore related spi_slave register values */
        g_spi_slave_register[slave_port]->CTRL_UNION.CTRL = g_spi_slave_ctrl_reg[slave_port];
        g_spi_slave_register[slave_port]->IE = g_spi_slave_ie_reg[slave_port];
        g_spi_slave_register[slave_port]->TMOUT_THR = g_spi_slave_tmout_reg[slave_port];
        g_spi_slave_register[slave_port]->CMD_DEF0_UNION.CMD_DEF0 = g_spi_slave_cmd_def0_reg[slave_port];
        g_spi_slave_register[slave_port]->CMD_DEF1_UNION.CMD_DEF1 = g_spi_slave_cmd_def1_reg[slave_port];
        g_spi_slave_register[slave_port]->CMD_DEF2 = g_spi_slave_cmd_def2_reg[slave_port];
    }
}
#endif

#endif /*HAL_SPI_SLAVE_MODULE_ENABLED*/

