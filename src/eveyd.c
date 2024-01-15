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
#include "evey_lf.h"
#include <math.h>


/* convert EVEYD into EVEYD_CTX */
#define EVEYD_ID_TO_CTX_R(id, ctx) \
    evey_assert_r((id)); \
    (ctx) = (EVEYD_CTX *)id; \
    evey_assert_r((ctx)->magic == EVEYD_MAGIC_CODE);

/* convert EVEYD into EVEYD_CTX with return value if assert on */
#define EVEYD_ID_TO_CTX_RV(id, ctx, ret) \
    evey_assert_rv((id), (ret)); \
    (ctx) = (EVEYD_CTX *)id; \
    evey_assert_rv((ctx)->magic == EVEYD_MAGIC_CODE, (ret));

static EVEYD_CTX * ctx_alloc(void)
{
    EVEYD_CTX * ctx;

    ctx = (EVEYD_CTX*)evey_malloc_fast(sizeof(EVEYD_CTX));

    evey_assert_rv(ctx != NULL, NULL);
    evey_mset_x64a(ctx, 0, sizeof(EVEYD_CTX));

    /* set default value */
    ctx->pic_cnt       = 0;

    return ctx;
}

static void ctx_free(EVEYD_CTX * ctx)
{
    evey_mfree_fast(ctx);
}

static EVEYD_CORE * core_alloc(void)
{
    EVEYD_CORE * core;

    core = (EVEYD_CORE*)evey_malloc_fast(sizeof(EVEYD_CORE));

    evey_assert_rv(core, NULL);
    evey_mset_x64a(core, 0, sizeof(EVEYD_CORE));

    return core;
}

static void core_free(EVEYD_CORE * core)
{
    evey_mfree_fast(core);
}

static void sequence_deinit(EVEYD_CTX * ctx)
{
    evey_mfree(ctx->map_scu);
    evey_mfree(ctx->map_split);
    evey_mfree(ctx->map_ipm);
    evey_mfree(ctx->map_pred_mode);
    evey_picman_deinit(&ctx->dpbm);
}

