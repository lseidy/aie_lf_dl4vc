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

#include "eveyd_def.h"
#include "eveyd_util.h"


int eveyd_picbuf_check_signature(EVEYD_CTX * ctx, int do_compare)
{
    EVEY_PIC * pic = ctx->pic;
    u8      (* signature)[16] = ctx->pic_sign;
    u16        width = ctx->w;
    u16        height = ctx->h;
    u8         pic_sign[N_C][16] = {{0}};
    int        ret;

    /* execute MD5 digest here */
    ret = evey_picbuf_signature(pic, pic_sign);
    evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

    if(do_compare)
    {
        if(evey_mcmp(signature, pic_sign[0], N_C * 16) != 0)
        {
            return EVEY_ERR_BAD_CRC;
        }
    }

    evey_mcpy(pic->digest, pic_sign, N_C * 16);

    return EVEY_OK;
}

void eveyd_set_mv_info(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y)
{
    s8  (* map_refi)[LIST_NUM];
    s16 (* map_mv)[LIST_NUM][MV_D];
    int    w_cu;
    int    h_cu;
    int    i, j;

    w_cu = (1 << core->log2_cuw) >> MIN_CU_LOG2;
    h_cu = (1 << core->log2_cuh) >> MIN_CU_LOG2;
    map_refi = ctx->map_refi + core->scup;
    map_mv = ctx->map_mv + core->scup;

    for(i = 0; i < h_cu; i++)
    {
        for(j = 0; j < w_cu; j++)
        {
            evey_mcpy(&map_refi[j], core->refi, sizeof(core->refi)); /* ref_idx can be updated in CU decoding process */
            evey_mcpy(&map_mv[j], core->mv, sizeof(core->mv));
        }
        map_refi += ctx->w_scu;
        map_mv += ctx->w_scu;
    }
}

