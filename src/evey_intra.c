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
#include "evey_intra.h"


static void ipred_hor(pel * src_le, pel * src_up, pel * dst, int w, int h)
{
    int i, j;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            dst[j] = src_le[0];
        }
        dst += w; src_le++;
    }
}

static void ipred_ver(pel * src_le, pel * src_up, pel * dst, int w, int h)
{
    int i;
    for(i = 0; i < h; i++)
    {
        evey_mcpy(dst, src_up, w * sizeof(pel));
        dst += w;
    }
}

static void ipred_dc(pel * src_le, pel * src_up, pel * dst, int w, int h)
{
    int dc = 0;
    int wh, i, j;

    for(i = 0; i < h; i++)
    {
        dc += src_le[i];
    }

    for(j = 0; j < w; j++)
    {
        dc += src_up[j];
    }

    dc = (dc + w) >> (evey_tbl_log2[w] + 1);

    wh = w * h;

    for(i = 0; i < wh; i++)
    {
        dst[i] = (pel)dc;
    }
}

static void ipred_ul(pel * src_le, pel * src_up, pel * dst, int w, int h)
{
    int i, j;
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            int diag = i - j;
            if(diag > 0)
            {
                dst[j] = src_le[diag - 1];
            }
            else if(diag == 0)
            {
                dst[j] = src_up[-1];
            }
            else
            {
                dst[j] = src_up[-diag - 1];
            }
        }
        dst += w;
    }
}

static void ipred_ur(pel * src_le, pel * src_up, pel * dst, int w, int h)
{
    int i, j;
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            dst[j] = (src_up[i + j + 1] + src_le[i + j + 1]) >> 1;
        }
        dst += w;
    }
}

void evey_get_nbr(void * ctx, void * core, pel * src, int s_src, EVEY_COMPONENT ch_type)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    int         cip = c_ctx->pps.constrained_intra_pred_flag;
    int         bit_depth = ch_type == Y_C ? c_ctx->sps.bit_depth_luma_minus8 + 8 : c_ctx->sps.bit_depth_chroma_minus8 + 8;
    int         i, j;
    int         scuw = 1 << (c_core->log2_cuw - MIN_CU_LOG2);
    int         scuh = 1 << (c_core->log2_cuh - MIN_CU_LOG2);
    int         unit_size = (ch_type == Y_C) ? MIN_CU_SIZE : (MIN_CU_SIZE >> 1);    
    pel       * left = c_core->nb[ch_type][0] + 1;
    pel       * up = c_core->nb[ch_type][1] + 1;
    pel       * tmp = src;

    scuh = ((ch_type != Y_C) && (c_ctx->sps.chroma_format_idc == 2)) ? scuh * 2 : scuh;
    unit_size = ((ch_type != Y_C) && (c_ctx->sps.chroma_format_idc == 3)) ? unit_size * 2 : unit_size;

    /* fill above-left sample */
    tmp = src - s_src - 1;
    if(IS_AVAIL(c_core->avail_cu, AVAIL_UP_LE) && (!cip || MCU_GET_IF(c_ctx->map_scu[c_core->scup - c_ctx->w_scu - 1])))
    {
        left[-1] = up[-1] = *tmp;
    }
    else
    {
        left[-1] = up[-1] = 1 << (bit_depth - 1);
    }

    /* fill above samples */
    tmp = src - s_src;
    for(i = 0; i < (scuw + scuh); i++)
    {
        int is_avail = (c_core->y_scu > 0) && (c_core->x_scu + i < c_ctx->w_scu);
        if(is_avail && MCU_GET_COD(c_ctx->map_scu[c_core->scup - c_ctx->w_scu + i]) && (!cip || MCU_GET_IF(c_ctx->map_scu[c_core->scup - c_ctx->w_scu + i])))
        {
            evey_mcpy(up, tmp, unit_size * sizeof(pel));
        }
        else
        {
            evey_mset_16b(up, 1 << (bit_depth - 1), unit_size);
        }
        tmp += unit_size;
        up += unit_size;
    }

    /* fill left samples */
    tmp = src - 1;
    for(i = 0; i < (scuh + scuw); ++i)
    {
        int is_avail = (c_core->x_scu > 0) && (c_core->y_scu + i < c_ctx->h_scu);
        if(is_avail && MCU_GET_COD(c_ctx->map_scu[c_core->scup - 1 + i * c_ctx->w_scu]) && (!cip || MCU_GET_IF(c_ctx->map_scu[c_core->scup - 1 + i * c_ctx->w_scu])))
        {
            for(j = 0; j < unit_size; ++j)
            {
                left[i * unit_size + j] = *tmp;
                tmp += s_src;
            }
        }
        else
        {
            evey_mset_16b(left + i * unit_size, 1 << (bit_depth - 1), unit_size);
            tmp += (s_src * unit_size);
        }
    }
}

