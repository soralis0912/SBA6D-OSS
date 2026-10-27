/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/spi/spi.h>
#include "inc/proslic.h"
#include "timer.h"
#include "spi.h"
#include "proslic_sys.h"

#ifdef SI3218X
#include "inc/si3218x.h"
#include "inc/si3218x_intf.h"
#include "config_inc/si3218x_LCCB_constants.h"

#define PROSLIC_DEVICE_TYPE SI3218X_TYPE
#endif

#ifdef SI3217X
#include "inc/si3217x.h"
#include "inc/si3217x_intf.h"
#include "config_inc/si3217x_LCQC_MP_constants.h"

#define PROSLIC_DEVICE_TYPE SI3217X_TYPE
#endif

struct si321xx_chip {
	struct spi_device *spi;
	struct device *dev;
	struct snd_soc_component *component;
};

#define NUMBER_OF_DEVICES 1
#define CHAN_PER_DEVICE 1
#define NUMBER_OF_CHAN (NUMBER_OF_DEVICES*CHAN_PER_DEVICE)
#define NUMBER_OF_PROSLIC (NUMBER_OF_CHAN)

extern proslicChanType_ptr arrayOfProslicChans[NUMBER_OF_CHAN];

/* Export function */
int si321xx_spi_probe(struct spi_device *spi, struct spi_driver *spi_drv);
int si321xx_spi_remove(struct spi_device *spi);
int si321xx_init_thread(void *arg);
int si321xx_ProSLIC_HWInit(void);


