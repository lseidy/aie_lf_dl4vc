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
#include "evey_lf.h"
#include "eveye_mode.h"
#include <math.h>


static s32 entropy_bits[1024];

void eveye_sbac_bit_reset(EVEYE_SBAC * sbac)
{
    sbac->code           &= 0x7FFFF;
    sbac->code_bits       = 11;
    sbac->pending_byte    = 0;
    sbac->is_pending_byte = 0;
    sbac->stacked_ff      = 0;
    sbac->stacked_zero    = 0;
    sbac->bit_counter     = 0;
    sbac->bin_counter     = 0;
}

u32 eveye_get_bit_number(EVEYE_SBAC *sbac)
{
    return sbac->bit_counter + 8 * (sbac->stacked_zero + sbac->stacked_ff) + 8 * (sbac->is_pending_byte ? 1 : 0) + 8 - sbac->code_bits + 3;
}

void eveye_rdo_bit_cnt_mvp(EVEYE_CTX * ctx, EVEYE_CORE * core, s8 refi[LIST_NUM], s16 mvd[LIST_NUM][MV_D], int pidx, int mvp_idx)
{
    s32 slice_type = ctx->sh.slice_type;
    int refi0, refi1;

    if(pidx != PRED_DIR)
    {
        refi0 = refi[LIST_0];
        refi1 = refi[LIST_1];
        if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
        {
            eveye_eco_mvp_idx(&core->bs_temp, mvp_idx);
            eveye_eco_mvd(&core->bs_temp, mvd[LIST_0]);
        }
        if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
        {
            eveye_eco_mvp_idx(&core->bs_temp, mvp_idx);
            eveye_eco_mvd(&core->bs_temp, mvd[LIST_1]);
        }
    }
}

void eveye_rdo_bit_cnt_cu_intra_luma(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM])
{
    EVEYE_SBAC * sbac = &core->s_temp_run;
    int        * nnz = core->nnz;
    s32          slice_type = ctx->sh.slice_type;

    if(slice_type != SLICE_I)
    {
        eveye_sbac_encode_bin(0, sbac, core->s_temp_run.ctx.skip_flag, &core->bs_temp); /* cu_skip_flag */
        eveye_eco_pred_mode(&core->bs_temp, MODE_INTRA, 0);
    }

    eveye_eco_intra_dir(&core->bs_temp, core->ipm[0], core->mpm_b_list);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        ctx->sh.qp_prev_eco = core->dqp_temp_run.prev_qp;
    }

    eveye_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTRA, core->nnz_sub, 0, RUN_L, -1, core->qp);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.prev_qp = ctx->sh.qp_prev_eco;
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void eveye_rdo_bit_cnt_cu_intra_chroma(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM])
{
    EVEYE_SBAC * sbac = &core->s_temp_run;
    int        * nnz = core->nnz;

    eveye_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTRA, core->nnz_sub, 0, RUN_CB | RUN_CR, -1, 0);
}

void eveye_rdo_bit_cnt_cu_intra(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM])
{
    EVEYE_SBAC * sbac = &core->s_temp_run;
    int        * nnz = core->nnz;
    s32          slice_type = ctx->sh.slice_type;
    s32          cup = core->scup;

    if(slice_type != SLICE_I)
    {
        eveye_sbac_encode_bin(0, sbac, core->s_temp_run.ctx.skip_flag, &core->bs_temp); /* cu_skip_flag */
        eveye_eco_pred_mode(&core->bs_temp, MODE_INTRA, 0);
    }

    eveye_eco_intra_dir(&core->bs_temp, core->ipm[0], core->mpm_b_list);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        ctx->sh.qp_prev_eco = core->dqp_temp_run.prev_qp;
    }

    eveye_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTRA, core->nnz_sub, 0, RUN_L | RUN_CB | RUN_CR, ctx->pps.cu_qp_delta_enabled_flag ? 1 : 0, core->qp);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.prev_qp = ctx->sh.qp_prev_eco;
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void eveye_rdo_bit_cnt_cu_inter_comp(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int ch_type, int pidx)
{
    int        * nnz = core->nnz;
    EVEYE_SBAC * sbac = &core->s_temp_run;
    int          b_no_cbf = 0;

    if(ch_type == Y_C)
    {
        eveye_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, core->nnz_sub, b_no_cbf, RUN_L, 0, core->qp);
    }

    if(ch_type == U_C)
    {
        eveye_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, core->nnz_sub, b_no_cbf, RUN_CB, -1, 0);
    }

    if(ch_type == V_C)
    {
        eveye_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, core->nnz_sub, b_no_cbf, RUN_CR, -1, 0);
    }
}

void eveye_rdo_bit_cnt_cu_inter(EVEYE_CTX * ctx, EVEYE_CORE * core, s8 refi[LIST_NUM], s16 mvd[LIST_NUM][MV_D], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 * mvp_idx)
{    
    EVEYE_PINTER * pi = &ctx->pinter;
    int            refi0, refi1;
    int            b_no_cbf = 0;
    s32            slice_type = ctx->sh.slice_type;
    s32            cup = core->scup;

    if(slice_type != SLICE_I)
    {
        eveye_sbac_encode_bin(0, &core->s_temp_run, core->s_temp_run.ctx.skip_flag, &core->bs_temp); /* cu_skip_flag */
        eveye_eco_pred_mode(&core->bs_temp, MODE_INTER, 0);
        if(slice_type == SLICE_B)
        {
            eveye_eco_direct_mode_flag(&core->bs_temp, pidx == PRED_DIR);
        }

        if(pidx != PRED_DIR) 
        {
            if (slice_type == SLICE_B)
            {
                eveye_eco_inter_pred_idc(&core->bs_temp, refi, slice_type);
            }

            refi0 = refi[LIST_0];
            refi1 = refi[LIST_1];
            if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
            {
                eveye_eco_refi(&core->bs_temp, ctx->dpbm.num_refp[LIST_0], refi0);
                eveye_eco_mvp_idx(&core->bs_temp, mvp_idx[LIST_0]);
                eveye_eco_mvd(&core->bs_temp, mvd[LIST_0]);
            }
            if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
            {
                eveye_eco_refi(&core->bs_temp, ctx->dpbm.num_refp[LIST_1], refi1);
                eveye_eco_mvp_idx(&core->bs_temp, mvp_idx[LIST_1]);
                eveye_eco_mvd(&core->bs_temp, mvd[LIST_1]);
            }
        }
    }

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        ctx->sh.qp_prev_eco = core->dqp_temp_run.prev_qp;
    }

    eveye_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, core->nnz_sub, b_no_cbf, RUN_L | RUN_CB | RUN_CR, ctx->pps.cu_qp_delta_enabled_flag ? 1 : 0, core->qp);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.prev_qp = ctx->sh.qp_prev_eco;
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void eveye_rdo_bit_cnt_cu_skip(EVEYE_CTX * ctx, EVEYE_CORE * core, int mvp_idx0, int mvp_idx1, int c_num)
{
    if(ctx->sh.slice_type != SLICE_I)
    {
        eveye_sbac_encode_bin(1, &core->s_temp_run, core->s_temp_run.ctx.skip_flag, &core->bs_temp); /* cu_skip_flag */
        eveye_eco_mvp_idx(&core->bs_temp, mvp_idx0);
        if(ctx->sh.slice_type == SLICE_B)
        {
            eveye_eco_mvp_idx(&core->bs_temp, mvp_idx1);
        }
    }
}

void eveye_init_bits_est()
{
    int    i = 0;
    double p;
    for(i = 0; i < 1024; i++)
    {
        p = (512 * (i + 0.5)) / 1024;
        entropy_bits[i] = (s32)(-32768 * (log(p) / log(2.0) - 9));
    }
}

static s32 biari_no_bits(int symbol, SBAC_CTX_MODEL* cm)
{
    u16 mps, state;
    mps = (*cm) & 1;
    state = (*cm) >> 1;
    state = ((u16)(symbol != 0) != mps) ? state : (512 - state);
    return entropy_bits[state << 1];
}

