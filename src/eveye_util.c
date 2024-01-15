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

#include "eveye_def.h"
#include "evey_util.h"
#include "eveye_util.h"


/******************************************************************************
 * implementation of bitstream writer
 ******************************************************************************/
void eveye_bsw_skip_slice_size(EVEYE_BSW *bs)
{
    eveye_bsw_write(bs, 0, 32);
}

int eveye_bsw_write_nalu_size(EVEYE_BSW *bs)
{
    u32 size;
    size = EVEYE_BSW_GET_WRITE_BYTE(bs) - 4;    
    bs->beg[0] = size & 0x000000ff;
    bs->beg[1] = (size & 0x0000ff00) >> 8;
    bs->beg[2] = (size & 0x00ff0000) >> 16;
    bs->beg[3] = (size & 0xff000000) >> 24;
    return size;
}

/* DIFF **********************************************************************/
static void diff_16b(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 * s1;
    s16 * s2;

    int   i, j;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            diff[j] = (s16)s1[j] - (s16)s2[j];
        }
        diff += s_diff;
        s1 += s_src1;
        s2 += s_src2;
    }
}

#if X86_SSE
#define SSE_DIFF_16B_4PEL(src1, src2, diff, m00, m01, m02) \
    m00 = _mm_loadl_epi64((__m128i*)(src1)); \
    m01 = _mm_loadl_epi64((__m128i*)(src2)); \
    m02 = _mm_sub_epi16(m00, m01); \
    _mm_storel_epi64((__m128i*)(diff), m02);

#define SSE_DIFF_16B_8PEL(src1, src2, diff, m00, m01, m02) \
    m00 = _mm_loadu_si128((__m128i*)(src1)); \
    m01 = _mm_loadu_si128((__m128i*)(src2)); \
    m02 = _mm_sub_epi16(m00, m01); \
    _mm_storeu_si128((__m128i*)(diff), m02);

static void diff_16b_sse_4x2(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 * s1;
    s16 * s2;
    __m128i m01, m02, m03, m04, m05, m06;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    SSE_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    SSE_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
}

static void diff_16b_sse_4x4(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 * s1;
    s16 * s2;
    __m128i m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    SSE_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    SSE_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    SSE_DIFF_16B_4PEL(s1 + s_src1*2, s2 + s_src2*2, diff + s_diff*2, m07, m08, m09);
    SSE_DIFF_16B_4PEL(s1 + s_src1*3, s2 + s_src2*3, diff + s_diff*3, m10, m11, m12);
}

static void diff_16b_sse_8x8(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 * s1;
    s16 * s2;
    __m128i m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
    SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    SSE_DIFF_16B_8PEL(s1 + s_src1*2, s2 + s_src2*2, diff + s_diff*2, m07, m08, m09);
    SSE_DIFF_16B_8PEL(s1 + s_src1*3, s2 + s_src2*3, diff + s_diff*3, m10, m11, m12);
    SSE_DIFF_16B_8PEL(s1 + s_src1*4, s2 + s_src2*4, diff + s_diff*4, m01, m02, m03);
    SSE_DIFF_16B_8PEL(s1 + s_src1*5, s2 + s_src2*5, diff + s_diff*5, m04, m05, m06);
    SSE_DIFF_16B_8PEL(s1 + s_src1*6, s2 + s_src2*6, diff + s_diff*6, m07, m08, m09);
    SSE_DIFF_16B_8PEL(s1 + s_src1*7, s2 + s_src2*7, diff + s_diff*7, m10, m11, m12);
}

static void diff_16b_sse_8nx2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 * s1;
    s16 * s2;
    int i, j;
    __m128i m01, m02, m03, m04, m05, m06;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < h >> 1; i++)
    {
        for(j = 0; j < (w >> 3); j++)
        {
            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8; s2 += 8; diff += 8;
        }

        s1   += ((s_src1<<1) - ((w>>3)<<3));
        s2   += ((s_src2<<1) - ((w>>3)<<3));
        diff += ((s_diff<<1) - ((w>>3)<<3));
    }
}

static void diff_16b_sse_16nx2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 * s1;
    s16 * s2;
    int i, j;
    __m128i m01, m02, m03, m04, m05, m06;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < h>>1; i++)
    {
        for(j = 0; j < (w >> 4); j++)
        {
            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8; s2 += 8; diff += 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8; s2 += 8; diff += 8;
        }

        s2   += ((s_src2<<1) - ((w>>4)<<4));
        s1   += ((s_src1<<1) - ((w>>4)<<4));
        diff += ((s_diff<<1) - ((w>>4)<<4));
    }
}

