/* cs_iotoken.h
 *
 * Top-level configuration file
 */

/*****************************************************************************
* Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef CS_IOTOKEN_H_
#define CS_IOTOKEN_H_

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */


/*----------------------------------------------------------------------------
 * Definitions and macros
 */
#ifdef DRIVER_64BIT_DEVICE
#define IOTOKEN_64BIT_DEVICE
#endif

// Enable strict argument checking (input parameters)
#define IOTOKEN_STRICT_ARGS

// Number of 32-bit words passed from Input to Output Token
// without modifications
//#define IOTOKEN_BYPASS_WORD_COUNT        4


/* end of file cs_iotoken.h */


#endif /* CS_IOTOKEN_H_ */