static void eveye_rdoq_bit_est(EVEYE_CORE * core, EVEYE_SBAC * sbac)
{
    int bin, ctx;

    for(bin = 0; bin < 2; bin++)
    {
        core->rdoq_est_cbf_luma[bin] = biari_no_bits(bin, sbac->ctx.cbf_luma);
        core->rdoq_est_cbf_cb[bin] = biari_no_bits(bin, sbac->ctx.cbf_cb);
        core->rdoq_est_cbf_cr[bin] = biari_no_bits(bin, sbac->ctx.cbf_cr);
        core->rdoq_est_cbf_all[bin] = biari_no_bits(bin, sbac->ctx.cbf_all);
    }

    for(ctx = 0; ctx < NUM_CTX_CC_RUN; ctx++)
    {
        for(bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_run[ctx][bin] = biari_no_bits(bin, sbac->ctx.run + ctx);
        }
    }

    for(ctx = 0; ctx < NUM_CTX_CC_LEVEL; ctx++)
    {
        for(bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_level[ctx][bin] = biari_no_bits(bin, sbac->ctx.level + ctx);
        }
    }

    for(ctx = 0; ctx < NUM_CTX_CC_LAST; ctx++)
    {
        for(bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_last[ctx][bin] = biari_no_bits(bin, sbac->ctx.last + ctx);
        }
    }
}

static int init_cu_data(EVEYE_CU_DATA * cu_data, int log2_cuw, int log2_cuh, int qp_y, int qp_u, int qp_v)
{
    int i, j;
    int cuw_scu, cuh_scu;

    cuw_scu = 1 << (log2_cuw - MIN_CU_LOG2);
    cuh_scu = 1 << (log2_cuh - MIN_CU_LOG2);

    for(i = 0; i < NUM_CU_DEPTH; i++)
    {
        for(j = 0; j < NUM_BLOCK_SHAPE; j++)
        {
            evey_mset(cu_data->split_mode[i][j], 0, cuw_scu * cuh_scu * sizeof(s8));
        }
    }

    evey_mset(cu_data->qp_y, qp_y, cuw_scu * cuh_scu * sizeof(u8));
    evey_mset(cu_data->qp_u, qp_u, cuw_scu * cuh_scu * sizeof(u8));
    evey_mset(cu_data->qp_v, qp_v, cuw_scu * cuh_scu * sizeof(u8));
    evey_mset(cu_data->ipm[0], 0, cuw_scu * cuh_scu * sizeof(s8));
    evey_mset(cu_data->ipm[1], 0, cuw_scu * cuh_scu * sizeof(s8));

#if TRACE_ENC_CU_DATA
    evey_mset(cu_data->trace_idx, 0, cuw_scu * cuh_scu * sizeof(cu_data->trace_idx[0]));
#endif

    return EVEY_OK;
}

static int copy_cu_data(EVEYE_CU_DATA * dst, EVEYE_CU_DATA * src, int x, int y, int log2_cuw, int log2_cuh, int log2_cus, int cud, int chroma_format_idc)
{
    int i, j, k;
    int cuw, cuh, cus;
    int cuw_scu, cuh_scu, cus_scu;
    int cx, cy;
    int size, idx_dst, idx_src;

    cx = PEL2SCU(x);
    cy = PEL2SCU(y);
    cuw = 1 << log2_cuw;    /* current CU width */
    cuh = 1 << log2_cuh;    /* current CU height */
    cus = 1 << log2_cus;    /* current CU buffer stride (= current CU width) */
    cuw_scu = 1 << (log2_cuw - MIN_CU_LOG2);    /* 4x4 CU number in width */
    cuh_scu = 1 << (log2_cuh - MIN_CU_LOG2);    /* 4x4 CU number in height */
    cus_scu = 1 << (log2_cus - MIN_CU_LOG2);    /* 4x4 CU number in stride */

    /* only copy src's first row of 4x4 CUs to dis's all 4x4 CUs */

    for(j = 0; j < cuh_scu; j++)
    {
        idx_dst = (cy + j) * cus_scu + cx;
        idx_src = j * cuw_scu;

        size = cuw_scu * sizeof(s8);
        for(k = cud; k < NUM_CU_DEPTH; k++)
        {
            for(i = 0; i < NUM_BLOCK_SHAPE; i++)
            {
                evey_mcpy(dst->split_mode[k][i] + idx_dst, src->split_mode[k][i] + idx_src, size);
            }
        }
        evey_mcpy(dst->qp_y + idx_dst, src->qp_y + idx_src, size);
        evey_mcpy(dst->pred_mode + idx_dst, src->pred_mode + idx_src, size);
        evey_mcpy(dst->ipm[0] + idx_dst, src->ipm[0] + idx_src, size);
        size = cuw_scu * sizeof(u32);
        evey_mcpy(dst->map_scu + idx_dst, src->map_scu + idx_src, size);
        size = cuw_scu * sizeof(u8) * LIST_NUM;
        evey_mcpy(*(dst->refi + idx_dst), *(src->refi + idx_src), size);
        evey_mcpy(*(dst->mvp_idx + idx_dst), *(src->mvp_idx + idx_src), size);
        size = cuw_scu * sizeof(s16) * LIST_NUM * MV_D;
        evey_mcpy(dst->mv + idx_dst, src->mv + idx_src, size);
        evey_mcpy(dst->mvd + idx_dst, src->mvd + idx_src, size);

        size = cuw_scu * sizeof(int);
        k = Y_C;
        evey_mcpy(dst->nnz[k] + idx_dst, src->nnz[k] + idx_src, size);
        for(i = 0; i < MAX_SUB_TB_NUM; i++)
        {
            evey_mcpy(dst->nnz_sub[k][i] + idx_dst, src->nnz_sub[k][i] + idx_src, size);
        }

#if TRACE_ENC_CU_DATA
        size = cuw_scu * sizeof(dst->trace_idx[0]);
        evey_mcpy(dst->trace_idx + idx_dst, src->trace_idx + idx_src, size);
#endif
    }

    for(j = 0; j < cuh; j++)
    {
        idx_dst = (y + j) * cus + x;
        idx_src = j * cuw;
        size = cuw * sizeof(s16);
        evey_mcpy(dst->coef[Y_C] + idx_dst, src->coef[Y_C] + idx_src, size);
        size = cuw * sizeof(pel);
        evey_mcpy(dst->reco[Y_C] + idx_dst, src->reco[Y_C] + idx_src, size);
    }

    if(chroma_format_idc != 0)
    {
        cuw >>= (GET_CHROMA_W_SHIFT(chroma_format_idc));
        cuh >>= (GET_CHROMA_H_SHIFT(chroma_format_idc));
        cus >>= (GET_CHROMA_W_SHIFT(chroma_format_idc));
        x >>= (GET_CHROMA_W_SHIFT(chroma_format_idc));
        y >>= (GET_CHROMA_H_SHIFT(chroma_format_idc));

        for(j = 0; j < cuh; j++)
        {
            idx_dst = (y + j) * cus + x;
            idx_src = j * cuw;
            size = cuw * sizeof(s16);
            evey_mcpy(dst->coef[U_C] + idx_dst, src->coef[U_C] + idx_src, size);
            evey_mcpy(dst->coef[V_C] + idx_dst, src->coef[V_C] + idx_src, size);
            size = cuw * sizeof(pel);
            evey_mcpy(dst->reco[U_C] + idx_dst, src->reco[U_C] + idx_src, size);
            evey_mcpy(dst->reco[V_C] + idx_dst, src->reco[V_C] + idx_src, size);
        }

        for(j = 0; j < cuh_scu; j++)
        {
            idx_dst = (cy + j) * cus_scu + cx;
            idx_src = j * cuw_scu;

            size = cuw_scu * sizeof(s8);
            evey_mcpy(dst->qp_u + idx_dst, src->qp_u + idx_src, size);
            evey_mcpy(dst->qp_v + idx_dst, src->qp_v + idx_src, size);
            evey_mcpy(dst->ipm[1] + idx_dst, src->ipm[1] + idx_src, size);
            size = cuw_scu * sizeof(int);
            for(k = U_C; k < N_C; k++)
            {
                evey_mcpy(dst->nnz[k] + idx_dst, src->nnz[k] + idx_src, size);

                for(i = 0; i < MAX_SUB_TB_NUM; i++)
                {
                    evey_mcpy(dst->nnz_sub[k][i] + idx_dst, src->nnz_sub[k][i] + idx_src, size);
                }
            }
        }
    }

    return EVEY_OK;
}

void get_min_max_qp(EVEYE_CTX * ctx, EVEYE_CORE * core, s8 * min_qp, s8 * max_qp, int * is_dqp_set, EVEY_SPLIT_MODE split_mode, int cuw, int cuh, u8 qp, int x0, int y0)
{
    *is_dqp_set = 0;
    if (!ctx->pps.cu_qp_delta_enabled_flag)
    {
        *min_qp = ctx->sh.qp;
        *max_qp = ctx->sh.qp;
    }
    else
    {
        if(split_mode != NO_SPLIT)
        {
            *min_qp = qp;
            *max_qp = qp;
        }
        else
        {
            *min_qp = ctx->sh.qp;
            *max_qp = ctx->sh.qp + ctx->sh.dqp;
        }
    }
}

