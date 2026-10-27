// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 MediaTek Inc.
 */


/*! \file smi230.h
 * \brief Sensor Driver for SMI230 sensors */
#ifndef _SMI230_GYRO_H
#define _SMI230_GYRO_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************/
/* header files */
#include <linux/module.h>
#include <linux/delay.h>

#define DRIVER_VERSION "1.1.0"
#define MODULE_NAME "SMI230GYRO"
#define SENSOR_GYRO_NAME "SMI230GYRO"

enum smi230_intf {
    /*! I2C interface */
    SMI230_I2C_INTF,

    /*! SPI interface */
    SMI230_SPI_INTF
};

struct smi230_cfg
{
    /*! power mode */
    uint8_t power;

    /*! range */
    uint8_t range;

    /*! bandwidth */
    uint8_t bw;

    /*! output data rate */
    uint8_t odr;
};

typedef int8_t (*smi230_com_fptr_t)(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

typedef void (*smi230_delay_fptr_t)(uint32_t period);

struct smi230_dev
{

    /*! Accel chip Id */
    uint8_t accel_chip_id;

    /*! Gyro chip Id */
    uint8_t gyro_chip_id;

    /*! Accel device Id */
    uint8_t accel_id;

    /*! Gyro device Id */
    uint8_t gyro_id;

    /*! 0 - I2C , 1 - SPI Interface */
    enum smi230_intf intf;

    /*! Decide SPI or I2C read mechanism */
    uint8_t dummy_byte;

    /*! Structure to configure gyro sensor  */
    struct smi230_cfg gyro_cfg;

    /*! Config stream data buffer address will be assigned */
    const uint8_t *config_file_ptr;

    /*! Max read/write length (maximum supported length is 32).
     * To be set by the user */
    uint8_t read_write_len;

    /*! Read function pointer */
    smi230_com_fptr_t read;

    /*! Write function pointer */
    smi230_com_fptr_t write;

    /*! Delay function pointer */
    smi230_delay_fptr_t delay_ms;
};

int smi230_gyro_core_probe(struct device *dev, struct smi230_dev *p_smi230_dev);

static inline void smi230_delay(uint32_t msec)
{
        unsigned long mseond = msec;
        unsigned long min = mseond * (1000);
        /* if the time less than 20ms */
        if (msec <= 20)
                usleep_range(min, (min + 1000));
        else
                msleep(msec);
}
#ifdef __cplusplus
}
#endif

#endif /* _SMI230_GRYO_H */

/** @}*/