void eveyd_set_dec_info(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y
#if ENC_DEC_TRACE
                        , u8 write_trace
#endif
)
{
    u32  * map_scu;
    s8   * map_ipm;
    u8   * map_pred_mode;
    s8  (* map_refi)[LIST_NUM];
    s16 (* map_mvd)[LIST_NUM][MV_D];
    int (* map_mvp_idx)[LIST_NUM];
    int  * map_inter_dir;
    int    w_cu;
    int    h_cu;
    int    cup;
    int    i, j;
    int    flag;
    int    w_ctu_in_scu;

    w_cu = (1 << core->log2_cuw) >> MIN_CU_LOG2;
    h_cu = (1 << core->log2_cuh) >> MIN_CU_LOG2;
    map_scu  = ctx->map_scu + core->scup;
    map_ipm  = ctx->map_ipm + core->scup;
    map_pred_mode = ctx->map_pred_mode + core->scup;
    map_refi = ctx->map_refi + core->scup;
    w_ctu_in_scu = PEL2SCU(ctx->ctu_size);
    cup = PEL2SCU(x - core->x_pel) + PEL2SCU(y - core->y_pel) * w_ctu_in_scu;
    map_mvd = core->map_mvd + cup;
    map_mvp_idx = core->map_mvp_idx + cup;
    map_inter_dir = core->map_inter_dir + cup;

    flag = (core->pred_mode == MODE_INTRA) ? 1 : 0;

    for(i = 0; i < h_cu; i++)
    {
        for(j = 0; j < w_cu; j++)
        {
            int sub_idx = ((!!(i & 32)) << 1) | (!!(j & 32));
            if (core->nnz_sub[Y_C][sub_idx])
            {
                MCU_SET_CBFL(map_scu[j]);
            }
            else
            {
                MCU_CLR_CBFL(map_scu[j]);
            }
            if(core->nnz_sub[U_C][sub_idx])
            {
                MCU_SET_CBFCB(map_scu[j]);
            }
            else
            {
                MCU_CLR_CBFCB(map_scu[j]);
            }
            if(core->nnz_sub[V_C][sub_idx])
            {
                MCU_SET_CBFCR(map_scu[j]);
            }
            else
            {
                MCU_CLR_CBFCR(map_scu[j]);
            }

            if(ctx->pps.cu_qp_delta_enabled_flag)
            {
                MCU_RESET_QP(map_scu[j]);
                MCU_SET_IF_COD_SN_QP(map_scu[j], flag, ctx->slice_num, core->qp);
            }
            else
            {
                MCU_SET_IF_COD_SN_QP(map_scu[j], flag, ctx->slice_num, ctx->sh.qp);
            }

            map_pred_mode[j] = core->pred_mode;

            evey_mcpy(&map_refi[j], core->refi, sizeof(core->refi));
            evey_mcpy(&map_mvd[j], core->mvd, sizeof(core->mvd));
            evey_mcpy(&map_mvp_idx[j], core->mvp_idx, sizeof(core->mvp_idx));
            map_inter_dir[j] = core->inter_dir;
            map_ipm[j] = core->ipm[0];
        }

        map_scu += ctx->w_scu;
        map_ipm += ctx->w_scu;
        map_pred_mode += ctx->w_scu;
        map_refi += ctx->w_scu;
        map_mvd += w_ctu_in_scu;
        map_mvp_idx += w_ctu_in_scu;
        map_inter_dir += w_ctu_in_scu;
    }

    int cuw = 1 << core->log2_cuw;
    int cuh = 1 << core->log2_cuh;
    int ctuw = ctx->ctu_size;
    int cuw_c = cuw >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int cuh_c = cuh >> (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));
    int ctuw_c = ctuw >> (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));
    int cu_offset = (x - core->x_pel) + (y - core->y_pel) * ctuw;
    int cu_c_offset = ((x - core->x_pel) >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc))) +
                      ((y - core->y_pel) >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc))) * ctuw_c;

    for(i = 0; i < cuh; i++)
    {
        evey_mcpy(core->map_coef[Y_C] + cu_offset + i * ctuw, core->coef[Y_C] + i * cuw, sizeof(s16) * cuw);
    }
    for(i = 0; i < cuh_c; i++)
    {
        evey_mcpy(core->map_coef[U_C] + cu_c_offset + i * ctuw_c, core->coef[U_C] + i * cuw_c, sizeof(s16) * cuw_c);
        evey_mcpy(core->map_coef[V_C] + cu_c_offset + i * ctuw_c, core->coef[V_C] + i * cuw_c, sizeof(s16) * cuw_c);
    }

#if MVF_TRACE
    // Trace MVF in decoder
#if ENC_DEC_TRACE
    if (write_trace)
#endif
    {
        map_refi = ctx->map_refi + core->scup;
        map_scu = ctx->map_scu + core->scup;
        map_mv = ctx->map_mv + core->scup;

        for(i = 0; i < h_cu; i++)
        {
            for (j = 0; j < w_cu; j++)
            {
                EVEY_TRACE_COUNTER;
                EVEY_TRACE_STR(" x: ");
                EVEY_TRACE_INT(j);
                EVEY_TRACE_STR(" y: ");
                EVEY_TRACE_INT(i);

                EVEY_TRACE_STR(" ref0: ");
                EVEY_TRACE_INT(map_refi[j][LIST_0]);
                EVEY_TRACE_STR(" mv: ");
                EVEY_TRACE_MV(map_mv[j][LIST_0][MV_X], map_mv[j][LIST_0][MV_Y]);

                EVEY_TRACE_STR(" ref1: ");
                EVEY_TRACE_INT(map_refi[j][LIST_1]);
                EVEY_TRACE_STR(" mv: ");
                EVEY_TRACE_MV(map_mv[j][LIST_1][MV_X], map_mv[j][LIST_1][MV_Y]);

                EVEY_TRACE_STR("\n");
            }
            map_refi += ctx->w_scu;
            map_mv += ctx->w_scu;
            map_scu += ctx->w_scu;
        }
    }
