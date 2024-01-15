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
#include "eveyd_eco.h"

#pragma warning(disable:4018)


u32 eveyd_sbac_decode_bin(EVEYD_BSR * bs, EVEYD_SBAC * sbac, SBAC_CTX_MODEL * model)
{
    u32 bin, lps, t0;
    u16 mps, state;

    state = (*model) >> 1;
    mps = (*model) & 1;

    lps = (state * (sbac->range)) >> 9;
    lps = lps < 437 ? 437 : lps;    

    bin = mps;

    sbac->range -= lps;

#if TRACE_BIN
    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("model ");
    EVEY_TRACE_INT(*model);
    EVEY_TRACE_STR("range ");
    EVEY_TRACE_INT(sbac->range);
    EVEY_TRACE_STR("lps ");
    EVEY_TRACE_INT(lps);
    EVEY_TRACE_STR("\n");
#endif

    if(sbac->value >= sbac->range)
    {
        bin = 1 - mps;
        sbac->value -= sbac->range;
        sbac->range = lps;

        state = state + ((512 - state + 16) >> 5);
        if(state > 256)
        {
            mps = 1 - mps;
            state = 512 - state;
        }
        *model = (state << 1) + mps;
    }
    else
    {
        bin = mps;
        state = state - ((state + 16) >> 5);
        *model = (state << 1) + mps;
    }

    while(sbac->range < 8192)
    {
        sbac->range <<= 1;
#if TRACE_HLS
        eveyd_bsr_read1_trace(bs, &t0, 0);
#else
        eveyd_bsr_read1(bs, &t0);
#endif
        sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;
    }

    return bin;
}

static u32 sbac_decode_bin_ep(EVEYD_BSR * bs, EVEYD_SBAC * sbac)
{
    u32 bin, t0;

    sbac->range >>= 1;

    if(sbac->value >= sbac->range)
    {
        bin = 1;
        sbac->value -= sbac->range;
    }
    else
    {        
        bin = 0;
    }

    sbac->range <<= 1;
#if TRACE_HLS
    eveyd_bsr_read1_trace(bs, &t0, 0);
#else
    eveyd_bsr_read1(bs, &t0);
#endif
    sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;

    return bin;
}

static u32 eveyd_sbac_decode_bin_trm(EVEYD_BSR * bs, EVEYD_SBAC * sbac)
{
    u32 bin, t0;

    sbac->range--;

    if(sbac->value >= sbac->range)
    {
        bin = 1;

        /*
        sbac->value -= sbac->range;
        sbac->range = 1;
        */

        while(!EVEYD_BSR_IS_BYTE_ALIGN(bs))
        {
#if TRACE_HLS
            eveyd_bsr_read1_trace(bs, &t0, 0);
#else
            eveyd_bsr_read1(bs, &t0);
#endif
            evey_assert_rv(t0 == 0, EVEY_ERR_MALFORMED_BITSTREAM);
        }
    }
    else
    {
        bin = 0;
        while(sbac->range < 8192)
        {
            sbac->range <<= 1;
#if TRACE_HLS
            eveyd_bsr_read1_trace(bs, &t0, 0);
#else
            eveyd_bsr_read1(bs, &t0);
#endif
            sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;
        }
    }

    return bin;
}

static u32 sbac_read_unary_sym_ep(EVEYD_BSR * bs, EVEYD_SBAC * sbac, u32 max_val)
{
    u32 t32u;
    u32 symbol;
    int counter = 0;

    symbol = sbac_decode_bin_ep(bs, sbac); counter++;

    if(symbol == 0)
    {
        return symbol;
    }

    symbol = 0;
    do
    {
        if(counter == max_val) t32u = 0;
        else t32u = sbac_decode_bin_ep(bs, sbac);
        counter++;
        symbol++;
    } while(t32u);

    return symbol;
}

static u32 sbac_decode_bins_ep(EVEYD_BSR *bs, EVEYD_SBAC *sbac, int num_bin)
{
    int bin = 0;
    u32 value = 0;

    for(bin = num_bin - 1; bin >= 0; bin--)
    {
        if(sbac_decode_bin_ep(bs, sbac))
        {
            value += (1 << bin);
        }
    }

    return value;
}

static u32 sbac_read_unary_sym(EVEYD_BSR * bs, EVEYD_SBAC * sbac, SBAC_CTX_MODEL * model, u32 num_ctx)
{
    u32 ctx_idx = 0;
    u32 t32u;
    u32 symbol;

    symbol = eveyd_sbac_decode_bin(bs, sbac, model);

    if(symbol == 0)
    {
        return symbol;
    }

    symbol = 0;
    do
    {
        if(ctx_idx < num_ctx - 1)
        {
            ctx_idx++;
        }
        t32u = eveyd_sbac_decode_bin(bs, sbac, &model[ctx_idx]);
        symbol++;
    } while(t32u);

    return symbol;
}

static u32 sbac_read_truncate_unary_sym(EVEYD_BSR * bs, EVEYD_SBAC * sbac, SBAC_CTX_MODEL * model, u32 num_ctx, u32 max_num)
{
    u32 ctx_idx = 0;
    u32 t32u;
    u32 symbol;

    if(max_num > 1)
    {
        for(; ctx_idx < max_num - 1; ++ctx_idx)
        {
            symbol = eveyd_sbac_decode_bin(bs, sbac, model + (ctx_idx > num_ctx - 1 ? num_ctx - 1 : ctx_idx));
            if(symbol == 0)
            {
                break;
            }
        }
    }
    t32u = ctx_idx;

    return t32u;
}

int eveyd_eco_tile_end_flag(EVEYD_BSR * bs)
{
    EVEYD_SBAC * sbac = GET_SBAC_DEC(bs);
    return eveyd_sbac_decode_bin_trm(bs, sbac);
}

