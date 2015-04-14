/*-
 * Copyright (c) 2013 Alexander Fedorov <alexander.fedorov@rtlservice.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _A10_MMC_H_
#define _A10_MMC_H_

#define A10_MMC_GCTRL              0x00              /* Global Control Register */
#define A10_MMC_CLKCR              0x04              /* Clock Control Register */
#define A10_MMC_TMOUT              0x08              /* Time Out Register */
#define A10_MMC_WIDTH              0x0C              /* Bus Width Register */
#define A10_MMC_BLKSZ              0x10              /* Block Size Register */
#define A10_MMC_BCNTR              0x14              /* Byte Count Register */
#define A10_MMC_CMDR               0x18              /* Command Register */
#define A10_MMC_CARG               0x1C              /* Argument Register */
#define A10_MMC_RESP0              0x20              /* Response Register 0 */
#define A10_MMC_RESP1              0x24              /* Response Register 1 */
#define A10_MMC_RESP2              0x28              /* Response Register 2 */
#define A10_MMC_RESP3              0x2C              /* Response Register 3 */
#define A10_MMC_IMASK              0x30              /* Interrupt Mask Register */
#define A10_MMC_MISTA              0x34              /* Masked Interrupt Status Register */
#define A10_MMC_RINTR              0x38              /* Raw Interrupt Status Register */
#define A10_MMC_STAS               0x3C              /* Status Register */
#define A10_MMC_FTRGL              0x40              /* FIFO Threshold Watermark Register */
#define A10_MMC_FUNS               0x44              /* Function Select Register */
#define A10_MMC_CBCR               0x48              /* CIU Byte Count Register */
#define A10_MMC_BBCR               0x4C              /* BIU Byte Count Register */
#define A10_MMC_DBGC               0x50              /* Debug Enable Register */
#define A10_MMC_DMAC               0x80              /* IDMAC Control Register */
#define A10_MMC_DLBA               0x84              /* IDMAC Descriptor List Base Address Register */
#define A10_MMC_IDST               0x88              /* IDMAC Status Register */
#define A10_MMC_IDIE               0x8C              /* IDMAC Interrupt Enable Register */
#define A10_MMC_CHDA               0x90
#define A10_MMC_CBDA               0x94
#define A10_MMC_FIFO               0x100             /* FIFO Access Address */

/* A10_MMC_GCTRL */
#define A10_MMC_SOFT_RESET               (1 << 0)
#define A10_MMC_FIFO_RESET               (1 << 1)
#define A10_MMC_DMA_RESET                (1 << 2)
#define A10_MMC_INT_ENABLE               (1 << 4)
#define A10_MMC_DMA_ENABLE               (1 << 5)
#define A10_MMC_DEBOUNCE_ENABLE          (1 << 8)
#define A10_MMC_POSEDGE_LATCH_DATA       (1 << 9)
#define A10_MMC_NEGEDGE_LATCH_DATA       (0 << 9)
#define A10_MMC_DDR_MODE                 (1 << 10)
#define A10_MMC_ACCESS_BY_AHB            (1 << 31)
#define A10_MMC_ACCESS_BY_DMA            (0 << 31)

/* A10_MMC_CLKCR */
#define A10_MMC_CARD_CLK_ON              (1 << 16)
#define A10_MMC_LOW_POWER_ON             (1 << 17)

/* A10_MMC_WIDTH */
#define A10_MMC_WIDTH1                   (0)
#define A10_MMC_WIDTH4                   (1)
#define A10_MMC_WIDTH8                   (2)

/* A10_MMC_CMDR */
#define A10_MMC_RESP_EXP                 (1 << 6)
#define A10_MMC_LONG_RESP                (1 << 7)
#define A10_MMC_CHECK_RESP_CRC           (1 << 8)
#define A10_MMC_DATA_EXP                 (1 << 9)
#define A10_MMC_READ                     (0 << 10)
#define A10_MMC_WRITE                    (1 << 10)
#define A10_MMC_BLOCK_MODE               (0 << 11)
#define A10_MMC_SEQ_MODE                 (1 << 11)
#define A10_MMC_SEND_AUTOSTOP            (1 << 12)
#define A10_MMC_WAIT_PREOVER             (1 << 13)
#define A10_MMC_STOP_ABORT_CMD           (1 << 14)
#define A10_MMC_SEND_INIT_SEQ            (1 << 15)
#define A10_MMC_UPCLK_ONLY               (1 << 21)
#define A10_MMC_RDCEATADEV               (1 << 22)
#define A10_MMC_CCS_EXP                  (1 << 23)
#define A10_MMC_ENB_BOOT                 (1 << 24)
#define A10_MMC_ALT_BOOT_OPT             (1 << 25)
#define A10_MMC_MAND_BOOT_OPT            (0 << 25)
#define A10_MMC_BOOT_ACK_EXP             (1 << 26)
#define A10_MMC_DISABLE_BOOT             (1 << 27)
#define A10_MMC_VOL_SWITCH               (1 << 28)
#define A10_MMC_START                    (1 << 31)