#endif
}

void eveyd_get_dec_info(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y)
{
    u32  * map_scu;
    s8   * map_ipm;
    u8   * map_pred_mode;
    s8  (* map_refi)[LIST_NUM];
    s16 (* map_mvd)[LIST_NUM][MV_D];
    int (* map_mvp_idx)[LIST_NUM];
    int  * map_inter_dir;
    int    w_cu;
    int    h_cu;
    int    cup;
    int    i, j;    
    int    cuw = 1 << core->log2_cuw;
    int    cuh = 1 << core->log2_cuh;
    int    ctuw = ctx->ctu_size;
    int    cuw_c = cuw >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int    cuh_c = cuh >> (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));
    int    ctuw_c = ctuw >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int    cu_offset = (x - core->x_pel) + (y - core->y_pel) * ctuw;
    int    cu_c_offset = ((x - core->x_pel) >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc))) +
                         ((y - core->y_pel) >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc))) * ctuw_c;

    cup = PEL2SCU(x - core->x_pel) + PEL2SCU(y - core->y_pel) * PEL2SCU(ctx->ctu_size);
    w_cu = (1 << core->log2_cuw) >> MIN_CU_LOG2;
    h_cu = (1 << core->log2_cuh) >> MIN_CU_LOG2;

    map_scu = ctx->map_scu + core->scup;

    for(i = 0; i < h_cu; i++)
    {
        for(j = 0; j < w_cu; j++)
        {
            MCU_SET_COD(map_scu[j]);
        }
        map_scu += ctx->w_scu;
    }

    for(i = 0; i < cuh; i++)
    {
        evey_mcpy(core->coef[Y_C] + i * cuw, core->map_coef[Y_C] + cu_offset + i * ctuw, sizeof(s16) * cuw);
    }
    for(i = 0; i < cuh_c; i++)
    {
        evey_mcpy(core->coef[U_C] + i * cuw_c, core->map_coef[U_C] + cu_c_offset + i * ctuw_c, sizeof(s16) * cuw_c);
        evey_mcpy(core->coef[V_C] + i * cuw_c, core->map_coef[V_C] + cu_c_offset + i * ctuw_c, sizeof(s16) * cuw_c);
    }

    map_pred_mode = ctx->map_pred_mode + core->scup;
    map_refi = ctx->map_refi + core->scup;
    map_ipm = ctx->map_ipm + core->scup;
    map_scu = ctx->map_scu + core->scup;
    map_mvd = core->map_mvd + cup;
    map_mvp_idx = core->map_mvp_idx + cup;
    map_inter_dir = core->map_inter_dir + cup;

    evey_mcpy(core->refi, map_refi, sizeof(core->refi));
    evey_mcpy(core->mvd, map_mvd, sizeof(core->mvd));
    evey_mcpy(core->mvp_idx, map_mvp_idx, sizeof(core->mvp_idx));
    core->inter_dir = *map_inter_dir;
    core->pred_mode = *map_pred_mode;
    core->ipm[0] = *map_ipm;
    core->ipm[1] = core->ipm[0];
    core->qp = MCU_GET_QP(*map_scu);
    core->nnz_sub[Y_C][0] = MCU_GET_CBFL (*map_scu);
    core->nnz_sub[U_C][0] = MCU_GET_CBFCB(*map_scu);
    core->nnz_sub[V_C][0] = MCU_GET_CBFCR(*map_scu);
    if(cuw > MAX_TR_SIZE)
    {
        core->nnz_sub[Y_C][1] = MCU_GET_CBFL (*(map_scu + w_cu / 2));
        core->nnz_sub[U_C][1] = MCU_GET_CBFCB(*(map_scu + w_cu / 2));
        core->nnz_sub[V_C][1] = MCU_GET_CBFCR(*(map_scu + w_cu / 2));
    }
    else
    {
        core->nnz_sub[Y_C][1] = 0;
        core->nnz_sub[U_C][1] = 0;
        core->nnz_sub[V_C][1] = 0;
    }
    if(cuh > MAX_TR_SIZE)
    {
        core->nnz_sub[Y_C][2] = MCU_GET_CBFL (*(map_scu + h_cu / 2));
        core->nnz_sub[U_C][2] = MCU_GET_CBFCB(*(map_scu + h_cu / 2));
        core->nnz_sub[V_C][2] = MCU_GET_CBFCR(*(map_scu + h_cu / 2));
    }
    else
    {
        core->nnz_sub[Y_C][2] = 0;
        core->nnz_sub[U_C][2] = 0;
        core->nnz_sub[V_C][2] = 0;
    }
    if(cuw > MAX_TR_SIZE && cuh > MAX_TR_SIZE)
    {
        core->nnz_sub[Y_C][3] = MCU_GET_CBFL (*(map_scu + h_cu / 2 + w_cu / 2));
        core->nnz_sub[U_C][3] = MCU_GET_CBFCB(*(map_scu + h_cu / 2 + w_cu / 2));
        core->nnz_sub[V_C][3] = MCU_GET_CBFCR(*(map_scu + h_cu / 2 + w_cu / 2));
    }
    else
    {
        core->nnz_sub[Y_C][3] = 0;
        core->nnz_sub[U_C][3] = 0;
        core->nnz_sub[V_C][3] = 0;
    }

    core->nnz[Y_C] = 0;
    core->nnz[U_C] = 0;
    core->nnz[V_C] = 0;
    for(i = 0; i < 4; i++)
    {
        core->nnz[Y_C] |= core->nnz_sub[Y_C][i];
        core->nnz[U_C] |= core->nnz_sub[U_C][i];
        core->nnz[V_C] |= core->nnz_sub[V_C][i];
    }
}

