/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

#ifndef __NANDX_CORE_H__
#define __NANDX_CORE_H__

#define NAND_MIN_OOB_REQ     16

#define NAND_MAX_FDM_SIZE    8
#define NAND_FDM_ECC_SIZE    1 /* it's a default value */
#define NAND_ECC_PARITY_BITS 14

/**
 * mtk_ic_version - indicates specifical IC, IP need this to load some info
 */
enum mtk_ic_version {
	NANDX_MT8518,
    NANDX_MT6880,
};

/**
 * nand_type - indicates nand type
 */
enum nand_type {
	NAND_SPI,
	NAND_SLC,
	NAND_MLC,
	NAND_TLC
};

/**
 * nandx_ioctl_cmd - operations supported by nandx
 *
 * @NFI_CTRL_DMA dma enable or not
 * @NFI_CTRL_NFI_MODE customer/read/program/erase...
 * @NFI_CTRL_ECC ecc enable or not
 * @NFI_CTRL_ECC_MODE nfi/dma/pio
 * @CHIP_CTRL_DRIVE_STRENGTH enum chip_ctrl_drive_strength
 */
enum nandx_ctrl_cmd {
	CORE_CTRL_NAND_INFO,
	CORE_CTRL_PERF_INFO,
	CORE_CTRL_PERF_INFO_CLEAR,

	NFI_CTRL_BASE_INFO,
	SNFI_CTRL_BASE_INFO,

	NFI_CTRL_NFI_IRQ, /*5*/
	NFI_CTRL_ECC_IRQ,
	NFI_CTRL_ECC_PAGE_IRQ,

	NFI_CTRL_DMA,
	NFI_BURST_EN,
	NFI_ADDR_ALIGNMENT_EN, /*10*/
	NFI_BYTE_RW_EN,

	NFI_CRC_EN,
	NFI_CTRL_RANDOMIZE,
	NFI_CTRL_RANDOMIZE_SEL,

	NFI_CTRL_IO_FORMAT, /*15*/

	NFI_CTRL_ECC,
	NFI_CTRL_ECC_MODE,
	NFI_CTRL_ECC_DECODE_MODE,
	NFI_CTRL_ECC_ERRNUM0,
	NFI_CTRL_ECC_GET_STATUS, /*20*/

	NFI_CTRL_BAD_MARK_SWAP,
	NFI_CTRL_IOCON,

	SNFI_CTRL_OP_MODE,
	SNFI_CTRL_RX_MODE,
	SNFI_CTRL_TX_MODE, /*25*/
	SNFI_CTRL_DELAY_MODE,
	SNFI_CTRL_4FIFO_EN,
	SNFI_CTRL_GF_CONFIG,
	SNFI_CTRL_SAMPLE_DELAY,
	SNFI_CTRL_LATCH_LATENCY, /*30*/
	SNFI_CTRL_MAC_QPI_MODE,

	CHIP_CTRL_OPS_CACHE,
	CHIP_CTRL_OPS_MULTI,
	CHIP_CTRL_PSLC_MODE,
	CHIP_CTRL_DRIVE_STRENGTH, /*35*/
	CHIP_CTRL_DDR_MODE,

	CHIP_CTRL_DEVICE_RESET,
	CHIP_CTRL_ONDIE_ECC,
	CHIP_CTRL_TIMING_MODE,

	CHIP_CTRL_PERF_INFO, /*40*/
	CHIP_CTRL_PERF_INFO_CLEAR,

	CHIP_CTRL_READ_UNIQUE_ID,
	CHIP_CTRL_GET_STATUS,
};

struct nfiecc_status {
	u32 corrected;
	u32 failed;
	u32 bitflips;
};

struct chip_status {
	u32 corrected;
	u32 failed;
};

enum snfi_ctrl_op_mode {
	SNFI_CUSTOM_MODE,
	SNFI_AUTO_MODE,
	SNFI_MAC_MODE
};

enum snfi_ctrl_rx_mode {
	SNFI_RX_111,
	SNFI_RX_112,
	SNFI_RX_114,
	SNFI_RX_122,
	SNFI_RX_144
};

enum snfi_ctrl_tx_mode {
	SNFI_TX_111,
	SNFI_TX_114,
};

enum chip_ctrl_drive_strength {
	CHIP_DRIVE_NORMAL,
	CHIP_DRIVE_HIGH,
	CHIP_DRIVE_MIDDLE,
	CHIP_DRIVE_LOW
};

enum chip_ctrl_timing_mode {
	CHIP_TIMING_MODE0,
	CHIP_TIMING_MODE1,
	CHIP_TIMING_MODE2,
	CHIP_TIMING_MODE3,
	CHIP_TIMING_MODE4,
	CHIP_TIMING_MODE5,
};

struct nand_unique_id {
	u8 uid_len;
	u8 *uid;
};

/**
 * page_performance - performance information of read/write a single page
 */
struct page_performance {
	int rx_page_total_time;
	int read_page_time;
	int read_data_time;

	int tx_page_total_time;
	int write_page_time;
	int write_data_time;
};

/**
 * nand_performance - performance information
 */