static int mode_cu_init(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cud)
{
#if TRACE_ENC_CU_DATA
    static u64  trace_idx = 1;
    core->trace_idx = trace_idx++;
#endif
    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    core->avail_cu = 0;
    core->nnz[Y_C] = core->nnz[U_C] = core->nnz[V_C] = 0;
    evey_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    core->pred_mode = MODE_INTRA;
    core->cost_best = MAX_COST;
    core->dist_cu_best = EVEY_INT32_MAX;

    /* Getting the appropriate QP based on dqp table*/
    int qp_i_cb, qp_i_cr;
    core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
    qp_i_cb = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
    qp_i_cr = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
    core->qp_u = p_evey_tbl_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps.bit_depth_chroma_minus8;
    core->qp_v = p_evey_tbl_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps.bit_depth_chroma_minus8;

    eveye_rdoq_bit_est(core, &core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);

    /* set original buffer */
    int   s_org   = PIC_ORIG(ctx)->s_l;
    int   s_org_c = PIC_ORIG(ctx)->s_c;
    pel * org     = PIC_ORIG(ctx)->y + (y * s_org) + x;
    pel * org_cb  = PIC_ORIG(ctx)->u + ((y >> (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc))) * s_org_c) + (x >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc)));
    pel * org_cr  = PIC_ORIG(ctx)->v + ((y >> (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc))) * s_org_c) + (x >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc)));
    int   cuw = 1 << core->log2_cuw;
    int   cuh = 1 << core->log2_cuh;

    for(int i = 0; i < cuh; i++)
    {
        evey_mcpy(core->org[Y_C] + i * cuw, org, sizeof(pel) * cuw);
        org += s_org;
    }

    cuw >>= (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    cuh >>= (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));

    for(int i = 0; i < cuh; i++)
    {
        evey_mcpy(core->org[U_C] + i * cuw, org_cb, sizeof(pel) * cuw);
        evey_mcpy(core->org[V_C] + i * cuw, org_cr, sizeof(pel) * cuw);
        org_cb += s_org_c;
        org_cr += s_org_c;
    }

    return EVEY_OK;
}

static void copy_rec_to_pic(EVEYE_CORE * core, int x, int y, int w, int h, EVEY_PIC * pic, int chroma_format_idc)
{
    EVEYE_CU_DATA * cu_data;
    pel           * src, *dst;
    int             j, s_pic, off, size;
    int             log2_w, log2_h;
    int             stride;

    log2_w = EVEY_CONV_LOG2(w);
    log2_h = EVEY_CONV_LOG2(h);

    cu_data = &core->cu_data_best[log2_w - 2][log2_h - 2];

    s_pic = pic->s_l;

    stride = w;

    if (x + w > pic->w_l)
    {
        w = pic->w_l - x;
    }

    if (y + h > pic->h_l)
    {
        h = pic->h_l - y;
    }

    /* luma */
    src = cu_data->reco[Y_C];
    dst = pic->y + x + y * s_pic;
    size = sizeof(pel) * w;

    for(j = 0; j < h; j++)
    {
        evey_mcpy(dst, src, size);
        src += stride;
        dst += s_pic;
    }

    if(chroma_format_idc != 0)
    {
        int w_shift = GET_CHROMA_W_SHIFT(chroma_format_idc);
        int h_shift = GET_CHROMA_H_SHIFT(chroma_format_idc);

        /* chroma */
        s_pic = pic->s_c;
        off = (x >> w_shift) + (y >> h_shift) * s_pic;
        size = (sizeof(pel) * w) >> w_shift;

        src = cu_data->reco[U_C];
        dst = pic->u + off;

        for(j = 0; j < (h >> h_shift); j++)
        {
            evey_mcpy(dst, src, size);
            src += (stride >> w_shift);
            dst += s_pic;
        }

        src = cu_data->reco[V_C];
        dst = pic->v + off;

        for(j = 0; j < (h >> h_shift); j++)
        {
            evey_mcpy(dst, src, size);
            src += (stride >> w_shift);
            dst += s_pic;
        }
    }
}

static void copy_best_cu_data(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    EVEYE_CU_DATA * cu_data;
    EVEYE_MODE    * mi = &ctx->mode;
    int             i, j, idx, size;
    int             cuw, cuh;

    cuw = 1 << core->log2_cuw;
    cuh = 1 << core->log2_cuh;
    cu_data = &core->cu_data_temp[core->log2_cuw - 2][core->log2_cuh - 2];

    /* copy coef */
    size = cuw * cuh * sizeof(s16);
    evey_mcpy(cu_data->coef[Y_C], mi->coef[Y_C], size);

    /* copy reco */
    size = cuw * cuh * sizeof(pel);
    evey_mcpy(cu_data->reco[Y_C], mi->rec[Y_C], size);

#if TRACE_ENC_CU_DATA_CHECK
    evey_assert(core->trace_idx == mi->trace_cu_idx);
    evey_assert(core->trace_idx != 0);
#endif

    /* copy mode info */
    idx = 0;
    for(j = 0; j < cuh >> MIN_CU_LOG2; j++)
    {
        for(i = 0; i < cuw >> MIN_CU_LOG2; i++)
        {
            cu_data->pred_mode[idx + i] = core->pred_mode;
            cu_data->nnz[Y_C][idx + i] = mi->nnz[Y_C];

            for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
            {
               cu_data->nnz_sub[Y_C][sb][idx + i] = mi->nnz_sub[Y_C][sb];
            }

            cu_data->qp_y[idx + i] = core->qp_y;
            MCU_RESET_QP(cu_data->map_scu[idx + i]);
            if (ctx->pps.cu_qp_delta_enabled_flag)
            {
                MCU_SET_IF_COD_SN_QP(cu_data->map_scu[idx + i], core->pred_mode == MODE_INTRA, ctx->slice_num, core->qp);
            }
            else
            {
                MCU_SET_IF_COD_SN_QP(cu_data->map_scu[idx + i], core->pred_mode == MODE_INTRA, ctx->slice_num, ctx->sh.qp);
            }

            if(core->pred_mode == MODE_INTRA)
            {
                cu_data->ipm[0][idx + i] = core->ipm[0];
                evey_mset(cu_data->mv[idx + i], 0, sizeof(s16) * LIST_NUM * MV_D);
                cu_data->refi[idx + i][LIST_0] = -1;
                cu_data->refi[idx + i][LIST_1] = -1;
            }
            else
            {
                evey_mcpy(cu_data->refi[idx + i], mi->refi, sizeof(s8) * LIST_NUM);
                evey_mcpy(cu_data->mvp_idx[idx + i], mi->mvp_idx, sizeof(u8) * LIST_NUM);
                evey_mcpy(cu_data->mv[idx + i], mi->mv, sizeof(s16) * LIST_NUM * MV_D);
                evey_mcpy(cu_data->mvd[idx + i], mi->mvd, sizeof(s16) * LIST_NUM * MV_D);
            }
#if TRACE_ENC_CU_DATA
            cu_data->trace_idx[idx + i] = core->trace_idx;
#endif
        }

        idx += cuw >> MIN_CU_LOG2;
    }

#if TRACE_ENC_CU_DATA_CHECK
    int w = 1 << (core->log2_cuw - MIN_CU_LOG2);
    int h = 1 << (core->log2_cuh - MIN_CU_LOG2);
    idx = 0;
    for (j = 0; j < h; ++j, idx += w)
    {
        for (i = 0; i < w; ++i)
        {
            evey_assert(cu_data->trace_idx[idx + i] == core->trace_idx);
        }
    }
#endif

    if(ctx->sps.chroma_format_idc)
    {
        /* copy coef */
        size = (cuw * cuh * sizeof(s16)) >> ((GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc)) + (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc)));
        evey_mcpy(cu_data->coef[U_C], mi->coef[U_C], size);
        evey_mcpy(cu_data->coef[V_C], mi->coef[V_C], size);

        /* copy reco */
        size = (cuw * cuh * sizeof(pel)) >> ((GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc)) + (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc)));
        evey_mcpy(cu_data->reco[U_C], mi->rec[U_C], size);
        evey_mcpy(cu_data->reco[V_C], mi->rec[V_C], size);

        /* copy mode info */
        idx = 0;
        for (j = 0; j < cuh >> MIN_CU_LOG2; j++)
        {
            for (i = 0; i < cuw >> MIN_CU_LOG2; i++)
            {
                cu_data->nnz[U_C][idx + i] = mi->nnz[U_C];
                cu_data->nnz[V_C][idx + i] = mi->nnz[V_C];
                for (int c = U_C; c < N_C; c++)
                {
                    for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
                    {
                        cu_data->nnz_sub[c][sb][idx + i] = mi->nnz_sub[c][sb];
                    }
                }

                cu_data->qp_u[idx + i] = core->qp_u;
                cu_data->qp_v[idx + i] = core->qp_v;

                if (core->pred_mode == MODE_INTRA)
                {
                    cu_data->ipm[1][idx + i] = core->ipm[1];
                }
            }
            idx += cuw >> MIN_CU_LOG2;
        }
    }
}