static int sequence_init(EVEYD_CTX * ctx, EVEY_SPS * sps)
{
    int size;
    int ret;

    if(sps->pic_width_in_luma_samples != ctx->w || sps->pic_height_in_luma_samples != ctx->h)
    {
        /* resolution was changed */
        sequence_deinit(ctx);

        ctx->w = sps->pic_width_in_luma_samples;
        ctx->h = sps->pic_height_in_luma_samples;

        ctx->ctu_size = 1 << 6; /* CTU size: 64 */
        ctx->min_cu_size = 1 << 2; /* Minimum CU size: 4 */

        ctx->log2_ctu_size = EVEY_CONV_LOG2(ctx->ctu_size);
        ctx->log2_min_cu_size = EVEY_CONV_LOG2(ctx->min_cu_size);
    }

    size = ctx->ctu_size;
    ctx->w_ctu = (ctx->w + (size - 1)) / size;
    ctx->h_ctu = (ctx->h + (size - 1)) / size;
    ctx->f_ctu = ctx->w_ctu * ctx->h_ctu;
    ctx->w_scu = (ctx->w + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->h_scu = (ctx->h + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->f_scu = ctx->w_scu * ctx->h_scu;

    /* alloc SCU map */
    if(ctx->map_scu == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_scu = (u32 *)evey_malloc(size);
        evey_assert_gv(ctx->map_scu, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
        evey_mset_x64a(ctx->map_scu, 0, size);
    }

    /* alloc map for CU split flag */
    if(ctx->map_split == NULL)
    {
        size = sizeof(s8) * ctx->f_ctu * NUM_CU_DEPTH * NUM_BLOCK_SHAPE * MAX_CU_CNT_IN_CTU;
        ctx->map_split = evey_malloc(size);
        evey_assert_gv(ctx->map_split, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
        evey_mset_x64a(ctx->map_split, 0, size);
    }

    /* alloc map for intra prediction mode */
    if(ctx->map_ipm == NULL)
    {
        size = sizeof(s8) * ctx->f_scu;
        ctx->map_ipm = (s8 *)evey_malloc(size);
        evey_assert_gv(ctx->map_ipm, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
        evey_mset_x64a(ctx->map_ipm, -1, size);
    }

    /* alloc CU mode map*/
    if(ctx->map_pred_mode == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->map_pred_mode = (u8 *)evey_malloc(size);
        evey_assert_gv(ctx->map_pred_mode, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
        evey_mset_x64a(ctx->map_pred_mode, 0, size);
    }

    /* initialize reference picture manager */
    EVEY_PICBUF_ALLOCATOR pa;
    pa.fn_alloc          = evey_pic_alloc;
    pa.fn_free           = evey_pic_free;
    pa.fn_expand         = evey_pic_expand;
    pa.w                 = ctx->w;
    pa.h                 = ctx->h;
    pa.pad_l             = PIC_PAD_SIZE_L;
    pa.pad_c             = PIC_PAD_SIZE_C;
    pa.chroma_format_idc = sps->chroma_format_idc;
    pa.bit_depth         = sps->bit_depth_luma_minus8 + 8;
    ret = evey_picman_init(&ctx->dpbm, MAX_PB_SIZE, MAX_NUM_REF_PICS, &pa);
    evey_assert_g(EVEY_SUCCEEDED(ret), ERR);

    ctx->ref_pic_gap_length = (int)pow(2.0, sps->log2_ref_pic_gap_length);

    evey_set_chroma_qp_tbl_loc(ctx->sps.bit_depth_luma_minus8 + 8);

    if (sps->chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        evey_derived_chroma_qp_mapping_tables(&(sps->chroma_qp_table_struct), sps->bit_depth_chroma_minus8 + 8);
    }
    else
    {
        evey_mcpy(&(evey_tbl_qp_chroma_dynamic_ext[0][6 * sps->bit_depth_chroma_minus8]), evey_tbl_qp_chroma_ajudst, MAX_QP_TABLE_SIZE * sizeof(int));
        evey_mcpy(&(evey_tbl_qp_chroma_dynamic_ext[1][6 * sps->bit_depth_chroma_minus8]), evey_tbl_qp_chroma_ajudst, MAX_QP_TABLE_SIZE * sizeof(int));
    }

    return EVEY_OK;
ERR:
    sequence_deinit(ctx);

    return ret;
}

static void slice_deinit(EVEYD_CTX * ctx)
{
}

static int slice_init(EVEYD_CTX * ctx, EVEYD_CORE * core, EVEY_SH * sh)
{
    core->ctu_num = 0;
    core->x_ctu   = 0;
    core->y_ctu   = 0;
    core->x_pel   = 0;
    core->y_pel   = 0;
    core->qp_y    = ctx->sh.qp + 6 * ctx->sps.bit_depth_luma_minus8;
    core->qp_u    = p_evey_tbl_qp_chroma_dynamic[0][sh->qp_u] + 6 * ctx->sps.bit_depth_chroma_minus8;
    core->qp_v    = p_evey_tbl_qp_chroma_dynamic[1][sh->qp_v] + 6 * ctx->sps.bit_depth_chroma_minus8;

    /* clear maps */
    evey_mset_x64a(ctx->map_scu, 0, sizeof(u32) * ctx->f_scu);
    
    if(ctx->sh.slice_type == SLICE_I)
    {
        ctx->last_intra_poc = ctx->poc.poc_val;
    }

    return EVEY_OK;
}

static void make_stat(EVEYD_CTX * ctx, int btype, EVEYD_STAT * stat)
{
    int i, j;
    stat->nalu_type = btype;
    stat->stype = 0;
    stat->fnum = -1;
    if(ctx)
    {
        stat->read += EVEYD_BSR_GET_READ_BYTE(&ctx->bs);
        if(btype < EVEY_SPS_NUT)
        {
            stat->fnum = ctx->pic_cnt;
            stat->stype = ctx->sh.slice_type;

            /* increase decoded picture count */
            ctx->pic_cnt++;
            stat->poc = ctx->poc.poc_val;
            stat->tid = ctx->nalu.nuh_temporal_id;

            for(i = 0; i < 2; i++)
            {
                stat->refpic_num[i] = ctx->dpbm.num_refp[i];
                for(j = 0; j < stat->refpic_num[i]; j++)
                {
                    stat->refpic[i][j] = ctx->refp[j][i].poc;
                }
            }
        }
    }
}

static void eveyd_itdq(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    evey_sub_block_itdq(ctx, core, core->coef, core->nnz_sub);
}

void eveyd_get_skip_motion(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    int REF_SET[3][MAX_NUM_ACTIVE_REF_FRAME] = { {0, 0,}, };
    int cuw, cuh, inter_dir = 0;
    s8  srefi[LIST_NUM][EVEY_MVP_NUM];
    s16 smvp[LIST_NUM][EVEY_MVP_NUM][MV_D];

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);

    evey_get_motion(ctx, core, LIST_0, srefi[LIST_0], smvp[LIST_0]);

    core->refi[LIST_0] = srefi[LIST_0][core->mvp_idx[LIST_0]];
    core->mv[LIST_0][MV_X] = smvp[LIST_0][core->mvp_idx[LIST_0]][MV_X];
    core->mv[LIST_0][MV_Y] = smvp[LIST_0][core->mvp_idx[LIST_0]][MV_Y];

    if(ctx->sh.slice_type == SLICE_P)
    {
        core->refi[LIST_1] = REFI_INVALID;
        core->mv[LIST_1][MV_X] = 0;
        core->mv[LIST_1][MV_Y] = 0;
    }
    else
    {
        evey_get_motion(ctx, core, LIST_1, srefi[LIST_1], smvp[LIST_1]);

        core->refi[LIST_1] = srefi[LIST_1][core->mvp_idx[LIST_1]];
        core->mv[LIST_1][MV_X] = smvp[LIST_1][core->mvp_idx[LIST_1]][MV_X];
        core->mv[LIST_1][MV_Y] = smvp[LIST_1][core->mvp_idx[LIST_1]][MV_Y];
    }
}

void eveyd_get_inter_motion(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    int cuw, cuh;
    s16 mvp[EVEY_MVP_NUM][MV_D];
    s8  refi[EVEY_MVP_NUM];

    cuw = (1 << core->log2_cuw);
    cuh = (1 << core->log2_cuh);

    int inter_dir_idx;
    for (inter_dir_idx = 0; inter_dir_idx < 2; inter_dir_idx++)
    {
        /* 0: forward, 1: backward */
        if(((core->inter_dir + 1) >> inter_dir_idx) & 1)
        {
            evey_get_motion(ctx, core, inter_dir_idx, refi, mvp);
            core->mv[inter_dir_idx][MV_X] = mvp[core->mvp_idx[inter_dir_idx]][MV_X] + core->mvd[inter_dir_idx][MV_X];
            core->mv[inter_dir_idx][MV_Y] = mvp[core->mvp_idx[inter_dir_idx]][MV_Y] + core->mvd[inter_dir_idx][MV_Y];
        }
        else
        {
            core->refi[inter_dir_idx] = REFI_INVALID;
            core->mv[inter_dir_idx][MV_X] = 0;
            core->mv[inter_dir_idx][MV_Y] = 0;
        }
    }
}

static int eveyd_eco_unit(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y, int log2_cuw, int log2_cuh)
{
    int ret;

    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = core->x_scu + core->y_scu * ctx->w_scu;

    /* parse CU info */
    ret = eveyd_eco_cu(ctx, core);
    evey_assert_g(ret == EVEY_OK, ERR);

    /* store CU information */
    eveyd_set_dec_info(ctx, core, x, y
#if ENC_DEC_TRACE
                      , (core->pred_mode == MODE_INTRA)
#endif
    );

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("poc: ");
    EVEY_TRACE_INT(ctx->poc.poc_val);
    EVEY_TRACE_STR("x pos ");
    EVEY_TRACE_INT(x);
    EVEY_TRACE_STR("y pos ");
    EVEY_TRACE_INT(y);
    EVEY_TRACE_STR("width ");
    EVEY_TRACE_INT(1 << log2_cuw);
    EVEY_TRACE_STR("height ");
    EVEY_TRACE_INT(1 << log2_cuh);
    EVEY_TRACE_STR("\n");

#if TRACE_ENC_CU_DATA
    static int core_counter = 1;
    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("RDO check id ");
    EVEY_TRACE_INT(core_counter++);
    EVEY_TRACE_STR("\n");
#endif

    return EVEY_OK;
ERR:
    return ret;
}

static int eveyd_dec_cu(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y, int log2_cuw, int log2_cuh)
{
    int cuw, cuh;

    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = core->x_scu + core->y_scu * ctx->w_scu;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;    

    /* load CU information */
    eveyd_get_dec_info(ctx, core, x, y);

    /* skip mode */
    if(core->pred_mode == MODE_SKIP) 
    {
        /* check availability of neighboring blocks */
        core->avail_cu = evey_get_avail_inter(ctx, core);

        /* MV derivation for skip mode */
        eveyd_get_skip_motion(ctx, core);

        /* motion compensation */
        evey_inter_pred(ctx, x, y, cuw, cuh, core->refi, core->mv, ctx->refp, core->pred);
    }
    /* non-skip mode */
    else
    {
        /* intra prediction */
        if(core->pred_mode == MODE_INTRA)
        {
            /* check availability of neighboring blocks */
            core->avail_cu = evey_get_avail_intra(ctx, core);

            /* prepare reference samples */
            evey_get_nbr_yuv(ctx, core, x, y);

            /* intra prediction */
            evey_intra_pred(ctx, core, core->pred[0][Y_C], core->ipm[0]); /* for Y */
            if(ctx->sps.chroma_format_idc != 0)
            {
                evey_intra_pred_uv(ctx, core, core->pred[0][U_C], core->ipm[1], U_C); /* for Cb */
                evey_intra_pred_uv(ctx, core, core->pred[0][V_C], core->ipm[1], V_C); /* for Cr */
            }

            /* set MV information for intra CU */
            core->refi[LIST_0] = REFI_INVALID;
            core->refi[LIST_1] = REFI_INVALID;
            core->mv[LIST_0][MV_X] = 0;
            core->mv[LIST_0][MV_Y] = 0;
            core->mv[LIST_1][MV_X] = 0;
            core->mv[LIST_1][MV_Y] = 0;
        }
        /* inter prediction */
        else
        {
            /* check availability of neighboring blocks */
            core->avail_cu = evey_get_avail_inter(ctx, core);

            if(core->pred_mode == MODE_DIR) /* direct mode */
            {                
                /* MV derivation for direct mode */
                evey_get_mv_dir(ctx, core, core->mv);
                core->refi[LIST_0] = 0;
                core->refi[LIST_1] = 0;
            }
            else /* general inter mode */
            {
                /* MV derivation for general inter mode */
                eveyd_get_inter_motion(ctx, core);
            }
            /* motion compensation */
            evey_inter_pred(ctx, x, y, cuw, cuh, core->refi, core->mv, ctx->refp, core->pred);
        }

        /* inverse quantization and inverse transform */
        eveyd_itdq(ctx, core);
    }

    /* reconstruction */
    evey_recon_yuv(ctx, core, x, y, core->nnz);

    /* store MV information (MV and ref_idx) */
    eveyd_set_mv_info(ctx, core, x, y);

    return EVEY_OK;
}

static int eveyd_eco_tree(EVEYD_CTX * ctx, EVEYD_CORE * core, int x0, int y0, int log2_cuw, int log2_cuh, int cup, int cud)
{
    int ret;
    s8  split_mode;
    int cuw, cuh;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    if (cuw > ctx->min_cu_size || cuh > ctx->min_cu_size)
    {
        split_mode = eveyd_eco_split_mode(&ctx->bs); /* split_cu_flag - 0: NO_SPLIT, 1: SPLIT_QUAD */

        EVEY_TRACE_COUNTER;
        EVEY_TRACE_STR("x pos ");
        EVEY_TRACE_INT(core->x_pel + ((cup % (ctx->ctu_size >> MIN_CU_LOG2) << MIN_CU_LOG2)));
        EVEY_TRACE_STR("y pos ");
        EVEY_TRACE_INT(core->y_pel + ((cup / (ctx->ctu_size >> MIN_CU_LOG2) << MIN_CU_LOG2)));
        EVEY_TRACE_STR("width ");
        EVEY_TRACE_INT(cuw);
        EVEY_TRACE_STR("height ");
        EVEY_TRACE_INT(cuh);
        EVEY_TRACE_STR("depth ");
        EVEY_TRACE_INT(cud);
        EVEY_TRACE_STR("split mode ");
        EVEY_TRACE_INT(split_mode);
        EVEY_TRACE_STR("\n");
    }
    else /* no split applied for the minimum CU size */
    {
        split_mode = NO_SPLIT;
    }

    /* split_cu_flag */
    evey_set_split_mode(split_mode, cud, cup, cuw, cuh, ctx->ctu_size, ctx->map_split[core->ctu_num]);

    if(split_mode != NO_SPLIT) /* quad-tree split */
    {
        EVEY_SPLIT_STRUCT split_struct;        
        evey_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_ctu_size - MIN_CU_LOG2, &split_struct );
        
        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int log2_sub_cuw = split_struct.log_cuw[part_num];
            int log2_sub_cuh = split_struct.log_cuh[part_num];
            int x_pos = split_struct.x_pos[part_num];
            int y_pos = split_struct.y_pos[part_num];

            if(x_pos < ctx->w && y_pos < ctx->h) /* only process CUs inside a picture */
            {
                ret = eveyd_eco_tree(ctx, core, x_pos, y_pos, log2_sub_cuw, log2_sub_cuh, split_struct.cup[part_num], split_struct.cud[part_num]);
                evey_assert_g(ret == EVEY_OK, ERR);
            }
        }
    }
    else /* (split_mode == NO_SPLIT) */
    {
        evey_assert(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h); /* should be inside a picture */

        /* parse a CU */
        ret = eveyd_eco_unit(ctx, core, x0, y0, log2_cuw, log2_cuh);
        evey_assert_g(ret == EVEY_OK, ERR);
    }

    return EVEY_OK;

ERR:
    return ret;
}

static int eveyd_dec_tree(EVEYD_CTX * ctx, EVEYD_CORE * core, int x0, int y0, int log2_cuw, int log2_cuh, int cup, int cud)
{
    int ret;
    int cuw, cuh;
    int ctu_num;
    s8  split_mode;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    ctu_num = (x0 >> ctx->log2_ctu_size) + (y0 >> ctx->log2_ctu_size) * ctx->w_ctu;
    evey_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->ctu_size, ctx->map_split[ctu_num]);
    
    if(split_mode != NO_SPLIT) /* quad-tree split */
    {
        EVEY_SPLIT_STRUCT split_struct;
        evey_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_ctu_size - MIN_CU_LOG2, &split_struct);

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int log2_sub_cuw = split_struct.log_cuw[part_num];
            int log2_sub_cuh = split_struct.log_cuh[part_num];
            int x_pos = split_struct.x_pos[part_num];
            int y_pos = split_struct.y_pos[part_num];

            if(x_pos < ctx->w && y_pos < ctx->h) /* only process CUs inside a picture */
            {
                ret = eveyd_dec_tree(ctx, core, x_pos, y_pos, log2_sub_cuw, log2_sub_cuh, split_struct.cup[part_num], split_struct.cud[part_num]);
                evey_assert_g(ret == EVEY_OK, ERR);
            }
        }
    }
    else /* (split_mode == NO_SPLIT) */
    {
        evey_assert(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h); /* should be inside a picture */

        /* decode a CU */
        ret = eveyd_dec_cu(ctx, core, x0, y0, log2_cuw, log2_cuh);
        evey_assert_g(ret == EVEY_OK, ERR);
    }

    return EVEY_OK;

ERR:
    return ret;
}