struct nand_performance {
	struct page_performance page_perf;

	int read_speed;
	int write_speed;
	int erase_speed;
};

/**
 * nandx_info - basic information
 */
struct nandx_info {
	u32 max_io_count;
	u32 min_write_pages;
	u32 plane_num;
	u32 oob_size;
	u32 page_parity_size;
	u32 page_size;
	u32 block_size;
	u64 total_size;
	u32 fdm_reg_size;
	u32 fdm_ecc_size;
	u32 ecc_strength;
	u32 sector_size;
	u64 ids;
	u32 bbt_goodblocks;
	u8 *nand_name;
};

/**
 * nfi_io_format - nfi format for accessing data
 */
struct nfi_io_format {
	int fdm_size;
	int fdm_ecc_size;
	int sec_spare_size;
	int sec_size;
	int cus_sec_en;
	int cus_sec_size;
	int last_cus_sec_en;
	int last_cus_sec_size;
	int ecc_level_sel;
};

/**
 * nfi_resource - the resource needed by nfi & ecc to do initialization
 */
struct nfi_resource {
	int ic_ver;
	void *dev;

	void *ecc_regs;
	int ecc_irq_id;

	void *nfi_regs;
	int nfi_irq_id;

	u32 clock_1x;
	u32 *clock_2x;
	int clock_2x_num;

	int min_oob_req;
	int force_sector_size;
	int force_spare_size;

	u8 nand_type;
};

/**
 * nandx_init - init all related modules below
 *
 * @res: basic resource of the project
 *
 * return 0 if init success, otherwise return negative error code
 */
int nandx_init(struct nfi_resource *res);

/**
 * nandx_exit - release resource those that obtained in init flow
 */
void nandx_exit(void);

/**
 * nandx_read - read data from nand this function can read data and related
 *   oob from specifical address
 *   if do multi_ops, set one operation per time, and call nandx_sync at last
 *   in multi mode, not support page partial read
 *   oob not support partial read
 *
 * @data: buf to receive data from nand
 * @oob: buf to receive oob data from nand which related to data page
 *   length of @oob should oob size aligned, oob not support partial read
 * @offset: offset address on the whole flash
 * @len: the length of @data that need to read
 *
 * if read success return 0, otherwise return negative error code
 */
int nandx_read(u8 *data, u8 *oob, u64 offset, size_t len);

/**
 * nandx_write -  write data to nand
 *   this function can write data and related oob to specifical address
 *   if do multi_ops, set one operation per time, and call nandx_sync at last
 *
 * @data: source data to be written to nand,
 *   for multi operation, the length of @data should be page size aliged
 * @oob: source oob which related to data page to be written to nand,
 *   length of @oob should oob size aligned
 * @offset: offset address on the whole flash, the value should be start address
 *   of a page
 * @len: the length of @data that need to write,
 *   for multi operation, the len should be page size aliged
 *
 * if write success return 0, otherwise return negative error code
 * if return value > 0, it indicates that how many pages still need to write,
 * and data has not been written to nand
 * please call nandx_sync after pages alligned $nandx_info.min_write_pages
 */
int nandx_write(u8 *data, u8 *oob, u64 offset, size_t len);

/**
 * nandx_erase - erase an area of nand
 *   if do multi_ops, set one operation per time, and call nandx_sync at last
 *
 * @offset: offset address on the flash
 * @len: erase length which should be block size aligned
 *
 * if erase success return 0, otherwise return negative error code
 */
int nandx_erase(u64 offset, size_t len);

/**
 * nandx_sync - sync all operations to nand
 *   when do multi_ops, this function will be called at last operation
 *   when write data, if number of pages not alligned
 *   by $nandx_info.min_write_pages, this interface could be called to do
 *   force write, 0xff will be padded to blanked pages.
 */
int nandx_sync(void);

/**
 * nandx_is_bad_block - check if the block is bad
 *   only check the flag that marked by the flash vendor
 *
 * @offset: offset address on the whole flash
 *
 * return true if the block is bad, otherwise return false
 */
bool nandx_is_bad_block(u64 offset);

/**
 * nandx_ioctl - set/get property of nand chip
 *
 * @cmd: parameter that defined in enum nandx_ioctl_cmd
 * @arg: operate parameter
 *
 * return 0 if operate success, otherwise return negative error code
 */
int nandx_ioctl(int cmd, void *arg);

/**
 * nandx_suspend - suspend nand, and store some data
 *
 * return 0 if suspend success, otherwise return negative error code
 */
int nandx_suspend(void);

/**
 * nandx_resume - resume nand, and replay some data
 *
 * return 0 if resume success, otherwise return negative error code
 */
int nandx_resume(void);

int nandx_write_raw_pages(u8 *data, u64 offset, size_t len);

/**
 * nandx_unit_test - unit test
 *
 * @offset: offset address on the whole flash
 * @len: should be not larger than a block size, we only test a block per time
 *
 * return 0 if test success, otherwise return negative error code
 */
int nandx_unit_test(u64 offset, size_t len);

#endif /* __NANDX_CORE_H__ */