static void update_map_scu(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int src_cuw, int src_cuh)
{
    u32  * map_scu = 0, * src_map_scu = 0;
    s8   * map_ipm = 0, * src_map_ipm = 0;
    s16 (* map_mv)[LIST_NUM][MV_D] = 0, (* src_map_mv)[LIST_NUM][MV_D] = 0;
    s8  (* map_refi)[LIST_NUM] = 0;
    s8  ** src_map_refi = NULL;
    int    size_depth;
    int    w, h, i, size, size_ipm, size_mv, size_refi;
    int    log2_src_cuw, log2_src_cuh;
    int    scu_x, scu_y;

    scu_x = PEL2SCU(x);
    scu_y = PEL2SCU(y);
    log2_src_cuw = EVEY_CONV_LOG2(src_cuw);
    log2_src_cuh = EVEY_CONV_LOG2(src_cuh);

    map_scu = ctx->map_scu + scu_y * ctx->w_scu + scu_x;
    src_map_scu = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].map_scu;
    map_ipm = ctx->map_ipm + scu_y * ctx->w_scu + scu_x;
    src_map_ipm = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].ipm[0];
    map_mv = ctx->map_mv + scu_y * ctx->w_scu + scu_x;
    src_map_mv = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].mv;
    map_refi = ctx->map_refi + scu_y * ctx->w_scu + scu_x;
    src_map_refi = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].refi;

    if(x + src_cuw > ctx->w)
    {
        w = (ctx->w - x) >> MIN_CU_LOG2;
    }
    else
    {
        w = (src_cuw >> MIN_CU_LOG2);
    }

    if(y + src_cuh > ctx->h)
    {
        h = (ctx->h - y) >> MIN_CU_LOG2;
    }
    else
    {
        h = (src_cuh >> MIN_CU_LOG2);
    }

    size = sizeof(u32) * w;
    size_ipm = sizeof(u8) * w;
    size_mv = sizeof(s16) * w * LIST_NUM * MV_D;
    size_refi = sizeof(s8) * w * LIST_NUM;
    size_depth = sizeof(s8) * w;

    for(i = 0; i < h; i++)
    {
        evey_mcpy(map_scu, src_map_scu, size);
        evey_mcpy(map_ipm, src_map_ipm, size_ipm);
        evey_mcpy(map_mv, src_map_mv, size_mv);
        evey_mcpy(map_refi, *(src_map_refi), size_refi);

        map_scu += ctx->w_scu;
        src_map_scu += (src_cuw >> MIN_CU_LOG2);

        map_ipm += ctx->w_scu;
        src_map_ipm += (src_cuw >> MIN_CU_LOG2);

        map_mv += ctx->w_scu;
        src_map_mv += (src_cuw >> MIN_CU_LOG2);
        
        map_refi += ctx->w_scu;
        src_map_refi += (src_cuw >> MIN_CU_LOG2);
    }
}

static void clear_map_scu(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int cuw, int cuh)
{
    u32 * map_scu;
    int   w, h, i, size;

    map_scu = ctx->map_scu + (y >> MIN_CU_LOG2) * ctx->w_scu + (x >> MIN_CU_LOG2);

    if(x + cuw > ctx->w)
    {
        cuw = ctx->w - x;
    }

    if(y + cuh > ctx->h)
    {
        cuh = ctx->h - y;
    }

    w = (cuw >> MIN_CU_LOG2);
    h = (cuh >> MIN_CU_LOG2);

    size = sizeof(u32) * w;

    for(i = 0; i < h; i++)
    {
        evey_mset(map_scu, 0, size);
        map_scu += ctx->w_scu;
    }
}

static double mode_coding_unit(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cud)
{
    double cost_intra = MAX_COST;
    double cost_inter = MAX_COST;

    /* prepare CU mode decision */
    mode_cu_init(ctx, core, x, y, log2_cuw, log2_cuh, cud);

    /* inter decision */
    if(ctx->sh.slice_type != SLICE_I)
    {
        /* inter mode decision */
        cost_inter = ctx->fn_pinter_analyze_cu(ctx, core, x, y);
        copy_best_cu_data(ctx, core);
    }

    /* intra decision */
#if ENC_FAST_SKIP_INTRA
    if(ctx->sh.slice_type == SLICE_I || core->nnz[Y_C] != 0 || core->nnz[U_C] != 0 || core->nnz[V_C] != 0 || cost_inter == MAX_COST)
#endif
    {
        /* intra mode decision */
        cost_intra = ctx->fn_pintra_analyze_cu(ctx, core, x, y);
        if(cost_intra < cost_inter)
        {
            copy_best_cu_data(ctx, core);
        }
    }

    return core->cost_best;
}

static double mode_coding_tree(EVEYE_CTX * ctx, EVEYE_CORE * core, int x0, int y0, int cup, int log2_cuw, int log2_cuh, int cud, u8 qp)
{
    /* x0 = CU's left up corner horizontal index in entrie frame */
    /* y0 = CU's left up corner vertical index in entire frame */
    /* cuw = CU width, log2_cuw = CU width in log2 */
    /* cuh = CU height, log2_chu = CU height in log2 */
    /* ctx->w = frame width, ctx->h = frame height */
    int             cuw = 1 << log2_cuw;
    int             cuh = 1 << log2_cuh;
    s8              best_split_mode = NO_SPLIT;
    int             bit_cnt;
    double          cost_best = MAX_COST;
    double          cost_temp = MAX_COST;
    EVEYE_SBAC      s_temp_depth = {0};
    int             boundary = !(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);
    int             split_allow[NUM_SPLIT_MODE]; /* allowed split by normative and non-normative selection */
    EVEY_SPLIT_MODE split_mode = NO_SPLIT;
    EVEYE_DQP       dqp_temp_depth = { 0 };
    u8              best_dqp = qp;
    s8              min_qp, max_qp;
    double          cost_temp_dqp;
    double          cost_best_dqp = MAX_COST;
    int             dqp_coded = 0;
    int             loop_counter;
    int             dqp_loop;
    int             cu_mode_dqp = 0;
    int             dist_cu_best_dqp = 0;
    int             split_test = 1;
    
    SBAC_LOAD(core->s_curr_before_split[log2_cuw - 2][log2_cuh - 2], core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);

    /* determine allowed split modes for the current CU */
    /* currently based on CU size */
    if(cuw > ctx->min_cu_size || cuh > ctx->min_cu_size)
    {        
        split_allow[NO_SPLIT] = 1;
        split_allow[SPLIT_QUAD] = 1;
        split_test = 1;
    }
    else
    {        
        split_allow[NO_SPLIT] = 1;
        split_allow[SPLIT_QUAD] = 0;
        split_test = 0;
    }

    /* NO_SPLIT */
    if(!boundary)
    {
        cost_temp = 0.0;
        init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->sh.qp, ctx->sh.qp, ctx->sh.qp);

        ctx->sh.qp_prev_mode = core->dqp_data[log2_cuw - 2][log2_cuh - 2].prev_qp;
        best_dqp = ctx->sh.qp_prev_mode;

        split_mode = NO_SPLIT;
        if(split_allow[split_mode])
        {
            if(cuw > ctx->min_cu_size || cuh > ctx->min_cu_size)
            {
                /* count bits for CU split flag */
                SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
                eveye_sbac_bit_reset(&core->s_temp_run);
                evey_set_split_mode(NO_SPLIT, cud, 0, cuw, cuh, cuw, core->cu_data_temp[log2_cuw - 2][log2_cuh - 2].split_mode);
                eveye_eco_split_mode(&core->bs_temp, ctx, core, cud, 0, cuw, cuh, cuw); /* split_cu_flag */
                bit_cnt = eveye_get_bit_number(&core->s_temp_run);
                cost_temp += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);
                SBAC_STORE(core->s_curr_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_run);
            }

            int is_dqp_set = 0;
            get_min_max_qp(ctx, core, &min_qp, &max_qp, &is_dqp_set, split_mode, cuw, cuh, qp, x0, y0);
            for (int dqp = min_qp; dqp <= max_qp; dqp++)
            {
                core->qp = GET_QP((s8)qp, dqp - (s8)qp);
                core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp = core->qp;
                cost_temp_dqp = cost_temp;
                init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->sh.qp, ctx->sh.qp, ctx->sh.qp);
                clear_map_scu(ctx, core, x0, y0, cuw, cuh);

                /* CU mode decision */
                cost_temp_dqp += mode_coding_unit(ctx, core, x0, y0, log2_cuw, log2_cuh, cud);

                if (cost_best > cost_temp_dqp)
                {
                    cu_mode_dqp = core->pred_mode;
                    dist_cu_best_dqp = core->dist_cu_best;
                    /* backup the current best data */
                    copy_cu_data(&core->cu_data_best[log2_cuw - 2][log2_cuh - 2], &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], 0, 0, log2_cuw, log2_cuh, log2_cuw, cud, ctx->sps.chroma_format_idc);
                    cost_best = cost_temp_dqp;
                    best_split_mode = NO_SPLIT;
                    SBAC_STORE(s_temp_depth, core->s_next_best[log2_cuw - 2][log2_cuh - 2]);
                    DQP_STORE(dqp_temp_depth, core->dqp_next_best[log2_cuw - 2][log2_cuh - 2]);
                    copy_rec_to_pic(core, x0, y0, cuw, cuh, PIC_CURR(ctx), ctx->sps.chroma_format_idc);
                }
            }

            cost_temp = cost_best;
            core->pred_mode = cu_mode_dqp;
            core->dist_cu_best = dist_cu_best_dqp;