static int eco_cbf(EVEYD_BSR * bs, EVEYD_SBAC * sbac, u8 pred_mode, u8 cbf[N_C], int b_no_cbf, int is_sub, int sub_pos, int *cbf_all, int chroma_format_idc)
{
    EVEY_SBAC_CTX * sbac_ctx;
    sbac_ctx = &sbac->ctx;

    /* decode allcbf */
    if (pred_mode != MODE_INTRA)
    {
        if (b_no_cbf == 0 && sub_pos == 0)
        {
            if (eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_all) == 0)
            {
                *cbf_all = 0;
                cbf[Y_C] = cbf[U_C] = cbf[V_C] = 0;

                EVEY_TRACE_COUNTER;
                EVEY_TRACE_STR("all_cbf ");
                EVEY_TRACE_INT(0);
                EVEY_TRACE_STR("\n");

                return EVEY_OK;
            }
            else
            {
                EVEY_TRACE_COUNTER;
                EVEY_TRACE_STR("all_cbf ");
                EVEY_TRACE_INT(1);
                EVEY_TRACE_STR("\n");
            }
        }

        if(chroma_format_idc != 0)
        {
            cbf[U_C] = eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cb);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf U ");
            EVEY_TRACE_INT(cbf[U_C]);
            EVEY_TRACE_STR("\n");

            cbf[V_C] = eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cr);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf V ");
            EVEY_TRACE_INT(cbf[V_C]);
            EVEY_TRACE_STR("\n");
        }
        else
        {
            cbf[U_C] = 0;
            cbf[V_C] = 0;
        }

        if (cbf[U_C] + cbf[V_C] == 0 && !is_sub)
        {
            cbf[Y_C] = 1;
        }
        else
        {
            cbf[Y_C] = eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_luma);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf Y ");
            EVEY_TRACE_INT(cbf[Y_C]);
            EVEY_TRACE_STR("\n");
        }
    }
    else
    {
        if(chroma_format_idc != 0)
        {
            cbf[U_C] = eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cb);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf U ");
            EVEY_TRACE_INT(cbf[U_C]);
            EVEY_TRACE_STR("\n");

            cbf[V_C] = eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_cr);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf V ");
            EVEY_TRACE_INT(cbf[V_C]);
            EVEY_TRACE_STR("\n");
        }
        else
        {
            cbf[U_C] = cbf[V_C] = 0;
        }

        cbf[Y_C] = eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->cbf_luma);
        EVEY_TRACE_COUNTER;
        EVEY_TRACE_STR("cbf Y ");
        EVEY_TRACE_INT(cbf[Y_C]);
        EVEY_TRACE_STR("\n");
    }

    return EVEY_OK;
}

static int eveyd_eco_run_length_cc(EVEYD_BSR * bs, EVEYD_SBAC * sbac, s16 * coef, int log2_w, int log2_h, int ch_type)
{
    EVEY_SBAC_CTX * sbac_ctx;
    int             sign, level, prev_level, run, last_flag;
    int             t0, scan_pos_offset, num_coeff, i, coef_cnt = 0;
    const u16     * scanp;
    int             ctx_last = 0;

    sbac_ctx = &sbac->ctx;
    scanp = evey_scan_tbl[COEF_SCAN_ZIGZAG][log2_w - 1][log2_h - 1];
    num_coeff = 1 << (log2_w + log2_h);
    scan_pos_offset = 0;
    prev_level = 6;

    do
    {
        t0 = ch_type == Y_C ? 0 : 2;

        /* Run parsing */
        run = sbac_read_unary_sym(bs, sbac, sbac_ctx->run + t0, 2);
        for(i = scan_pos_offset; i < scan_pos_offset + run; i++)
        {
            coef[scanp[i]] = 0;
        }
        scan_pos_offset += run;

        /* Level parsing */
        level = sbac_read_unary_sym(bs, sbac, sbac_ctx->level + t0, 2);
        level++;
        prev_level = level;

        /* Sign parsing */
        sign = sbac_decode_bin_ep(bs, sbac);
        coef[scanp[scan_pos_offset]] = sign ? -(s16)level : (s16)level;

        coef_cnt++;

        if(scan_pos_offset >= num_coeff - 1)
        {
            break;
        }
        scan_pos_offset++;

        /* Last flag parsing */
        ctx_last = (ch_type == Y_C) ? 0 : 1;
        last_flag = eveyd_sbac_decode_bin(bs, sbac, sbac_ctx->last + ctx_last);
    } while(!last_flag);

#if ENC_DEC_TRACE
    EVEY_TRACE_STR("coef luma ");
    for(scan_pos_offset = 0; scan_pos_offset < num_coeff; scan_pos_offset++)
    {
        EVEY_TRACE_INT(coef[scan_pos_offset]);
    }
    EVEY_TRACE_STR("\n");
#endif

    return EVEY_OK;
}

static int eveyd_eco_xcoef(EVEYD_BSR *bs, EVEYD_SBAC *sbac, s16 *coef, int log2_w, int log2_h, int ch_type, int is_intra)
{
    eveyd_eco_run_length_cc(bs, sbac, coef, log2_w, log2_h, (ch_type == Y_C ? 0 : 1));

#if TRACE_COEFFS
    int cuw = 1 << log2_w;
    int cuh = 1 << log2_h;
    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("Coeff for ");
    EVEY_TRACE_INT(ch_type);
    EVEY_TRACE_STR(": ");
    for (int i = 0; i < (cuw * cuh); ++i)
    {
        if (i != 0)
            EVEY_TRACE_STR(", ");
        EVEY_TRACE_INT(coef[i]);
    }
    EVEY_TRACE_STR("\n");
#endif
    return EVEY_OK;
}

static int eveyd_eco_refi(EVEYD_BSR * bs, EVEYD_SBAC * sbac, int num_refp)
{
    EVEY_SBAC_CTX * c = &sbac->ctx;
    int             ref_num = 0;

    if(num_refp > 1)
    {
        if(eveyd_sbac_decode_bin(bs, sbac, c->refi))
        {
            ref_num++;
            if(num_refp > 2 && eveyd_sbac_decode_bin(bs, sbac, c->refi + 1))
            {
                ref_num++;
                for(; ref_num < num_refp - 1; ref_num++)
                {
                    if(!sbac_decode_bin_ep(bs, sbac))
                    {
                        break;
                    }
                }
                return ref_num;
            }
        }
    }
    return ref_num;
}