/* A10_MMC_IMASK and A10_MMC_RINTR */
#define A10_MMC_RESP_ERR                 (1 << 1)
#define A10_MMC_CMD_DONE                 (1 << 2)
#define A10_MMC_DATA_OVER                (1 << 3)
#define A10_MMC_TX_DATA_REQ              (1 << 4)
#define A10_MMC_RX_DATA_REQ              (1 << 5)
#define A10_MMC_RESP_CRC_ERR             (1 << 6)
#define A10_MMC_DATA_CRC_ERR             (1 << 7)
#define A10_MMC_RESP_TIMEOUT             (1 << 8)
#define A10_MMC_ACK_RECV                 (1 << 8)
#define A10_MMC_DATA_TIMEOUT             (1 << 9)
#define A10_MMC_BOOT_START               (1 << 9)
#define A10_MMC_DATA_STARVE              (1 << 10)
#define A10_MMC_VOL_CHG_DONE             (1 << 10)
#define A10_MMC_FIFO_RUN_ERR             (1 << 11)
#define A10_MMC_HARDW_LOCKED             (1 << 12)
#define A10_MMC_START_BIT_ERR            (1 << 13)
#define A10_MMC_AUTOCMD_DONE             (1 << 14)
#define A10_MMC_END_BIT_ERR              (1 << 15)
#define A10_MMC_SDIO_INT                 (1 << 16)
#define A10_MMC_CARD_INSERT              (1 << 30)
#define A10_MMC_CARD_REMOVE              (1 << 31)
#define A10_MMC_INT_ERR_BIT              (A10_MMC_RESP_ERR | A10_MMC_RESP_CRC_ERR | A10_MMC_DATA_CRC_ERR | A10_MMC_RESP_TIMEOUT | A10_MMC_DATA_TIMEOUT  \
                                        | A10_MMC_FIFO_RUN_ERR | A10_MMC_HARDW_LOCKED | A10_MMC_START_BIT_ERR | A10_MMC_END_BIT_ERR)
/* A10_MMC_STAS */
#define A10_MMC_RX_WLFLAG                (1 << 0)
#define A10_MMC_TX_WLFLAG                (1 << 1)
#define A10_MMC_FIFO_EMPTY               (1 << 2)
#define A10_MMC_FIFO_FULL                (1 << 3)
#define A10_MMC_CARD_PRESENT             (1 << 8)
#define A10_MMC_CARD_DATA_BUSY           (1 << 9)
#define A10_MMC_DATA_FSM_BUSY            (1 << 10)
#define A10_MMC_DMA_REQ                  (1 << 31)
#define A10_MMC_FIFO_SIZE                (16)

/* A10_MMC_FUNS */
#define A10_MMC_CE_ATA_ON                (0XCEAAU<< 16)
#define A10_MMC_SEND_IRQ_RESP            (1 << 0)
#define A10_MMC_SDIO_RD_WAIT             (1 << 1)
#define A10_MMC_ABT_RD_DATA              (1 << 2)
#define A10_MMC_SEND_CC_SD               (1 << 8)
#define A10_MMC_SEND_AUTOSTOP_CC_SD      (1 << 9)
#define A10_MMC_CE_ATA_DEV_INT_ENB       (1 << 10)

/* IDMA CONTROLLER BUS MOD BIT FIELD */
#define A10_MMC_IDMAC_SOFT_RST           (1 << 0)
#define A10_MMC_IDMAC_FIX_BURST          (1 << 1)
#define A10_MMC_IDMAC_IDMA_ON            (1 << 7)
#define A10_MMC_IDMAC_REFETCH_DES        (1 << 31)

/* A10_MMC_IDST */
#define A10_MMC_IDMAC_TRANSMIT_INT       (1 << 0)
#define A10_MMC_IDMAC_RECEIVE_INT        (1 << 1)
#define A10_MMC_IDMAC_FATAL_BUS_ERR      (1 << 2)
#define A10_MMC_IDMAC_DES_INVALID        (1 << 4)
#define A10_MMC_IDMAC_CARD_ERR_SUM       (1 << 5)
#define A10_MMC_IDMAC_NORMAL_INT_SUM     (1 << 8)
#define A10_MMC_IDMAC_ABNORMAL_INT_SUM   (1 << 9)
#define A10_MMC_IDMAC_HOST_ABT_INTX      (1 << 10)
#define A10_MMC_IDMAC_HOST_ABT_INRX      (1 << 10)
#define A10_MMC_IDMAC_IDLE               (0 << 13)
#define A10_MMC_IDMAC_SUSPEND            (1 << 13)
#define A10_MMC_IDMAC_DESC_RD            (0X2U<< 13)
#define A10_MMC_IDMAC_DESC_CHECK         (0X3U<< 13)
#define A10_MMC_IDMAC_RD_REQ_WAIT        (0X4U<< 13)
#define A10_MMC_IDMAC_WR_REQ_WAIT        (0X5U<< 13)
#define A10_MMC_IDMAC_RD                 (0X6U<< 13)
#define A10_MMC_IDMAC_WR                 (0X7U<< 13)
#define A10_MMC_IDMAC_DESC_CLOSE         (0X8U<< 13)

#define A10_MMC_IDMA_OVER       (A10_MMC_IDMAC_TRANSMIT_INT|A10_MMC_IDMAC_RECEIVE_INT|A10_MMC_IDMAC_NORMAL_INT_SUM)
#define A10_MMC_IDMA_ERR        (A10_MMC_IDMAC_FATAL_BUS_ERR|A10_MMC_IDMAC_DES_INVALID|A10_MMC_IDMAC_CARD_ERR_SUM|A10_MMC_IDMAC_ABNORMAL_INT_SUM)

#endif /* _A10_MMC_H_ */

