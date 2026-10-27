/* iotoken.c
 *
 * IOToken API implementation for the EIP-97
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

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */

// Extended IOToken API
#include "iotoken_ext_97.h"


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

// Default configuration
#include "c_iotoken.h"              // IOTOKEN_STRICT_ARGS

// Driver Framework Basic Definitions API
#include "basic_defs.h"             // IDENTIFIER_NOT_USED, bool, uint32_t

// [mtk] porting, to get eip97 dma addr offset
#include "lkm.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#define IOTOKEN_ARGUMENT_ERROR      -1
#define IOTOKEN_INTERNAL_ERROR      -2

#ifdef IOTOKEN_STRICT_ARGS
#define IOTOKEN_CHECK_POINTER(_p) \
    if (NULL == (_p)) \
        return IOTOKEN_ARGUMENT_ERROR;
#define IOTOKEN_CHECK_INT_INRANGE(_i, _min, _max) \
    if ((_i) < (_min) || (_i) > (_max)) \
        return IOTOKEN_ARGUMENT_ERROR;
#define IOTOKEN_CHECK_INT_ATLEAST(_i, _min) \
    if ((_i) < (_min)) \
        return IOTOKEN_ARGUMENT_ERROR;
#define IOTOKEN_CHECK_INT_ATMOST(_i, _max) \
    if ((_i) > (_max)) \
        return IOTOKEN_ARGUMENT_ERROR;
#else
/* IOTOKEN_STRICT_ARGS undefined */
#define IOTOKEN_CHECK_POINTER(_p)
#define IOTOKEN_CHECK_INT_INRANGE(_i, _min, _max)
#define IOTOKEN_CHECK_INT_ATLEAST(_i, _min)
#define IOTOKEN_CHECK_INT_ATMOST(_i, _max)
#endif /*end of IOTOKEN_STRICT_ARGS */

// Input Token words offsets
#define IOTOKEN_HDR_IN_WORD_OFFS           0
#define IOTOKEN_APP_ID_IN_WORD_OFFS        1
#define IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS    2
#define IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS    3
#define IOTOKEN_BP_DATA_IN_WORD_OFFS       6

// Output Token words offsets
#define IOTOKEN_HDR_OUT_WORD_OFFS          0
#define IOTOKEN_BP_LEN_OUT_WORD_OFFS       1
#define IOTOKEN_APP_ID_OUT_WORD_OFFS       2
#define IOTOKEN_PAD_NH_OUT_WORD_OFFS       3
#define IOTOKEN_SA_ADDR_LO_OUT_WORD_OFFS   4
#define IOTOKEN_SA_ADDR_HI_OUT_WORD_OFFS   5

#define IOTOKEN_MARK                       0xEC00


/*----------------------------------------------------------------------------
 * Local variables
 */

#define IOTOKEN_IN_WORD_COUNT32 (IOTOKEN_IN_WORD_COUNT-1)
#define IOTOKEN_IN_WORD_COUNT64 IOTOKEN_IN_WORD_COUNT

/*----------------------------------------------------------------------------
 * IOToken_InWordCount_Get
 */
unsigned int
IOToken_InWordCount_Get(void)
{
#ifdef IOTOKEN_64BIT_DEVICE
    return IOTOKEN_IN_WORD_COUNT64;
#else
    return IOTOKEN_IN_WORD_COUNT32;
#endif
}


/*----------------------------------------------------------------------------
 * IOToken_OutWordCount_Get
 */
unsigned int
IOToken_OutWordCount_Get(void)
{
    return IOTOKEN_OUT_WORD_COUNT;
}


/*----------------------------------------------------------------------------
 * IOToken_Create
 */
