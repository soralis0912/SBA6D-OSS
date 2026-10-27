/* iotoken.h
 *
 * IO Token API
 *
 * Standard interface for the EIP-96 Crypto Engine.
 *
 * This interface specifies features common to all implementations
 * of the IOToken API and its extensions.
 *
 * All the functions in this API are re-entrant for different tokens.
 * All the functions in this API can be used concurrently for different tokens.
 *
 */

/*****************************************************************************
* Copyright (c) 2016 INSIDE Secure B.V. All Rights Reserved.
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

#ifndef IOTOKEN_H_
#define IOTOKEN_H_


/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */



/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Driver Framework Basic Definitions API
#include "basic_defs.h"                // uint32_t


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

// Physical address
typedef struct
{
    uint32_t Lo; // Low 32 bits of the 64-bit address
    uint32_t Hi; // High 32 bits of the 64-bit address, set to 0 if not used
} IOToken_PhysAddr_t;

// Standard EIP-96 Input Token header word options
typedef struct
{
    // true - Use Context (SA) buffer 64-bit addresses, otherwise 32-bit
    bool f64bitSA;

    // true - there have been already packets processed for this SA
    bool fReuseSA;

} IOToken_Options_t;

// Standard EIP-96 Input Token descriptor
typedef struct
{
    // Input packet length (in bytes)
    unsigned int InPacket_ByteCount;

    // Initialization value for the Token Header Word
    unsigned int TknHdrWordInit;

    IOToken_Options_t Options;

    // Packet flow type
    unsigned int Type;

    // Application-specific identifier that will be passed to Output Token
    unsigned int AppID;

    // Context record (SA) physical address
    IOToken_PhysAddr_t SA_PhysAddr;

    // Length (in 32-bit words) of optional bypass data
    unsigned int BypassData_WordCount;

    // Input Token extension
    // Optional pointer to the token descriptor extension.
    // This must be a pointer to the data structure of type
    // IOToken_Input_Dscr_Ext_t. Set to NULL if not used.
    void * Ext_p;

} IOToken_Input_Dscr_t;

// Standard EIP-96 Output Token descriptor
typedef struct
{
    // Output packet length (in bytes)
    unsigned int OutPacket_ByteCount;

    // EIP-96 and EIP-202 packet processing error codes
    unsigned int ErrorCode;

    // Bypass data length (in bytes)
    unsigned int BypassData_ByteCount;

    // true - Hash byte(s) is/are appended at the end of packet data
    bool fHashAppended;

    // If fHashAppended is true then Hash_ByteCount contains a number of
    // appended bytes
    unsigned int Hash_ByteCount;

    // true - Generic Bytes are appended at the end of packet data
    bool fBytesAppended;

    // true - Checksum field is appended at the end of packet data
    bool fChecksumAppended;

    // true - Next Header field is appended at the end of packet data
    bool fNextHeaderAppended;

    // true - Length field is appended at the end of packet data
    bool fLengthAppended;

    // Application-specific identifier that will be passed to Output Token
    unsigned int AppID;

    // Next Header result value retrieved from the IPsec trailer
    // Only for packets with IPsec padding
    unsigned int NextHeader;

    // Number of detected (and removed) padding bytes
    // Only for Inbound processing PKCS#7, RTP, IPSec, TLS and SSL
    unsigned int Pad_ByteCount;

    // Output Token descriptor extension
    // Optional pointer to the Output Token descriptor extension where
    // the parsed data from the token will be stored.
    // This must be a pointer to the data structure of type
    // IOToken_Output_Dscr_Ext_t. Set to NULL if not used.
    void * Ext_p;

} IOToken_Output_Dscr_t;

// Note: IOTOKEN_IN_WORD_COUNT and IOTOKEN_OUT_WORD_COUNT are
//       implementation-specific parameters and hence must be defined by
//       the implementation instead of this generic IOToken interface.
//
//       The IOToken_InWordCount_Get() and IOToken_OutWordCount_Get()
//       functions can also be used to obtain these values.
//
//       These values are constant per implementation and do not change
//       at run-time.