#if USE_DRAW_PARTITION_DEC
void cpy_pic(EVEY_PIC * pic_src, EVEY_PIC * pic_dst)
{
    int i, aw, ah, s, e, bsize;

    int a_size, p_size;
    for (i = 0; i<3; i++)
    {

        a_size = MIN_CU_SIZE >> (!!i);
        p_size = i ? pic_dst->pad_c : pic_dst->pad_l;

        aw = EVEY_ALIGN(pic_dst->w_l >> (!!i), a_size);
        ah = EVEY_ALIGN(pic_dst->h_l >> (!!i), a_size);

        s = aw + p_size + p_size;
        e = ah + p_size + p_size;

        bsize = s * ah * sizeof(pel);
        switch (i)
        {
        case 0:
            evey_mcpy(pic_dst->y, pic_src->y, bsize); break;
        case 1:
            evey_mcpy(pic_dst->u, pic_src->u, bsize); break;
        case 2:
            evey_mcpy(pic_dst->v, pic_src->v, bsize); break;
        default:
            break;
        }
    }
}

int write_pic(char * fname, EVEY_PIC * pic)
{
    pel    * p;
    int      j;
    FILE   * fp;
    static int cnt = 0;

    if (cnt == 0)
        fp = fopen(fname, "wb");
    else
        fp = fopen(fname, "ab");
    cnt++;
    if (fp == NULL)
    {
        assert(!"cannot open file");
        return -1;
    }

    {
        /* Crop image supported */
        /* luma */
        p = pic->y;
        for (j = 0; j<pic->h_l; j++)
        {
            fwrite(p, pic->w_l, sizeof(pel), fp);
            p += pic->s_l;
        }

        /* chroma */
        p = pic->u;
        for (j = 0; j<pic->h_c; j++)
        {
            fwrite(p, pic->w_c, sizeof(pel), fp);
            p += pic->s_c;
        }

        p = pic->v;
        for (j = 0; j<pic->h_c; j++)
        {
            fwrite(p, pic->w_c, sizeof(pel), fp);
            p += pic->s_c;
        }
    }

    fclose(fp);
    return 0;
}

