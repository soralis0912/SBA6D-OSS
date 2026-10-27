/* iotoken_ext_97.h
 *
 * IOToken API for Extended Server
 *
 * All the functions in this API are re-entrant for different tokens.
 * All the functions in this API can be used concurrently for different tokens.
 *
 */

/*****************************************************************************
* Copyright (c) 2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef IOTOKEN_EXT_97_H_
#define IOTOKEN_EXT_97_H_

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Standard IOToken API
#include "iotoken.h"

/*----------------------------------------------------------------------------
 * Definitions and macros
 */
#define IOTOKEN_MAX_BYPASS_WORD_COUNT 0

// Length of the EIP-197 Server extended command token (in 32-bit words)
// Note: Based on 64-bit Context (SA) address only!
#define IOTOKEN_IN_WORD_COUNT        (4 + IOTOKEN_MAX_BYPASS_WORD_COUNT)

// Length of the EIP-197 Server extended result token (in 32-bit words)
#define IOTOKEN_OUT_WORD_COUNT       (4 + IOTOKEN_MAX_BYPASS_WORD_COUNT)

#define IOTOKEN_FLOW_TYPE            0

// Input Token descriptor extension
typedef int
IOToken_Input_Dscr_Ext_t;

// Output Token descriptor extension
typedef int
IOToken_Output_Dscr_Ext_t;


/* end of file iotoken_ext_97.h */


#endif /* IOTOKEN_EXT_97_H_ */
