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

#include "evey_def.h"
#include "evey_util.h"

#include <math.h>


#if ENC_DEC_TRACE
FILE *fp_trace;
#if TRACE_RDO
#if TRACE_RDO_EXCLUDE_I
int fp_trace_print = 0;
#else
int fp_trace_print = 1;
#endif
#else
int fp_trace_print = 0;
#endif
int fp_trace_counter = 0;
#endif

#if TRACE_START_POC
int fp_trace_started = 0;
#endif

int evey_atomic_inc(volatile int * pcnt)
{
    int ret;
    ret = *pcnt;
    ret++;
    *pcnt = ret;
    return ret;
}

int evey_atomic_dec(volatile int * pcnt)
{
    int ret;
    ret = *pcnt;
    ret--;
    *pcnt = ret;
    return ret;
}

static void imgb_delete(EVEY_IMGB * imgb)
{
    int i;
    evey_assert_r(imgb);

    for(i = 0; i < EVEY_IMGB_MAX_PLANE; i++)
    {
        evey_mfree(imgb->baddr[i]);
    }
    evey_mfree(imgb);
}

static int imgb_addref(EVEY_IMGB * imgb)
{
    evey_assert_rv(imgb, EVEY_ERR_INVALID_ARGUMENT);
    return evey_atomic_inc(&imgb->refcnt);
}

static int imgb_getref(EVEY_IMGB * imgb)
{
    evey_assert_rv(imgb, EVEY_ERR_INVALID_ARGUMENT);
    return imgb->refcnt;
}

static int imgb_release(EVEY_IMGB * imgb)
{
    int refcnt;
    evey_assert_rv(imgb, EVEY_ERR_INVALID_ARGUMENT);
    refcnt = evey_atomic_dec(&imgb->refcnt);
    if(refcnt == 0)
    {
        imgb_delete(imgb);
    }
    return refcnt;
}

