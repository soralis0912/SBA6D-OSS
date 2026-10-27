/* cs_driver_ext.h
 *
 * Top-level Product Configuration Settings specific for FPGA.
 */

/*****************************************************************************
* Copyright (c) 2010-2018 INSIDE Secure B.V. All Rights Reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef INCLUDE_GUARD_CS_DRIVER_EXT_H
#define INCLUDE_GUARD_CS_DRIVER_EXT_H

// Enable Board control and reset.
//#define ADAPTER_GLOBAL_BOARDCTRL_SUPPORT_ENABLE  [mtk] not support
//#define ADAPTER_GLOBAL_FPGA_HW_RESET_ENABLE      [mtk] not support

// Driver license for the module registration with the Linux kernel
#define DRIVER_LICENSE  "GPL"

/* EIP-97 FPGA board specific parameters */
//#define ADAPTER_GLOBAL_FPGA_HW_RESET_ENABLE

// FPGA DMA buffer alignment is 8 bytes
//#define DRIVER_DMA_ALIGNMENT_BYTE_COUNT     8

// Disable usage of the EIP-202 Ring Arbiter
#define DRIVER_EIP202_RA_DISABLE

#endif /* Include Guard */


/* end of file cs_driver_ext.h */