#if TRACE_COSTS
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("Block [");
            EVEY_TRACE_INT(x0);
            EVEY_TRACE_STR(", ");
            EVEY_TRACE_INT(y0);
            EVEY_TRACE_STR("]x(");
            EVEY_TRACE_INT(cuw);
            EVEY_TRACE_STR("x");
            EVEY_TRACE_INT(cuh);
            EVEY_TRACE_STR(") split_type ");
            EVEY_TRACE_INT(NO_SPLIT);
            EVEY_TRACE_STR(" cost is ");
            EVEY_TRACE_DOUBLE(cost_temp);
            EVEY_TRACE_STR("\n");
#endif
        }
        else
        {
            cost_temp = MAX_COST;
        }
    }

#if ENC_ECU_SKIP /* determine whether or not to test split mode */
    if(split_test && (core->pred_mode == MODE_SKIP) && cud >= (ctx->poc.poc_val % 2 ? ENC_ECU_DEPTH - 2 : ENC_ECU_DEPTH) && cost_best != MAX_COST)
    {
        split_test = 0;
    }

    if(split_test && ctx->sh.slice_type == SLICE_I && cost_best != MAX_COST)
    {
        const int bits = 6; /* split_cu_flag and others. approximately 6 bits */
        if(core->dist_cu_best < ctx->lambda[0] * bits)
        {
            split_test = 0;
        }
    }
#endif

    /* SPLIT_QUAD */
    if(split_test && (cuw > ctx->min_cu_size || cuh > ctx->min_cu_size))
    {
        split_mode = SPLIT_QUAD;

        if(split_allow[split_mode])
        {
            EVEY_SPLIT_STRUCT split_struct;
            evey_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_ctu_size - MIN_CU_LOG2, &split_struct);

            int prev_log2_sub_cuw = split_struct.log_cuw[0];
            int prev_log2_sub_cuh = split_struct.log_cuh[0];
            int is_dqp_set = 0;

            init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->sh.qp, ctx->sh.qp, ctx->sh.qp);
            clear_map_scu(ctx, core, x0, y0, cuw, cuh);
            cost_temp = 0.0;

            /* count bits for CU split flag. Currently, split flag is signaled although CU is laid on picture boundary. */
            {
                SBAC_LOAD(core->s_temp_run, core->s_curr_before_split[log2_cuw - 2][log2_cuh - 2]);
                eveye_sbac_bit_reset(&core->s_temp_run);
                evey_set_split_mode(split_mode, cud, 0, cuw, cuh, cuw, core->cu_data_temp[log2_cuw - 2][log2_cuh - 2].split_mode);                
                eveye_eco_split_mode(&core->bs_temp, ctx, core, cud, 0, cuw, cuh, cuw); /* split_cu_flag */
                bit_cnt = eveye_get_bit_number(&core->s_temp_run);
                cost_temp += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);
                SBAC_STORE(core->s_curr_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_run);
            }

            get_min_max_qp(ctx, core, &min_qp, &max_qp, &is_dqp_set, split_mode, cuw, cuh, qp, x0, y0);

            loop_counter = 0;
            if(is_dqp_set)
            {
                loop_counter = EVEY_ABS(max_qp - min_qp);
            }
            cost_best_dqp = MAX_COST;
            for(dqp_loop = 0; dqp_loop <= loop_counter; dqp_loop++)
            {
                int dqp = min_qp + dqp_loop;
                core->qp = GET_QP((s8)qp, dqp - (s8)qp);
                if(is_dqp_set)
                {
                    core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp = core->qp;
                }

                cost_temp_dqp = cost_temp;
                init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->sh.qp, ctx->sh.qp, ctx->sh.qp);
                clear_map_scu(ctx, core, x0, y0, cuw, cuh);

#if TRACE_ENC_CU_DATA_CHECK
                static int counter_in[MAX_CU_LOG2 - MIN_CU_LOG2][MAX_CU_LOG2 - MIN_CU_LOG2] = {0,};
                counter_in[log2_cuw - MIN_CU_LOG2][log2_cuh - MIN_CU_LOG2]++;
#endif
                for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
                {
                    int log2_sub_cuw = split_struct.log_cuw[part_num];
                    int log2_sub_cuh = split_struct.log_cuh[part_num];
                    int x_pos = split_struct.x_pos[part_num];
                    int y_pos = split_struct.y_pos[part_num];
                    int cur_cuw = split_struct.width[part_num];
                    int cur_cuh = split_struct.height[part_num];

                    if((x_pos < ctx->w) && (y_pos < ctx->h)) /* only process CUs inside a picture */
                    {
                        if(part_num == 0)
                        {
                            SBAC_LOAD(core->s_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
                            DQP_STORE(core->dqp_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
                        }
                        else
                        {
                            SBAC_LOAD(core->s_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->s_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                            DQP_STORE(core->dqp_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->dqp_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                        }

                        /* CU split test */
                        cost_temp_dqp += mode_coding_tree(ctx, core, x_pos, y_pos, split_struct.cup[part_num], log2_sub_cuw, log2_sub_cuh, split_struct.cud[part_num], core->qp);

                        core->qp = GET_QP((s8)qp, dqp - (s8)qp);

                        copy_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], &core->cu_data_best[log2_sub_cuw - 2][log2_sub_cuh - 2], x_pos - split_struct.x_pos[0], y_pos - split_struct.y_pos[0], log2_sub_cuw, log2_sub_cuh, log2_cuw, cud, ctx->sps.chroma_format_idc);
                        update_map_scu(ctx, core, x_pos, y_pos, cur_cuw, cur_cuh);
                        prev_log2_sub_cuw = log2_sub_cuw;
                        prev_log2_sub_cuh = log2_sub_cuh;
                    }
                }
#if TRACE_COSTS
                EVEY_TRACE_COUNTER;
                EVEY_TRACE_STR("Block [");
                EVEY_TRACE_INT(x0);
                EVEY_TRACE_STR(", ");
                EVEY_TRACE_INT(y0);
                EVEY_TRACE_STR("]x(");
                EVEY_TRACE_INT(cuw);
                EVEY_TRACE_STR("x");
                EVEY_TRACE_INT(cuh);
                EVEY_TRACE_STR(") split_type ");
                EVEY_TRACE_INT(split_mode);
                EVEY_TRACE_STR(" cost is ");
                EVEY_TRACE_DOUBLE(cost_temp);
                EVEY_TRACE_STR("\n");
#endif
#if TRACE_ENC_CU_DATA_CHECK
                static int counter_out = 0;
                counter_out++;
                {
                    EVEYE_CU_DATA *cu_data = &(core->cu_data_temp[log2_cuw - 2][log2_cuh - 2]);
                    int cuw = 1 << (log2_cuw - MIN_CU_LOG2);
                    int cuh = 1 << (log2_cuh - MIN_CU_LOG2);
                    int cus = cuw;
                    int idx = 0;
                    for(int j = 0; j < cuh; ++j)
                    {
                        int y_pos = y0 + (j << MIN_CU_LOG2);
                        for(int i = 0; i < cuw; ++i)
                        {
                            int x_pos = x0 + (i << MIN_CU_LOG2);
                            if((x_pos < ctx->w) && (y_pos < ctx->h))
                                evey_assert(cu_data->trace_idx[idx + i] != 0);
                        }
                        idx += cus;
                    }
                }
#endif
                if(cost_best_dqp > cost_temp_dqp)
                {
                    cost_best_dqp = cost_temp_dqp;
                }

                if(cost_best > cost_temp_dqp)
                {
                    /* backup the current best data */
                    copy_cu_data(&core->cu_data_best[log2_cuw - 2][log2_cuh - 2], &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], 0, 0, log2_cuw, log2_cuh, log2_cuw, cud, ctx->sps.chroma_format_idc);

                    cost_best = cost_temp_dqp;
                    best_dqp = core->dqp_data[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2].prev_qp;
                    DQP_STORE(dqp_temp_depth, core->dqp_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                    SBAC_STORE(s_temp_depth, core->s_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                    best_split_mode = split_mode;
                }
            }
            cost_temp = cost_best_dqp;
        }
    }

    copy_rec_to_pic(core, x0, y0, cuw, cuh, PIC_CURR(ctx), ctx->sps.chroma_format_idc);

    /* set best split mode */
    evey_set_split_mode(best_split_mode, cud, 0, cuw, cuh, cuw, core->cu_data_best[log2_cuw - 2][log2_cuh - 2].split_mode);

    SBAC_LOAD(core->s_next_best[log2_cuw - 2][log2_cuh - 2], s_temp_depth);
    DQP_LOAD(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], dqp_temp_depth);

    evey_assert(cost_best != MAX_COST);