static int eveyd_eco_mvp_idx(EVEYD_BSR * bs, EVEYD_SBAC * sbac)
{
    int idx;
    idx = sbac_read_truncate_unary_sym(bs, sbac, sbac->ctx.mvp_idx, 3, 4);

#if ENC_DEC_TRACE
    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("mvp idx ");
    EVEY_TRACE_INT(idx);
    EVEY_TRACE_STR("\n");
#endif

    return idx;
}

static int eveyd_eco_dqp(EVEYD_BSR * bs)
{
    EVEYD_SBAC    * sbac;
    EVEY_SBAC_CTX * sbac_ctx;
    int             dqp, sign;
    sbac = GET_SBAC_DEC(bs);
    sbac_ctx = &sbac->ctx;

    dqp = sbac_read_unary_sym(bs, sbac, sbac_ctx->delta_qp, NUM_CTX_DELTA_QP);

    if (dqp > 0)
    {
        sign = sbac_decode_bin_ep(bs, sbac);
        dqp = EVEY_SIGN_SET(dqp, sign);
    }

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("dqp ");
    EVEY_TRACE_INT(dqp);
    EVEY_TRACE_STR("\n");

    return dqp;
}

static u32 eveyd_eco_abs_mvd(EVEYD_BSR *bs, EVEYD_SBAC *sbac, SBAC_CTX_MODEL *model)
{
    u32 val = 0;
    u32 code = 0;
    u32 len;

    code = eveyd_sbac_decode_bin(bs, sbac, model); /* use one model */

    if(code == 0)
    {
        len = 0;

        while(!(code & 1))
        {
            if(len == 0)
                code = eveyd_sbac_decode_bin(bs, sbac, model);
            else
                code = sbac_decode_bin_ep(bs, sbac);
            len++;
        }
        val = (1 << len) - 1;

        while(len != 0)
        {
            if(len == 0)
                code = eveyd_sbac_decode_bin(bs, sbac, model);
            else
                code = sbac_decode_bin_ep(bs, sbac);
            val += (code << (--len));
        }
    }

    return val;
}

static int eveyd_eco_get_mvd(EVEYD_BSR * bs, EVEYD_SBAC * sbac, s16 mvd[MV_D])
{
    u32 sign;
    s16 t16;

    /* MV_X */
    t16 = (s16)eveyd_eco_abs_mvd(bs, sbac, sbac->ctx.mvd);

    if(t16 == 0) mvd[MV_X] = 0;
    else
    {
        /* sign */
        sign = sbac_decode_bin_ep(bs, sbac);

        if(sign) mvd[MV_X] = -t16;
        else mvd[MV_X] = t16;
    }

    /* MV_Y */
    t16 = (s16)eveyd_eco_abs_mvd(bs, sbac, sbac->ctx.mvd);

    if(t16 == 0)
    {
        mvd[MV_Y] = 0;
    }
    else
    {
        /* sign */
        sign = sbac_decode_bin_ep(bs, sbac);

        if(sign) mvd[MV_Y] = -t16;
        else mvd[MV_Y] = t16;
    }

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("mvd x ");
    EVEY_TRACE_INT(mvd[MV_X]);
    EVEY_TRACE_STR("mvd y ");
    EVEY_TRACE_INT(mvd[MV_Y]);
    EVEY_TRACE_STR("\n");

    return EVEY_OK;
}