static int set_active_pps_info(EVEYD_CTX * ctx)
{
    int active_pps_id = ctx->sh.slice_pic_parameter_set_id;
    evey_mcpy(&(ctx->pps), &(ctx->pps_array[active_pps_id]), sizeof(EVEY_PPS) );

    return EVEY_OK;
}

static int eveyd_dec_slice(EVEYD_CTX * ctx, EVEYD_CORE * core)
{
    int ret;

    ctx->sh.qp_prev_eco = ctx->sh.qp;

    /* Initialize arithmetic decoder */
    eveyd_sbac_reset(ctx);

    core->x_ctu = 0;
    core->y_ctu = 0;
        
    /* CTU decoding loop */
    while(ctx->ctu_cnt > 0)
    {
        evey_update_core_loc_param(ctx, core);        
        evey_assert_rv(core->ctu_num < ctx->f_ctu, EVEY_ERR_UNEXPECTED);

        /* initialize the map for split flags */
        evey_mset(ctx->map_split[core->ctu_num], 0, sizeof(s8) * NUM_CU_DEPTH * NUM_BLOCK_SHAPE * MAX_CU_CNT_IN_CTU);
        
        /* parse a CTU */
        ret = eveyd_eco_tree(ctx, core, core->x_pel, core->y_pel, ctx->log2_ctu_size, ctx->log2_ctu_size, 0, 0);
        evey_assert_g(EVEY_SUCCEEDED(ret), ERR);

        /* reset all coded flags for the current ctu */
        evey_update_core_loc_param(ctx, core);
        u32 *map_scu;
        int i, j, w, h;
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

        /* decode a CTU */
        eveyd_dec_tree(ctx, core, core->x_pel, core->y_pel, ctx->log2_ctu_size, ctx->log2_ctu_size, 0, 0);

        core->x_ctu++;
        if(core->x_ctu >= ctx->w_ctu)
        {
            core->x_ctu = 0;
            core->y_ctu++;
        }
        ctx->ctu_cnt--;
    }

    /* read tile_end_flag */
    ret = eveyd_eco_tile_end_flag(&ctx->bs);
    assert(ret == 1);

    return EVEY_OK;

ERR:
    return ret;
}