#if TRACE_ENC_CU_DATA_CHECK
    int i, j, w, h, w_scu;
    w = 1 << (core->log2_cuw - MIN_CU_LOG2);
    h = 1 << (core->log2_cuh - MIN_CU_LOG2);
    w_scu = 1 << (log2_cuw - MIN_CU_LOG2);
    for (j = 0; j < h; ++j)
    {
        int y_pos = core->y_pel + (j << MIN_CU_LOG2);
        for (i = 0; i < w; ++i)
        {
            int x_pos = core->x_pel + (i << MIN_CU_LOG2);
            if (x_pos < ctx->w && y_pos < ctx->h)
                evey_assert(core->cu_data_best[log2_cuw - 2][log2_cuh - 2].trace_idx[i + j * w_scu] != 0);
        }
    }
#endif

    return cost_best;
}

static int mode_init_ctu(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    int ret;

    /* initialize pintra */
    if(ctx->fn_pintra_init_ctu)
    {
        ret = ctx->fn_pintra_init_ctu(ctx, core);
        evey_assert_rv(ret == EVEY_OK, ret);
    }

    /* initialize pinter */
    if(ctx->fn_pinter_init_ctu)
    {
        ret = ctx->fn_pinter_init_ctu(ctx, core);
        evey_assert_rv(ret == EVEY_OK, ret);
    }

    return EVEY_OK;
}

static void update_to_ctx_map(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    EVEYE_CU_DATA * cu_data;
    int             cuw, cuh, i, j, w, h;
    int             x, y;
    int             core_idx, ctx_idx;
    s8           (* map_refi)[LIST_NUM];
    s16          (* map_mv)[LIST_NUM][MV_D];
    s8            * map_ipm;

    cu_data = &core->cu_data_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2];
    cuw = ctx->ctu_size;
    cuh = ctx->ctu_size;
    x = core->x_pel;
    y = core->y_pel;

    if(x + cuw > ctx->w)
    {
        cuw = ctx->w - x;
    }

    if(y + cuh > ctx->h)
    {
        cuh = ctx->h - y;
    }

    w = cuw >> MIN_CU_LOG2;
    h = cuh >> MIN_CU_LOG2;

    /* copy mode info */
    core_idx = 0;
    ctx_idx = PEL2SCU(y) * ctx->w_scu + PEL2SCU(x);

    map_ipm = ctx->map_ipm;
    map_refi = ctx->map_refi;
    map_mv = ctx->map_mv;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            if(cu_data->pred_mode[core_idx + j] == MODE_INTRA)
            {
                map_ipm[ctx_idx + j] = cu_data->ipm[0][core_idx + j];
                map_mv[ctx_idx + j][LIST_0][MV_X] = 0;
                map_mv[ctx_idx + j][LIST_0][MV_Y] = 0;
                map_mv[ctx_idx + j][LIST_1][MV_X] = 0;
                map_mv[ctx_idx + j][LIST_1][MV_Y] = 0;
            }
            else
            {
                map_refi[ctx_idx + j][LIST_0] = cu_data->refi[core_idx + j][LIST_0];
                map_refi[ctx_idx + j][LIST_1] = cu_data->refi[core_idx + j][LIST_1];
                map_mv[ctx_idx + j][LIST_0][MV_X] = cu_data->mv[core_idx + j][LIST_0][MV_X];
                map_mv[ctx_idx + j][LIST_0][MV_Y] = cu_data->mv[core_idx + j][LIST_0][MV_Y];
                map_mv[ctx_idx + j][LIST_1][MV_X] = cu_data->mv[core_idx + j][LIST_1][MV_X];
                map_mv[ctx_idx + j][LIST_1][MV_Y] = cu_data->mv[core_idx + j][LIST_1][MV_Y];
            }
        }
        ctx_idx += ctx->w_scu;
        core_idx += (ctx->ctu_size >> MIN_CU_LOG2);
    }

    update_map_scu(ctx, core, core->x_pel, core->y_pel, ctx->ctu_size, ctx->ctu_size);
    copy_cu_data(&ctx->map_cu_data[core->ctu_num], &core->cu_data_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2],
                 0, 0, ctx->log2_ctu_size, ctx->log2_ctu_size, ctx->log2_ctu_size, 0, ctx->sps.chroma_format_idc);
    /* copy split flags from the CTU map to the picture map */
    evey_mcpy(ctx->map_split[core->ctu_num], ctx->map_cu_data[core->ctu_num].split_mode, sizeof(s8) * NUM_CU_DEPTH * NUM_BLOCK_SHAPE * MAX_CU_CNT_IN_CTU);
}

/* entry point for CTU level decision */
static int mode_analyze_ctu(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    u32 * map_scu;
    int   i, j, w, h;

    /* initialize cu data */
    init_cu_data(&core->cu_data_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2], ctx->log2_ctu_size, ctx->log2_ctu_size, ctx->sh.qp, ctx->sh.qp, ctx->sh.qp);
    init_cu_data(&core->cu_data_temp[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2], ctx->log2_ctu_size, ctx->log2_ctu_size, ctx->sh.qp, ctx->sh.qp, ctx->sh.qp);

    /* determine split mode */
    mode_coding_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->log2_ctu_size, ctx->log2_ctu_size, 0, ctx->sh.qp);

    /* copy CTU data to picture map */
    update_to_ctx_map(ctx, core);    

    /* reset coded flags for the current ctu */
    core->x_scu = PEL2SCU(core->x_pel);
    core->y_scu = PEL2SCU(core->y_pel);
    map_scu = ctx->map_scu + ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    w = EVEY_MIN(1 << (ctx->log2_ctu_size - MIN_CU_LOG2), ctx->w_scu - core->x_scu);
    h = EVEY_MIN(1 << (ctx->log2_ctu_size - MIN_CU_LOG2), ctx->h_scu - core->y_scu);

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            MCU_CLR_COD(map_scu[j]);
        }
        map_scu += ctx->w_scu;
    }

#if TRACE_ENC_CU_DATA_CHECK
    h = w = 1 << (ctx->log2_ctu_size - MIN_CU_LOG2);
    for(j = 0; j < h; ++j)
    {
        int y_pos = core->y_pel + (j << MIN_CU_LOG2);
        for(i = 0; i < w; ++i)
        {
            int x_pos = core->x_pel + (i << MIN_CU_LOG2);
            if(x_pos < ctx->w && y_pos < ctx->h)
                evey_assert(core->cu_data_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2].trace_idx[i + h * j] != 0);
        }
    }
    for(j = 0; j < h; ++j)
    {
        int y_pos = core->y_pel + (j << MIN_CU_LOG2);
        for(i = 0; i < w; ++i)
        {
            int x_pos = core->x_pel + (i << MIN_CU_LOG2);
            if(x_pos < ctx->w && y_pos < ctx->h)
                evey_assert(ctx->map_cu_data[core->ctu_num].trace_idx[i + h * j] != 0);
        }
    }
#endif

    return EVEY_OK;
}

static int mode_init_frame(EVEYE_CTX * ctx)
{
    int ret;

    /* initialize pintra */
    if(ctx->fn_pintra_init_frame)
    {
        ret = ctx->fn_pintra_init_frame(ctx);
        evey_assert_rv(ret == EVEY_OK, ret);
    }

    /* initialize pinter */
    if(ctx->fn_pinter_init_frame)
    {
        ret = ctx->fn_pinter_init_frame(ctx);
        evey_assert_rv(ret == EVEY_OK, ret);
    }

    return EVEY_OK;
}

