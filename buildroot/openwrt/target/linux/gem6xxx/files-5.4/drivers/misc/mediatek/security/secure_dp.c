// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/arm-smccc.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/types.h>
#include <linux/platform_data/sec_process.h>
#include <linux/vmalloc.h>

#define DRIVER_VERSION              "1.0"
#define AES_BLOCK_SIZE              16

/* MAX_BUFFER_SIZE must be a multiple of 16 */
#define MAX_BUFFER_SIZE             1024

/* Action of crypto SMC call */
#define ACTION_PROCESS_INIT         0x01000000
#define ACTION_PROCESS_ENCRYPT      0x02000000
#define ACTION_PROCESS_DECRYPT      0x03000000
#define ACTION_PROCESS_FINAL        0x04000000

/* Mask of flag */
#define MASK_ACTION                 0xFF000000
#define MASK_KEY_LABEL              0xF0000
#define MASK_CIPHER_TYPE            0xF00
#define MASK_CIPHER_MODE            0xF0
#define MASK_PADDING                0xF

#define MODE_OF_PADDING(mode, type) (((mode) & MASK_PADDING) == (type))

/* lock of API */
static DEFINE_MUTEX(secure_device_lock);

/*
 * sec_memcpy() - For memory copy
 * @dest: Point of destination
 * @src: Point of source
 * @size: Length of data need to copy
 *
 */
static void sec_memcpy(u8 *dest, const u8 *src,
			u32 size)
{
	u32 i = 0;

	for (i = 0; i < size; i++)
		dest[i] = src[i];
}

/*
 * sec_memset() - For memory set
 * @buffer: Point of data
 * @size: Length of data need to set
 * @value: value to set
 *
 */
static void sec_memset(u8 *buffer, u32 size,
			u8 value)
{
	u32 i = 0;

	for (i = 0; i < size; i++)
		buffer[i] = value;
}

/*
 * validate() - Validate input value
 * @encrypt: Mark if it is enerypt or decrypt
 * @flag: Encryption flag
 * @in: buffer of input
 * @in_size: size of input buffer
 * @out: buffer of output
 * @out_size: size of output buffer
 * @return status
 *
 */
static u32 validate(bool encrypt, u32 flag,
			u8 *in, u32 in_size,
			u8 *out, u32 *out_size)
{
	u32 label = flag & MASK_KEY_LABEL;
	u32 cihper_type = flag & MASK_CIPHER_TYPE;
	u32 mode = flag & MASK_CIPHER_MODE;
	u32 padding = flag & MASK_PADDING;
	u32 buffer_size = 0;

	/* Check supported key label */
	if (label < 0 || label > 5)
		return ERR_SECDP_INVALID_LABEL;

	/* Check supported cipher type */
	switch (cihper_type) {
	case CIPHER_AES_256:
		break;
	default:
		return ERR_SECDP_INVALID_CIPHER_TYPE;
	}

	/* Check supported cipher type */
	switch (mode) {
	case MODE_CBC:
	case MODE_ECB:
		break;
	default:
		return ERR_SECDP_INVALID_CIPHER_MODE;
	}

	/* Check supported padding type */
	switch (padding) {
	case PKCS7PADDING:
	case NOPADDING:
		break;
	default:
		return ERR_SECDP_INVALID_PADDING;
	}

	/* Check input data */
	if (in == NULL || in_size <= 0)
		return ERR_SECDP_INVALID_IN_DATA;

	/* Check output buffer */
	if (out == NULL || *out_size <= 0)
		return ERR_SECDP_INVALID_OUT_BUFFER;

	/* Check input size and buffer size */
	if (encrypt) {
		if (padding == PKCS7PADDING)
			buffer_size = (in_size / AES_BLOCK_SIZE + 1)
					* AES_BLOCK_SIZE;
		else {
			if (in_size % AES_BLOCK_SIZE > 0)
				return ERR_SECDP_INVALID_IN_DATA;
			buffer_size = in_size;
		}

		if (*out_size < buffer_size)
			return ERR_SECDP_INVALID_OUT_BUFFER_SIZE;
	} else {
		if (in_size % AES_BLOCK_SIZE > 0)
			return ERR_SECDP_INVALID_IN_DATA;

		if (*out_size < in_size)
			return ERR_SECDP_INVALID_OUT_BUFFER_SIZE;
	}

	return ERR_OK;
}

/*
 * sec_smc_init() - initialize cipher
 * @flag control flag of encryption
 * @return status
 *
 */