void evey_get_nbr_yuv(void * ctx, void * core, int x, int y)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    pel       * rec;
    int         s_rec;

    /* Y */
    s_rec = c_ctx->pic->s_l;
    rec = c_ctx->pic->y + (y * s_rec) + x;
    evey_get_nbr(ctx, core, rec, s_rec, Y_C);

    if(c_ctx->sps.chroma_format_idc != 0)
    {
        x >>= (GET_CHROMA_W_SHIFT(c_ctx->sps.chroma_format_idc));
        y >>= (GET_CHROMA_H_SHIFT(c_ctx->sps.chroma_format_idc));

        s_rec = c_ctx->pic->s_c;
        /* U */
        rec = c_ctx->pic->u + (y * s_rec) + x;
        evey_get_nbr(ctx, core, rec, s_rec, U_C);
        /* V */
        rec = c_ctx->pic->v + (y * s_rec) + x;
        evey_get_nbr(ctx, core, rec, s_rec, V_C);
    }
}

void evey_get_mpm(void * ctx, void * core)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    u8          ipm_l = IPD_DC;
    u8          ipm_u = IPD_DC;

    if(c_core->x_scu > 0 && MCU_GET_IF(c_ctx->map_scu[c_core->scup - 1]) && MCU_GET_COD(c_ctx->map_scu[c_core->scup - 1]))
    {
        ipm_l = c_ctx->map_ipm[c_core->scup - 1] + 1;
    }
    if(c_core->y_scu > 0 && MCU_GET_IF(c_ctx->map_scu[c_core->scup - c_ctx->w_scu]) && MCU_GET_COD(c_ctx->map_scu[c_core->scup - c_ctx->w_scu]))
    {
        ipm_u = c_ctx->map_ipm[c_core->scup - c_ctx->w_scu] + 1;
    }
    c_core->mpm_b_list = (u8*)&evey_tbl_mpm[ipm_l][ipm_u];
}


void evey_intra_pred(void * ctx, void * core, pel * dst, int ipm)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    pel       * src_le = c_core->nb[0][0] + 1;
    pel       * src_up = c_core->nb[0][1] + 1;
    int         w = 1 << c_core->log2_cuw;
    int         h = 1 << c_core->log2_cuh;

    switch(ipm)
    {
        case IPD_DC:
            ipred_dc(src_le, src_up, dst, w, h);
            break;
        case IPD_HOR:
            ipred_hor(src_le, src_up, dst, w, h);
        break; 
        case IPD_VER:
            ipred_ver(src_le, src_up, dst, w, h);
            break;                
        case IPD_UL:
            ipred_ul(src_le, src_up, dst, w, h);
            break;
        case IPD_UR:
            ipred_ur(src_le, src_up, dst, w, h);
            break;
        default:
            evey_assert(0);
            evey_trace("\n illegal intra prediction mode\n");            
            break;
    }
}

void evey_intra_pred_uv(void * ctx, void * core, pel * dst, int ipm_c, EVEY_COMPONENT ch)
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    pel       * src_le = c_core->nb[ch][0] + 1;
    pel       * src_up = c_core->nb[ch][1] + 1;
    int         w = 1 << (c_core->log2_cuw - GET_CHROMA_W_SHIFT(c_ctx->sps.chroma_format_idc));
    int         h = 1 << (c_core->log2_cuh - GET_CHROMA_H_SHIFT(c_ctx->sps.chroma_format_idc));

    switch(ipm_c)
    {
        case IPD_DC:
            ipred_dc(src_le, src_up, dst, w, h);
            break;
        case IPD_HOR:
            ipred_hor(src_le, src_up, dst, w, h);
            break;
        case IPD_VER:
            ipred_ver(src_le, src_up, dst, w, h);
            break;
        case IPD_UL:
            ipred_ul(src_le, src_up, dst, w, h);
            break;
        case IPD_UR:
            ipred_ur(src_le, src_up, dst, w, h);
            break;
        default:
            evey_assert(0);
            evey_trace("\n illegal chroma intra prediction mode\n");            
            break;
    }
}
