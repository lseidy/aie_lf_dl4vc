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
#include "evey_recon.h"


void evey_recon(s16 * resi, pel * pred, int nnz, int cuw, int cuh, int s_rec, pel * rec, int bit_depth)
{
    int i, j;
    s16 t0;

    if(nnz == 0) /* just copy pred to rec */
    {
        for(i = 0; i < cuh; i++)
        {
            for(j = 0; j < cuw; j++)
            {
                rec[i * s_rec + j] = EVEY_CLIP3(0, (1 << bit_depth) - 1, pred[i * cuw + j]);
            }
        }
    }
    else  /* add b/w pred and coef and copy it into rec */
    {
        for(i = 0; i < cuh; i++)
        {
            for(j = 0; j < cuw; j++)
            {
                t0 = resi[i * cuw + j] + pred[i * cuw + j];
                rec[i * s_rec + j] = EVEY_CLIP3(0, (1 << bit_depth) - 1, t0);
            }
        }
    }
}

void evey_recon_yuv(void * ctx, void * core, int x, int y, int nnz[N_C])
{
    EVEY_CTX  * c_ctx = (EVEY_CTX*)ctx;
    EVEY_CORE * c_core = (EVEY_CORE*)core;
    int         cuw = 1 << c_core->log2_cuw;
    int         cuh = 1 << c_core->log2_cuh;
    s16      (* resi)[MAX_CU_DIM] = c_core->coef;
    pel      (* pred)[MAX_CU_DIM] = c_core->pred[0];
    EVEY_PIC  * pic = c_ctx->pic;
    int         bit_depth_l = c_ctx->sps.bit_depth_luma_minus8 + 8;
    int         bit_depth_c = c_ctx->sps.bit_depth_chroma_minus8 + 8;
    int         chroma_format_idc = c_ctx->sps.chroma_format_idc;
    pel       * rec;
    int         s_rec, off;

    /* Y */
    s_rec = pic->s_l;
    rec = pic->y + (y * s_rec) + x;
    evey_recon(resi[Y_C], pred[Y_C], nnz[Y_C], cuw, cuh, s_rec, rec, bit_depth_l);
    
    if(chroma_format_idc != 0)
    {
        /* chroma */
        cuw >>= (GET_CHROMA_W_SHIFT(chroma_format_idc));
        cuh >>= (GET_CHROMA_H_SHIFT(chroma_format_idc));
        off = (x >> (GET_CHROMA_W_SHIFT(chroma_format_idc))) + (y >> (GET_CHROMA_H_SHIFT(chroma_format_idc))) * pic->s_c;

        evey_recon(resi[U_C], pred[U_C], nnz[U_C], cuw, cuh, pic->s_c, pic->u + off, bit_depth_c);
        evey_recon(resi[V_C], pred[V_C], nnz[V_C], cuw, cuh, pic->s_c, pic->v + off, bit_depth_c);
    }
}