static u32 sec_smc_init(u32 flag)
{
	struct arm_smccc_res res;
	u32 cmd = flag & (MASK_KEY_LABEL | MASK_CIPHER_MODE | MASK_CIPHER_TYPE);

	cmd |= ACTION_PROCESS_INIT;

	arm_smccc_smc(MTK_SIP_CRYPTO_CONTROL,
		cmd, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

/*
 * sec_smc_final() - clean up cipher
 * @return status
 *
 */
static u32 sec_smc_final(void)
{
	struct arm_smccc_res res;
	u32 cmd = ACTION_PROCESS_FINAL;

	arm_smccc_smc(MTK_SIP_CRYPTO_CONTROL,
		cmd, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

/*
 * sec_smc_call() - Invoke ATF function
 * @is_encrypt: encryption or decryption
 * @buffer_pa: phyical address of buffer
 * @buffer_size: buffer size
 * @data_size: data size
 * @out_size: the size of return
 *
 */
static u32 sec_smc_call(bool is_encrypt, u32 buffer_pa,
			u32 buffer_size, u32 data_size, u32 *out_size)
{
	int rc = 0;
	struct arm_smccc_res res;
	u32 cmd = ACTION_PROCESS_DECRYPT;

	if (is_encrypt)
		cmd = ACTION_PROCESS_ENCRYPT;

	arm_smccc_smc(MTK_SIP_CRYPTO_CONTROL,
		cmd, buffer_pa, buffer_size, data_size, 0, 0, 0, &res);

	rc = res.a0;
	if (!rc)
		*out_size = res.a1;

	return rc;
}

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
			u8 *out, u32 *out_size)
{
	int i = 0, j = 0;
	int rc = ERR_OK;
	int padding_size = 0;
	int buffer_size = size;
	int data_size = 0;
	int page = 0;
	u8 *buffer = NULL;
	u8 *in_block = NULL;
	u8 *buf_block = NULL;
	u8 *out_block = NULL;
	u32 buffer_pa = 0;
	u32 buffer_data_size = 0;
	u32 out_data_size = 0;
	u32 out_size_local = 0;

	rc = validate(true, flag, plaintext, size, out, out_size);
	if (rc != ERR_OK)
		return rc;

	/* if not pkcs7 padding, use no paddng as default */
	if (MODE_OF_PADDING(flag, PKCS7PADDING)) {
		buffer_size = (size / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
		padding_size = buffer_size - size;
	}

	page = buffer_size / MAX_BUFFER_SIZE;
	if ((buffer_size % MAX_BUFFER_SIZE) > 0)
		page += 1;
	if (page < 1)
		return ERR_SECDP_INVALID_IN_DATA;

	/* alloc buffer memory by kmalloc */
	buffer = kmalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
	if (!buffer)
		return ERR_SECDP_BUF_ALLOC_MEM_FAIL;

	/* Get phyical memory address of buffer */
	buffer_pa = virt_to_phys(buffer);

	out_block = out;
	in_block = plaintext;
	data_size = size;
	out_size_local = *out_size;

	mutex_lock(&secure_device_lock);
	rc = sec_smc_init(flag);
	if (rc)
		goto end;

	/* Process 0 to N-2 block, no need to handle padding */
	for (i = 0; i < page - 1; i++) {
		if (data_size < MAX_BUFFER_SIZE) {
			rc = ERR_SECDP_BUF_ADDR_INVALID;
			goto end_session;
		}

		sec_memcpy(buffer, in_block, MAX_BUFFER_SIZE);
		in_block += MAX_BUFFER_SIZE;
		data_size -= MAX_BUFFER_SIZE;

		rc = sec_smc_call(true, buffer_pa, MAX_BUFFER_SIZE,
				MAX_BUFFER_SIZE, &out_data_size);
		if (rc)
			goto end_session;

		if (out_size_local < out_data_size) {
			rc = ERR_SECDP_BUF_OVERFLOW;
			goto end_session;
		}

		/* copy result data from buffer to out buffer */
		sec_memcpy(out_block, buffer, out_data_size);
		out_size_local -= out_data_size;
		out_block += out_data_size;
	}

	/* Process N-1 block, need to handle padding */
	buf_block = buffer;
	buffer_data_size = 0;
	if (data_size > 0) {
		sec_memcpy(buf_block, in_block, data_size);
		buf_block += data_size;
		buffer_data_size = data_size;
	}
	if (padding_size > 0) {
		for (j = 0; j < padding_size; j++)
			buf_block[j] = padding_size;
		buffer_data_size += padding_size;
	}
	if (buffer_data_size > 0) {
		rc = sec_smc_call(true, buffer_pa, MAX_BUFFER_SIZE,
				buffer_data_size, &out_data_size);
		if (rc)
			goto end_session;

		if (out_size_local < out_data_size) {
			rc = ERR_SECDP_BUF_OVERFLOW;
			goto end_session;
		}

		/* copy result data from buffer to out buffer */
		sec_memcpy(out_block, buffer, out_data_size);
		out_size_local -= out_data_size;
		out_block += out_data_size;
	}

	*out_size = out_block - out;

end_session:
	if (rc)
		sec_smc_final();
	else
		rc = sec_smc_final();
end:

	mutex_unlock(&secure_device_lock);

	if (buffer) {
		sec_memset(buffer, MAX_BUFFER_SIZE, 0);
		kfree(buffer);
	}

	return rc;
}
EXPORT_SYMBOL(sec_encrypt);

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
			u8 *out, u32 *out_size)
{
	int i = 0, j = 0;
	int rc = ERR_OK;
	int padding_size = 0;
	int data_size = 0;
	int page = 0;
	u8 *buffer = NULL;
	u8 *in_block = NULL;
	u8 *out_block = NULL;
	u32 buffer_pa = 0;
	u32 out_data_size = 0;
	u32 out_size_local = 0;

	rc = validate(false, flag, ciphertext, size, out, out_size);
	if (rc != ERR_OK)
		return rc;

	page = size / MAX_BUFFER_SIZE;
	if ((size % MAX_BUFFER_SIZE) > 0)
		page += 1;
	if (page < 1)
		return ERR_SECDP_INVALID_IN_DATA;

	/* alloc buffer memory by kmalloc */
	buffer = kmalloc(MAX_BUFFER_SIZE, GFP_KERNEL);
	if (!buffer)
		return ERR_SECDP_BUF_ALLOC_MEM_FAIL;

	/* Get phyical memory address of buffer */
	buffer_pa = virt_to_phys(buffer);

	out_block = out;
	in_block = ciphertext;
	data_size = size;
	out_size_local = *out_size;

	mutex_lock(&secure_device_lock);

	rc = sec_smc_init(flag);
	if (rc)
		goto end;

	/* Process 0 to N-2 blocks, no need to handle padding here */
	for (i = 0; i < page - 1; i++) {
		sec_memcpy(buffer, in_block, MAX_BUFFER_SIZE);
		in_block += MAX_BUFFER_SIZE;
		data_size -= MAX_BUFFER_SIZE;

		rc = sec_smc_call(false, buffer_pa, MAX_BUFFER_SIZE,
				MAX_BUFFER_SIZE, &out_data_size);
		if (rc)
			goto end_session;

		if (out_size_local < out_data_size) {
			rc = ERR_SECDP_BUF_OVERFLOW;
			goto end_session;
		}

		/* copy result data from buffer to out buffer */
		sec_memcpy(out_block, buffer, out_data_size);
		out_size_local -= out_data_size;
		out_block += out_data_size;
	}

	/* Process last block */
	if (data_size > 0) {
		sec_memcpy(buffer, in_block, data_size);
		in_block += MAX_BUFFER_SIZE;

		rc = sec_smc_call(false, buffer_pa, MAX_BUFFER_SIZE,
				data_size, &out_data_size);
		if (rc)
			goto end_session;

		if (out_size_local < out_data_size) {
			rc = ERR_SECDP_BUF_OVERFLOW;
			goto end_session;
		}

		/* copy result data from buffer to out buffer */
		sec_memcpy(out_block, buffer, out_data_size);
		out_size_local -= out_data_size;
		out_block += out_data_size;
	}

	/* if not pkcs7 padding, use no paddng as default */
	if (MODE_OF_PADDING(flag, PKCS7PADDING)) {
		if (out_data_size < AES_BLOCK_SIZE) {
			rc = ERR_SECDP_VERIFY_PADDING_ERROR;
			goto end_session;
		}

		/* verify remove padding */
		padding_size = *(out_block - 1);
		for (j = 1; j <= padding_size; j++) {
			if (*(out_block - j) != padding_size) {
				rc = ERR_SECDP_VERIFY_PADDING_ERROR;
				goto end_session;
			}

			*(out_block - j) = 0;
		}

		out_block -= padding_size;
	}

	*out_size = out_block - out;

end_session:
	if (rc)
		sec_smc_final();
	else
		rc = sec_smc_final();
end:
	mutex_unlock(&secure_device_lock);
	if (buffer) {
		sec_memset(buffer, MAX_BUFFER_SIZE, 0);
		kfree(buffer);
	}

	return rc;
}
EXPORT_SYMBOL(sec_decrypt);

/* module load/unload record keeping */
static int __init secure_dp_init(void)
{
	return 0;
}

static void __exit secure_dp_exit(void)
{

}

module_init(secure_dp_init);
module_exit(secure_dp_exit);

MODULE_AUTHOR("Mediatek");
MODULE_DESCRIPTION("Mediatek Secure Data Protection Driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