int eveyd_eco_coef(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    EVEYD_SBAC * sbac;
    EVEYD_BSR  * bs;
    int          w_shift = (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int          h_shift = (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));
    int          ret;
    int          b_no_cbf = 0;
    int          log2_tuw = core->log2_cuw;
    int          log2_tuh = core->log2_cuh;
    u8           cbf[N_C];
    s16        * coef_temp[N_C];
    s16       (* coef_temp_buf)[MAX_TR_DIM] = core->coef_temp;
    int          i, j, c;
    int          log2_w_sub = (core->log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : core->log2_cuw;
    int          log2_h_sub = (core->log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : core->log2_cuh;
    int          loop_w = (core->log2_cuw > MAX_TR_LOG2) ? (1 << (core->log2_cuw - MAX_TR_LOG2)) : 1;
    int          loop_h = (core->log2_cuh > MAX_TR_LOG2) ? (1 << (core->log2_cuh - MAX_TR_LOG2)) : 1;
    int          stride = (1 << core->log2_cuw);
    int          sub_stride = (1 << log2_w_sub);
    int          tmp_coef[N_C] = {0};
    int          is_sub = loop_h + loop_w > 2 ? 1 : 0;
    int          cbf_all = 1;
    u8           is_intra = (core->pred_mode == MODE_INTRA) ? 1 : 0;

    bs = &ctx->bs;
    sbac = GET_SBAC_DEC(bs);    

    evey_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);

    for (j = 0; j < loop_h; j++)
    {
        for (i = 0; i < loop_w; i++)
        {
            if(cbf_all)
            {
                ret = eco_cbf(bs, sbac, core->pred_mode, cbf, b_no_cbf, is_sub, j + i, &cbf_all, ctx->sps.chroma_format_idc);
                evey_assert_rv(ret == EVEY_OK, ret);
            }
            else
            {
                cbf[Y_C] = cbf[U_C] = cbf[V_C] = 0;
            }            
   
            int dqp;
            int qp_i_cb, qp_i_cr;
            if(ctx->pps.cu_qp_delta_enabled_flag && (cbf[Y_C] || cbf[U_C] || cbf[V_C]))
            {
                dqp = eveyd_eco_dqp(bs);
                core->qp = GET_QP(ctx->sh.qp_prev_eco, dqp);
                core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
                ctx->sh.qp_prev_eco = core->qp;
            }
            else
            {
                dqp = 0;
                core->qp = GET_QP(ctx->sh.qp_prev_eco, dqp);
                core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
            }

            qp_i_cb = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
            qp_i_cr = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
            core->qp_u = p_evey_tbl_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps.bit_depth_chroma_minus8;
            core->qp_v = p_evey_tbl_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps.bit_depth_chroma_minus8;            

            for (c = 0; c < N_C; c++)
            {
                if (cbf[c])
                {
                    int pos_sub_x = c == 0 ? (i * (1 << (log2_w_sub))) : (i * (1 << (log2_w_sub - w_shift)));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - h_shift)) * (stride >> w_shift);

                    if (is_sub)
                    {
                        if(c == 0)
                        {
                            evey_block_copy(core->coef[c] + pos_sub_x + pos_sub_y, stride, coef_temp_buf[c], sub_stride, log2_w_sub, log2_h_sub);
                        }
                        else
                        {
                            evey_block_copy(core->coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, coef_temp_buf[c], sub_stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        }
                        coef_temp[c] = coef_temp_buf[c];
                    }
                    else
                    {
                        coef_temp[c] = core->coef[c];
                    }

                    if(c == 0)
                    {
                        eveyd_eco_xcoef(bs, sbac, coef_temp[c], log2_w_sub, log2_h_sub, c, core->pred_mode == MODE_INTRA);
                    }
                    else
                    {
                        eveyd_eco_xcoef(bs, sbac, coef_temp[c], log2_w_sub - w_shift, log2_h_sub - h_shift, c, core->pred_mode == MODE_INTRA);
                    }

                    evey_assert_rv(ret == EVEY_OK, ret);

                    core->nnz_sub[c][(j << 1) | i] = 1;
                    tmp_coef[c] += 1;

                    if (is_sub)
                    {
                        if(c == 0)
                        {
                            evey_block_copy(coef_temp_buf[c], sub_stride, core->coef[c] + pos_sub_x + pos_sub_y, stride, log2_w_sub, log2_h_sub);
                        }
                        else
                        {
                            evey_block_copy(coef_temp_buf[c], sub_stride >> w_shift, core->coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        }
                    }
                }
                else
                {
                    core->nnz_sub[c][(j << 1) | i] = 0;
                    tmp_coef[c] += 0;
                }
            }
        }
    }
    for (c = 0; c < N_C; c++)
    {
        core->nnz[c] = tmp_coef[c] ? 1 : 0;
    }

    return EVEY_OK;
}

void eveyd_sbac_reset(EVEYD_CTX * ctx)
{
    EVEYD_BSR  * bs = &ctx->bs;    
    EVEYD_SBAC * sbac = GET_SBAC_DEC(bs);
    u8           slice_type = ctx->sh.slice_type;
    u8           slice_qp = ctx->sh.qp;

    /* Initialization of the context models */
    evey_eco_init_ctx_model(&sbac->ctx);

    /* Initialization of the internal variables */
    int i;
    u32 t0;
    sbac->range = 16384;
    sbac->value = 0;
    for(i = 0; i < 14; i++)
    {
#if TRACE_HLS
        eveyd_bsr_read1_trace(bs, &t0, 0);
#else
        eveyd_bsr_read1(bs, &t0);
#endif
        sbac->value = ((sbac->value << 1) | t0) & 0xFFFF;
    }
}

static int eveyd_eco_intra_dir(EVEYD_BSR * bs, EVEYD_SBAC * sbac, u8 * mpm)
{
    u32 t0;
    int ipm = 0;
    int i;

    t0 = sbac_read_unary_sym(bs, sbac, sbac->ctx.intra_dir, 2);    
    for(i = 0; i < IPD_CNT; i++)
    {
        if(t0 == mpm[i])
        {
            ipm = i;
        }
    }

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("ipm Y ");
    EVEY_TRACE_INT(ipm);
    EVEY_TRACE_STR("\n");
    return ipm;
}

void eveyd_eco_direct_mode_flag(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    EVEYD_SBAC * sbac;
    EVEYD_BSR  * bs;
    int          direct_mode_flag = 0;

    bs = &ctx->bs;
    sbac = GET_SBAC_DEC(bs);

    direct_mode_flag = eveyd_sbac_decode_bin(bs, sbac, sbac->ctx.direct_mode_flag);

    if(direct_mode_flag)
    {
        core->pred_mode = MODE_DIR;
    }
    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("direct_mode_flag ");
    EVEY_TRACE_INT(core->pred_mode);
    EVEY_TRACE_STR("\n");
}

void eveyd_eco_inter_pred_idc(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    int          tmp = 1;
    EVEYD_SBAC * sbac;
    EVEYD_BSR  * bs;

    bs = &ctx->bs;
    sbac = GET_SBAC_DEC(bs);

    if(ctx->sh.slice_type == SLICE_B)
    {
        tmp = eveyd_sbac_decode_bin(bs, sbac, sbac->ctx.inter_dir);
    }

    if (!tmp)
    {
        core->inter_dir = PRED_BI;
    }
    else
    {
        tmp = eveyd_sbac_decode_bin(bs, sbac, sbac->ctx.inter_dir + 1);
        core->inter_dir = tmp ? PRED_L1 : PRED_L0;
    }

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("inter dir ");
    EVEY_TRACE_INT(core->inter_dir);
    EVEY_TRACE_STR("\n");
}

s8 eveyd_eco_split_mode(EVEYD_BSR * bs)
{
    EVEYD_SBAC * sbac = GET_SBAC_DEC(bs);
    s8           split_mode = NO_SPLIT;

    split_mode = eveyd_sbac_decode_bin(bs, sbac, sbac->ctx.split_cu_flag); /* split_cu_flag */
    split_mode = split_mode ? SPLIT_QUAD : NO_SPLIT;

    return split_mode;
}

void eveyd_eco_pred_mode( EVEYD_CTX * ctx, EVEYD_CORE * core )
{
    EVEYD_BSR  * bs = &ctx->bs;
    EVEYD_SBAC * sbac = GET_SBAC_DEC( bs );
    int          pred_mode_flag = 0;

    if(ctx->sh.slice_type != SLICE_I)
    {
        pred_mode_flag = eveyd_sbac_decode_bin(bs, sbac, sbac->ctx.pred_mode);
        EVEY_TRACE_COUNTER;
        EVEY_TRACE_STR("pred mode ");
        EVEY_TRACE_INT(pred_mode_flag ? MODE_INTRA : MODE_INTER);
        EVEY_TRACE_STR("\n");
    }
    else
    {
        pred_mode_flag = 1;
    }

    core->pred_mode = pred_mode_flag ? MODE_INTRA : MODE_INTER;
}

void eveyd_eco_cu_skip_flag(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    EVEYD_SBAC * sbac;
    EVEYD_BSR  * bs;
    int          cu_skip_flag = 0;

    bs = &ctx->bs;
    sbac = GET_SBAC_DEC(bs);

    cu_skip_flag = eveyd_sbac_decode_bin(bs, sbac, sbac->ctx.skip_flag); /* cu_skip_flag */

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("skip flag ");
    EVEY_TRACE_INT(cu_skip_flag);
    EVEY_TRACE_STR("ctx ");
    EVEY_TRACE_INT(0);
    EVEY_TRACE_STR("\n");

    if (cu_skip_flag)
    {
        core->pred_mode = MODE_SKIP;
    }
}

int eveyd_eco_cu(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    EVEYD_SBAC * sbac;
    EVEYD_BSR  * bs;
    int          ret, cuw, cuh, mvp_idx[LIST_NUM] = { 0, 0 };
    
    core->pred_mode = MODE_INTRA;
    core->mvp_idx[LIST_0] = 0;
    core->mvp_idx[LIST_1] = 0;
    core->inter_dir = 0;
    evey_mset(core->mvd, 0, sizeof(s16) * LIST_NUM * MV_D);
    bs = &ctx->bs;
    sbac = GET_SBAC_DEC(bs);
    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);

    if(ctx->sh.slice_type != SLICE_I)
    {
        /* CU skip flag */
        eveyd_eco_cu_skip_flag(ctx, core);
    }
       
    /* parse prediction info */
    if (core->pred_mode == MODE_SKIP)
    {
        core->mvp_idx[LIST_0] = eveyd_eco_mvp_idx(bs, sbac);
        if(ctx->sh.slice_type == SLICE_B)
        {
            core->mvp_idx[LIST_1] = eveyd_eco_mvp_idx(bs, sbac);
        }

        core->nnz[Y_C] = core->nnz[U_C] = core->nnz[V_C] = 0;
        evey_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);

        if(ctx->pps.cu_qp_delta_enabled_flag)
        {
            int qp_i_cb, qp_i_cr;
            core->qp = ctx->sh.qp_prev_eco;
            core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
            qp_i_cb = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
            qp_i_cr = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
            core->qp_u = p_evey_tbl_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps.bit_depth_chroma_minus8;
            core->qp_v = p_evey_tbl_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps.bit_depth_chroma_minus8;
        }
        else
        {
            int qp_i_cb, qp_i_cr;
            core->qp = ctx->sh.qp;
            core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
            qp_i_cb = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_u_offset);
            qp_i_cr = EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh.qp_v_offset);
            core->qp_u = p_evey_tbl_qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps.bit_depth_chroma_minus8;
            core->qp_v = p_evey_tbl_qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps.bit_depth_chroma_minus8;
        }
    }
    else
    {
        eveyd_eco_pred_mode(ctx, core);

        if(core->pred_mode == MODE_INTER)
        {
            if (ctx->sh.slice_type == SLICE_B)
            {
                eveyd_eco_direct_mode_flag(ctx, core);
            }

            if(core->pred_mode != MODE_DIR)
            {
                if(ctx->sh.slice_type == SLICE_B)
                {
                    eveyd_eco_inter_pred_idc(ctx, core); /* inter_pred_idc */
                }

                for(int inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
                {
                    /* 0: forward, 1: backward */
                    if(((core->inter_dir + 1) >> inter_dir_idx) & 1)
                    {
                        core->refi[inter_dir_idx] = eveyd_eco_refi(bs, sbac, ctx->dpbm.num_refp[inter_dir_idx]);
                        core->mvp_idx[inter_dir_idx] = eveyd_eco_mvp_idx(bs, sbac);
                        eveyd_eco_get_mvd(bs, sbac, core->mvd[inter_dir_idx]);
                    }
                }
            }
        }
        else if (core->pred_mode == MODE_INTRA)
        {
            evey_get_mpm(ctx, core);

            int luma_ipm = IPD_DC;

            core->ipm[0] = eveyd_eco_intra_dir(bs, sbac, core->mpm_b_list);
            luma_ipm = core->ipm[0];
            core->ipm[1] = luma_ipm;

            SET_REFI(core->refi, REFI_INVALID, REFI_INVALID);

            core->mv[LIST_0][MV_X] = core->mv[LIST_0][MV_Y] = 0;
            core->mv[LIST_1][MV_X] = core->mv[LIST_1][MV_Y] = 0;
        }
        else
        {
            evey_assert_rv(0, EVEY_ERR_MALFORMED_BITSTREAM);
        }

        /* clear coefficient buffer */
        evey_mset(core->coef[Y_C], 0, cuw * cuh * sizeof(s16));
        evey_mset(core->coef[U_C], 0, (cuw >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc))) * (cuh >> (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc))) * sizeof(s16));
        evey_mset(core->coef[V_C], 0, (cuw >> (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc))) * (cuh >> (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc))) * sizeof(s16));

        /* parse coefficients */
        ret = eveyd_eco_coef(ctx, core);
        evey_assert_rv(ret == EVEY_OK, ret);
    }

    return EVEY_OK;
}

