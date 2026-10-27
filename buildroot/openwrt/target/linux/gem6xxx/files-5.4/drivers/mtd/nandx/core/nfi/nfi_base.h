/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017 MediaTek Inc.
 */

#ifndef __NFI_BASE_H__
#define __NFI_BASE_H__

#define NFI_TIMEOUT             1000000

enum randomizer_op {
	RAND_ENCODE,
	RAND_DECODE
};

struct bad_mark_ctrl {
	void (*bad_mark_swap)(struct nfi *nfi, u8 *buf, u8 *fdm);
	u8 *(*fdm_shift)(struct nfi *nfi, u8 *fdm, int sector);
	u32 sector;
	u32 position;
};

struct nfi_caps {
	u8 max_fdm_size;
	u8 fdm_ecc_size;
	u8 ecc_parity_bits;
	const int *spare_size;
	u32 spare_size_num;
};

enum nfi_error_code {
	/******** NFI successful code *********/
	NFI_SUCCESS							  = 0x00
	, NFI_ECC_CORRECTED                   = 0x01

	/******** NFI error code *********/
	, NFI_ERROR                           = 0x30
	, NFI_ECC_UNCORRECT                   = 0x31
	, NFI_ECC_TIMEOUT                     = 0x32
	, NFI_CMD_TIMEOUT                     = 0x33
	, NFI_ADDR_TIMEOUT                    = 0x34
	, NFI_DATA_TIMEOUT                    = 0x35
	, NFI_PROG_TIMEOUT                    = 0x36
	, NFI_ERASE_TIMEOUT                   = 0x37
	, NFI_READ_TIMEOUT                    = 0x38
	, NFI_RESET_TIMEOUT                   = 0x39
	, NFI_DEVICE_TIMEOUT                  = 0x3A
	, NFI_PROG_FAILED                     = 0x3B
	, NFI_ERASE_FAILED                    = 0x3C
	, NFI_INVALID_PARAM                   = 0x3E
	, NFI_BAD_BLOCK                       = 0x3F
};

struct nfi_base {
	struct nfi nfi;
	struct nfi_resource res;
	struct nfiecc *ecc;
	struct nfi_format format;
	struct nfi_caps *caps;
	struct bad_mark_ctrl bad_mark_ctrl;

	/* page_size + spare_size */
	u8 *buf;
	u8 *buf_align;

	/* used for spi nand */
	u8 cmd_mode;
	u32 op_mode;

	int page_sectors;

	void *done;

	/* for read/write */
	int col;
	int row;
	int access_len;
	int rw_sectors;
	void *dma_addr;
	int read_status;

	bool dma_en;
	bool dma_burst_en;
	bool byte_rw_en;

	/* CPU IRQ for NFI/NFIECC */
	bool nfi_irq_en;
	u32 nfi_irq_status;
	u32 nfi_irq_all;

	u8 fdm_size_sel;
	u8 fdm_ecc_size_sel;
	u8 sector_spare_size_sel;
	u16 sector_size_sel;

	bool cus_sec_size_en;
	u8 cus_sec_size_sel;

	/* No need for SLC/SPI */
	bool last_seccus_size_en;
	u8 last_seccus_size_sel;

	bool ecc_en;

	/* No need for SLC/SPI */
	bool randomize_en;
	u8 randomize_sel;
	bool crc_en;

	bool bad_mark_swap_en;

	enum nfiecc_deccon ecc_deccon;
	enum nfiecc_mode ecc_mode;

	void (*set_op_mode)(void *regs, u32 mode);
	bool (*is_page_empty)(struct nfi_base *nb, u8 *data, u8 *fdm,
			      int sectors);

	int (*rw_prepare)(struct nfi_base *nb, int sectors, u8 *data, u8 *fdm,
			  bool read);
	void (*rw_trigger)(struct nfi_base *nb, bool read);
	int (*rw_wait_done)(struct nfi_base *nb, int sectors, bool read);
	int (*rw_data)(struct nfi_base *nb, u8 *data, u8 *fdm, int sectors,
		       bool read);
	void (*rw_complete)(struct nfi_base *nb, u8 *data, u8 *fdm, bool read);
};

static inline struct nfi_base *nfi_to_base(struct nfi *nfi)
{
	return container_of(nfi, struct nfi_base, nfi);
}

struct nfi *nfi_extend_init(struct nfi_base *nb);
void nfi_extend_exit(struct nfi_base *nb);
struct nfi *nfi_extend_spi_init(struct nfi_base *nb);
void nfi_extend_spi_exit(struct nfi_base *nb);

#endif /* __NFI_BASE_H__ */