static int mode_analyze_frame(EVEYE_CTX *ctx)
{
    return EVEY_OK;
}

static int mode_set_complexity(EVEYE_CTX * ctx, int complexity)
{
    EVEYE_MODE * mi = &ctx->mode;
    evey_assert_rv(mi != NULL, EVEY_ERR_UNEXPECTED);
    return EVEY_OK;
}

int eveye_mode_create(EVEYE_CTX * ctx, int complexity)
{
    EVEYE_MODE * mi = &ctx->mode;

    /* create mode information structure */
    evey_assert_rv(mi, EVEY_ERR_OUT_OF_MEMORY);
    evey_mset(mi, 0, sizeof(EVEYE_MODE));

    /* set function addresses */
    ctx->fn_mode_init_frame = mode_init_frame;
    ctx->fn_mode_init_ctu = mode_init_ctu;
    ctx->fn_mode_analyze_frame = mode_analyze_frame;
    ctx->fn_mode_analyze_ctu = mode_analyze_ctu;
    ctx->fn_mode_set_complexity = mode_set_complexity;

    return ctx->fn_mode_set_complexity(ctx, complexity);
}

void eveye_calc_delta_dist_filter_boundary(EVEYE_CTX * ctx, EVEYE_CORE * core, pel(*src)[MAX_CU_DIM], int s_src, int x, int y, u8 intra_flag, u8 cbf_l, s8 * refi, s16(*mv)[MV_D], u8 is_mv_from_mvf)
{
    int        chroma_format_idc = ctx->sps.chroma_format_idc;
    int        w_shift = GET_CHROMA_W_SHIFT(chroma_format_idc);
    int        h_shift = GET_CHROMA_H_SHIFT(chroma_format_idc);
    int        bit_depth_l = ctx->sps.bit_depth_luma_minus8 + 8;
    int        bit_depth_c = ctx->sps.bit_depth_chroma_minus8 + 8;
    int        i, j;
    EVEY_PIC * pic_rec = PIC_CURR(ctx);
    EVEY_PIC * pic_org = PIC_ORIG(ctx);
    int        log2_cuw = core->log2_cuw;
    int        log2_cuh = core->log2_cuh;
    int        cuw = 1 << log2_cuw;
    int        cuh = 1 << log2_cuh;
    int        x_offset = 2; /* for preparing deblocking filter taps */ /* Note: should be equal to the longest tap size */
    int        y_offset = 2; /* for preparing deblocking filter taps */ /* Note: should be equal to the longest tap size */
    int        x_tm = 2; /* for calculating template dist */
    int        y_tm = 2; /* must be the same as x_tm */
    int        log2_x_tm = EVEY_CONV_LOG2(x_tm);
    int        log2_y_tm = EVEY_CONV_LOG2(y_tm);
    EVEY_PIC * pic_dbk = ctx->pic_dbk;
    int        s_l_dbk = pic_dbk->s_l;
    int        s_c_dbk = pic_dbk->s_c;
    int        s_l_org = pic_org->s_l;
    int        s_c_org = pic_org->s_c;
    pel      * dst_y = pic_dbk->y + y * s_l_dbk + x;
    pel      * org_y = pic_org->y + y * s_l_org + x;
    pel      * dst_u = pic_dbk->u + (y >> h_shift) * s_c_dbk + (x >> w_shift);
    pel      * dst_v = pic_dbk->v + (y >> h_shift) * s_c_dbk + (x >> w_shift);
    pel      * org_u = pic_org->u + (y >> h_shift) * s_c_org + (x >> w_shift);
    pel      * org_v = pic_org->v + (y >> h_shift) * s_c_org + (x >> w_shift);
    int        x_scu = PEL2SCU(x);
    int        y_scu = PEL2SCU(y);
    int        t = x_scu + y_scu * ctx->w_scu;
    /* cu info to save */
    u8         intra_flag_save, cbf_l_save;
    u8         do_filter = 0;
    int        y_begin = 0;
    int        y_begin_uv = 0;
    int        x_begin = 0;
    int        x_begin_uv = 0;

    if(ctx->sh.slice_deblocking_filter_flag)
    {
        do_filter = 1;
    }

    if(do_filter == 0)
    {
        core->delta_dist[Y_C] = core->delta_dist[U_C] = core->delta_dist[V_C] = 0;
        return; /* if no filter is applied, just return delta_dist as 0 */
    }

    /* reset */
    for(i = 0; i < N_C; i++)
    {
        core->dist_filter[i] = core->dist_nofilt[i] = 0;
    }

    /********************** prepare pred/rec pixels (not filtered) ****************************/

    /* fill src to dst */
    for(i = 0; i < cuh; i++)
    {
        evey_mcpy(dst_y + i * s_l_dbk, src[Y_C] + i * s_src, cuw * sizeof(pel));
    }

    /* fill top */
    if(y != y_begin)
    {
        for(i = 0; i < y_offset; i++)
        {
            evey_mcpy(dst_y + (-y_offset + i)*s_l_dbk, pic_rec->y + (y - y_offset + i)*s_l_dbk + x, cuw * sizeof(pel));
        }
    }

    /* fill left */
    if(x != x_begin)
    {
        for(i = 0; i < cuh; i++)
        {
            evey_mcpy(dst_y + i * s_l_dbk - x_offset, pic_rec->y + (y + i)*s_l_dbk + (x - x_offset), x_offset * sizeof(pel));
        }
    }

    /* modify parameters from y to uv */
    cuw >>= w_shift;
    cuh >>= h_shift;
    s_src >>= w_shift;
    x >>= w_shift;
    y >>= h_shift;
    x_tm >>= w_shift;
    y_tm >>= h_shift;
    log2_cuw -= w_shift;
    log2_cuh -= h_shift;
    log2_x_tm -= w_shift;
    log2_y_tm -= h_shift;

    if(chroma_format_idc != 0)
    {
        /* fill src to dst */
        for(i = 0; i < cuh; i++)
        {
            evey_mcpy(dst_u + i * s_c_dbk, src[U_C] + i * s_src, cuw * sizeof(pel));
            evey_mcpy(dst_v + i * s_c_dbk, src[V_C] + i * s_src, cuw * sizeof(pel));
        }

        /* fill top */
        if(y != y_begin_uv)
        {
            for(i = 0; i < y_offset; i++)
            {
                evey_mcpy(dst_u + (-y_offset + i)*s_c_dbk, pic_rec->u + (y - y_offset + i)*s_c_dbk + x, cuw * sizeof(pel));
                evey_mcpy(dst_v + (-y_offset + i)*s_c_dbk, pic_rec->v + (y - y_offset + i)*s_c_dbk + x, cuw * sizeof(pel));
            }
        }

        /* fill left */
        if(x != x_begin_uv)
        {
            for(i = 0; i < cuh; i++)
            {
                evey_mcpy(dst_u + i * s_c_dbk - x_offset, pic_rec->u + (y + i)*s_c_dbk + (x - x_offset), x_offset * sizeof(pel));
                evey_mcpy(dst_v + i * s_c_dbk - x_offset, pic_rec->v + (y + i)*s_c_dbk + (x - x_offset), x_offset * sizeof(pel));
            }
        }
    }

    /* recover */
    cuw <<= w_shift;
    cuh <<= h_shift;
    s_src <<= w_shift;
    x <<= w_shift;
    y <<= h_shift;
    x_tm <<= w_shift;
    y_tm <<= h_shift;
    log2_cuw += w_shift;
    log2_cuh += h_shift;
    log2_x_tm += w_shift;
    log2_y_tm += h_shift;

    /* add distortion of current */
    core->dist_nofilt[Y_C] += eveye_ssd_16b(log2_cuw, log2_cuh, dst_y, org_y, s_l_dbk, s_l_org, bit_depth_l);

    /* add distortion of top */
    if(y != y_begin)
    {
        core->dist_nofilt[Y_C] += eveye_ssd_16b(log2_cuw, log2_y_tm, dst_y - y_tm * s_l_dbk, org_y - y_tm * s_l_org, s_l_dbk, s_l_org, bit_depth_l);
    }

    /* add distortion of left */
    if(x != x_begin)
    {
        core->dist_nofilt[Y_C] += eveye_ssd_16b(log2_x_tm, log2_cuh, dst_y - x_tm, org_y - x_tm, s_l_dbk, s_l_org, bit_depth_l);
    }

    cuw >>= w_shift;
    cuh >>= h_shift;
    s_src >>= w_shift;
    x >>= w_shift;
    y >>= h_shift;
    x_tm >>= w_shift;
    y_tm >>= h_shift;
    log2_cuw -= w_shift;
    log2_cuh -= h_shift;
    log2_x_tm -= w_shift;
    log2_y_tm -= h_shift;

    if(chroma_format_idc != 0)
    {
        core->dist_nofilt[U_C] += eveye_ssd_16b(log2_cuw, log2_cuh, dst_u, org_u, s_c_dbk, s_c_org, bit_depth_c);
        core->dist_nofilt[V_C] += eveye_ssd_16b(log2_cuw, log2_cuh, dst_v, org_v, s_c_dbk, s_c_org, bit_depth_c);

        if(y != y_begin_uv)
        {
            core->dist_nofilt[U_C] += eveye_ssd_16b(log2_cuw, log2_y_tm, dst_u - y_tm * s_c_dbk, org_u - y_tm * s_c_org, s_c_dbk, s_c_org, bit_depth_c);
            core->dist_nofilt[V_C] += eveye_ssd_16b(log2_cuw, log2_y_tm, dst_v - y_tm * s_c_dbk, org_v - y_tm * s_c_org, s_c_dbk, s_c_org, bit_depth_c);
        }

        if(x != x_begin_uv)
        {
            core->dist_nofilt[U_C] += eveye_ssd_16b(log2_x_tm, log2_cuh, dst_u - x_tm, org_u - x_tm, s_c_dbk, s_c_org, bit_depth_c);
            core->dist_nofilt[V_C] += eveye_ssd_16b(log2_x_tm, log2_cuh, dst_v - x_tm, org_v - x_tm, s_c_dbk, s_c_org, bit_depth_c);
        }
    }

    /* recover */
    cuw <<= w_shift;
    cuh <<= h_shift;
    s_src <<= w_shift;
    x <<= w_shift;
    y <<= h_shift;
    x_tm <<= w_shift;
    y_tm <<= h_shift;
    log2_cuw += w_shift;
    log2_cuh += h_shift;
    log2_x_tm += w_shift;
    log2_y_tm += h_shift;

    /********************************* filter the pred/rec **************************************/
    if(do_filter)
    {
        int w_scu = cuw >> MIN_CU_LOG2;
        int h_scu = cuh >> MIN_CU_LOG2;
        int ind, k;
        /* save current best cu info */
        intra_flag_save = MCU_GET_IF(ctx->map_scu[t]);
        cbf_l_save = MCU_GET_CBFL(ctx->map_scu[t]);
        /* set map info of current cu to current mode */
        for(j = 0; j < h_scu; j++)
        {
            ind = (y_scu + j) * ctx->w_scu + x_scu;
            for(i = 0; i < w_scu; i++)
            {
                k = ind + i;

                if(intra_flag)
                {
                    MCU_SET_IF(ctx->map_scu[k]);
                }
                else
                {
                    MCU_CLR_IF(ctx->map_scu[k]);
                }

                if(cbf_l)
                {
                    MCU_SET_CBFL(ctx->map_scu[k]);
                }
                else
                {
                    MCU_CLR_CBFL(ctx->map_scu[k]);
                }

                if(refi != NULL && !is_mv_from_mvf)
                {
                    ctx->map_refi[k][LIST_0] = refi[LIST_0];
                    ctx->map_refi[k][LIST_1] = refi[LIST_1];
                    ctx->map_mv[k][LIST_0][MV_X] = mv[LIST_0][MV_X];
                    ctx->map_mv[k][LIST_0][MV_Y] = mv[LIST_0][MV_Y];
                    ctx->map_mv[k][LIST_1][MV_X] = mv[LIST_1][MV_X];
                    ctx->map_mv[k][LIST_1][MV_Y] = mv[LIST_1][MV_Y];
                }

                if(ctx->pps.cu_qp_delta_enabled_flag)
                {
                    MCU_RESET_QP(ctx->map_scu[k]);
                    MCU_SET_QP(ctx->map_scu[k], ctx->core->qp);
                }
                else
                {
                    MCU_SET_QP(ctx->map_scu[k], ctx->sh.qp);
                }

                /* clear coded (necessary) */
                MCU_CLR_COD(ctx->map_scu[k]);
            }
        }

        /* horizontal filtering */
        evey_deblock_cu_hor(pic_dbk, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi, ctx->map_mv, ctx->w_scu, bit_depth_l, bit_depth_c, chroma_format_idc);

        /* clean coded flag in between two directional filtering (not necessary here) */
        for(j = 0; j < h_scu; j++)
        {
            ind = (y_scu + j) * ctx->w_scu + x_scu;
            for(i = 0; i < w_scu; i++)
            {
                k = ind + i;
                MCU_CLR_COD(ctx->map_scu[k]);
            }
        }

        /* vertical filtering */
        evey_deblock_cu_ver(pic_dbk, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi, ctx->map_mv, ctx->w_scu, bit_depth_l, bit_depth_c, chroma_format_idc);

        /* recover best cu info */
        for(j = 0; j < h_scu; j++)
        {
            ind = (y_scu + j) * ctx->w_scu + x_scu;
            for(i = 0; i < w_scu; i++)
            {
                k = ind + i;

                if(intra_flag_save)
                {
                    MCU_SET_IF(ctx->map_scu[k]);
                }
                else
                {
                    MCU_CLR_IF(ctx->map_scu[k]);
                }

                if(cbf_l_save)
                {
                    MCU_SET_CBFL(ctx->map_scu[k]);
                }
                else
                {
                    MCU_CLR_CBFL(ctx->map_scu[k]);
                }

                MCU_CLR_COD(ctx->map_scu[k]);
            }
        }
    }
    /*********************** calc dist of filtered pixels *******************************/
    /* add current */
    core->dist_filter[Y_C] += eveye_ssd_16b(log2_cuw, log2_cuh, dst_y, org_y, s_l_dbk, s_l_org, bit_depth_l);

    /* add top */
    if(y != y_begin)
    {
        core->dist_filter[Y_C] += eveye_ssd_16b(log2_cuw, log2_y_tm, dst_y - y_tm * s_l_dbk, org_y - y_tm * s_l_org, s_l_dbk, s_l_org, bit_depth_l);
    }

    /* add left */
    if(x != x_begin)
    {
        core->dist_filter[Y_C] += eveye_ssd_16b(log2_x_tm, log2_cuh, dst_y - x_tm, org_y - x_tm, s_l_dbk, s_l_org, bit_depth_l);
    }

    /* modify parameters from y to uv */
    cuw >>= w_shift;
    cuh >>= h_shift;
    s_src >>= w_shift;
    x >>= w_shift;
    y >>= h_shift;
    x_tm >>= w_shift;
    y_tm >>= h_shift;
    log2_cuw -= w_shift;
    log2_cuh -= h_shift;
    log2_x_tm -= w_shift;
    log2_y_tm -= h_shift;

    if(chroma_format_idc != 0)
    {
        /* add current */
        core->dist_filter[U_C] += eveye_ssd_16b(log2_cuw, log2_cuh, dst_u, org_u, s_c_dbk, s_c_org, bit_depth_c);
        core->dist_filter[V_C] += eveye_ssd_16b(log2_cuw, log2_cuh, dst_v, org_v, s_c_dbk, s_c_org, bit_depth_c);

        /* add top */
        if(y != y_begin_uv)
        {
            core->dist_filter[U_C] += eveye_ssd_16b(log2_cuw, log2_y_tm, dst_u - y_tm * s_c_dbk, org_u - y_tm * s_c_org, s_c_dbk, s_c_org, bit_depth_c);
            core->dist_filter[V_C] += eveye_ssd_16b(log2_cuw, log2_y_tm, dst_v - y_tm * s_c_dbk, org_v - y_tm * s_c_org, s_c_dbk, s_c_org, bit_depth_c);
        }

        /* add left */
        if(x != x_begin_uv)
        {
            core->dist_filter[U_C] += eveye_ssd_16b(log2_x_tm, log2_cuh, dst_u - x_tm, org_u - x_tm, s_c_dbk, s_c_org, bit_depth_c);
            core->dist_filter[V_C] += eveye_ssd_16b(log2_x_tm, log2_cuh, dst_v - x_tm, org_v - x_tm, s_c_dbk, s_c_org, bit_depth_c);
        }
    }

    /* delta distortion */
    core->delta_dist[Y_C] = core->dist_filter[Y_C] - core->dist_nofilt[Y_C];
    core->delta_dist[U_C] = core->dist_filter[U_C] - core->dist_nofilt[U_C];
    core->delta_dist[V_C] = core->dist_filter[V_C] - core->dist_nofilt[V_C];
}