int eveyd_eco_nalu(EVEYD_BSR * bs, EVEY_NALU * nalu)
{
    eveyd_bsr_read(bs, &nalu->forbidden_zero_bit, 1);

    if (nalu->forbidden_zero_bit != 0)
    {
        evey_trace("malformed bitstream: forbidden_zero_bit != 0\n");
        return EVEY_ERR_MALFORMED_BITSTREAM;
    }

    eveyd_bsr_read(bs, &nalu->nal_unit_type_plus1, 6);
    if(nalu->nal_unit_type_plus1 == 0)
    {
        evey_trace("malformed bitstream: nal_unit_type_plus1 == 0\n");
        return EVEY_ERR_MALFORMED_BITSTREAM;
    }

    eveyd_bsr_read(bs, &nalu->nuh_temporal_id, 3);
    eveyd_bsr_read(bs, &nalu->nuh_reserved_zero_5bits, 5);
    if (nalu->nuh_reserved_zero_5bits != 0)
    {
        evey_trace("malformed bitstream: nuh_reserved_zero_5bits != 0");
        return EVEY_ERR_MALFORMED_BITSTREAM;
    }

    eveyd_bsr_read(bs, &nalu->nuh_extension_flag, 1);
    if (nalu->nuh_extension_flag != 0)
    {
        evey_trace("malformed bitstream: nuh_extension_flag != 0");
        return EVEY_ERR_MALFORMED_BITSTREAM;
    }

    return EVEY_OK;
}

