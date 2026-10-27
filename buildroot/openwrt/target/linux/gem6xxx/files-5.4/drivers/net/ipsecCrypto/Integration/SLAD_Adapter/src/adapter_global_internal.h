/* adapter_global_internal.h
 *
 * Internal interface between parts of the Global Control adapter.
 */

/*****************************************************************************
* Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef INCLUDE_GUARD_ADAPTER_GLOBAL_INTERNAL_H
#define INCLUDE_GUARD_ADAPTER_GLOBAL_INTERNAL_H

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */


/*----------------------------------------------------------------------------
 * Adapter_Global_Interfaces_Get
 *
 * Read the number of available packet interfaces.
 *
 * NofRings_p (output)
 *    Number of available ring pairs.
 * NofLAInterfaces_p (output)
 *    Number of available Look-Aside FIFO interfaces.
 * NofInlineInterfaces_p (output)
 *    Number of available Inline interfaces.
 *
 * If all returned values are zero, the device has not been initialized.
 * This is considered an error.
 */
void
Adapter_Global_Interfaces_Get(
    unsigned int * const NofRings_p,
    unsigned int * const NofLAInterfaces_p,
    unsigned int * const NofInlineInterfaces_p);


#endif /* Include Guard */


/* end of file adapter_global_internal.h */