static int eveyd_ready(EVEYD_CTX * ctx)
{
    int          ret = EVEY_OK;
    EVEYD_CORE * core = NULL;

    evey_assert(ctx);

    core = core_alloc();
    evey_assert_gv(core != NULL, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);

    ctx->core = core;
    return EVEY_OK;
ERR:
    if(core)
    {
        core_free(core);
    }

    return ret;
}

static void eveyd_flush(EVEYD_CTX * ctx)
{
    if(ctx->core)
    {
        core_free(ctx->core);
        ctx->core = NULL;
    }
}

static int eveyd_dec_nalu(EVEYD_CTX * ctx, EVEY_BITB * bitb, EVEYD_STAT * stat)
{
    EVEYD_BSR * bs = &ctx->bs;
    EVEY_SPS  * sps = &ctx->sps;
    EVEY_PPS  * pps = &ctx->pps;
    EVEY_SH   * sh = &ctx->sh;
    EVEY_NALU * nalu = &ctx->nalu;
    int         ret;

    ret = EVEY_OK;
    /* set error status */
    ctx->bs_err = bitb->err;
#if TRACE_START_POC
    if (fp_trace_started == 1)
    {
        EVEY_TRACE_SET(1);
    }
    else
    {
        EVEY_TRACE_SET(0);
    }
#else
#if TRACE_RDO_EXCLUDE_I
    if (sh->slice_type != SLICE_I)
    {
#endif
#if !TRACE_DBF
        EVEY_TRACE_SET(1);
#endif
#if TRACE_RDO_EXCLUDE_I
    }
    else
    {
        EVEY_TRACE_SET(0);
    }
#endif
#endif
    /* bitstream reader initialization */
    eveyd_bsr_init(bs, bitb->addr, bitb->ssize, NULL);
    SET_SBAC_DEC(bs, &ctx->sbac_dec);

    /* parse nalu header */
    ret = eveyd_eco_nalu(bs, nalu);
    evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

    if(nalu->nal_unit_type_plus1 - 1 == EVEY_SPS_NUT)
    {
        ret = eveyd_eco_sps(bs, sps);
        evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

        ret = sequence_init(ctx, sps);
        evey_assert_rv(EVEY_SUCCEEDED(ret), ret);
    }
    else if (nalu->nal_unit_type_plus1 - 1 == EVEY_PPS_NUT)
    {
        ret = eveyd_eco_pps(bs, sps, pps);
        evey_assert_rv(EVEY_SUCCEEDED(ret), ret);
        int pps_id = pps->pps_pic_parameter_set_id;
        evey_mcpy(&(ctx->pps_array[pps_id]), pps, sizeof(EVEY_PPS));
    }
    else if (nalu->nal_unit_type_plus1 - 1 < EVEY_SPS_NUT)
    {
        /* decode slice header */
        ret = eveyd_eco_sh(bs, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);

        evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

        set_active_pps_info(ctx);

        /* POC derivation process */
        if(ctx->nalu.nal_unit_type_plus1 - 1 == EVEY_IDR_NUT)
        {
            sh->poc_lsb = 0;
            ctx->poc.prev_doc_offset = -1;
            ctx->poc.prev_poc_val = 0;
            ctx->slice_ref_flag = (ctx->nalu.nuh_temporal_id == 0 || ctx->nalu.nuh_temporal_id < ctx->sps.log2_sub_gop_length);
            ctx->poc.poc_val = 0;
        }
        else
        {
            ctx->slice_ref_flag = (ctx->nalu.nuh_temporal_id == 0 || ctx->nalu.nuh_temporal_id < ctx->sps.log2_sub_gop_length);
            evey_poc_derivation(&ctx->sps, ctx->nalu.nuh_temporal_id, &ctx->poc);
            sh->poc_lsb = ctx->poc.poc_val;
        }

        ret = slice_init(ctx, ctx->core, sh);
        evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

        static u16 slice_num = 0;        
        if (ctx->ctu_cnt == 0)
        {
            ctx->ctu_cnt = ctx->f_ctu;
            slice_num = 0;
        }
        ctx->slice_num = slice_num;
        slice_num++;

#if TRACE_START_POC
        if (ctx->poc.poc_val == TRACE_START_POC)
        {
            fp_trace_started = 1;
            EVEY_TRACE_SET(1);
        }
#endif

        /* initialize reference pictures */
        ret = evey_picman_refp_init(ctx);
        evey_assert_rv(ret == EVEY_OK, ret);

        if (ctx->ctu_cnt == ctx->f_ctu)
        {
            /* get available frame buffer for decoded image */
            ctx->pic = evey_picman_get_empty_pic(&ctx->dpbm, &ret);
            evey_assert_rv(ctx->pic, ret);

            /* get available frame buffer for decoded image */
            ctx->map_refi = ctx->pic->map_refi;
            ctx->map_mv = ctx->pic->map_mv;

            int size;
            size = sizeof(s8) * ctx->f_scu * LIST_NUM;
            evey_mset_x64a(ctx->map_refi, -1, size);

            size = sizeof(s16) * ctx->f_scu * LIST_NUM * MV_D;
            evey_mset_x64a(ctx->map_mv, 0, size);

            ctx->pic->imgb->imgb_active_pps_id = ctx->pps.pps_pic_parameter_set_id;
        }

        /* decode slice layer */
        ret = ctx->fn_dec_slice(ctx, ctx->core);
        evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

        /* deblocking filter */
        if(ctx->sh.slice_deblocking_filter_flag)
        {
#if TRACE_DBF
            EVEY_TRACE_SET(1);
#endif
            ret = ctx->fn_deblock(ctx);
            evey_assert_rv(EVEY_SUCCEEDED(ret), ret);
#if TRACE_DBF
            EVEY_TRACE_SET(0);
#endif
        }

#if USE_DRAW_PARTITION_DEC
        eveyd_draw_partition(ctx, ctx->pic);
#endif
        if (ctx->ctu_cnt == 0)
        {
#if PIC_PAD_SIZE_L > 0
            /* expand pixels to padding area */
            ctx->dpbm.pa.fn_expand(ctx->pic);
#endif

            if (ctx->use_opl)
            {
                int do_compare_md5 = 0;
                ret = eveyd_picbuf_check_signature(ctx, do_compare_md5);
                evey_assert_rv(EVEY_SUCCEEDED(ret), ret);
            }

            /* put decoded picture to DPB */
            ret = evey_picman_put_pic(ctx, ctx->pic, 1);
            evey_assert_rv(EVEY_SUCCEEDED(ret), ret);
        }
        slice_deinit(ctx);
    }
    else if (nalu->nal_unit_type_plus1 - 1 == EVEY_SEI_NUT)
    {

        ret = eveyd_eco_sei(ctx, bs);

        if (ctx->pic_sign_exist)
        {
            if (ctx->use_pic_sign)
            {
                int do_compare_md5 = 1;
                ret = eveyd_picbuf_check_signature(ctx, do_compare_md5);
                ctx->pic_sign_exist = 0; 
            }
            else
            {
                ret = EVEY_WARN_CRC_IGNORED;
            }
        }
    }
    else
    {
        assert(!"wrong NALU type");
    }
    
    make_stat(ctx, nalu->nal_unit_type_plus1 - 1, stat);

    if (ctx->ctu_cnt > 0)
    {
        stat->fnum = -1;
    }

    return ret;
}