int eveyd_eco_hrd_parameters(EVEYD_BSR * bs, EVEY_HRD * hrd)
{
    eveyd_bsr_read_ue(bs, &hrd->cpb_cnt_minus1);
    eveyd_bsr_read(bs, &hrd->bit_rate_scale, 4);
    eveyd_bsr_read(bs, &hrd->cpb_size_scale, 4);
    for (int sched_sel_idx = 0; sched_sel_idx <= hrd->cpb_cnt_minus1; sched_sel_idx++)
    {
        eveyd_bsr_read_ue(bs, &hrd->bit_rate_value_minus1[sched_sel_idx]);
        eveyd_bsr_read_ue(bs, &hrd->cpb_size_value_minus1[sched_sel_idx]);
        eveyd_bsr_read1(bs, &hrd->cbr_flag[sched_sel_idx]);
    }
    eveyd_bsr_read(bs, &hrd->initial_cpb_removal_delay_length_minus1, 5);
    eveyd_bsr_read(bs, &hrd->cpb_removal_delay_length_minus1, 5);
    eveyd_bsr_read(bs, &hrd->cpb_removal_delay_length_minus1, 5);
    eveyd_bsr_read(bs, &hrd->time_offset_length, 5);

    return EVEY_OK;
}

int eveyd_eco_vui(EVEYD_BSR * bs, EVEY_VUI * vui)
{
    eveyd_bsr_read1(bs, &vui->aspect_ratio_info_present_flag);
    if(vui->aspect_ratio_info_present_flag)
    {
        eveyd_bsr_read(bs, &vui->aspect_ratio_idc, 8);
        if(vui->aspect_ratio_idc == EXTENDED_SAR)
        {
            eveyd_bsr_read(bs, &vui->sar_width, 16);
            eveyd_bsr_read(bs, &vui->sar_height, 16);
        }
    }
    eveyd_bsr_read1(bs, &vui->overscan_info_present_flag);
    if(vui->overscan_info_present_flag)
    {
        eveyd_bsr_read1(bs, &vui->overscan_appropriate_flag);
    }
    eveyd_bsr_read1(bs, &vui->video_signal_type_present_flag);
    if(vui->video_signal_type_present_flag)
    {
        eveyd_bsr_read(bs, &vui->video_format, 3);
        eveyd_bsr_read1(bs, &vui->video_full_range_flag);
        eveyd_bsr_read1(bs, &vui->colour_description_present_flag);
        if(vui->colour_description_present_flag)
        {
            eveyd_bsr_read(bs, &vui->colour_primaries, 8);
            eveyd_bsr_read(bs, &vui->transfer_characteristics, 8);
            eveyd_bsr_read(bs, &vui->matrix_coefficients, 8);
        }
    }
    eveyd_bsr_read1(bs, &vui->chroma_loc_info_present_flag);
    if(vui->chroma_loc_info_present_flag)
    {
        eveyd_bsr_read_ue(bs, &vui->chroma_sample_loc_type_top_field);
        eveyd_bsr_read_ue(bs, &vui->chroma_sample_loc_type_bottom_field);
    }
    eveyd_bsr_read1(bs, &vui->neutral_chroma_indication_flag);
    eveyd_bsr_read1(bs, &vui->field_seq_flag);
    eveyd_bsr_read1(bs, &vui->timing_info_present_flag);
    if(vui->timing_info_present_flag)
    {
        eveyd_bsr_read(bs, &vui->num_units_in_tick, 32);
        eveyd_bsr_read(bs, &vui->time_scale, 32);
        eveyd_bsr_read1(bs, &vui->fixed_pic_rate_flag);
    }
    eveyd_bsr_read1(bs, &vui->nal_hrd_parameters_present_flag);
    if(vui->nal_hrd_parameters_present_flag)
    {
        eveyd_eco_hrd_parameters(bs, &vui->hrd_parameters);
    }
    eveyd_bsr_read1(bs, &vui->vcl_hrd_parameters_present_flag);
    if(vui->vcl_hrd_parameters_present_flag)
    {
        eveyd_eco_hrd_parameters(bs, &vui->hrd_parameters);
    }
    if(vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
    {
        eveyd_bsr_read1(bs, &vui->low_delay_hrd_flag);
    }
    eveyd_bsr_read1(bs, &vui->pic_struct_present_flag);
    eveyd_bsr_read1(bs, &vui->bitstream_restriction_flag);
    if(vui->bitstream_restriction_flag)
    {
        eveyd_bsr_read1(bs, &vui->motion_vectors_over_pic_boundaries_flag);
        eveyd_bsr_read_ue(bs, &vui->max_bytes_per_pic_denom);
        eveyd_bsr_read_ue(bs, &vui->max_bits_per_mb_denom);
        eveyd_bsr_read_ue(bs, &vui->log2_max_mv_length_horizontal);
        eveyd_bsr_read_ue(bs, &vui->log2_max_mv_length_vertical);
        eveyd_bsr_read_ue(bs, &vui->num_reorder_pics);
        eveyd_bsr_read_ue(bs, &vui->max_dec_pic_buffering);
    }

    return EVEY_OK;
}

int eveyd_eco_sps(EVEYD_BSR * bs, EVEY_SPS * sps)
{
#if TRACE_HLS
    EVEY_TRACE_STR("***********************************\n");
    EVEY_TRACE_STR("************ SPS Start ************\n");
#endif
    eveyd_bsr_read_ue(bs, &sps->sps_seq_parameter_set_id);
    eveyd_bsr_read(bs, &sps->profile_idc, 8);
    eveyd_bsr_read(bs, &sps->level_idc, 8);
    eveyd_bsr_read(bs, &sps->toolset_idc_h, 32);
    eveyd_bsr_read(bs, &sps->toolset_idc_l, 32);
    eveyd_bsr_read_ue(bs, &sps->chroma_format_idc);
    eveyd_bsr_read_ue(bs, &sps->pic_width_in_luma_samples);
    eveyd_bsr_read_ue(bs, &sps->pic_height_in_luma_samples);
    eveyd_bsr_read_ue(bs, &sps->bit_depth_luma_minus8);
    eveyd_bsr_read_ue(bs, &sps->bit_depth_chroma_minus8);
    eveyd_bsr_read1(bs, &sps->sps_btt_flag);      /* Should be 0 */
    evey_assert(sps->sps_btt_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_suco_flag);     /* Should be 0 */
    evey_assert(sps->sps_suco_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_admvp_flag);    /* Should be 0 */
    evey_assert(sps->sps_admvp_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_eipd_flag);     /* Should be 0 */
    evey_assert(sps->sps_eipd_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_cm_init_flag);  /* Should be 0 */
    evey_assert(sps->sps_cm_init_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_iqt_flag);      /* Should be 0 */
    evey_assert(sps->sps_iqt_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_addb_flag);     /* Should be 0 */
    evey_assert(sps->sps_addb_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_alf_flag);      /* Should be 0 */
    evey_assert(sps->sps_alf_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_htdf_flag);     /* Should be 0 */
    evey_assert(sps->sps_htdf_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_rpl_flag);      /* Should be 0 */
    evey_assert(sps->sps_rpl_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_pocs_flag);     /* Should be 0 */
    evey_assert(sps->sps_pocs_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_dquant_flag);   /* Should be 0 */
    evey_assert(sps->sps_dquant_flag == 0);
    eveyd_bsr_read1(bs, &sps->sps_dra_flag);      /* Should be 0 */
    evey_assert(sps->sps_dra_flag == 0);

    if (!sps->sps_rpl_flag || !sps->sps_pocs_flag)
    {
        eveyd_bsr_read_ue(bs, &sps->log2_sub_gop_length);
        if (sps->log2_sub_gop_length == 0)
        {
            eveyd_bsr_read_ue(bs, &sps->log2_ref_pic_gap_length);
        }
    }

    if (!sps->sps_rpl_flag)
    {
         eveyd_bsr_read_ue(bs, &sps->max_num_ref_pics);
    }

    eveyd_bsr_read1(bs, &sps->picture_cropping_flag);
    if(sps->picture_cropping_flag)
    {
        eveyd_bsr_read_ue(bs, &sps->picture_crop_left_offset);
        eveyd_bsr_read_ue(bs, &sps->picture_crop_right_offset);
        eveyd_bsr_read_ue(bs, &sps->picture_crop_top_offset);
        eveyd_bsr_read_ue(bs, &sps->picture_crop_bottom_offset);
    }

    if(sps->chroma_format_idc != 0)
    {
        eveyd_bsr_read1(bs, &sps->chroma_qp_table_struct.chroma_qp_table_present_flag);
        if(sps->chroma_qp_table_struct.chroma_qp_table_present_flag)
        {
            eveyd_bsr_read1(bs, &sps->chroma_qp_table_struct.same_qp_table_for_chroma);
            eveyd_bsr_read1(bs, &sps->chroma_qp_table_struct.global_offset_flag);
            for(int i = 0; i < (sps->chroma_qp_table_struct.same_qp_table_for_chroma ? 1 : 2); i++)
            {
                eveyd_bsr_read_ue(bs, &sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]);
                for(int j = 0; j <= sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]; j++)
                {
                    eveyd_bsr_read(bs, &sps->chroma_qp_table_struct.delta_qp_in_val_minus1[i][j], 6);
                    eveyd_bsr_read_se(bs, &sps->chroma_qp_table_struct.delta_qp_out_val[i][j]);
                }
            }
        }
    }

    eveyd_bsr_read1(bs, &sps->vui_parameters_present_flag);
    if(sps->vui_parameters_present_flag)
    {
        eveyd_eco_vui(bs, &(sps->vui_parameters));
    }

    u32 t0;
    while(!EVEYD_BSR_IS_BYTE_ALIGN(bs))
    {
        eveyd_bsr_read1(bs, &t0);
    }