/*----------------------------------------------------------------------------
 * Local variables
 */


/*----------------------------------------------------------------------------
 * IOToken_InWordCount_Get
 *
 * This function returns the size (in 32-bit words) of the Input Token.
 *
 * Return value
 *     >0  : Input Token size (IOTOKEN_IN_WORD_COUNT)
 *
 */
unsigned int
IOToken_InWordCount_Get(void);


/*----------------------------------------------------------------------------
 * IOToken_OutWordCount_Get
 *
 * This function returns the size (in 32-bit words) of the Output Token.
 *
 * Return value
 *     >0  : Output Token size (IOTOKEN_OUT_WORD_COUNT)
 *
 */
unsigned int
IOToken_OutWordCount_Get(void);


/*----------------------------------------------------------------------------
 * IOToken_Create
 *
 * This function creates an Input Token.
 *
 * Dscr_p (input)
 *     Pointer to the Input Token descriptor.
 *
 * Data_p (output)
 *     Pointer to the memory location where the Input Token data will be stored.
 *     The size of this buffer must be IOTOKEN_IN_WORD_COUNT 32-bit words
 *     at least.
 *     Each word in this buffer will be written using values from Token_p,
 *     not specified (reserved) fields will be set to 0.
 *
 * Return value
 *     <0  : Error, token not created
 *      0  : Reserved
 *     >0  : Success, number of created 32-bit Input Token words
 *
 */
int
IOToken_Create(
        const IOToken_Input_Dscr_t * const Dscr_p,
        uint32_t * Data_p);


/*----------------------------------------------------------------------------
 * IOToken_SAAddr_Update
 *
 * This function updates the SA physical address in
 * an already existing Input Token.
 *
 * SA_PhysAddr_p (input)
 *     Pointer to the SA physical address.
 *
 * InTokenData_p (output)
 *     Pointer to the memory location where the Input Token data will be updated.
 *     The size of this buffer must be IOTOKEN_IN_WORD_COUNT 32-bit words
 *     at least.
 *
 * Return value
 *     <0  : Error, token not updated
 *      0  : Reserved
 *     >0  : Success, number of updated 32-bit token words
 *
 */
int
IOToken_SAAddr_Update(
        const IOToken_PhysAddr_t * const SA_PhysAddr_p,
        uint32_t * InTokenData_p);


/*----------------------------------------------------------------------------
 * IOToken_SAReuse_Update
 *
 * This function updates the SA reuse flag in
 * an already existing Input Token.
 *
 * fReuseSA (input)
 *     True - enable Context Reuse auto detect, otherwise disable.
 *
 * InTokenData_p (output)
 *     Pointer to the memory location where the Input Token data will be updated.
 *     The size of this buffer must be IOTOKEN_IN_WORD_COUNT 32-bit words
 *     at least.
 *
 * Return value
 *     <0  : Error, token not updated
 *      0  : Reserved
 *     >0  : Success, number of updated 32-bit token words
 *
 */
int
IOToken_SAReuse_Update(
        const bool fReuseSA,
        uint32_t * InTokenData_p);


/*----------------------------------------------------------------------------
 * IOToken_Mark_Set
 *
 * This function sets a mark in an already existing Input Token. This mark will
 * be passed from the Input Token to the Output Token where its presence can
 * be checked with the IOToken_Mark_Check() function.
 *
 * The function implementation determines which field in the Input Token can
 * be used for this.
 *
 * InTokenData_p (output)
 *     Pointer to the memory location where the Input Token data will be updated.
 *     The size of this buffer must be IOTOKEN_IN_WORD_COUNT 32-bit words
 *     at least.
 *
 * Return value
 *     <0  : Error, token not updated
 *      0  : Reserved
 *     >0  : Success, number of updated 32-bit token words
 *
 */
int
IOToken_Mark_Set(
        uint32_t * InTokenData_p);


/*----------------------------------------------------------------------------
 * IOToken_Mark_Offset_Get
 *
 * This function returns a word offset in the output token where the mark
 * is stored.
 *
 * Return value
 *     <0  : Error
 *      0  : Reserved
 *     >0  : Success, offset of the 32-bit word in the output token where
 *           the mark is stored
 *
 */
