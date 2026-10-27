/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/spi/spi.h>
#include "inc/proslic.h"
#include "timer.h"
#include "spi2.h"

#if 1
#include "inc/si3218x.h"
#include "inc/si3218x_intf.h"
#include "config_inc/si3218x_LCCB_constants.h"
#endif

typedef struct chanStatus chanState; //forward declaration

typedef void (*procState) (chanState *pState, ProslicInt eInput);

/* Export function */
int si3218x2_spi_probe(struct spi_device *spi, struct spi_driver *spi_drv);
int si3218x2_spi_remove(struct spi_device *spi);
int ProSLIC_HWInit2(void);
void ProSLIC_LP2(bool enable);


