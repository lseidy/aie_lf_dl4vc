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
#include "eveye_pintra.h"
// XXNN
#include "eveye_networking.h"

static double pintra_residue_rdo(EVEYE_CTX * ctx, EVEYE_CORE * core, s32 * dist, int mode, int x, int y)
{
    EVEYE_PINTRA * pi = &ctx->pintra;
    double         cost = 0;
    int            bit_depth;
    int            log2_cuw, log2_cuh;    
    int            cuw, cuh;
    u32            bit_cnt;

    if(mode == 0) /* luma */
    {        
        pel * pred_cache = pi->pred_cache[core->ipm[0]]; /* get prediction from cache buffer */

        bit_depth = ctx->sps.bit_depth_luma_minus8 + 8;
        log2_cuw = core->log2_cuw;
        log2_cuh = core->log2_cuh;
        cuw = 1 << log2_cuw;
        cuh = 1 << log2_cuh;
        
        /* get residual */
        eveye_diff_16b(log2_cuw, log2_cuh, core->org[Y_C], pred_cache, cuw, cuw, cuw, pi->coef_tmp[Y_C], bit_depth);

        /* transform and quantization */
        eveye_sub_block_tq(ctx, core, pi->coef_tmp, 1, RUN_L);
        evey_mcpy(core->coef[Y_C], pi->coef_tmp[Y_C], sizeof(s16) * (cuw * cuh));

        /* inverse quantization and inverse transform */
        evey_mset(core->nnz_sub[U_C], 0, sizeof(int) * MAX_SUB_TB_NUM);
        evey_mset(core->nnz_sub[V_C], 0, sizeof(int) * MAX_SUB_TB_NUM);
        evey_sub_block_itdq(ctx, core, pi->coef_tmp, core->nnz_sub);

        /* reconstruction */
        evey_recon(pi->coef_tmp[Y_C], pred_cache, core->nnz[Y_C], cuw, cuh, cuw, pi->rec[Y_C], bit_depth);
        
        /* calculate distoration */
        cost += eveye_ssd_16b(log2_cuw, log2_cuh, pi->rec[Y_C], core->org[Y_C], cuw, cuw, bit_depth);
        if(ctx->cdsc.rdo_dbk_switch)
        {
            eveye_calc_delta_dist_filter_boundary(ctx, core, pi->rec, 1 << core->log2_cuw, x, y, 1, core->nnz[Y_C] != 0, NULL, NULL, 0);
            cost += core->delta_dist[Y_C];
        }
        *dist = (s32)cost; 

        /* calculate bit cost */
        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
        eveye_sbac_bit_reset(&core->s_temp_run);
        eveye_rdo_bit_cnt_cu_intra_luma(ctx, core, core->coef);
        bit_cnt = eveye_get_bit_number(&core->s_temp_run);
        cost += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);
    }
    else if(mode == 1) /* chroma */
    {
        bit_depth = ctx->sps.bit_depth_chroma_minus8 + 8;
        log2_cuw = core->log2_cuw - GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc);
        log2_cuh = core->log2_cuh - GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc);
        cuw = 1 << log2_cuw;
        cuh = 1 << log2_cuh;

        /* intra prediction */
        evey_intra_pred_uv(ctx, core, core->pred[0][U_C], core->ipm[1], U_C);
        evey_intra_pred_uv(ctx, core, core->pred[0][V_C], core->ipm[1], V_C);

        /* get residual */
        eveye_diff_16b(log2_cuw, log2_cuh, core->org[U_C], core->pred[0][U_C], cuw, cuw, cuw, pi->coef_tmp[U_C], bit_depth);
        eveye_diff_16b(log2_cuw, log2_cuh, core->org[V_C], core->pred[0][V_C], cuw, cuw, cuw, pi->coef_tmp[V_C], bit_depth);

        /* transform and quantization */
        eveye_sub_block_tq(ctx, core, pi->coef_tmp, 1, RUN_CB | RUN_CR);
        evey_mcpy(core->coef[U_C], pi->coef_tmp[U_C], sizeof(s16) * cuw * cuh);
        evey_mcpy(core->coef[V_C], pi->coef_tmp[V_C], sizeof(s16) * cuw * cuh);

        /* inverse quantization and inverse transform */
        evey_mset(core->nnz_sub[Y_C], 0, sizeof(int) * MAX_SUB_TB_NUM);
        evey_sub_block_itdq(ctx, core, pi->coef_tmp, core->nnz_sub);

        /* reconstruction */
        evey_recon(pi->coef_tmp[U_C], core->pred[0][U_C], core->nnz[U_C], cuw, cuh, cuw, pi->rec[U_C], bit_depth);
        evey_recon(pi->coef_tmp[V_C], core->pred[0][V_C], core->nnz[V_C], cuw, cuh, cuw, pi->rec[V_C], bit_depth);

        /* calculate distoration */
        if(ctx->cdsc.rdo_dbk_switch)
        {
            eveye_calc_delta_dist_filter_boundary(ctx, core, pi->rec, 1 << core->log2_cuw, x, y, 1, pi->nnz_best[Y_C] != 0, NULL, NULL, 0);
            cost += (ctx->dist_chroma_weight[0] * (eveye_ssd_16b(log2_cuw, log2_cuh, pi->rec[U_C], core->org[U_C], cuw, cuw, bit_depth) + core->delta_dist[U_C]));
            cost += (ctx->dist_chroma_weight[1] * (eveye_ssd_16b(log2_cuw, log2_cuh, pi->rec[V_C], core->org[V_C], cuw, cuw, bit_depth) + core->delta_dist[V_C]));
        }
        else
        {
            cost += (ctx->dist_chroma_weight[0] * eveye_ssd_16b(log2_cuw, log2_cuh, pi->rec[U_C], core->org[U_C], cuw, cuw, bit_depth));
            cost += (ctx->dist_chroma_weight[1] * eveye_ssd_16b(log2_cuw, log2_cuh, pi->rec[V_C], core->org[V_C], cuw, cuw, bit_depth));
        }
        *dist = (s32)cost;

        /* calculate bit cost */
        eveye_sbac_bit_reset(&core->s_temp_run);
        eveye_rdo_bit_cnt_cu_intra_chroma(ctx, core, core->coef);
        bit_cnt = eveye_get_bit_number(&core->s_temp_run);
        cost += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);
    }
    else
    {
        evey_assert(0);
    }

    return cost;
}