static int draw_tree(EVEYD_CTX * ctx, EVEY_PIC * pic, int x, int y,
                     int cuw, int cuh, int cud, int cup, int next_split)
{
    s8      split_mode;
    int     cup_x1, cup_y1;
    int     x1, y1, ctu_num;
    int     dx, dy, cup1, cup2, cup3;

    ctu_num = (x >> ctx->log2_ctu_size) + (y >> ctx->log2_ctu_size) * ctx->w_ctu;
    evey_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->ctu_size, ctx->map_split[ctu_num]);

    if (split_mode != NO_SPLIT && !(cuw == 4 && cuh == 4))
    {
        if (split_mode == SPLIT_QUAD)
        {
            cud += 2;
            cuw >>= 1;
            cuh >>= 1;
            x1 = x + cuw;
            y1 = y + cuh;
            dx = (cuw >> MIN_CU_LOG2);
            dy = (dx * (ctx->ctu_size >> MIN_CU_LOG2));

            cup1 = cup + dx;
            cup2 = cup + dy;
            cup3 = cup + dx + dy;

            draw_tree(ctx, pic, x, y, cuw, cuh, cud, cup, next_split - 1);
            if (x1 < pic->w_l)
            {
                draw_tree(ctx, pic, x1, y, cuw, cuh, cud, cup1, next_split - 1);
            }
            if (y1 < pic->h_l)
            {
                draw_tree(ctx, pic, x, y1, cuw, cuh, cud, cup2, next_split - 1);
            }
            if (x1 < pic->w_l && y1 < pic->h_l)
            {
                draw_tree(ctx, pic, x1, y1, cuw, cuh, cud, cup3, next_split - 1);
            }
        }
    }
    else
    {
        int     i, s_l;
        s16 * luma;
        /* draw rectangle */
        s_l = pic->s_l;
        luma = pic->y + (y * s_l) + x;

        for (i = 0; i<cuw; i++) luma[i] = sizeof(pel) << 7;
        for (i = 0; i<cuh; i++) luma[i*s_l] = sizeof(pel) << 7;
    }

    return EVEY_OK;
}

void eveyd_draw_partition(EVEYD_CTX * ctx, EVEY_PIC * pic)
{
    int i, j, k, cuw, cuh, s_l;
    s16 * luma;

    EVEY_PIC * tmp;
    int * ret = NULL;
    char file_name[256];

    tmp = evey_picbuf_alloc(ctx->w, ctx->h, pic->pad_l, pic->pad_c, ctx->param.chroma_format_idc, ctx->param.bit_depth, ret);

    cpy_pic(pic, tmp);

    /* CU partition line */
    for (i = 0; i<ctx->h_ctu; i++)
    {
        for (j = 0; j<ctx->w_ctu; j++)
        {
            draw_tree(ctx, tmp, (j << ctx->log2_ctu_size), (i << ctx->log2_ctu_size), ctx->ctu_size, ctx->ctu_size, 0, 0, 255);
        }
    }

    s_l = tmp->s_l;
    luma = tmp->y;

    /* CTU boundary line */
    for (i = 0; i<ctx->h; i += ctx->ctu_size)
    {
        for (j = 0; j<ctx->w; j += ctx->ctu_size)
        {
            cuw = j + ctx->ctu_size > ctx->w ? ctx->w - j : ctx->ctu_size;
            cuh = i + ctx->ctu_size > ctx->h ? ctx->h - i : ctx->ctu_size;
            
            for (k = 0; k<cuw; k++)
            {
                luma[i*s_l + j + k] = 0;
            }
                
            for (k = 0; k < cuh; k++)
            {
                luma[(i + k)*s_l + j] = 0;
            }
        }
    }

    sprintf(file_name, "dec_partition_%dx%d.yuv", pic->w_l, pic->h_l);

    write_pic(file_name, tmp);
    evey_picbuf_free(tmp);
}
#endif