int
IOToken_Create(
        const IOToken_Input_Dscr_t * const Dscr_p,
        uint32_t * Data_p)
{
    unsigned int i;
	uint32_t DmaAddrOffset = LKM_DmaAddrOffset_Get();

    IOTOKEN_CHECK_POINTER(Dscr_p);
    IOTOKEN_CHECK_POINTER(Data_p);

    // Input Token word: EIP-96 Token Header Word
    {
        i = IOTOKEN_HDR_IN_WORD_OFFS;

        // Set initialization value in the Token Header Word excluding packet
        // size field
        Data_p[i] = Dscr_p->TknHdrWordInit & ~MASK_16_BITS;

        // Input packet size
        Data_p[i] |= Dscr_p->InPacket_ByteCount & MASK_16_BITS;

        Data_p[i] |= BIT_17; // Set token header format to EIP-(1)97

#ifdef IOTOKEN_64BIT_DEVICE
        Data_p[i] |= BIT_18; // Only 64-bit Context (SA) pointer is supported
#endif
        // Enable Context Reuse auto detect if no new SA
        if (Dscr_p->Options.fReuseSA)
            Data_p[i] |= BIT_21;
    }

    // Input Token word: Application ID
    {
        i = IOTOKEN_APP_ID_IN_WORD_OFFS;
        Data_p[i] = (Dscr_p->AppID & MASK_7_BITS) << 9;
    }

	// [mtk] porting, eip97/host-cpu dma address view might be different

    // Input Token word: Context (SA) low 32 bits physical address
    //{
    //    i = IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS;
    //    Data_p[i] = Dscr_p->SA_PhysAddr.Lo;
    //}

//#ifdef IOTOKEN_64BIT_DEVICE
    // Input Token word: Context (SA) high 64 bits physical address
    //{
    //    i = IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS;
    //    Data_p[i] = Dscr_p->SA_PhysAddr.Hi;
    //}
//#endif
	if (Dscr_p->SA_PhysAddr.Lo || Dscr_p->SA_PhysAddr.Hi)
	{
#ifdef IOTOKEN_64BIT_DEVICE
		if (Dscr_p->SA_PhysAddr.Lo >= DmaAddrOffset)
		{
			Data_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Lo - DmaAddrOffset;
			Data_p[IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Hi;
		}
		else
		{
			Data_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Lo + (~DmaAddrOffset) + 1;
			Data_p[IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Hi - 1;
		}
#else
		Data_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Lo - DmaAddrOffset;
#endif
	}
	else /* invalid SA_PhysAddr */
	{
#ifdef IOTOKEN_64BIT_DEVICE
		Data_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Lo;
		Data_p[IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Hi;
#else
		Data_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = Dscr_p->SA_PhysAddr.Lo;
#endif
	}

    return IOTOKEN_IN_WORD_COUNT;
}


/*----------------------------------------------------------------------------
 * IOToken_SAAddr_Update
 */
int
IOToken_SAAddr_Update(
        const IOToken_PhysAddr_t * const SA_PhysAddr_p,
        uint32_t * InTokenData_p)
{
	uint32_t DmaAddrOffset = LKM_DmaAddrOffset_Get();

    IOTOKEN_CHECK_POINTER(SA_PhysAddr_p);
    IOTOKEN_CHECK_POINTER(InTokenData_p);

    // [mtk] porting, eip97/host-cpu dma address view might be different

	// Input Token word: Context (SA) low 32 bits physical address
    //InTokenData_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = SA_PhysAddr_p->Lo;

//#ifdef IOTOKEN_64BIT_DEVICE
    // Input Token word: Context (SA) high 32 bits physical address
    //InTokenData_p[IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS] = SA_PhysAddr_p->Hi;
//#endif
#ifdef IOTOKEN_64BIT_DEVICE
	if (SA_PhysAddr_p->Lo >= DmaAddrOffset)
	{
		InTokenData_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = SA_PhysAddr_p->Lo - DmaAddrOffset;
		InTokenData_p[IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS] = SA_PhysAddr_p->Hi;
	}
	else
	{
		InTokenData_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = SA_PhysAddr_p->Lo + (~DmaAddrOffset) + 1;
		InTokenData_p[IOTOKEN_SA_ADDR_HI_IN_WORD_OFFS] = SA_PhysAddr_p->Hi - 1;
	}
#else
	InTokenData_p[IOTOKEN_SA_ADDR_LO_IN_WORD_OFFS] = SA_PhysAddr_p->Lo - DmaAddrOffset;
#endif

    return 2; // updated 32-bit token words
}


/*----------------------------------------------------------------------------
 * IOToken_SAReuse_Update
 */
int
IOToken_SAReuse_Update(
        const bool fReuseSA,
        uint32_t * InTokenData_p)
{
    IOTOKEN_CHECK_POINTER(InTokenData_p);

    // Enable Context Reuse auto detect if no new SA
    if (fReuseSA)
        InTokenData_p[IOTOKEN_HDR_IN_WORD_OFFS] |= BIT_21;
    else
        InTokenData_p[IOTOKEN_HDR_IN_WORD_OFFS] &= ~BIT_21;

    return 1;
}


/*----------------------------------------------------------------------------
 * IOToken_Mark_Set
 */
int
IOToken_Mark_Set(
        uint32_t * InTokenData_p)
{
    IOTOKEN_CHECK_POINTER(InTokenData_p);

    InTokenData_p[IOTOKEN_APP_ID_IN_WORD_OFFS] &= ~(MASK_7_BITS << 9);
    InTokenData_p[IOTOKEN_APP_ID_IN_WORD_OFFS] |= IOTOKEN_MARK;

    return 1;
}


/*----------------------------------------------------------------------------
 * IOToken_Mark_Offset_Get
 */
int
IOToken_OutMarkOffset_Get(void)
{
    return IOTOKEN_APP_ID_OUT_WORD_OFFS;
}


/*----------------------------------------------------------------------------
 * IOToken_Mark_Check
 */
int
IOToken_Mark_Check(
        uint32_t * OutTokenData_p)
{
    uint32_t Mark;

    IOTOKEN_CHECK_POINTER(OutTokenData_p);

    Mark = OutTokenData_p[IOTOKEN_APP_ID_OUT_WORD_OFFS] & IOTOKEN_MARK;

    if (Mark == IOTOKEN_MARK)
        return 0;
    else
        return 1;
}


/*----------------------------------------------------------------------------
 * IOToken_Parse
 */
int
IOToken_Parse(
        const uint32_t * Data_p,
        IOToken_Output_Dscr_t * const Dscr_p)
{
    unsigned int i, j = 0;

    IOTOKEN_CHECK_POINTER(Data_p);
    IOTOKEN_CHECK_POINTER(Dscr_p);

    // Output Token word: EIP-96 Output Token Header Word
    {
        i = IOTOKEN_HDR_OUT_WORD_OFFS;
        Dscr_p->OutPacket_ByteCount = Data_p[i]       & MASK_17_BITS;
        Dscr_p->ErrorCode           = Data_p[i] >> 17 & MASK_15_BITS;
    }

    // Output Token word: EIP-96 Output Token Bypass Data Length Word
    {
        i = IOTOKEN_BP_LEN_OUT_WORD_OFFS;
        Dscr_p->BypassData_ByteCount =  Data_p[i] & MASK_4_BITS;
        Dscr_p->fHashAppended        = (Data_p[i] & BIT_21) != 0;

        Dscr_p->Hash_ByteCount       = Data_p[i] >> 22 & MASK_6_BITS;

        Dscr_p->fBytesAppended       = (Data_p[i] & BIT_28) != 0;
        Dscr_p->fChecksumAppended    = (Data_p[i] & BIT_29) != 0;
        Dscr_p->fNextHeaderAppended  = (Data_p[i] & BIT_30) != 0;
        Dscr_p->fLengthAppended      = (Data_p[i] & BIT_31) != 0;
    }

    // Output Token word: EIP-96 Output Token Application ID Word
    {
        i = IOTOKEN_APP_ID_OUT_WORD_OFFS;
        Dscr_p->AppID = Data_p[i] >> 9 & MASK_7_BITS;
    }

    // Output Token word: Pad Length and Next Header
    {
        i = IOTOKEN_PAD_NH_OUT_WORD_OFFS;
        Dscr_p->NextHeader    = Data_p[i]      & MASK_8_BITS;
        Dscr_p->Pad_ByteCount = Data_p[i] >> 8 & MASK_8_BITS;
    }

    return MAX(i, j);
}


/*----------------------------------------------------------------------------
 * IOToken_PacketLegth_Get
 */
int
IOToken_PacketLegth_Get(
        const uint32_t * Data_p,
        unsigned int * Pkt_ByteCount_p)
{
    IOTOKEN_CHECK_POINTER(Data_p);
    IOTOKEN_CHECK_POINTER(Pkt_ByteCount_p);

    *Pkt_ByteCount_p = Data_p[IOTOKEN_HDR_OUT_WORD_OFFS] & MASK_17_BITS;

    return 1;
}


/*----------------------------------------------------------------------------
 * IOToken_BypassLegth_Get
 */
int
IOToken_BypassLegth_Get(
        const uint32_t * Data_p,
        unsigned int * BD_ByteCount_p)
{
    IOTOKEN_CHECK_POINTER(Data_p);
    IOTOKEN_CHECK_POINTER(BD_ByteCount_p);

    *BD_ByteCount_p = Data_p[IOTOKEN_BP_LEN_OUT_WORD_OFFS] & MASK_4_BITS;

    return 1;
}


/*----------------------------------------------------------------------------
 * IOToken_ErrorCode_Get
 */
int
IOToken_ErrorCode_Get(
        const uint32_t * Data_p,
        unsigned int * ErrorCode_p)
{
    IOTOKEN_CHECK_POINTER(Data_p);
    IOTOKEN_CHECK_POINTER(ErrorCode_p);

    *ErrorCode_p = Data_p[IOTOKEN_HDR_OUT_WORD_OFFS] >> 17 & MASK_15_BITS;

    return 1;
}


/* end of file iotoken.c */