static int get_ipred_cand_list(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int * ipred_list)
{
    EVEYE_PINTRA * pi = &ctx->pintra;
    const int      ipd_rdo_cnt = IPD_RDO_CNT;
    int            pred_cnt, i, j;
    double         cost, cand_cost[IPD_RDO_CNT];
    u32            cand_satd_cost[IPD_RDO_CNT];
    u32            cost_satd;
    u32            inter_satd = EVEY_UINT32_MAX;

    if(core->cost_best != MAX_COST)
    {
        inter_satd = eveye_satd_16b(core->log2_cuw, core->log2_cuh, core->org[Y_C], ctx->pinter.pred_y_best, 1 << core->log2_cuw, 1 << core->log2_cuw, ctx->sps.bit_depth_luma_minus8 + 8);
    }

    for(i = 0; i < ipd_rdo_cnt; i++)
    {
        ipred_list[i] = IPD_DC;
        cand_cost[i] = MAX_COST;
        cand_satd_cost[i] = EVEY_UINT32_MAX;
    }

    pred_cnt = IPD_CNT;

    for (i = 0; i < pred_cnt; i++)
    {
        int bit_cnt = 0;
        int shift = 0;
        pel * pred_buf = pi->pred_cache[i]; /* keep luma predictions in cache buffer */

        /* intra prediction */
        evey_intra_pred(ctx, core, pred_buf, i);

        /* calculate SATD cost */
        cost_satd = eveye_satd_16b(core->log2_cuw, core->log2_cuh, core->org[Y_C], pred_buf, 1 << core->log2_cuw, 1 << core->log2_cuw, ctx->sps.bit_depth_luma_minus8 + 8);
        cost = (double)cost_satd;

        /* count bit */
        SBAC_LOAD(core->s_temp_run, core->s_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);
        eveye_sbac_bit_reset(&core->s_temp_run);
        eveye_eco_intra_dir(&core->bs_temp, i, core->mpm_b_list); /* count intra mode bits */
        bit_cnt = eveye_get_bit_number(&core->s_temp_run);
        cost += RATE_TO_COST_SQRT_LAMBDA(ctx->sqrt_lambda[0], bit_cnt); /* calculate bit cost */

        /* sorting */
        while(shift < ipd_rdo_cnt && cost < cand_cost[ipd_rdo_cnt - 1 - shift])
        {
            shift++;
        }
        if(shift != 0)
        {
            for(j = 1; j < shift; j++)
            {
                ipred_list[ipd_rdo_cnt - j] = ipred_list[ipd_rdo_cnt - 1 - j];
                cand_cost[ipd_rdo_cnt - j] = cand_cost[ipd_rdo_cnt - 1 - j];
                cand_satd_cost[ipd_rdo_cnt - j] = cand_satd_cost[ipd_rdo_cnt - 1 - j];
            }
            ipred_list[ipd_rdo_cnt - shift] = i;
            cand_cost[ipd_rdo_cnt - shift] = cost;
            cand_satd_cost[ipd_rdo_cnt - shift] = cost_satd;
        }
    }

    pred_cnt = ipd_rdo_cnt;
    /* (i >= 2): to allow at least two RDO tests for intra prediction */
    /* Note: the (i >= 2) term should be modified depending on intra prediction method */
    for(i = ipd_rdo_cnt - 1; i >= 2; i--)
    {
        /* Note: the value of weighting factor (1.2) should be modified depending on intra prediction method */
        if(cand_satd_cost[i] > inter_satd * (1.2))
        {
            pred_cnt--;
        }
        else
        {
            break;
        }
    }

    return EVEY_MIN(pred_cnt, ipd_rdo_cnt);
}

