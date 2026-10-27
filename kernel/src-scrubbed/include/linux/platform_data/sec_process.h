/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef ____LINUX_PLATFORM_DATA_SEC_PROCESS_H
#define ____LINUX_PLATFORM_DATA_SEC_PROCESS_H

#include <linux/types.h>

#define CIPHER_AES_128  0xA00
#define CIPHER_AES_192  0xB00
#define CIPHER_AES_256  0xC00
#define MODE_CBC        0xA0
#define MODE_ECB        0xF0
#define PKCS7PADDING    0x0A
#define NOPADDING       0x0F

#define ERR_OK                              0x0
#define ERR_SECDP_BUF_ADDR_INVALID          0x0010A000
#define ERR_SECDP_BUF_OVERFLOW              0x0010A001
#define ERR_SECDP_BUF_ALLOC_MEM_FAIL        0x0010A002
#define ERR_SECDP_INVALID_STATUS            0x0010A003
#define ERR_SECDP_INVALID_LABEL             0x0010A010
#define ERR_SECDP_INVALID_CIPHER_TYPE       0x0010A011
#define ERR_SECDP_INVALID_CIPHER_MODE       0x0010A012
#define ERR_SECDP_INVALID_PADDING           0x0010A013
#define ERR_SECDP_INVALID_IN_DATA           0x0010A014
#define ERR_SECDP_INVALID_OUT_BUFFER        0x0010A015
#define ERR_SECDP_INVALID_OUT_BUFFER_SIZE   0x0010A016
#define ERR_SECDP_VERIFY_PADDING_ERROR      0x0010A017
#define ERR_SECDP_VERIFY_HEADER_ERROR       0x0010A018
#define ERR_SECDP_INVALID_ACTION            0x0010A030

/* Flag of query status */
#define SECDP_STATUS_SBC_EN                 0x00001000
#define SECDP_STATUS_JTAG_EN                0x00002000

/*
 * sec_encrypt() - Encrypt data by device key
 *                 Example: sec_encrypt(TYPE_AES_256|MODE_CBC|PKCS7PADDING,
 *                                      plaintext,
 *                                      size,
 *                                      buffer,
 *                                      &buffer_len);
 *
 * @flag: cipher mode and padding
 * @plaintext: pointer of plaintext to encrypt
 * @size: size of data to encrypt
 * @out: pointer of output data buffer
 * @out_size: size of output buffer and result size
 *
 * Return: Status of execution
 */
u32 sec_encrypt(u32 flag, u8 *plaintext, u32 size,
			u8 *out, u32 *out_size);

/*
 * sec_decrypt() - Decrypt data by device key
 *                 Example: sec_decrypt(TYPE_AES_256|MODE_CBC|PKCS7PADDING,
 *                                      cihpertext,
 *                                      size,
 *                                      buffer,
 *                                      &buffer_len);
 *
 * @flag: cipher mode and padding
 * @ciphertext: pointer of ciphertext to decrypt
 * @size: size of data to decrypt
 * @out: pointer of output data buffer
 * @out_size: size of output buffer and result size
 *
 * Return: Status of execution
 */
u32 sec_decrypt(u32 flag, u8 *ciphertext, u32 size,
			u8 *out, u32 *out_size);

/*
 * sec_query_status() - Query the status of security device
 *
 * @flag: Which status need to be queried
 * @status: Output data of query
 *
 * Return: Status of execution
 */
u32 sec_query_status(u32 flag, u32 *status);

#endif