static int eveyd_pull_frm(EVEYD_CTX * ctx, EVEY_IMGB ** imgb, EVEYD_OPL * opl)
{
    int        ret;
    EVEY_PIC * pic;

    *imgb = NULL;

    pic = evey_picman_out_pic(&ctx->dpbm, &ret);

    if(pic)
    {
        evey_assert_rv(pic->imgb != NULL, EVEY_ERR);

        /* increase reference count */
        pic->imgb->addref(pic->imgb);
        *imgb = pic->imgb;
        if (ctx->sps.picture_cropping_flag)
        {
            for(int i = 0; i < N_C; i++)
            {
                int cs_offset = i == Y_C ? 2 : 1;
                (*imgb)->x[i] = ctx->sps.picture_crop_left_offset * cs_offset;
                (*imgb)->y[i] = ctx->sps.picture_crop_top_offset * cs_offset;
                (*imgb)->h[i] = (*imgb)->ah[i] - (ctx->sps.picture_crop_top_offset + ctx->sps.picture_crop_bottom_offset) * cs_offset;
                (*imgb)->w[i] = (*imgb)->aw[i] - (ctx->sps.picture_crop_left_offset + ctx->sps.picture_crop_left_offset) * cs_offset;
            }
        }

        opl->poc = pic->poc;
        evey_mcpy(opl->digest, pic->digest, N_C * 16);
    }
    return ret;
}