static void copy_rec_to_pic_ODP(pel * rec, int x, int y, int w, int h, EVEY_PIC  ** pic, int chroma_format_idc)
{
    pel           * src, *dst;
    int             j, s_pic, off, size;
    int             log2_w, log2_h;
    int             stride;

    log2_w = EVEY_CONV_LOG2(w);
    log2_h = EVEY_CONV_LOG2(h);

    s_pic = (*pic)->s_l;

    stride = w;

    if (x + w > (*pic)->w_l)
    {
        w = (*pic)->w_l - x;
    }

    if (y + h > (*pic)->h_l)
    {
        h = (*pic)->h_l - y;
    }

    /* luma */
    src = rec;
    dst = (*pic)->y + x + y * s_pic;
    size = sizeof(pel) * w;
    
    for (j = 0; j < h; j++)
    {
        evey_mcpy(dst, src, size);
        src += stride;
        dst += s_pic;
    }

    if (chroma_format_idc != 0)
    {
        
    }
}


/* entry point for intra mode decision */
static double pintra_analyze_cu(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y)
{   
    EVEYE_MODE   * mi = &ctx->mode;
    EVEYE_PINTRA * pi = &ctx->pintra;
    EVEY_PIC**       pi_ctx = &ctx->pintra.recon_fig;
    EVEY_PIC**       ctx_pred = &ctx->pintra.pred_fig;
    int            chroma_format_idc = ctx->sps.chroma_format_idc;
    int            w_shift = (GET_CHROMA_W_SHIFT(chroma_format_idc));
    int            h_shift = (GET_CHROMA_H_SHIFT(chroma_format_idc));
    int            i, j;
    int            cuw = 1 << core->log2_cuw;
    int            cuh = 1 << core->log2_cuh;
    int            best_ipd = IPD_INVALID;
    int            best_ipd_c = IPD_INVALID;
    s32            best_dist_y = 0, best_dist_c = 0;
    int            bit_cnt = 0;
    int            ipred_list[IPD_CNT];
    int            pred_cnt = IPD_CNT;
    double         cost_t, cost = MAX_COST;

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        eveye_set_qp(ctx, core, core->dqp_curr_best[core->log2_cuw - 2][core->log2_cuh - 2].curr_qp);
    }

    /* check availability of neighboring blocks */
    core->avail_cu = evey_get_avail_intra(ctx, core);

    /* prepare reference samples */
    evey_get_nbr_yuv(ctx, core, x, y);

    /* set mpm table */
    evey_get_mpm(ctx, core);

    /* pre-decision w/ satd for luma */
    pred_cnt = get_ipred_cand_list(ctx, core, x, y, ipred_list);
    if(pred_cnt == 0)
    {
        return MAX_COST;
    }

    /* luma decision w/ rdo */
    for(j = 0; j < pred_cnt; j++)
    {
        s32 dist_t = 0;
        // AF i is the actual intra preditor mode index
        i = ipred_list[j];
        core->ipm[0] = i;        
        core->ipm[1] = i; /* currently, chroma mode is set to luma mode */
        // XXNN insertion
#if 1
        // AF At the moment, we replace mode DC 0 (i == 0) with our NN predictor
        if (ctx->cdsc.nn_base_port > 0 && (core->avail_cu & (AVAIL_LE | AVAIL_UP | AVAIL_UP_LE)) && ((cuw == 32 && NN_INTRA_32) || (cuw == 16 && NN_INTRA_16) || (cuw == 8 && NN_INTRA_8) || (cuw == 4 && NN_INTRA_4))  &&  NN_pintra_context_available (x, y, cuw, cuh)  &&  i == 0)
            {
            /* Width of the context pi_ctx, for the sake of clarity */
            int   s_pic = (*pi_ctx)->s_l; // stride of pi_ctx
        
            /* NN_CONTEXT_SIZE x NN_CONTEXT_SIZE "DP" input for the server*/
            pel *sent16bpp = (pel*)malloc(sizeof(pel) * NN_CONTEXT_SIZE * NN_CONTEXT_SIZE); //DP format
            memset(sent16bpp, 0, sizeof(pel) * NN_CONTEXT_SIZE * NN_CONTEXT_SIZE);
            
            /* Copying the context in the DP block allocated above and then the predictor as well */
            pel * src = (*pi_ctx)->y + ((y - (NN_CONTEXT_SIZE - cuh)) * s_pic) + (x - (NN_CONTEXT_SIZE - cuw)); //Picture buffer of reconstructed blocks
            for (int i = 0; i < NN_CONTEXT_SIZE; i++)
            {
                evey_mcpy(sent16bpp + (i * NN_CONTEXT_SIZE), src, sizeof(pel) * NN_CONTEXT_SIZE);
                src += s_pic;
            }
            pel * pred_cache = pi->pred_cache[core->ipm[0]];
            NN_CopyPredictorIntoContext16 (sent16bpp, pi->pred_cache[core->ipm[0]], NN_CONTEXT_SIZE, NN_CONTEXT_SIZE, cuw, cuh); //copy predictor into the DP format
            NN_savePredictor("sent16bpp.yuv", sent16bpp, NN_CONTEXT_SIZE, NN_CONTEXT_SIZE, NN_CONTEXT_SIZE, true);
            
            /* We send the context + predictor in DP format to the server listening at port base_port + cuw to support distinct severs */
            NN_sendTo16(sent16bpp, sizeof(Pel) * NN_CONTEXT_SIZE * NN_CONTEXT_SIZE, cuw);  //send DP context
            free(sent16bpp);
            
            /* We wait for the server to send back the new predictor and we store in rcvd16bpp as an array of Pel */
            Pel *rcvd16bpp;
            int nRcvdcBytes = NN_recvFrom16(&rcvd16bpp);
            if (nRcvdcBytes != cuw * cuh * sizeof(Pel)) {
                printf("WARNING received %d rather than %d bytes\n", nRcvdcBytes, (int)(cuw * cuh * sizeof(Pel)));
            }
            NN_savePredictor("rcvd16bpp.yuv", rcvd16bpp, cuw, cuh, cuw, true);
            
            /* In "Oracle" mode, we replace the EVC predictor with the NN predictor if the latter has lower rate */
            float cost_evc = pintra_residue_rdo(ctx, core, &dist_t, 0, x, y);
            // We store a copy of the orginal predictor ...
            pel *backupPredEVC = (pel*)malloc(sizeof(pel) * cuw * cuw); //DP format
            evey_mcpy(backupPredEVC, pi->pred_cache[core->ipm[0]], sizeof(pel) * cuw * cuh);
            // ... since pintra_residue_rdo() requires us to temporarily overwite it ...
            evey_mcpy(pi->pred_cache[core->ipm[0]], rcvd16bpp, sizeof(pel) * cuw * cuh);
            float cost_nn = pintra_residue_rdo(ctx, core, &dist_t, 0, x, y);
            // ... so that in the case can switch back to the original predictor, iff the Oracle mode is used however!
            if (NN_ORACLE && cost_nn > cost_evc) {
                evey_mcpy(pi->pred_cache[core->ipm[0]], backupPredEVC, sizeof(pel) * cuw * cuh);
            }
            free(backupPredEVC);
            
            free(rcvd16bpp);
            printf("x %d y %d cuw %d cuy %d type %d COST_EVC %.0f COST_NN %.0f\n", x, y, cuw, cuh, i, cost_evc, cost_nn);
        }
#endif
//        printf("x %d y %d cuw %d cuh %d type %d\n", x, y, cuw, cuh, j);
        /* RD cost for luma */
        cost_t = pintra_residue_rdo(ctx, core, &dist_t, 0, x, y);

#if TRACE_COSTS
        EVEY_TRACE_COUNTER;
        EVEY_TRACE_STR("Luma mode ");
        EVEY_TRACE_INT(i);
        EVEY_TRACE_STR(" cost is ");
        EVEY_TRACE_DOUBLE(cost_t);
        EVEY_TRACE_STR("Predictor number ");
        EVEY_TRACE_INT(j);
        EVEY_TRACE_STR("\n");
#endif
        if(cost_t < cost)
        {
            cost = cost_t;
            best_dist_y = dist_t;
            best_ipd = i;

            evey_mcpy(pi->coef_best[Y_C], core->coef[Y_C], (cuw * cuh) * sizeof(s16));
            evey_mcpy(pi->rec_best[Y_C], pi->rec[Y_C], (cuw * cuh) * sizeof(pel));            
            evey_mcpy(pi->nnz_sub_best[Y_C], core->nnz_sub[Y_C], sizeof(int) * MAX_SUB_TB_NUM);
            pi->nnz_best[Y_C] = core->nnz[Y_C];
            SBAC_STORE(core->s_temp_prev_comp_best, core->s_temp_run);
        }
    } // end for() loop over the modes
    
    
    /* Here we update the picture buffer pi_ctx; placing this code block here rather
     * than in the above for() loop seems to be ok to produce the most correct context 
     */
    if (ctx->cdsc.nn_base_port > 0) {
        copy_rec_to_pic_ODP(pi->rec_best[Y_C], x, y, cuw, cuh, (pi_ctx), 0); //copy all reconstructed blocks into a buffer picture
        //NN_savePredictor("pi_ctx.yuv", (*pi_ctx)->y, (*pi_ctx)->w_l, (*pi_ctx)->h_l, (*pi_ctx)->s_l, true);
    }
    
    /* chroma decision w/ rdo */
    if(chroma_format_idc != 0)
    {
        s32 dist_tc = 0;
        core->ipm[0] = best_ipd;        
        core->ipm[1] = best_ipd; /* Currently, chroma mode is the same /w luma one */

        /* RD cost for chroma */
        cost_t = pintra_residue_rdo(ctx, core, &dist_tc, 1, x, y);

        best_ipd_c = core->ipm[1];
        best_dist_c = dist_tc;
        for(j = U_C; j < N_C; j++)
        {
            int size_tmp = (cuw * cuh) >> (h_shift + w_shift);
            evey_mcpy(pi->coef_best[j], core->coef[j], size_tmp * sizeof(s16));
            evey_mcpy(pi->rec_best[j], pi->rec[j], size_tmp * sizeof(pel));            
            evey_mcpy(pi->nnz_sub_best[j], core->nnz_sub[j], sizeof(int) * MAX_SUB_TB_NUM);
            pi->nnz_best[j] = core->nnz[j];
        }
    }

    /* restore best data for bit cost calculation */
    for(j = Y_C; j < N_C; j++)
    {
        if(j != 0 && !ctx->sps.chroma_format_idc)
        {
            continue;
        }
        int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (h_shift + w_shift));
        evey_mcpy(core->coef[j], pi->coef_best[j], size_tmp * sizeof(u16));             /* coefficient */
        evey_mcpy(core->nnz_sub[j], pi->nnz_sub_best[j], sizeof(int) * MAX_SUB_TB_NUM); /* nnz_sub */
        core->nnz[j] = pi->nnz_best[j];                                                 /* nnz */
    }    
    core->ipm[0] = best_ipd;
    if(chroma_format_idc != 0)
    {
        core->ipm[1] = best_ipd_c;
        evey_assert(best_ipd_c != IPD_INVALID);
    }

    /* calculate bit cost */
    SBAC_LOAD(core->s_temp_run, core->s_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);
    DQP_STORE(core->dqp_temp_run, core->dqp_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);
    eveye_sbac_bit_reset(&core->s_temp_run);
    eveye_rdo_bit_cnt_cu_intra(ctx, core, core->coef);
    bit_cnt = eveye_get_bit_number(&core->s_temp_run);
    cost = RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);

    core->dist_cu = 0;
    /* add luma distortion */
    cost += best_dist_y;
    core->dist_cu += best_dist_y;
    
    if(chroma_format_idc != 0)
    {
        /* add chroma distortion */
        cost += best_dist_c;
        core->dist_cu += best_dist_c;
    }

    SBAC_STORE(core->s_temp_best, core->s_temp_run);
    DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);

    if(cost < core->cost_best)
    {
        core->dist_cu_best = core->dist_cu;
        core->cost_best = cost;
        core->pred_mode = MODE_INTRA;

        mi->rec = pi->rec_best;
        mi->coef = pi->coef_best;
        mi->nnz = pi->nnz_best;
        mi->nnz_sub = pi->nnz_sub_best;

        SBAC_STORE(core->s_next_best[core->log2_cuw - 2][core->log2_cuh - 2], core->s_temp_best);
        DQP_STORE(core->dqp_next_best[core->log2_cuw - 2][core->log2_cuh - 2], core->dqp_temp_best);
    }

    /* return intra best cost */
    return cost;
}

static int pintra_init_frame(EVEYE_CTX * ctx)
{
    return EVEY_OK;
}

static int pintra_analyze_ctu(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    return EVEY_OK;
}

static int pintra_init_ctu(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    return EVEY_OK;
}

static int pintra_set_complexity(EVEYE_CTX * ctx, int complexity)
{
    EVEYE_PINTRA * pi;

    pi = &ctx->pintra;
    pi->complexity = complexity;

    return EVEY_OK;
}

int eveye_pintra_create(EVEYE_CTX * ctx, int complexity)
{
    /* set function addresses */
    ctx->fn_pintra_set_complexity = pintra_set_complexity;
    ctx->fn_pintra_init_frame = pintra_init_frame;
    ctx->fn_pintra_init_ctu = pintra_init_ctu;
    ctx->fn_pintra_analyze_cu = pintra_analyze_cu;

    return ctx->fn_pintra_set_complexity(ctx, complexity);
}