int
IOToken_OutMarkOffset_Get(void);


/*----------------------------------------------------------------------------
 * IOToken_Parse
 *
 * This function parses an Output Token.
 *
 * Data_p (input)
 *     Pointer to the memory location where the Output Token data will be stored.
 *     The size of this buffer must be IOTOKEN_OUT_WORD_COUNT 32-bit words
 *     at least.
 *
 * Dscr_p (output)
 *     Pointer to the Output Token descriptor where the parsed data from
 *     the token will be stored.
 *     Fields in this descriptor will be written using values from Data_p.
 *
 * Return value
 *     <0  : Error, token not created
 *      0  : Reserved
 *     >0  : Success, number of parsed 32-bit Output Token words
 *
 */
int
IOToken_Parse(
        const uint32_t * Data_p,
        IOToken_Output_Dscr_t * const Dscr_p);


/*----------------------------------------------------------------------------
 * IOToken_Mark_Check
 *
 * This function checks if the mark set by the IOToken_Mark_Set() function
 * in the Input Token is also present in the corresponding Output Token.
 *
 * OutTokenData_p (input)
 *     Pointer to the memory location where the Output Token data is stored
 *     where the mark must be checked.
 *     The size of this buffer must be IOTOKEN_OUT_WORD_COUNT 32-bit words
 *     at least.
 *
 * Return value
 *     <0  : Error, token not checked
 *      0  : Mark found in Output Token
 *     >0  : Mark not found in Output Token,
 *           number of checked 32-bit token words
 */
int
IOToken_Mark_Check(
        uint32_t * OutTokenData_p);


/*----------------------------------------------------------------------------
 * IOToken_PacketLegth_Get
 *
 * This function reads the packet size (in bytes) from
 * an already existing Output Token.
 *
 * Data_p (input)
 *     Pointer to the memory location where the Output Token data is stored.
 *     The size of this buffer must be IOTOKEN_OUT_WORD_COUNT 32-bit words
 *     at least.
 *
 * Pkt_ByteCount_p (output)
 *     Pointer to the memory location where in packet size will be stored.
 *
 * Return value
 *     <0  : Error, packet size not read
 *      0  : Reserved
 *     >0  : Success, number of read 32-bit token words
 *
 */
int
IOToken_PacketLegth_Get(
        const uint32_t * Data_p,
        unsigned int * Pkt_ByteCount_p);


/*----------------------------------------------------------------------------
 * IOToken_BypassLegth_Get
 *
 * This function reads the bypass data length (in 32-bit words) from
 * an already existing Output Token.
 *
 * Data_p (input)
 *     Pointer to the memory location where the Output Token data is stored.
 *     The size of this buffer must be IOTOKEN_OUT_WORD_COUNT 32-bit words
 *     at least.
 *
 * BD_ByteCount_p (output)
 *     Pointer to the memory location where in bypass data size will be stored.
 *
 * Return value
 *     <0  : Error, packet size not read
 *      0  : Reserved
 *     >0  : Success, number of read 32-bit token words
 *
 */
int
IOToken_BypassLegth_Get(
        const uint32_t * Data_p,
        unsigned int * BD_ByteCount_p);


/*----------------------------------------------------------------------------
 * IOToken_ErrorCode_Get
 *
 * This function reads the packet processing error codes if any from
 * an already existing Output Token.
 *
 * Data_p (input)
 *     Pointer to the memory location where the Output Token data is stored.
 *     The size of this buffer must be IOTOKEN_OUT_WORD_COUNT 32-bit words
 *     at least.
 *
 * ErrorCode_p (output)
 *     Pointer to the memory location where packet processing error code
 *     will be stored.
 *
 * Return value
 *     <0  : Error, packet processing error code not read
 *      0  : Reserved
 *     >0  : Success, number of read 32-bit token words
 *
 */
int
IOToken_ErrorCode_Get(
        const uint32_t * Data_p,
        unsigned int * ErrorCode_p);


/* end of file iotoken.h */


#endif /* IOTOKEN_H_ */