static int eveyd_platform_init(EVEYD_CTX * ctx)
{
    ctx->fn_ready         = eveyd_ready;
    ctx->fn_flush         = eveyd_flush;
    ctx->fn_dec_cnk       = eveyd_dec_nalu;
    ctx->fn_dec_slice     = eveyd_dec_slice;
    ctx->fn_pull          = eveyd_pull_frm;
    ctx->fn_deblock       = evey_deblock;
    ctx->pf               = NULL;

    int ret = evey_scan_tbl_init();
    evey_assert_rv(ret == EVEY_OK, ret);

    return EVEY_OK;
}

static void eveyd_platform_deinit(EVEYD_CTX * ctx)
{
    evey_assert(ctx->pf == NULL);

    ctx->fn_ready         = NULL;
    ctx->fn_flush         = NULL;
    ctx->fn_dec_cnk       = NULL;
    ctx->fn_dec_slice     = NULL;
    ctx->fn_pull          = NULL;
    ctx->fn_deblock       = NULL;

    evey_scan_tbl_delete();
}

EVEYD eveyd_create(EVEYD_CDSC * cdsc, int * err)
{
    EVEYD_CTX *ctx = NULL;
    int ret;

#if ENC_DEC_TRACE
#if TRACE_DBF
    fp_trace = fopen("dec_trace_dbf.txt", "w+");
#else
    fp_trace = fopen("dec_trace.txt", "w+");
#endif
#endif

    ctx = ctx_alloc();
    evey_assert_gv(ctx != NULL, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
    evey_mcpy(&ctx->cdsc, cdsc, sizeof(EVEYD_CDSC));
    /* additional initialization for each platform, if needed */
    ret = eveyd_platform_init(ctx);
    evey_assert_g(ret == EVEY_OK, ERR);

    if(ctx->fn_ready)
    {
        ret = ctx->fn_ready(ctx);
        evey_assert_g(ret == EVEY_OK, ERR);
    }

    /* Set CTX variables to default value */
    ctx->magic = EVEYD_MAGIC_CODE;
    ctx->id = (EVEYD)ctx;

    return (ctx->id);
ERR:
    if(ctx)
    {
        if(ctx->fn_flush) ctx->fn_flush(ctx);
        eveyd_platform_deinit(ctx);
        ctx_free(ctx);
    }

    if(err) *err = ret;

    return NULL;
}

void eveyd_delete(EVEYD id)
{
    EVEYD_CTX * ctx;

    EVEYD_ID_TO_CTX_R(id, ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif

    sequence_deinit(ctx);

    if(ctx->fn_flush) ctx->fn_flush(ctx);

    /* addtional deinitialization for each platform, if needed */
    eveyd_platform_deinit(ctx);

    ctx_free(ctx);
}

int eveyd_config(EVEYD id, int cfg, void * buf, int * size)
{
    EVEYD_CTX *ctx;

    EVEYD_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);

    switch(cfg)
    {
        /* set config ************************************************************/
        case EVEYD_CFG_SET_USE_PIC_SIGNATURE:
            ctx->use_pic_sign = (*((int *)buf)) ? 1 : 0;
            break;

        case EVEYD_CFG_SET_USE_OPL_OUTPUT:
            ctx->use_opl = (*((int *)buf)) ? 1 : 0;
            break;

        /* get config ************************************************************/
        default:
            evey_assert_rv(0, EVEY_ERR_UNSUPPORTED);
    }
    return EVEY_OK;
}

int eveyd_decode(EVEYD id, EVEY_BITB * bitb, EVEYD_STAT * stat)
{
    EVEYD_CTX * ctx;
    EVEYD_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv(ctx->fn_dec_cnk, EVEY_ERR_UNEXPECTED);
    return ctx->fn_dec_cnk(ctx, bitb, stat);
}

int eveyd_pull(EVEYD id, EVEY_IMGB ** img, EVEYD_OPL * opl)
{
    EVEYD_CTX * ctx;
    EVEYD_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv(ctx->fn_pull, EVEY_ERR_UNKNOWN);
    return ctx->fn_pull(ctx, img, opl);
}