#if TRACE_HLS
    EVEY_TRACE_STR("************ SPS End   ************\n");
    EVEY_TRACE_STR("***********************************\n");
#endif

    return EVEY_OK;
}

int eveyd_eco_pps(EVEYD_BSR * bs, EVEY_SPS * sps, EVEY_PPS * pps)
{
#if TRACE_HLS
    EVEY_TRACE_STR("***********************************\n");
    EVEY_TRACE_STR("************ PPS Start ************\n");
#endif
    eveyd_bsr_read_ue(bs, &pps->pps_pic_parameter_set_id);
    evey_assert(pps->pps_pic_parameter_set_id >= 0 && pps->pps_pic_parameter_set_id < MAX_NUM_PPS);
    eveyd_bsr_read_ue(bs, &pps->pps_seq_parameter_set_id);
    eveyd_bsr_read_ue(bs, &pps->num_ref_idx_default_active_minus1[0]);
    eveyd_bsr_read_ue(bs, &pps->num_ref_idx_default_active_minus1[1]);
    eveyd_bsr_read_ue(bs, &pps->additional_lt_poc_lsb_len);
    eveyd_bsr_read1(bs, &pps->rpl1_idx_present_flag);
    eveyd_bsr_read1(bs, &pps->single_tile_in_pic_flag); /* Should be 1 */
    evey_assert(pps->single_tile_in_pic_flag == 1);
    eveyd_bsr_read_ue(bs, &pps->tile_id_len_minus1);    /* Not used, but should be read */
    eveyd_bsr_read1(bs, &pps->explicit_tile_id_flag);   /* Not used, but should be read */
    if(pps->explicit_tile_id_flag)
    {
        for(int i = 0; i <= pps->num_tile_rows_minus1; ++i)
        {
            for(int j = 0; j <= pps->num_tile_columns_minus1; ++j)
            {
                eveyd_bsr_read(bs, &pps->tile_id_val[i][j], pps->tile_id_len_minus1 + 1); /* Not used, but should be read */
            }
        }
    }

    pps->pic_dra_enabled_flag = 0;
    eveyd_bsr_read1(bs, &pps->pic_dra_enabled_flag); /* Should be 0 */
    evey_assert(pps->pic_dra_enabled_flag == 0);
    eveyd_bsr_read1(bs, &pps->arbitrary_slice_present_flag); /* Should be 0 */
    evey_assert(pps->arbitrary_slice_present_flag == 0);
    eveyd_bsr_read1(bs, &pps->constrained_intra_pred_flag);
    eveyd_bsr_read1(bs, &pps->cu_qp_delta_enabled_flag);
    if(pps->cu_qp_delta_enabled_flag)
    {
        eveyd_bsr_read_ue(bs, &pps->cu_qp_delta_area); /* Not used, but should be read */
        pps->cu_qp_delta_area += 6;
    }

    u32 t0;
    while(!EVEYD_BSR_IS_BYTE_ALIGN(bs))
    {
        eveyd_bsr_read1(bs, &t0);
    }

#if TRACE_HLS    
    EVEY_TRACE_STR("************ PPS End   ************\n");
    EVEY_TRACE_STR("***********************************\n");
#endif

    return EVEY_OK;
}

