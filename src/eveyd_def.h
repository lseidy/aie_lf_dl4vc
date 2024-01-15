/* Copyright (c) 2020, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   
   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
   
   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _EVEYD_DEF_H_
#define _EVEYD_DEF_H_

#include "evey_def.h"
#include "eveyd_bsr.h"

/* evey decoder magic code */
#define EVEYD_MAGIC_CODE          0x45565944 /* EVYD */

/*****************************************************************************
 * SBAC structure
 *****************************************************************************/
typedef struct _EVEYD_SBAC
{
    u32                     range;
    u32                     value;
    EVEY_SBAC_CTX           ctx;

} EVEYD_SBAC;

/*****************************************************************************
 * CORE information used for decoding process.
 *
 * The variables in this structure are very often used in decoding process.
 *****************************************************************************/
typedef struct _EVEYD_CORE
{
    EVEY_CORE; /* should be first */

    /* reference index for current CU */
    s8                      refi[LIST_NUM];
    /* motion vector for current CU */
    s16                     mv[LIST_NUM][MV_D];
    /* motion vector predictio index for current CU */
    int                     mvp_idx[LIST_NUM];
    /* motion vector difference for current CU */
    s16                     mvd[LIST_NUM][MV_D];
    /* inter prediction indicator for current CU */
    int                     inter_dir;
    /* coefficient map for current CTU */
    s16                     map_coef[N_C][MAX_CU_DIM];
    /* mvd map for current CTU */
    s16                     map_mvd[MAX_CU_CNT_IN_CTU][LIST_NUM][MV_D];
    /* mvp index map for current CTU */
    int                     map_mvp_idx[MAX_CU_CNT_IN_CTU][LIST_NUM];
    /* inter_dir map for current CTU */
    int                     map_inter_dir[MAX_CU_CNT_IN_CTU];

#if TRACE_ENC_CU_DATA
    u64                     trace_idx;
#endif

    /* platform specific data, if needed */
    void                  * pf;

} EVEYD_CORE;

/******************************************************************************
 * CONTEXT used for decoding process.
 *
 * All have to be stored are in this structure.
 *****************************************************************************/
typedef struct _EVEYD_CTX EVEYD_CTX;
struct _EVEYD_CTX
{
    EVEY_CTX; /* should be first */

    /* EVEYD identifier */
    EVEYD                   id;
    /* create descriptor */
    EVEYD_CDSC              cdsc;    
    /* CORE information used for fast operation */
    EVEYD_CORE            * core;
    /* SBAC */
    EVEYD_SBAC              sbac_dec;
    /* current decoding bitstream */
    EVEYD_BSR               bs;
    /* bitstream has an error? */
    u8                      bs_err;    
    /* flag for picture signature enabling */
    u8                      use_pic_sign;
    /* picture signature (MD5 digest 128bits) for each component */
    u8                      pic_sign[N_C][16];
    /* flag to indicate picture signature existing or not */
    u8                      pic_sign_exist;
    /* flag to indicate opl decoder output */
    u8                      use_opl;
    
    /* address of ready function */
    int  (* fn_ready)(EVEYD_CTX * ctx);
    /* address of flush function */
    void (* fn_flush)(EVEYD_CTX * ctx);
    /* function address of decoding input bitstream */
    int  (* fn_dec_cnk)(EVEYD_CTX * ctx, EVEY_BITB * bitb, EVEYD_STAT * stat);
    /* function address of decoding slice */
    int  (* fn_dec_slice)(EVEYD_CTX * ctx, EVEYD_CORE * core);
    /* function address of pulling decoded picture */
    int  (* fn_pull)(EVEYD_CTX * ctx, EVEY_IMGB ** img, EVEYD_OPL * opl);
    /* function address of deblocking filter */
    int  (* fn_deblock)(void * ctx);
    
    /* platform specific data, if needed */
    void                  * pf;
};

#include "eveyd_util.h"
#include "eveyd_eco.h"
#include "evey_picman.h"

#endif /* _EVEYD_DEF_H_ */