static void diff_16b_sse_32nx4n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 * s1;
    s16 * s2;
    int i, j;
    __m128i m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < (h>>2); i++)
    {
        for(j = 0; j < (w>>5); j++)
        {
            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1+s_src1, s2+s_src2, diff+s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1+s_src1*2, s2+s_src2*2, diff+s_diff*2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1+s_src1*3, s2+s_src2*3, diff+s_diff*3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1+s_src1, s2+s_src2, diff+s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1+s_src1*2, s2+s_src2*2, diff+s_diff*2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1+s_src1*3, s2+s_src2*3, diff+s_diff*3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1+s_src1, s2+s_src2, diff+s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1+s_src1*2, s2+s_src2*2, diff+s_diff*2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1+s_src1*3, s2+s_src2*3, diff+s_diff*3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1+s_src1, s2+s_src2, diff+s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1+s_src1*2, s2+s_src2*2, diff+s_diff*2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1+s_src1*3, s2+s_src2*3, diff+s_diff*3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;
        }

        s1   += ((s_src1<<2) - ((w>>5)<<5));
        s2   += ((s_src2<<2) - ((w>>5)<<5));
        diff += ((s_diff<<2) - ((w>>5)<<5));
    }
}
#endif /* X86_SSE */

const EVEYE_FN_DIFF eveye_tbl_diff_16b[8][8] =
{
#if X86_SSE
    /* width == 1 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        diff_16b,          /* height == 1 */
        diff_16b_sse_4x2,  /* height == 2 */
        diff_16b_sse_4x4,  /* height == 4 */
        diff_16b,          /* height == 8 */
        diff_16b,          /* height == 16 */
        diff_16b,          /* height == 32 */
        diff_16b,          /* height == 64 */
        diff_16b,          /* height == 128 */
    },
    /* width == 8 */
    {
        diff_16b,           /* height == 1 */
        diff_16b_sse_8nx2n, /* height == 2 */
        diff_16b_sse_8nx2n, /* height == 4 */
        diff_16b_sse_8x8,   /* height == 8 */
        diff_16b_sse_8nx2n, /* height == 16 */
        diff_16b_sse_8nx2n, /* height == 32 */
        diff_16b_sse_8nx2n, /* height == 64 */
        diff_16b_sse_8nx2n, /* height == 128 */
    },
    /* width == 16 */
    {
        diff_16b,            /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_16nx2n, /* height == 4 */
        diff_16b_sse_16nx2n, /* height == 8 */
        diff_16b_sse_16nx2n, /* height == 16 */
        diff_16b_sse_16nx2n, /* height == 32 */
        diff_16b_sse_16nx2n, /* height == 64 */
        diff_16b_sse_16nx2n, /* height == 128 */
    },
    /* width == 32 */
    {
        diff_16b,            /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_32nx4n, /* height == 4 */
        diff_16b_sse_32nx4n, /* height == 8 */
        diff_16b_sse_32nx4n, /* height == 16 */
        diff_16b_sse_32nx4n, /* height == 32 */
        diff_16b_sse_32nx4n, /* height == 64 */
        diff_16b_sse_32nx4n, /* height == 128 */
    },
    /* width == 64 */
    {
        diff_16b,            /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_32nx4n, /* height == 4 */
        diff_16b_sse_32nx4n, /* height == 8 */
        diff_16b_sse_32nx4n, /* height == 16 */
        diff_16b_sse_32nx4n, /* height == 32 */
        diff_16b_sse_32nx4n, /* height == 64 */
        diff_16b_sse_32nx4n, /* height == 128 */
    },
    /* width == 128 */
    {
        diff_16b,            /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_32nx4n, /* height == 4 */
        diff_16b_sse_32nx4n, /* height == 8 */
        diff_16b_sse_32nx4n, /* height == 16 */
        diff_16b_sse_32nx4n, /* height == 32 */
        diff_16b_sse_32nx4n, /* height == 64 */
        diff_16b_sse_32nx4n, /* height == 128 */
    }
#else
    /* width == 1 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 16 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 32 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 64 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 128 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    }
#endif
};

void eveye_diff_pred(EVEYE_CTX * ctx, int log2_cuw, int log2_cuh, pel org[N_C][MAX_CU_DIM], pel pred[N_C][MAX_CU_DIM], s16 diff[N_C][MAX_CU_DIM])
{
    int cuw = 1 << log2_cuw;
    int cuh = 1 << log2_cuh;

    /* Y */
    eveye_diff_16b(log2_cuw, log2_cuh, org[Y_C], pred[Y_C], cuw, cuw, cuw, diff[Y_C], ctx->sps.bit_depth_luma_minus8 + 8);

    if(ctx->sps.chroma_format_idc != 0)
    {
        cuw >>= (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
        cuh >>= (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));
        log2_cuw -= (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
        log2_cuh -= (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));

        /* U */
        eveye_diff_16b(log2_cuw, log2_cuh, org[U_C], pred[U_C], cuw, cuw, cuw, diff[U_C], ctx->sps.bit_depth_chroma_minus8 + 8);

        /* V */
        eveye_diff_16b(log2_cuw, log2_cuh, org[V_C], pred[V_C], cuw, cuw, cuw, diff[V_C], ctx->sps.bit_depth_chroma_minus8 + 8);
    }
}

void eveye_set_qp(EVEYE_CTX *ctx, EVEYE_CORE *core, u8 qp)
{
    u8 qp_i_cb, qp_i_cr;
    core->qp = qp;
    core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
    qp_i_cb = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
    qp_i_cr = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
    core->qp_u = p_evey_tbl_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps.bit_depth_chroma_minus8;
    core->qp_v = p_evey_tbl_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps.bit_depth_chroma_minus8;
}