int eveyd_eco_sh(EVEYD_BSR * bs, EVEY_SPS * sps, EVEY_PPS * pps, EVEY_SH * sh, int nut)
{
#if TRACE_HLS    
    EVEY_TRACE_STR("***********************************\n");
    EVEY_TRACE_STR("************ SH  Start ************\n");
#endif
    int num_tiles_in_slice = 0;

    eveyd_bsr_read_ue(bs, &sh->slice_pic_parameter_set_id);
    assert(sh->slice_pic_parameter_set_id >= 0 && sh->slice_pic_parameter_set_id < MAX_NUM_PPS);

    eveyd_bsr_read_ue(bs, &sh->slice_type);

    if(nut == EVEY_IDR_NUT)
    {
        eveyd_bsr_read1(bs, &sh->no_output_of_prior_pics_flag);
    }

    if(sh->slice_type != SLICE_I)
    {
        eveyd_bsr_read1(bs, &sh->num_ref_idx_active_override_flag);
        if(sh->num_ref_idx_active_override_flag)
        {
            u32 num_ref_idx_active_minus1;
            eveyd_bsr_read_ue(bs, &num_ref_idx_active_minus1);
            if(sh->slice_type == SLICE_B)
            {
                eveyd_bsr_read_ue(bs, &num_ref_idx_active_minus1);
            }

            /* Need to do something? Seems no */
        }
        else
        {
            /* Need to do something? Seems no */
        }
    }
    eveyd_bsr_read1(bs, &sh->slice_deblocking_filter_flag);

    eveyd_bsr_read(bs, &sh->qp, 6);
    if(sh->qp < 0 || sh->qp > 51)
    {
        evey_trace("malformed bitstream: slice_qp should be in the range of 0 to 51\n");
        return EVEY_ERR_MALFORMED_BITSTREAM;
    }

    eveyd_bsr_read_se(bs, &sh->qp_u_offset);
    eveyd_bsr_read_se(bs, &sh->qp_v_offset);

    sh->qp_u = (s8)EVEY_CLIP3(-6 * sps->bit_depth_luma_minus8, 57, sh->qp + sh->qp_u_offset);
    sh->qp_v = (s8)EVEY_CLIP3(-6 * sps->bit_depth_luma_minus8, 57, sh->qp + sh->qp_v_offset);

    /* byte align */
    u32 t0;
    while(!EVEYD_BSR_IS_BYTE_ALIGN(bs))
    {
        eveyd_bsr_read1(bs, &t0);
        evey_assert_rv(0 == t0, EVEY_ERR_MALFORMED_BITSTREAM);
    }

#if TRACE_HLS    
    EVEY_TRACE_STR("************ SH  End   ************\n");
    EVEY_TRACE_STR("***********************************\n");
#endif

    return EVEY_OK;
}

int eveyd_eco_sei(EVEYD_CTX * ctx, EVEYD_BSR * bs)
{
    u32 payload_type, payload_size;
    u32 pic_sign[N_C][16];

    /* should be aligned before adding user data */
    evey_assert_rv(EVEYD_BSR_IS_BYTE_ALIGN(bs), EVEY_ERR_UNKNOWN);

    eveyd_bsr_read(bs, &payload_type, 8);
    eveyd_bsr_read(bs, &payload_size, 8);

    switch(payload_type)
    {
        case EVEY_UD_PIC_SIGNATURE:
            /* read signature (HASH) from bitstream */
            for(int i = 0; i < ctx->pic[0].imgb->np; ++i)
            {
                for(int j = 0; j < payload_size; ++j)
                {
                    eveyd_bsr_read(bs, &pic_sign[i][j], 8);
                    ctx->pic_sign[i][j] = pic_sign[i][j];
                }
            }
            ctx->pic_sign_exist = 1;
            break;

        default:
            evey_assert_rv(0, EVEY_ERR_UNEXPECTED);
    }

    return EVEY_OK;
}