static void imgb_cpy_shift_left_8b(EVEY_IMGB * imgb_dst, EVEY_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned char * s;
    short         * d;

    for(i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + imgb_src->s[i];
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_cpy_shift_right_8b(EVEY_IMGB *dst, EVEY_IMGB *src, int shift)
{
    int i, j, k, t0, add;

    short         *s;
    unsigned char *d;

    if(shift)
        add = 1 << (shift - 1);
    else
        add = 0;

    for(i = 0; i < 3; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->ah[i]; j++)
        {
            for(k = 0; k < src->aw[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (unsigned char)(EVEY_CLIP3(0, 255, t0));
            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = d + dst->s[i];
        }
    }
}

static void imgb_cpy_plane(EVEY_IMGB * dst, EVEY_IMGB * src)
{
    int i, j;
    unsigned char *s, *d;
    int numbyte = EVEY_CS_GET_BYTE_DEPTH(src->cs);

    for(i = 0; i < src->np; i++)
    {
        s = (unsigned char*)src->a[i];
        d = (unsigned char*)dst->a[i];

        for(j = 0; j < src->ah[i]; j++)
        {
            memcpy(d, s, numbyte * src->aw[i]);
            s += src->s[i];
            d += dst->s[i];
        }
    }
}

static void imgb_cpy_shift_left(EVEY_IMGB * dst, EVEY_IMGB * src, int shift)
{
    int i, j, k;

    unsigned short * s;
    unsigned short * d;

    for(i = 0; i < 3; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->h[i]; j++)
        {
            for(k = 0; k < src->w[i]; k++)
            {
                d[k] = (unsigned short)(s[k] << shift);
            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = (short*)(((unsigned char *)d) + dst->s[i]);
        }
    }
}

static void imgb_cpy_shift_right(EVEY_IMGB * dst, EVEY_IMGB * src, int shift)
{

    int i, j, k, t0, add;

    int clip_min = 0;
    int clip_max = 0;

    unsigned short         * s;
    unsigned short         * d;
    if(shift)
        add = 1 << (shift - 1);
    else
        add = 0;

    clip_max = (1 << (EVEY_CS_GET_BIT_DEPTH(dst->cs))) - 1;

    for(i = 0; i < 3; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->h[i]; j++)
        {
            for(k = 0; k < src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (EVEY_CLIP3(clip_min, clip_max, t0));

            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = (short*)(((unsigned char *)d) + dst->s[i]);
        }
    }
}


void evey_imgb_cpy(EVEY_IMGB * dst, EVEY_IMGB * src)
{
    int i, bd_src, bd_dst;
    bd_src = EVEY_CS_GET_BIT_DEPTH(src->cs);
    bd_dst = EVEY_CS_GET_BIT_DEPTH(dst->cs);

    if(src->cs == dst->cs)
    {
        imgb_cpy_plane(dst, src);
    }
    else if(bd_src == 8 && bd_dst > 8)
    {
        imgb_cpy_shift_left_8b(dst, src, bd_dst - bd_src);
    }
    else if(bd_src > 8 && bd_dst == 8)
    {
        imgb_cpy_shift_right_8b(dst, src, bd_src - bd_dst);
    }
    else if(bd_src < bd_dst)
    {
        imgb_cpy_shift_left(dst, src, bd_dst - bd_src);
    }
    else if(bd_src > bd_dst)
    {
        imgb_cpy_shift_right(dst, src, bd_src - bd_dst);
    }
    else
    {
        evey_trace("ERROR: unsupported image copy\n");
        return;
    }
    for(i = 0; i < EVEY_IMGB_MAX_PLANE; i++)
    {
        dst->x[i] = src->x[i];
        dst->y[i] = src->y[i];
        dst->w[i] = src->w[i];
        dst->h[i] = src->h[i];
        dst->ts[i] = src->ts[i];
    }
}

EVEY_IMGB * evey_imgb_create(int w, int h, int cs, int opt, int pad[EVEY_IMGB_MAX_PLANE], int align[EVEY_IMGB_MAX_PLANE])
{    
    EVEY_IMGB * imgb;
    int         i, p_size, a_size, bd;
    int         bit_depth;
    int         chroma_format_idc;
    int         np;

    imgb = (EVEY_IMGB *)evey_malloc(sizeof(EVEY_IMGB));
    evey_assert_rv(imgb, NULL);
    evey_mset(imgb, 0, sizeof(EVEY_IMGB));

    imgb->imgb_active_pps_id = -1;
    bit_depth = EVEY_CS_GET_BIT_DEPTH(cs);
    chroma_format_idc = CFI_FROM_CF(EVEY_CS_GET_FORMAT(cs));
    np = (chroma_format_idc == 0) ? 1 : 3;

    if(bit_depth >= 8 && bit_depth <= 14)
    {
        bd = EVEY_CS_GET_BYTE_DEPTH(cs); /* byteunit */

        for(i = 0; i < np; i++)
        {
            imgb->w[i] = w;
            imgb->h[i] = h;
            imgb->x[i] = 0;
            imgb->y[i] = 0;

            a_size = (align != NULL) ? align[i] : 0;
            p_size = (pad != NULL) ? pad[i] : 0;

            imgb->aw[i] = EVEY_ALIGN(w, a_size);
            imgb->ah[i] = EVEY_ALIGN(h, a_size);

            imgb->padl[i] = imgb->padr[i] = imgb->padu[i] = imgb->padb[i] = p_size;

            imgb->s[i] = (imgb->aw[i] + imgb->padl[i] + imgb->padr[i]) * bd;
            imgb->e[i] = imgb->ah[i] + imgb->padu[i] + imgb->padb[i];

            imgb->bsize[i] = imgb->s[i] * imgb->e[i];
            imgb->baddr[i] = evey_malloc(imgb->bsize[i]);

            imgb->a[i] = ((u8*)imgb->baddr[i]) + imgb->padu[i] * imgb->s[i] +
                imgb->padl[i] * bd;

            if(i == 0)
            {
                if(GET_CHROMA_W_SHIFT(chroma_format_idc))
                {
                    w = (w + GET_CHROMA_W_SHIFT(chroma_format_idc)) >> GET_CHROMA_W_SHIFT(chroma_format_idc);
                }
                if((GET_CHROMA_H_SHIFT(chroma_format_idc)))
                {
                    h = (h + GET_CHROMA_H_SHIFT(chroma_format_idc)) >> GET_CHROMA_H_SHIFT(chroma_format_idc);
                }
            }
        }
        imgb->np = np;
    }
    else
    {
        evey_trace("unsupported color space\n");
        evey_mfree(imgb);
        return NULL;
    }
    imgb->addref = imgb_addref;
    imgb->getref = imgb_getref;
    imgb->release = imgb_release;
    imgb->cs = cs;
    imgb->addref(imgb);

    return imgb;
}

/******************************************************************************
 * picture buffer alloc/free/expand
 ******************************************************************************/
EVEY_PIC * evey_pic_alloc(EVEY_PICBUF_ALLOCATOR * pa, int * ret)
{
    return evey_picbuf_alloc(pa->w, pa->h, pa->pad_l, pa->pad_c, pa->chroma_format_idc, pa->bit_depth, ret);
}

void evey_pic_free(EVEY_PIC * pic)
{
    evey_picbuf_free(pic);
}

void evey_pic_expand(EVEY_PIC * pic)
{
    evey_picbuf_expand(pic, pic->pad_l, pic->pad_c);
}

EVEY_PIC * evey_picbuf_alloc(int w, int h, int pad_l, int pad_c, int chroma_format_idc, int bit_depth, int * err)
{
    EVEY_PIC  * pic = NULL;
    EVEY_IMGB * imgb = NULL;
    int         ret, opt, align[EVEY_IMGB_MAX_PLANE], pad[EVEY_IMGB_MAX_PLANE];
    int         w_scu, h_scu, f_scu, size;
    int         cs;

    /* allocate PIC structure */
    pic = evey_malloc(sizeof(EVEY_PIC));
    evey_assert_gv(pic != NULL, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
    evey_mset(pic, 0, sizeof(EVEY_PIC));

    opt = EVEY_IMGB_OPT_NONE;

    /* set align value*/
    align[0] = MIN_CU_SIZE;
    align[1] = MIN_CU_SIZE >> 1;
    align[2] = MIN_CU_SIZE >> 1;

    /* set padding value*/
    pad[0] = pad_l;
    pad[1] = pad_c;
    pad[2] = pad_c;
    
    cs = EVEY_CS_SET(CF_FROM_CFI(chroma_format_idc), bit_depth, 0);

    imgb = evey_imgb_create(w, h, cs, opt, pad, align);
    imgb->cs = cs;
    evey_assert_gv(imgb != NULL, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);

    /* set EVEY_PIC */
    pic->buf_y = imgb->baddr[0];
    pic->buf_u = imgb->baddr[1];
    pic->buf_v = imgb->baddr[2];
    pic->y     = imgb->a[0];
    pic->u     = imgb->a[1];
    pic->v     = imgb->a[2];

    pic->w_l   = imgb->w[0];
    pic->h_l   = imgb->h[0];
    pic->w_c   = imgb->w[1];
    pic->h_c   = imgb->h[1];

    pic->s_l   = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c   = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->pad_l = pad_l;
    pic->pad_c = pad_c;

    pic->imgb  = imgb;

    /* allocate maps */
    w_scu = (pic->w_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    h_scu = (pic->h_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    f_scu = w_scu * h_scu;

    size = sizeof(s8) * f_scu * LIST_NUM;
    pic->map_refi = evey_malloc_fast(size);
    evey_assert_gv(pic->map_refi, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
    evey_mset_x64a(pic->map_refi, -1, size);

    size = sizeof(s16) * f_scu * LIST_NUM * MV_D;
    pic->map_mv = evey_malloc_fast(size);
    evey_assert_gv(pic->map_mv, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
    evey_mset_x64a(pic->map_mv, 0, size);

    if(err)
    {
        *err = EVEY_OK;
    }
    return pic;

ERR:
    if(pic)
    {
        evey_mfree(pic->map_mv);
        evey_mfree(pic->map_refi);
        evey_mfree(pic);
    }
    if(err) *err = ret;
    return NULL;
}

void evey_picbuf_free(EVEY_PIC *pic)
{
    EVEY_IMGB *imgb;

    if(pic)
    {
        imgb = pic->imgb;

        if(imgb)
        {
            imgb->release(imgb);

            pic->y = NULL;
            pic->u = NULL;
            pic->v = NULL;
            pic->w_l = 0;
            pic->h_l = 0;
            pic->w_c = 0;
            pic->h_c = 0;
            pic->s_l = 0;
            pic->s_c = 0;
        }
        evey_mfree(pic->map_mv);
        evey_mfree(pic->map_refi);
        evey_mfree(pic);
    }
}

static void picbuf_expand(pel * a, int s, int w, int h, int exp)
{
    int   i, j;
    pel   pixel;
    pel * src, * dst;

    /* left */
    src = a;
    dst = a - exp;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* right */
    src = a + (w - 1);
    dst = a + w;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* upper */
    src = a - exp;
    dst = a - exp - (exp * s);

    for(i = 0; i < exp; i++)
    {
        evey_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }

    /* below */
    src = a + ((h - 1)*s) - exp;
    dst = a + ((h - 1)*s) - exp + s;

    for(i = 0; i < exp; i++)
    {
        evey_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }
}

void evey_picbuf_expand(EVEY_PIC * pic, int exp_l, int exp_c)
{
    picbuf_expand(pic->y, pic->s_l, pic->w_l, pic->h_l, exp_l);
    picbuf_expand(pic->u, pic->s_c, pic->w_c, pic->h_c, exp_c);
    picbuf_expand(pic->v, pic->s_c, pic->w_c, pic->h_c, exp_c);
}

void evey_poc_derivation(EVEY_SPS * sps, int tid, EVEY_POC *poc)
{
    int sub_gop_length = (int)pow(2.0, sps->log2_sub_gop_length);
    int expected_tid = 0;
    int doc_offset, poc_offset;
    
    if (tid == 0)
    {
        poc->poc_val = poc->prev_poc_val + sub_gop_length;
        poc->prev_doc_offset = 0;
        poc->prev_poc_val = poc->poc_val;
        return;
    }
    doc_offset = (poc->prev_doc_offset + 1) % sub_gop_length;
    if (doc_offset == 0)
    {
        poc->prev_poc_val += sub_gop_length;
    }
    else
    {
        expected_tid = 1 + (int)log2(doc_offset);
    }
    while (tid != expected_tid)
    {
        doc_offset = (doc_offset + 1) % sub_gop_length;
        if (doc_offset == 0)
        {
            expected_tid = 0;
        }
        else
        {
            expected_tid = 1 + (int)log2(doc_offset);
        }
    }
    poc_offset = (int)(sub_gop_length * ((2.0 * doc_offset + 1) / (int)pow(2.0, tid) - 2));
    poc->poc_val = poc->prev_poc_val + poc_offset;
    poc->prev_doc_offset = doc_offset;
}

void evey_get_motion(void * ctx, void * core, EVEY_REF_PIC_LIST lidx, s8 refi[EVEY_MVP_NUM], s16 mvp[EVEY_MVP_NUM][MV_D])
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;

    /* mvp_idx = 0 */
    if (IS_AVAIL(c_core->avail_cu, AVAIL_LE))
    {
        refi[0] = 0;
        mvp[0][MV_X] = c_ctx->map_mv[c_core->scup - 1][lidx][MV_X];
        mvp[0][MV_Y] = c_ctx->map_mv[c_core->scup - 1][lidx][MV_Y];
    }
    else
    {
        refi[0] = 0;
        mvp[0][MV_X] = 1;
        mvp[0][MV_Y] = 1;
    }

    /* mvp_idx = 1 */
    if (IS_AVAIL(c_core->avail_cu, AVAIL_UP))
    {
        refi[1] = 0;
        mvp[1][MV_X] = c_ctx->map_mv[c_core->scup - c_ctx->w_scu][lidx][MV_X];
        mvp[1][MV_Y] = c_ctx->map_mv[c_core->scup - c_ctx->w_scu][lidx][MV_Y];
    }
    else
    {
        refi[1] = 0;
        mvp[1][MV_X] = 1;
        mvp[1][MV_Y] = 1;
    }

    /* mvp_idx = 2 */
    if (IS_AVAIL(c_core->avail_cu, AVAIL_UP_RI))
    {
        refi[2] = 0;
        mvp[2][MV_X] = c_ctx->map_mv[c_core->scup - c_ctx->w_scu + (1 << (c_core->log2_cuw - MIN_CU_LOG2))][lidx][MV_X];
        mvp[2][MV_Y] = c_ctx->map_mv[c_core->scup - c_ctx->w_scu + (1 << (c_core->log2_cuw - MIN_CU_LOG2))][lidx][MV_Y];
    }
    else
    {
        refi[2] = 0;
        mvp[2][MV_X] = 1;
        mvp[2][MV_Y] = 1;
    }

    /* mvp_idx = 3 */
    refi[3] = 0;
    mvp[3][MV_X] = c_ctx->refp[0][lidx].map_mv[c_core->scup][0][MV_X];
    mvp[3][MV_Y] = c_ctx->refp[0][lidx].map_mv[c_core->scup][0][MV_Y];
}

void evey_get_mv_dir(void * ctx, void * core, s16 mvp[LIST_NUM][MV_D])
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    EVEY_REFP * refp = c_ctx->refp[0];
    u32         poc = c_ctx->poc.poc_val;
    int         scup = c_core->scup + ((1 << (c_core->log2_cuw - MIN_CU_LOG2)) - 1) + ((1 << (c_core->log2_cuh - MIN_CU_LOG2)) - 1) * c_ctx->w_scu;
    s16         mvc[MV_D];
    int         dpoc_co, dpoc_L0, dpoc_L1;

    mvc[MV_X] = refp[LIST_1].map_mv[scup][0][MV_X];
    mvc[MV_Y] = refp[LIST_1].map_mv[scup][0][MV_Y];

    dpoc_co = refp[LIST_1].poc - refp[LIST_1].list_poc[0];
    dpoc_L0 = poc - refp[LIST_0].poc;
    dpoc_L1 = refp[LIST_1].poc - poc;

    if(dpoc_co == 0)
    {
        mvp[LIST_0][MV_X] = 0;
        mvp[LIST_0][MV_Y] = 0;
        mvp[LIST_1][MV_X] = 0;
        mvp[LIST_1][MV_Y] = 0;
    }
    else
    {
        mvp[LIST_0][MV_X] = dpoc_L0 * mvc[MV_X] / dpoc_co;
        mvp[LIST_0][MV_Y] = dpoc_L0 * mvc[MV_Y] / dpoc_co;
        mvp[LIST_1][MV_X] = -dpoc_L1 * mvc[MV_X] / dpoc_co;
        mvp[LIST_1][MV_Y] = -dpoc_L1 * mvc[MV_Y] / dpoc_co;
    }
}

u16 evey_get_avail_inter(void * ctx, void * core)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    int         scuw = 1 << (c_core->log2_cuw - MIN_CU_LOG2);
    u16         avail = 0;

    if(c_core->x_scu > 0 && !MCU_GET_IF(c_ctx->map_scu[c_core->scup - 1]) && MCU_GET_COD(c_ctx->map_scu[c_core->scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);
    }

    if(c_core->y_scu > 0)
    {
        if(!MCU_GET_IF(c_ctx->map_scu[c_core->scup - c_ctx->w_scu]))
        {
            SET_AVAIL(avail, AVAIL_UP);
        }

        if(c_core->x_scu > 0 && !MCU_GET_IF(c_ctx->map_scu[c_core->scup - c_ctx->w_scu - 1]) && MCU_GET_COD(c_ctx->map_scu[c_core->scup - c_ctx->w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }

        if(c_core->x_scu + scuw < c_ctx->w_scu  && MCU_IS_COD_NIF(c_ctx->map_scu[c_core->scup - c_ctx->w_scu + scuw]) && MCU_GET_COD(c_ctx->map_scu[c_core->scup - c_ctx->w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    return avail;
}

u16 evey_get_avail_intra(void * ctx, void * core)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    int         scuw = 1 << (c_core->log2_cuw - MIN_CU_LOG2);
    u16         avail = 0;

    if(c_core->x_scu > 0 && MCU_GET_COD(c_ctx->map_scu[c_core->scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);
    }

    if(c_core->y_scu > 0)
    {
        SET_AVAIL(avail, AVAIL_UP);


        if(c_core->x_scu > 0 && MCU_GET_COD(c_ctx->map_scu[c_core->scup - c_ctx->w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }

        if(c_core->x_scu + scuw < c_ctx->w_scu  && MCU_GET_COD(c_ctx->map_scu[c_core->scup - c_ctx->w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    return avail;
}

int evey_picbuf_signature(EVEY_PIC * pic, u8 signature[N_C][16])
{
    return evey_md5_imgb(pic->imgb, signature);
}

/* MD5 functions */
#define MD5FUNC(f, w, x, y, z, msg1, s,msg2 )  ( w += f(x, y, z) + msg1 + msg2,  w = w<<s | w>>(32-s),  w += x )
#define FF(x, y, z) (z ^ (x & (y ^ z)))
#define GG(x, y, z) (y ^ (z & (x ^ y)))
#define HH(x, y, z) (x ^ y ^ z)
#define II(x, y, z) (y ^ (x | ~z))

static void evey_md5_trans(u32 * buf, u32 * msg)
{
    register u32 a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5FUNC(FF, a, b, c, d, msg[ 0],  7, 0xd76aa478); /* 1 */
    MD5FUNC(FF, d, a, b, c, msg[ 1], 12, 0xe8c7b756); /* 2 */
    MD5FUNC(FF, c, d, a, b, msg[ 2], 17, 0x242070db); /* 3 */
    MD5FUNC(FF, b, c, d, a, msg[ 3], 22, 0xc1bdceee); /* 4 */

    MD5FUNC(FF, a, b, c, d, msg[ 4],  7, 0xf57c0faf); /* 5 */
    MD5FUNC(FF, d, a, b, c, msg[ 5], 12, 0x4787c62a); /* 6 */
    MD5FUNC(FF, c, d, a, b, msg[ 6], 17, 0xa8304613); /* 7 */
    MD5FUNC(FF, b, c, d, a, msg[ 7], 22, 0xfd469501); /* 8 */

    MD5FUNC(FF, a, b, c, d, msg[ 8],  7, 0x698098d8); /* 9 */
    MD5FUNC(FF, d, a, b, c, msg[ 9], 12, 0x8b44f7af); /* 10 */
    MD5FUNC(FF, c, d, a, b, msg[10], 17, 0xffff5bb1); /* 11 */
    MD5FUNC(FF, b, c, d, a, msg[11], 22, 0x895cd7be); /* 12 */

    MD5FUNC(FF, a, b, c, d, msg[12],  7, 0x6b901122); /* 13 */
    MD5FUNC(FF, d, a, b, c, msg[13], 12, 0xfd987193); /* 14 */
    MD5FUNC(FF, c, d, a, b, msg[14], 17, 0xa679438e); /* 15 */
    MD5FUNC(FF, b, c, d, a, msg[15], 22, 0x49b40821); /* 16 */

    /* Round 2 */
    MD5FUNC(GG, a, b, c, d, msg[ 1],  5, 0xf61e2562); /* 17 */
    MD5FUNC(GG, d, a, b, c, msg[ 6],  9, 0xc040b340); /* 18 */
    MD5FUNC(GG, c, d, a, b, msg[11], 14, 0x265e5a51); /* 19 */
    MD5FUNC(GG, b, c, d, a, msg[ 0], 20, 0xe9b6c7aa); /* 20 */

    MD5FUNC(GG, a, b, c, d, msg[ 5],  5, 0xd62f105d); /* 21 */
    MD5FUNC(GG, d, a, b, c, msg[10],  9,  0x2441453); /* 22 */
    MD5FUNC(GG, c, d, a, b, msg[15], 14, 0xd8a1e681); /* 23 */
    MD5FUNC(GG, b, c, d, a, msg[ 4], 20, 0xe7d3fbc8); /* 24 */

    MD5FUNC(GG, a, b, c, d, msg[ 9],  5, 0x21e1cde6); /* 25 */
    MD5FUNC(GG, d, a, b, c, msg[14],  9, 0xc33707d6); /* 26 */
    MD5FUNC(GG, c, d, a, b, msg[ 3], 14, 0xf4d50d87); /* 27 */
    MD5FUNC(GG, b, c, d, a, msg[ 8], 20, 0x455a14ed); /* 28 */

    MD5FUNC(GG, a, b, c, d, msg[13],  5, 0xa9e3e905); /* 29 */
    MD5FUNC(GG, d, a, b, c, msg[ 2],  9, 0xfcefa3f8); /* 30 */
    MD5FUNC(GG, c, d, a, b, msg[ 7], 14, 0x676f02d9); /* 31 */
    MD5FUNC(GG, b, c, d, a, msg[12], 20, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    MD5FUNC(HH, a, b, c, d, msg[ 5],  4, 0xfffa3942); /* 33 */
    MD5FUNC(HH, d, a, b, c, msg[ 8], 11, 0x8771f681); /* 34 */
    MD5FUNC(HH, c, d, a, b, msg[11], 16, 0x6d9d6122); /* 35 */
    MD5FUNC(HH, b, c, d, a, msg[14], 23, 0xfde5380c); /* 36 */

    MD5FUNC(HH, a, b, c, d, msg[ 1],  4, 0xa4beea44); /* 37 */
    MD5FUNC(HH, d, a, b, c, msg[ 4], 11, 0x4bdecfa9); /* 38 */
    MD5FUNC(HH, c, d, a, b, msg[ 7], 16, 0xf6bb4b60); /* 39 */
    MD5FUNC(HH, b, c, d, a, msg[10], 23, 0xbebfbc70); /* 40 */

    MD5FUNC(HH, a, b, c, d, msg[13],  4, 0x289b7ec6); /* 41 */
    MD5FUNC(HH, d, a, b, c, msg[ 0], 11, 0xeaa127fa); /* 42 */
    MD5FUNC(HH, c, d, a, b, msg[ 3], 16, 0xd4ef3085); /* 43 */
    MD5FUNC(HH, b, c, d, a, msg[ 6], 23,  0x4881d05); /* 44 */

    MD5FUNC(HH, a, b, c, d, msg[ 9],  4, 0xd9d4d039); /* 45 */
    MD5FUNC(HH, d, a, b, c, msg[12], 11, 0xe6db99e5); /* 46 */
    MD5FUNC(HH, c, d, a, b, msg[15], 16, 0x1fa27cf8); /* 47 */
    MD5FUNC(HH, b, c, d, a, msg[ 2], 23, 0xc4ac5665); /* 48 */

    /* Round 4 */
    MD5FUNC(II, a, b, c, d, msg[ 0],  6, 0xf4292244); /* 49 */
    MD5FUNC(II, d, a, b, c, msg[ 7], 10, 0x432aff97); /* 50 */
    MD5FUNC(II, c, d, a, b, msg[14], 15, 0xab9423a7); /* 51 */
    MD5FUNC(II, b, c, d, a, msg[ 5], 21, 0xfc93a039); /* 52 */

    MD5FUNC(II, a, b, c, d, msg[12],  6, 0x655b59c3); /* 53 */
    MD5FUNC(II, d, a, b, c, msg[ 3], 10, 0x8f0ccc92); /* 54 */
    MD5FUNC(II, c, d, a, b, msg[10], 15, 0xffeff47d); /* 55 */
    MD5FUNC(II, b, c, d, a, msg[ 1], 21, 0x85845dd1); /* 56 */

    MD5FUNC(II, a, b, c, d, msg[ 8],  6, 0x6fa87e4f); /* 57 */
    MD5FUNC(II, d, a, b, c, msg[15], 10, 0xfe2ce6e0); /* 58 */
    MD5FUNC(II, c, d, a, b, msg[ 6], 15, 0xa3014314); /* 59 */
    MD5FUNC(II, b, c, d, a, msg[13], 21, 0x4e0811a1); /* 60 */

    MD5FUNC(II, a, b, c, d, msg[ 4],  6, 0xf7537e82); /* 61 */
    MD5FUNC(II, d, a, b, c, msg[11], 10, 0xbd3af235); /* 62 */
    MD5FUNC(II, c, d, a, b, msg[ 2], 15, 0x2ad7d2bb); /* 63 */
    MD5FUNC(II, b, c, d, a, msg[ 9], 21, 0xeb86d391); /* 64 */

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

void evey_md5_init(EVEY_MD5 * md5)
{
    md5->h[0] = 0x67452301;
    md5->h[1] = 0xefcdab89;
    md5->h[2] = 0x98badcfe;
    md5->h[3] = 0x10325476;

    md5->bits[0] = 0;
    md5->bits[1] = 0;
}

void evey_md5_update(EVEY_MD5 * md5, void * buf_t, u32 len)
{
    u8 * buf;
    u32  i, idx, part_len;

    buf = (u8*)buf_t;

    idx = (u32)((md5->bits[0] >> 3) & 0x3f);

    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3))
    {
        (md5->bits[1])++;
    }

    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;

    if(len >= part_len)
    {
        evey_mcpy(md5->msg + idx, buf, part_len);
        evey_md5_trans(md5->h, (u32 *)md5->msg);

        for(i = part_len; i + 63 < len; i += 64)
        {
            evey_md5_trans(md5->h, (u32 *)(buf + i));
        }
        idx = 0;
    }
    else
    {
        i = 0;
    }

    if(len - i > 0)
    {
        evey_mcpy(md5->msg + idx, buf + i, len - i);
    }
}

void evey_md5_update_16(EVEY_MD5 * md5, void * buf_t, u32 len)
{
    u16 * buf;
    u32   i, idx, part_len, j;
    u8    t[512];

    buf = (u16 *)buf_t;
    idx = (u32)((md5->bits[0] >> 3) & 0x3f);

    len = len * 2;
    for(j = 0; j < len; j += 2)
    {
        t[j] = (u8)(*(buf));
        t[j + 1] = *(buf) >> 8;
        buf++;
    }

    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3))
    {
        (md5->bits[1])++;
    }

    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;

    if(len >= part_len)
    {
        evey_mcpy(md5->msg + idx, t, part_len);
        evey_md5_trans(md5->h, (u32 *)md5->msg);

        for(i = part_len; i + 63 < len; i += 64)
        {
            evey_md5_trans(md5->h, (u32 *)(t + i));
        }
        idx = 0;
    }
    else
    {
        i = 0;
    }

    if(len - i > 0)
    {
        evey_mcpy(md5->msg + idx, t + i, len - i);
    }
}

void evey_md5_finish(EVEY_MD5 * md5, u8 digest[16])
{
    u8 * pos;
    int  cnt;

    cnt = (md5->bits[0] >> 3) & 0x3F;
    pos = md5->msg + cnt;
    *pos++ = 0x80;
    cnt = 64 - 1 - cnt;

    if(cnt < 8)
    {
        evey_mset(pos, 0, cnt);
        evey_md5_trans(md5->h, (u32 *)md5->msg);
        evey_mset(md5->msg, 0, 56);
    }
    else
    {
        evey_mset(pos, 0, cnt - 8);
    }

    evey_mcpy((md5->msg + 14 * sizeof(u32)), &md5->bits[0], sizeof(u32));
    evey_mcpy((md5->msg + 15 * sizeof(u32)), &md5->bits[1], sizeof(u32));

    evey_md5_trans(md5->h, (u32 *)md5->msg);
    evey_mcpy(digest, md5->h, 16);
    evey_mset(md5, 0, sizeof(EVEY_MD5));
}

int evey_md5_imgb(EVEY_IMGB * imgb, u8 digest[N_C][16])
{
    EVEY_MD5 md5[N_C];
    int      i, j;

    for(i = 0; i < imgb->np; i++)
    {
        evey_md5_init(&md5[i]);
        
        for(j = imgb->y[i]; j < imgb->h[i]; j++)
        {
            evey_md5_update(&md5[i], ((u8 *)imgb->a[i]) + j*imgb->s[i] + imgb->x[i] , imgb->w[i] * 2);
        }

        evey_md5_finish(&md5[i], digest[i]);
    }

    return EVEY_OK;
}

void init_scan(u16 * scan, int size_x, int size_y, int scan_type)
{
    int x, y, l, pos, num_line;

    pos = 0;
    num_line = size_x + size_y - 1;

    if(scan_type == COEF_SCAN_ZIGZAG)
    {
        /* starting point */
        scan[pos] = 0;
        pos++;

        /* loop */
        for(l = 1; l < num_line; l++)
        {
            if(l % 2) /* decreasing loop */
            {
                x = EVEY_MIN(l, size_x - 1);
                y = EVEY_MAX(0, l - (size_x - 1));

                while(x >= 0 && y < size_y)
                {
                    scan[pos] = y * size_x + x;
                    pos++;
                    x--;
                    y++;
                }
            }
            else /* increasing loop */
            {
                y = EVEY_MIN(l, size_y - 1);
                x = EVEY_MAX(0, l - (size_y - 1));
                while(y >= 0 && x < size_x)
                {
                    scan[pos] = y * size_x + x;
                    pos++;
                    x++;
                    y--;
                }
            }
        }
    }
}

static void evey_init_inverse_scan_sr(u16 * scan_inv, u16 * scan_orig, int width, int height, int scan_type)
{
    int x, num_line;

    num_line = width * height;
    if(scan_type == COEF_SCAN_ZIGZAG)
    {
        for(x = 0; x < num_line; x++)
        {
            int scan_pos = scan_orig[x];
            assert(scan_pos >= 0);
            assert(scan_pos < num_line);
            scan_inv[scan_pos] = x;
        }
    }
    else
    {
        evey_assert(0);
        evey_trace("Not supported scan_type\n");
    }
}

int evey_scan_tbl_init()
{
    int x, y, scan_type;
    int size_y, size_x;

    for(scan_type = 0; scan_type < COEF_SCAN_TYPE_NUM; scan_type++)
    {
        for(y = 0; y < MAX_TR_LOG2; y++)
        {
            size_y = 1 << (y + 1);
            for(x = 0; x < MAX_TR_LOG2; x++)
            {
                size_x = 1 << (x + 1);
                evey_scan_tbl[scan_type][x][y] = (u16*)evey_malloc_fast(size_y * size_x * sizeof(u16));
                init_scan(evey_scan_tbl[scan_type][x][y], size_x, size_y, scan_type);
                evey_inv_scan_tbl[scan_type][x][y] = (u16*)evey_malloc_fast(size_y * size_x * sizeof(u16));
                evey_init_inverse_scan_sr(evey_inv_scan_tbl[scan_type][x][y], evey_scan_tbl[scan_type][x][y], size_x, size_y, scan_type);
            }
        }
    }
    return EVEY_OK;
}

int evey_scan_tbl_delete()
{
    int x, y, scan_type;

    for(scan_type = 0; scan_type < COEF_SCAN_TYPE_NUM; scan_type++)
    {
        for(y = 0; y < MAX_TR_LOG2; y++)
        {
            for(x = 0; x < MAX_TR_LOG2; x++)
            {
                evey_mfree(evey_scan_tbl[scan_type][x][y]);
                evey_mfree(evey_inv_scan_tbl[scan_type][x][y]);
            }
        }
    }
    return EVEY_OK;
}

int evey_get_split_mode(s8 * split_mode, int cud, int cup, int cuw, int cuh, int ctu_s, s8 (* split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_CTU])
{
    int ret = EVEY_OK;
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (ctu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (EVEY_CONV_LOG2(cuw) - EVEY_CONV_LOG2(cuh));

    if(cuw < 8 && cuh < 8)
    {
        *split_mode = NO_SPLIT;
        return ret;
    }

    *split_mode = split_mode_buf[cud][shape][pos];

    return ret;
}

void evey_set_split_mode(s8 split_mode, int cud, int cup, int cuw, int cuh, int ctu_s, s8 (* split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_CTU])
{
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (ctu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (EVEY_CONV_LOG2(cuw) - EVEY_CONV_LOG2(cuh));

    if(cuw >= 8 || cuh >= 8)
    {
        split_mode_buf[cud][shape][pos] = split_mode;
    }
}

/* Count of partitions, correspond to split_mode */
static int evey_split_part_count(int split_mode)
{
    switch(split_mode)
    {
        case SPLIT_QUAD:
            return 4;
        default: /* NO_SPLIT */
            return 0;
    }
}

void evey_split_get_part_structure(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, EVEY_SPLIT_STRUCT * split_struct)
{
    int i;
    int log_cuw, log_cuh;
    int cup_w, cup_h;

    split_struct->part_count = evey_split_part_count(split_mode);

    log_cuw = EVEY_CONV_LOG2(cuw);
    log_cuh = EVEY_CONV_LOG2(cuh);
    split_struct->x_pos[0] = x0;
    split_struct->y_pos[0] = y0;
    split_struct->cup[0] = cup;

    switch(split_mode)
    {
        case NO_SPLIT:
        {
            split_struct->width[0] = cuw;
            split_struct->height[0] = cuh;
            split_struct->log_cuw[0] = log_cuw;
            split_struct->log_cuh[0] = log_cuh;
        }
        break;

        case SPLIT_QUAD:
        {
            int x1 = x0 + (cuw >> 1);
            int y1 = y0 + (cuh >> 1);

            for(i = 0; i < split_struct->part_count; ++i)
            {
                split_struct->width  [i] = cuw >> 1;
                split_struct->height [i] = cuh >> 1;
                split_struct->log_cuw[i] = log_cuw - 1;
                split_struct->log_cuh[i] = log_cuh - 1;
            }

            split_struct->x_pos[1] = x1;
            split_struct->y_pos[1] = y0;
            split_struct->x_pos[2] = x0;
            split_struct->y_pos[2] = y1;
            split_struct->x_pos[3] = x1;
            split_struct->y_pos[3] = y1;

            cup_w = (split_struct->width[0] >> MIN_CU_LOG2);
            cup_h = ((split_struct->height[0] >> MIN_CU_LOG2) << log2_culine);

            split_struct->cup[1] = cup + cup_w;
            split_struct->cup[2] = cup + cup_h;
            split_struct->cup[3] = cup + cup_w + cup_h;

            split_struct->cud[0] = cud + 2;
            split_struct->cud[1] = cud + 2;
            split_struct->cud[2] = cud + 2;
            split_struct->cud[3] = cud + 2;
        }
        break;

        default:
        break;
    }
}

void evey_block_copy(s16 * src, int src_stride, s16 * dst, int dst_stride, int log2_copy_w, int log2_copy_h)
{
    int   h;
    int   copy_size = (1 << log2_copy_w) * (int)sizeof(s16);
    s16 * tmp_src = src;
    s16 * tmp_dst = dst;
    for (h = 0; h < (1<< log2_copy_h); h++)
    {
        evey_mcpy(tmp_dst, tmp_src, copy_size);
        tmp_dst += dst_stride;
        tmp_src += src_stride;
    }
}

void evey_update_core_loc_param(void * ctx, void * core)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;

    c_core->x_pel = c_core->x_ctu << c_ctx->log2_ctu_size;                 // entry point's x location in pixel
    c_core->y_pel = c_core->y_ctu << c_ctx->log2_ctu_size;                 // entry point's y location in pixel
    c_core->x_scu = c_core->x_ctu << (c_ctx->log2_ctu_size - MIN_CU_LOG2); // set x_scu location 
    c_core->y_scu = c_core->y_ctu << (c_ctx->log2_ctu_size - MIN_CU_LOG2); // set y_scu location 
    c_core->ctu_num = c_core->x_ctu + c_core->y_ctu * c_ctx->w_ctu;
}

/* cabac initialization value with probability 1/2 and mps = 0 */
#define PROB_INIT                 (512) 

void evey_eco_init_ctx_model(EVEY_SBAC_CTX * sbac_ctx)
{
    int i;

    evey_mset(sbac_ctx, 0, sizeof(EVEY_SBAC_CTX));

    for(i = 0; i < NUM_CTX_SPLIT_CU_FLAG; i++)
        sbac_ctx->split_cu_flag[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CC_RUN; i++)
        sbac_ctx->run[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CC_LAST; i++)
        sbac_ctx->last[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CC_LEVEL; i++)
        sbac_ctx->level[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_LUMA; i++)
        sbac_ctx->cbf_luma[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_CB; i++)
        sbac_ctx->cbf_cb[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_CR; i++)
        sbac_ctx->cbf_cr[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_CBF_ALL; i++)
        sbac_ctx->cbf_all[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_PRED_MODE; i++)
        sbac_ctx->pred_mode[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_DIRECT_MODE_FLAG; i++)
        sbac_ctx->direct_mode_flag[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_INTER_PRED_IDC; i++)
        sbac_ctx->inter_dir[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_INTRA_PRED_MODE; i++)
        sbac_ctx->intra_dir[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_MVP_IDX; i++)
        sbac_ctx->mvp_idx[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_MVD; i++)
        sbac_ctx->mvd[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_REF_IDX; i++)
        sbac_ctx->refi[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_DELTA_QP; i++)
        sbac_ctx->delta_qp[i] = PROB_INIT;
    for(i = 0; i < NUM_CTX_SKIP_FLAG; i++)
        sbac_ctx->skip_flag[i] = PROB_INIT;
}
