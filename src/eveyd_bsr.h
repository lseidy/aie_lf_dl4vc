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

#ifndef _EVEYD_BSR_H_
#define _EVEYD_BSR_H_

#include "eveyd_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _EVEYD_BSR EVEYD_BSR;

/*! Function pointer for */
typedef int (*EVEYD_BSR_FN_FLUSH)(EVEYD_BSR *bs, int byte);

/*!
 * bitstream structure for decoder.
 *
 * NOTE: Don't change location of variable because this variable is used
 *       for assembly coding!
 */
struct _EVEYD_BSR
{
    /* temporary read code buffer */
    u32                 code;
    /* left bits count in code */
    int                 leftbits;
    /*! address of current bitstream position */
    u8                * cur;
    /*! address of bitstream end */
    u8                * end;
    /*! address of bitstream begin */
    u8                * beg;
    /*! size of original bitstream in byte */
    int                 size;
    /*! Function pointer for bs_flush */
    EVEYD_BSR_FN_FLUSH  fn_flush;
    /*! arbitrary data, if needs */
    int                 ndata[4];
    /*! arbitrary address, if needs */
    void              * pdata[4];
};

/* get number of byte consumed */
#define EVEYD_BSR_GET_READ_BYTE(bs) ((int)((bs)->cur - (bs)->beg) - ((bs)->leftbits >> 3))
/*! Is bitstream byte aligned? */
#define EVEYD_BSR_IS_BYTE_ALIGN(bs) ((((bs)->leftbits & 0x7) == 0) ? 1: 0)

void eveyd_bsr_init(EVEYD_BSR * bs, u8 * buf, int size, EVEYD_BSR_FN_FLUSH fn_flush);
#if TRACE_HLS
#define eveyd_bsr_read(A, B, C) eveyd_bsr_read_trace(A, B, #B, C)
void eveyd_bsr_read_trace(EVEYD_BSR * bs, u32 * val, char * name, int size);

#define eveyd_bsr_read1(A, B) eveyd_bsr_read1_trace(A, B, #B)
void eveyd_bsr_read1_trace(EVEYD_BSR * bs, u32 * val, char * name);

#define eveyd_bsr_read_ue(A, B) eveyd_bsr_read_ue_trace(A, B, #B)
void eveyd_bsr_read_ue_trace(EVEYD_BSR * bs, u32 * val, char * name);

#define eveyd_bsr_read_se(A, B) eveyd_bsr_read_se_trace(A, B, #B)
void eveyd_bsr_read_se_trace(EVEYD_BSR * bs, s32 * val, char * name);
#else
void eveyd_bsr_read(EVEYD_BSR * bs, u32 * val, int size);
void eveyd_bsr_read1(EVEYD_BSR * bs, u32 * val);
void eveyd_bsr_read_ue(EVEYD_BSR * bs, u32 * val);
void eveyd_bsr_read_se(EVEYD_BSR * bs, s32 * val);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EVEYD_BSR_H_ */
