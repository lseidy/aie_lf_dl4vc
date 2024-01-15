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
#include <math.h>


#define CABAC_ZERO_PARAM                   32

int last_intra_poc = EVEY_INT_MAX;

/* Convert EVEYE into EVEYE_CTX */
#define EVEYE_ID_TO_CTX_R(id, ctx) \
    evey_assert_r((id)); \
    (ctx) = (EVEYE_CTX *)id; \
    evey_assert_r((ctx)->magic == EVEYE_MAGIC_CODE);

/* Convert EVEYE into EVEYE_CTX with return value if assert on */
#define EVEYE_ID_TO_CTX_RV(id, ctx, ret) \
    evey_assert_rv((id), (ret)); \
    (ctx) = (EVEYE_CTX *)id; \
    evey_assert_rv((ctx)->magic == EVEYE_MAGIC_CODE, (ret));

static const s8 tbl_slice_depth_P[5][16] =
{
    /* gop_size = 2 */
    { FRM_DEPTH_2, FRM_DEPTH_1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 4 */
    { FRM_DEPTH_3, FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_1, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 8 */
    { FRM_DEPTH_4, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_2, FRM_DEPTH_4, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_1,\
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 12 */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* gop_size = 16 */
    { FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_3, FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_2, \
      FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_3, FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_1 }
};

static const s8 tbl_slice_depth[5][15] =
{
    /* gop_size = 2 */
    { FRM_DEPTH_2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 4 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_3, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 8 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4,\
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 12 */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 16 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_5, \
      FRM_DEPTH_5,  FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5 }
};

static EVEYE_CTX * ctx_alloc(void)
{
    EVEYE_CTX * ctx;
    ctx = (EVEYE_CTX*)evey_malloc_fast(sizeof(EVEYE_CTX));
    evey_assert_rv(ctx, NULL);
    evey_mset_x64a(ctx, 0, sizeof(EVEYE_CTX));
    return ctx;
}

static void eveye_malloc_1d(void ** dst, int size)
{
    if(*dst == NULL)
    {
        *dst = evey_malloc_fast(size);
        evey_mset(*dst, 0, size);
    }
}

static void eveye_malloc_2d(s8 *** dst, int size_1d, int size_2d, int type_size)
{
    int i;

    if(*dst == NULL)
    {
        *dst = evey_malloc_fast(size_1d * sizeof(s8*));
        evey_mset(*dst, 0, size_1d * sizeof(s8*));


        (*dst)[0] = evey_malloc_fast(size_1d * size_2d * type_size);
        evey_mset((*dst)[0], 0, size_1d * size_2d * type_size);

        for(i = 1; i < size_1d; i++)
        {
            (*dst)[i] = (*dst)[i - 1] + size_2d * type_size;
        }
    }
}

static void ctx_free(EVEYE_CTX * ctx)
{
    evey_mfree_fast(ctx);
}

static void eveye_free_1d(void * dst)
{
    if(dst != NULL)
    {
        evey_mfree_fast(dst);
    }
}

static void eveye_free_2d(void ** dst)
{
    if(dst)
    {
        if(dst[0])
        {
            evey_mfree_fast(dst[0]);
        }
        evey_mfree_fast(dst);
    }
}

static int eveye_create_cu_data(EVEYE_CU_DATA * cu_data, int log2_cuw, int log2_cuh, int chroma_format_idc)
{
    int i, j;
    int cuw_scu, cuh_scu;
    int size_8b, size_16b, size_32b, cu_cnt, pixel_cnt;

    cuw_scu = 1 << log2_cuw;
    cuh_scu = 1 << log2_cuh;

    size_8b = cuw_scu * cuh_scu * sizeof(s8);
    size_16b = cuw_scu * cuh_scu * sizeof(s16);
    size_32b = cuw_scu * cuh_scu * sizeof(s32);
    cu_cnt = cuw_scu * cuh_scu;
    pixel_cnt = cu_cnt << 4;

    eveye_malloc_1d((void**)&cu_data->qp_y, size_8b);
    eveye_malloc_1d((void**)&cu_data->qp_u, size_8b);
    eveye_malloc_1d((void**)&cu_data->qp_v, size_8b);
    eveye_malloc_1d((void**)&cu_data->pred_mode, size_8b);
    eveye_malloc_2d((s8***)&cu_data->ipm, 2, cu_cnt, sizeof(u8));
    eveye_malloc_2d((s8***)&cu_data->refi, cu_cnt, LIST_NUM, sizeof(u8));
    eveye_malloc_2d((s8***)&cu_data->mvp_idx, cu_cnt, LIST_NUM, sizeof(u8));

    for(i = 0; i < N_C; i++)
    {
        eveye_malloc_1d((void**)&cu_data->nnz[i], size_32b);
    }
    for(i = 0; i < N_C; i++)
    {
        for(j = 0; j < 4; j++)
        {
            eveye_malloc_1d((void**)&cu_data->nnz_sub[i][j], size_32b);
        }
    }
    eveye_malloc_1d((void**)&cu_data->map_scu, size_32b);

    int shift = 0;
    for(i = 0; i < N_C; i++)
    {
        eveye_malloc_1d((void**)&cu_data->coef[i], (pixel_cnt >> (!!(i)* shift)) * sizeof(s16));
        eveye_malloc_1d((void**)&cu_data->reco[i], (pixel_cnt >> (!!(i)* shift)) * sizeof(pel));
    }

    return EVEY_OK;
}

static int eveye_delete_cu_data(EVEYE_CU_DATA * cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;

    eveye_free_1d((void*)cu_data->qp_y);
    eveye_free_1d((void*)cu_data->qp_u);
    eveye_free_1d((void*)cu_data->qp_v);
    eveye_free_1d((void*)cu_data->pred_mode);
    eveye_free_2d((void**)cu_data->ipm);
    eveye_free_2d((void**)cu_data->refi);
    eveye_free_2d((void**)cu_data->mvp_idx);

    for(i = 0; i < N_C; i++)
    {
        eveye_free_1d((void*)cu_data->nnz[i]);
    }
    for(i = 0; i < N_C; i++)
    {
        for(j = 0; j < 4; j++)
        {
            eveye_free_1d((void*)cu_data->nnz_sub[i][j]);
        }
    }
    eveye_free_1d((void*)cu_data->map_scu);

    for(i = 0; i < N_C; i++)
    {
        eveye_free_1d((void*)cu_data->coef[i]);
        eveye_free_1d((void*)cu_data->reco[i]);
    }

    return EVEY_OK;
}


static EVEYE_CORE * core_alloc(int chroma_format_idc)
{
    EVEYE_CORE * core;
    int i, j;

    core = (EVEYE_CORE*)evey_malloc_fast(sizeof(EVEYE_CORE));

    evey_assert_rv(core, NULL);
    evey_mset_x64a(core, 0, sizeof(EVEYE_CORE));

    for(i = 0; i < MAX_CU_DEPTH; i++)
    {
        for(j = 0; j < MAX_CU_DEPTH; j++)
        {
            eveye_create_cu_data(&core->cu_data_best[i][j], i, j, chroma_format_idc);
            eveye_create_cu_data(&core->cu_data_temp[i][j], i, j, chroma_format_idc);
        }
    }

    return core;
}

static void core_free(EVEYE_CORE * core)
{
    int i, j;

    for(i = 0; i < MAX_CU_DEPTH; i++)
    {
        for(j = 0; j < MAX_CU_DEPTH; j++)
        {
            eveye_delete_cu_data(&core->cu_data_best[i][j], i, j);
            eveye_delete_cu_data(&core->cu_data_temp[i][j], i, j);
        }
    }

    evey_mfree_fast(core);
}

void eveye_copy_chroma_qp_mapping_params(EVEY_CHROMA_TABLE * dst, EVEY_CHROMA_TABLE * src)
{
    dst->chroma_qp_table_present_flag = src->chroma_qp_table_present_flag;
    dst->same_qp_table_for_chroma = src->same_qp_table_for_chroma;
    dst->global_offset_flag = src->global_offset_flag;
    dst->num_points_in_qp_table_minus1[0] = src->num_points_in_qp_table_minus1[0];
    dst->num_points_in_qp_table_minus1[1] = src->num_points_in_qp_table_minus1[1];
    evey_mcpy(&(dst->delta_qp_in_val_minus1), &(src->delta_qp_in_val_minus1), sizeof(int) * 2 * MAX_QP_TABLE_SIZE);
    evey_mcpy(&(dst->delta_qp_out_val), &(src->delta_qp_out_val), sizeof(int) * 2 * MAX_QP_TABLE_SIZE);
}

static void eveye_parse_chroma_qp_mapping_params(EVEY_CHROMA_TABLE * dst_struct, EVEY_CHROMA_TABLE * src_struct, int bit_depth)
{
    int qpBdOffsetC = 6 * (bit_depth - 8);
    EVEY_CHROMA_TABLE *p_chroma_qp_table = dst_struct;
    p_chroma_qp_table->chroma_qp_table_present_flag = src_struct->chroma_qp_table_present_flag;
    p_chroma_qp_table->num_points_in_qp_table_minus1[0] = src_struct->num_points_in_qp_table_minus1[0];
    p_chroma_qp_table->num_points_in_qp_table_minus1[1] = src_struct->num_points_in_qp_table_minus1[1];

    if(p_chroma_qp_table->chroma_qp_table_present_flag)
    {
        p_chroma_qp_table->same_qp_table_for_chroma = 1;
        if(src_struct->num_points_in_qp_table_minus1[0] != src_struct->num_points_in_qp_table_minus1[1])
            p_chroma_qp_table->same_qp_table_for_chroma = 0;
        else
        {
            for(int i = 0; i < src_struct->num_points_in_qp_table_minus1[0]; i++)
            {
                if((src_struct->delta_qp_in_val_minus1[0][i] != src_struct->delta_qp_in_val_minus1[1][i]) ||
                    (src_struct->delta_qp_out_val[0][i] != src_struct->delta_qp_out_val[1][i]))
                {
                    p_chroma_qp_table->same_qp_table_for_chroma = 0;
                    break;
                }
            }
        }

        p_chroma_qp_table->global_offset_flag = (src_struct->delta_qp_in_val_minus1[0][0] > 15 && src_struct->delta_qp_out_val[0][0] > 15) ? 1 : 0;
        if(!p_chroma_qp_table->same_qp_table_for_chroma)
        {
            p_chroma_qp_table->global_offset_flag = p_chroma_qp_table->global_offset_flag && ((src_struct->delta_qp_in_val_minus1[1][0] > 15 && src_struct->delta_qp_out_val[1][0] > 15) ? 1 : 0);
        }

        int startQp = (p_chroma_qp_table->global_offset_flag == 1) ? 16 : -qpBdOffsetC;
        for(int ch = 0; ch < (p_chroma_qp_table->same_qp_table_for_chroma ? 1 : 2); ch++)
        {
            p_chroma_qp_table->delta_qp_in_val_minus1[ch][0] = src_struct->delta_qp_in_val_minus1[ch][0] - startQp;
            p_chroma_qp_table->delta_qp_out_val[ch][0] = src_struct->delta_qp_out_val[ch][0] - startQp - p_chroma_qp_table->delta_qp_in_val_minus1[ch][0];

            for(int k = 1; k <= p_chroma_qp_table->num_points_in_qp_table_minus1[ch]; k++)
            {
                p_chroma_qp_table->delta_qp_in_val_minus1[ch][k] = (src_struct->delta_qp_in_val_minus1[ch][k] - src_struct->delta_qp_in_val_minus1[ch][k - 1]) - 1;
                p_chroma_qp_table->delta_qp_out_val[ch][k] = (src_struct->delta_qp_out_val[ch][k] - src_struct->delta_qp_out_val[ch][k - 1]) - (p_chroma_qp_table->delta_qp_in_val_minus1[ch][k] + 1);
            }
        }
    }
}

static int set_init_param(EVEYE_CDSC * cdsc, EVEYE_PARAM * param)
{
    /* check input parameters */
    int pic_m = 8; /* EVEY_MAX(min_cu, 8) */
    evey_assert_rv(cdsc->w > 0 && cdsc->h > 0, EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv((cdsc->w & (pic_m -1)) == 0,EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv((cdsc->h & (pic_m -1)) == 0,EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv(cdsc->qp >= MIN_QUANT && cdsc->qp <= MAX_QUANT, EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv(cdsc->iperiod >= 0 ,EVEY_ERR_INVALID_ARGUMENT);

    if(cdsc->disable_hgop == 0)
    {
        evey_assert_rv(cdsc->max_b_frames == 0 || cdsc->max_b_frames == 1 || \
                      cdsc->max_b_frames == 3 || cdsc->max_b_frames == 7 || \
                      cdsc->max_b_frames == 15, EVEY_ERR_INVALID_ARGUMENT);

        if(cdsc->max_b_frames != 0)
        {
            if(cdsc->iperiod % (cdsc->max_b_frames + 1) != 0)
            {
                evey_assert_rv(0, EVEY_ERR_INVALID_ARGUMENT);
            }
        }
    }

    if (cdsc->ref_pic_gap_length != 0)
    {
        evey_assert_rv(cdsc->max_b_frames == 0, EVEY_ERR_INVALID_ARGUMENT);
    }

    if (cdsc->max_b_frames == 0)
    {
        if (cdsc->ref_pic_gap_length == 0)
        {
            cdsc->ref_pic_gap_length = 1;
        }
        evey_assert_rv(cdsc->ref_pic_gap_length == 1 || cdsc->ref_pic_gap_length == 2 || \
                       cdsc->ref_pic_gap_length == 4 || cdsc->ref_pic_gap_length == 8 || \
                       cdsc->ref_pic_gap_length == 16, EVEY_ERR_INVALID_ARGUMENT);
    }

    /* set default encoding parameter */
    param->w                   = cdsc->w;
    param->h                   = cdsc->h;
    param->bit_depth           = cdsc->out_bit_depth;
    param->qp                  = cdsc->qp;
    param->fps                 = cdsc->fps;
    param->i_period            = cdsc->iperiod;
    param->f_ifrm              = 0;
    param->use_deblock         = cdsc->use_deblock;
    param->qp_max              = MAX_QUANT;
    param->qp_min              = MIN_QUANT;
    param->use_pic_sign        = 0;
    param->toolset_idc_h       = cdsc->toolset_idc_h;
    param->toolset_idc_l       = cdsc->toolset_idc_l;
    param->max_b_frames        = cdsc->max_b_frames;
    param->max_num_ref_pics    = cdsc->max_num_ref_pics;
    param->ref_pic_gap_length  = cdsc->ref_pic_gap_length;
    param->gop_size            = param->max_b_frames + 1;
    param->use_closed_gop      = (cdsc->closed_gop) ? 1 : 0;
    param->use_hgop            = (cdsc->disable_hgop) ? 0 : 1;
    param->qp_incread_frame    = cdsc->add_qp_frame;
    param->use_dqp             = cdsc->use_dqp;
    param->chroma_format_idc   = cdsc->chroma_format_idc;

    evey_set_chroma_qp_tbl_loc(cdsc->codec_bit_depth);

    EVEY_CHROMA_TABLE chroma_qp_table_struct;    

    chroma_qp_table_struct.chroma_qp_table_present_flag = cdsc->chroma_qp_table_present_flag;
    chroma_qp_table_struct.same_qp_table_for_chroma = cdsc->same_qp_table_for_chroma;
    chroma_qp_table_struct.global_offset_flag = cdsc->global_offset_flag;
    evey_mcpy(chroma_qp_table_struct.num_points_in_qp_table_minus1, cdsc->num_points_in_qp_table_minus1, sizeof(cdsc->num_points_in_qp_table_minus1));
    evey_mcpy(chroma_qp_table_struct.delta_qp_in_val_minus1, cdsc->delta_qp_in_val_minus1, sizeof(cdsc->delta_qp_in_val_minus1));
    evey_mcpy(chroma_qp_table_struct.delta_qp_out_val, cdsc->delta_qp_out_val, sizeof(cdsc->delta_qp_out_val));

    eveye_parse_chroma_qp_mapping_params(&(param->chroma_qp_table_struct), &chroma_qp_table_struct, cdsc->codec_bit_depth);  /* parse input params and create chroma_qp_table_struct structure */
    evey_derived_chroma_qp_mapping_tables(&(param->chroma_qp_table_struct), cdsc->codec_bit_depth);

    if (param->chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        evey_derived_chroma_qp_mapping_tables(&(param->chroma_qp_table_struct), cdsc->codec_bit_depth);
    }
    else 
    {
        evey_mcpy(&(evey_tbl_qp_chroma_dynamic_ext[0][6 *( cdsc->codec_bit_depth - 8)]), evey_tbl_qp_chroma_ajudst, MAX_QP_TABLE_SIZE * sizeof(int));
        evey_mcpy(&(evey_tbl_qp_chroma_dynamic_ext[1][6 * (cdsc->codec_bit_depth - 8)]), evey_tbl_qp_chroma_ajudst, MAX_QP_TABLE_SIZE * sizeof(int));
    }

    return EVEY_OK;
}

static int set_enc_param(EVEYE_CTX * ctx, EVEYE_PARAM * param)
{
    int ret = EVEY_OK;
    ctx->sh.qp = (u8)param->qp;
    return ret;
}

static void set_nalu(EVEY_NALU * nalu, int nalu_type, int nuh_temporal_id)
{
    nalu->nal_unit_size = 0;
    nalu->forbidden_zero_bit = 0;
    nalu->nal_unit_type_plus1 = nalu_type + 1;
    nalu->nuh_temporal_id = nuh_temporal_id;
    nalu->nuh_reserved_zero_5bits = 0;
    nalu->nuh_extension_flag = 0;
}

// Dummy VUI initialization 
static void set_vui(EVEYE_CTX * ctx, EVEY_VUI * vui) 
{
    vui->aspect_ratio_info_present_flag = 1;
    vui->aspect_ratio_idc = 1;
    vui->sar_width = 1;
    vui->sar_height = 1;
    vui->overscan_info_present_flag = 1; 
    vui->overscan_appropriate_flag = 1;
    vui->video_signal_type_present_flag = 1;
    vui->video_format = 1;
    vui->video_full_range_flag = 1;
    vui->colour_description_present_flag = 1;
    vui->colour_primaries = 1;
    vui->transfer_characteristics = 1;
    vui->matrix_coefficients = 1;
    vui->chroma_loc_info_present_flag = 1;
    vui->chroma_sample_loc_type_top_field = 1;
    vui->chroma_sample_loc_type_bottom_field = 1;
    vui->neutral_chroma_indication_flag = 1;
    vui->field_seq_flag = 1;
    vui->timing_info_present_flag = 1;
    vui->num_units_in_tick = 1;
    vui->time_scale = 1;
    vui->fixed_pic_rate_flag = 1;
    vui->nal_hrd_parameters_present_flag = 1;
    vui->vcl_hrd_parameters_present_flag = 1;
    vui->low_delay_hrd_flag = 1;
    vui->pic_struct_present_flag = 1;
    vui->bitstream_restriction_flag = 1;
    vui->motion_vectors_over_pic_boundaries_flag = 1;
    vui->max_bytes_per_pic_denom = 1;
    vui->max_bits_per_mb_denom = 1;
    vui->log2_max_mv_length_horizontal = 1;
    vui->log2_max_mv_length_vertical = 1;
    vui->num_reorder_pics = 1;
    vui->max_dec_pic_buffering = 1;
    vui->hrd_parameters.cpb_cnt_minus1 = 1;
    vui->hrd_parameters.bit_rate_scale = 1;
    vui->hrd_parameters.cpb_size_scale = 1;
    evey_mset(&(vui->hrd_parameters.bit_rate_value_minus1), 0, sizeof(int)*NUM_CPB);
    evey_mset(&(vui->hrd_parameters.cpb_size_value_minus1), 0, sizeof(int)*NUM_CPB);
    evey_mset(&(vui->hrd_parameters.cbr_flag), 0, sizeof(int)*NUM_CPB);
    vui->hrd_parameters.initial_cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.dpb_output_delay_length_minus1 = 1;
    vui->hrd_parameters.time_offset_length = 1;
}

static void set_sps(EVEYE_CTX * ctx, EVEY_SPS * sps)
{
    sps->profile_idc = ctx->cdsc.profile;
    sps->level_idc = ctx->cdsc.level * 3;
    sps->pic_width_in_luma_samples = ctx->param.w;
    sps->pic_height_in_luma_samples = ctx->param.h;
    sps->toolset_idc_h = ctx->cdsc.toolset_idc_h;
    sps->toolset_idc_l = ctx->cdsc.toolset_idc_l;
    sps->bit_depth_luma_minus8 = ctx->cdsc.codec_bit_depth - 8;
    sps->bit_depth_chroma_minus8 = ctx->cdsc.codec_bit_depth - 8;
    sps->chroma_format_idc = ctx->cdsc.chroma_format_idc;
    sps->ibc_flag = 0;
    sps->max_num_ref_pics = ctx->cdsc.max_num_ref_pics;
    sps->sps_btt_flag = 0;
    sps->sps_suco_flag = 0;   
    sps->sps_addb_flag = 0;
    sps->sps_dra_flag = 0;
    sps->sps_alf_flag = 0;
    sps->sps_htdf_flag = 0;
    sps->sps_admvp_flag = 0;
    sps->sps_eipd_flag = 0;
    sps->sps_iqt_flag = 0;
    sps->sps_cm_init_flag = 0;
    sps->sps_rpl_flag = 0;
    sps->sps_pocs_flag = 0;
    sps->log2_sub_gop_length = (int)(log2(ctx->param.gop_size) + .5);
    ctx->ref_pic_gap_length = ctx->param.ref_pic_gap_length;
    sps->log2_ref_pic_gap_length = (int)(log2(ctx->param.ref_pic_gap_length) + .5);
    sps->vui_parameters_present_flag = 0;
    set_vui(ctx, &(sps->vui_parameters));
    sps->sps_dquant_flag = 0; /* Active SPSs shall have sps_sps_dquant_flag equal to 0 only */

    if (ctx->param.chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        eveye_copy_chroma_qp_mapping_params(&(sps->chroma_qp_table_struct), &(ctx->param.chroma_qp_table_struct));
    }

    sps->picture_cropping_flag = ctx->cdsc.picture_cropping_flag;
    if (sps->picture_cropping_flag)
    {
        sps->picture_crop_left_offset = ctx->cdsc.picture_crop_left_offset;
        sps->picture_crop_right_offset = ctx->cdsc.picture_crop_right_offset;
        sps->picture_crop_top_offset = ctx->cdsc.picture_crop_top_offset;
        sps->picture_crop_bottom_offset = ctx->cdsc.picture_crop_bottom_offset;
    }
}

static void set_pps(EVEYE_CTX * ctx, EVEY_PPS * pps)
{
    pps->single_tile_in_pic_flag = 1;
    pps->constrained_intra_pred_flag = ctx->cdsc.constrained_intra_pred;
    pps->cu_qp_delta_enabled_flag = EVEY_ABS(ctx->cdsc.use_dqp);
    pps->cu_qp_delta_area = 0;
    pps->num_tile_rows_minus1 = 0;
    pps->num_tile_columns_minus1 = 0;
    pps->uniform_tile_spacing_flag = 1;
    pps->tile_offset_lens_minus1 = 31;
    pps->arbitrary_slice_present_flag = 0;
    pps->tile_id_len_minus1 = 0;
    pps->num_ref_idx_default_active_minus1[LIST_0] = 0;
    pps->num_ref_idx_default_active_minus1[LIST_1] = 0;
}

typedef struct _QP_ADAPT_PARAM
{
    int    qp_offset_layer;
    double qp_offset_model_offset;
    double qp_offset_model_scale;

} QP_ADAPT_PARAM;

QP_ADAPT_PARAM qp_adapt_param_ra[8] = 
{
    {-3,  0.0000, 0.0000},
    { 1,  0.0000, 0.0000},
    { 1, -4.8848, 0.2061},
    { 4, -5.7476, 0.2286},
    { 5, -5.9000, 0.2333},
    { 6, -7.1444, 0.3000},
    { 7, -7.1444, 0.3000},
    { 8, -7.1444, 0.3000},
};

QP_ADAPT_PARAM qp_adapt_param_ld[8] =
{
    {-1,  0.0000, 0.0000 },
    { 1,  0.0000, 0.0000 },
    { 4, -6.5000, 0.2590 },
    { 4, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
};

QP_ADAPT_PARAM qp_adapt_param_ai[8] =
{
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
};

static void set_sh(EVEYE_CTX * ctx, EVEY_SH * sh)
{
    double qp;
    int qp_l_i;
    int qp_c_i;

    QP_ADAPT_PARAM * qp_adapt_param = ctx->param.max_b_frames == 0 ? (ctx->param.i_period == 1 ? qp_adapt_param_ai : qp_adapt_param_ld) : qp_adapt_param_ra;

    sh->no_output_of_prior_pics_flag = 0;
    sh->slice_deblocking_filter_flag = (ctx->param.use_deblock) ? 1 : 0;
    sh->sh_deblock_alpha_offset = 0;
    sh->sh_deblock_beta_offset = 0;
    sh->num_ref_idx_active_override_flag = 0;

    /* set lambda */
    qp = EVEY_CLIP3(0, MAX_QUANT, (ctx->param.qp_incread_frame != 0 && (int)(ctx->poc.poc_val) >= ctx->param.qp_incread_frame) ? ctx->sh.qp + 1.0 : ctx->sh.qp);

    sh->dqp = EVEY_ABS(ctx->param.use_dqp);

    if(ctx->param.use_hgop)
    {
        double dqp_offset;
        int qp_offset;

        qp += qp_adapt_param[ctx->slice_depth].qp_offset_layer;
        dqp_offset = qp * qp_adapt_param[ctx->slice_depth].qp_offset_model_scale + qp_adapt_param[ctx->slice_depth].qp_offset_model_offset + 0.5;

        qp_offset = (int)floor(EVEY_CLIP3(0.0, 3.0, dqp_offset));
        qp += qp_offset;
    }

    sh->qp   = (u8)EVEY_CLIP3(0, MAX_QUANT, qp);
    sh->qp_u_offset = ctx->cdsc.cb_qp_offset;
    sh->qp_v_offset = ctx->cdsc.cr_qp_offset;
    sh->qp_u = (s8)EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_u_offset);
    sh->qp_v = (s8)EVEY_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_v_offset);
    sh->sh_deblock_alpha_offset = 0;
    sh->sh_deblock_beta_offset = 0;

    qp_l_i = sh->qp;
    ctx->lambda[0] = 0.57 * pow(2.0, (qp_l_i - 12.0) / 3.0);
    qp_c_i = p_evey_tbl_qp_chroma_dynamic[0][sh->qp_u];
    ctx->dist_chroma_weight[0] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    qp_c_i = p_evey_tbl_qp_chroma_dynamic[1][sh->qp_v];
    ctx->dist_chroma_weight[1] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    ctx->lambda[1] = ctx->lambda[0] / ctx->dist_chroma_weight[0];
    ctx->lambda[2] = ctx->lambda[0] / ctx->dist_chroma_weight[1];
    ctx->sqrt_lambda[0] = sqrt(ctx->lambda[0]);
    ctx->sqrt_lambda[1] = sqrt(ctx->lambda[1]);
    ctx->sqrt_lambda[2] = sqrt(ctx->lambda[2]);
}

static int set_active_pps_info(EVEYE_CTX * ctx)
{
    int active_pps_id = ctx->sh.slice_pic_parameter_set_id;
    evey_mcpy(&(ctx->pps), &(ctx->pps_array[active_pps_id]), sizeof(EVEY_PPS));

    return EVEY_OK;
}

static int eveye_eco_tree(EVEYE_CTX * ctx, EVEYE_CORE * core, int x0, int y0, int cup, int cuw, int cuh, int cud)
{
    int ret = EVEY_OK;   
    EVEYE_BSW * bs;
    s8  split_mode;

    evey_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->ctu_size, ctx->map_cu_data[core->ctu_num].split_mode);
    
    bs = &ctx->bs;
    
    if(split_mode != NO_SPLIT) /* quad-tree split (SPLIT_QUAD) */
    {
        /* split_cu_flag */
        eveye_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->ctu_size);

        EVEY_SPLIT_STRUCT split_struct;
        evey_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_ctu_size - MIN_CU_LOG2, &split_struct);

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int sub_cuw = split_struct.width[part_num];
            int sub_cuh = split_struct.height[part_num];
            int x_pos = split_struct.x_pos[part_num];
            int y_pos = split_struct.y_pos[part_num];

            if(x_pos < ctx->w && y_pos < ctx->h) /* only process CUs inside a picture */
            {
                ret = eveye_eco_tree(ctx, core, x_pos, y_pos, split_struct.cup[part_num], sub_cuw, sub_cuh, split_struct.cud[part_num]);
                evey_assert_g(EVEY_SUCCEEDED(ret), ERR);
            }
        }
    }
    else /* NO_SPLIT */
    {
        evey_assert(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);  /* should be inside a picture */

        if(cuw > ctx->min_cu_size || cuh > ctx->min_cu_size)
        {
            /* split_cu_flag */
            eveye_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->ctu_size);
        }

        /* encode a CU */
        ret = eveye_eco_cu(ctx, core, x0, y0, cup, cuw, cuh);
        evey_assert_g(EVEY_SUCCEEDED(ret), ERR);
    }

    return EVEY_OK;
ERR:
    return ret;
}

static int eveye_ready(EVEYE_CTX * ctx)
{
    EVEYE_CORE * core = NULL;
    int          w, h, ret, i;
    s64          size;

    evey_assert(ctx);
    core = core_alloc(ctx->param.chroma_format_idc);
    evey_assert_gv(core != NULL, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);

    /* set various value */
    ctx->core = core;

    w = ctx->w = ctx->param.w;
    h = ctx->h = ctx->param.h;

    eveye_init_bits_est();

    ctx->ctu_size = 64;
    ctx->min_cu_size = 1 << 2;
    ctx->log2_min_cu_size = 2;
    ctx->log2_ctu_size = EVEY_CONV_LOG2(ctx->ctu_size);
    ctx->w_ctu = (w + ctx->ctu_size - 1) >> ctx->log2_ctu_size;
    ctx->h_ctu = (h + ctx->ctu_size - 1) >> ctx->log2_ctu_size;
    ctx->f_ctu = ctx->w_ctu * ctx->h_ctu;
    ctx->w_scu = (w + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->h_scu = (h + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    ctx->f_scu = ctx->w_scu * ctx->h_scu;

    /*  allocate CU data map*/
    if(ctx->map_cu_data == NULL)
    {
        size = sizeof(EVEYE_CU_DATA) * ctx->f_ctu;
        ctx->map_cu_data = (EVEYE_CU_DATA*)evey_malloc_fast(size);
        evey_assert_gv(ctx->map_cu_data, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
        evey_mset_x64a(ctx->map_cu_data, 0, size);

        for(i = 0; i < (int)ctx->f_ctu; i++)
        {
            eveye_create_cu_data(ctx->map_cu_data + i, ctx->log2_ctu_size - MIN_CU_LOG2, ctx->log2_ctu_size - MIN_CU_LOG2, ctx->param.chroma_format_idc);
        }
    }

    /* allocate maps */
    if(ctx->map_scu == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_scu = evey_malloc_fast(size);
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
        ctx->map_ipm = evey_malloc_fast(size);
        evey_assert_gv(ctx->map_ipm, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
        evey_mset(ctx->map_ipm, -1, size);
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
    pa.chroma_format_idc = ctx->cdsc.chroma_format_idc;
    pa.bit_depth         = ctx->cdsc.codec_bit_depth;
    ret = evey_picman_init(&ctx->dpbm, MAX_PB_SIZE, MAX_NUM_REF_PICS, &pa);
    evey_assert_g(EVEY_SUCCEEDED(ret), ERR);

    ctx->pic_cnt = 0;
    ctx->pic_icnt = -1;
    ctx->poc.poc_val = 0;
    ctx->pico_max_cnt = 1 + (ctx->param.max_b_frames << 1) ;
    ctx->frm_rnum = ctx->param.max_b_frames;
    ctx->sh.qp = ctx->param.qp;

    for(i = 0; i < ctx->pico_max_cnt; i++)
    {
        ctx->pico_buf[i] = (EVEYE_PICO*)evey_malloc(sizeof(EVEYE_PICO));
        evey_assert_gv(ctx->pico_buf[i], ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
        evey_mset(ctx->pico_buf[i], 0, sizeof(EVEYE_PICO));
    }

    return EVEY_OK;
ERR:
    for (i = 0; i < (int)ctx->f_ctu; i++)
    {
        eveye_delete_cu_data(ctx->map_cu_data + i, ctx->log2_ctu_size - MIN_CU_LOG2, ctx->log2_ctu_size - MIN_CU_LOG2);
    }

    evey_mfree_fast(ctx->map_cu_data);
    evey_mfree_fast(ctx->map_ipm);

    for(i = 0; i < ctx->pico_max_cnt; i++)
    {
        evey_mfree_fast(ctx->pico_buf[i]);
    }

    if(core)
    {
        core_free(core);
    }
    return ret;
}

static void eveye_flush(EVEYE_CTX * ctx)
{
    int i;
    evey_assert(ctx);

    evey_mfree_fast(ctx->map_scu);
    evey_mfree_fast(ctx->map_split);
    for(i = 0; i < (int)ctx->f_ctu; i++)
    {
        eveye_delete_cu_data(ctx->map_cu_data + i, ctx->log2_ctu_size - MIN_CU_LOG2, ctx->log2_ctu_size - MIN_CU_LOG2);
    }
    evey_mfree_fast(ctx->map_cu_data);
    evey_mfree_fast(ctx->map_ipm);
    
    if(ctx->cdsc.rdo_dbk_switch)
    {
        evey_picbuf_free(ctx->pic_dbk);
    }

    evey_picman_deinit(&ctx->dpbm);
    core_free(ctx->core);

    for(i = 0; i < ctx->pico_max_cnt; i++)
    {
        evey_mfree_fast(ctx->pico_buf[i]);
    }
    for(i = 0; i < EVEYE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i]) ctx->inbuf[i]->release(ctx->inbuf[i]);
    }
}

static int eveye_picbuf_get_inbuf(EVEYE_CTX * ctx, EVEY_IMGB ** imgb)
{
    int i, opt, align[EVEY_IMGB_MAX_PLANE], pad[EVEY_IMGB_MAX_PLANE];

    for(i = 0; i < EVEYE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i] == NULL)
        {
            opt = EVEY_IMGB_OPT_NONE;

            /* set align value*/
            align[0] = MIN_CU_SIZE;
            align[1] = MIN_CU_SIZE >> 1;
            align[2] = MIN_CU_SIZE >> 1;

            /* no padding */
            pad[0] = 0;
            pad[1] = 0;
            pad[2] = 0;

            int cs = EVEY_CS_SET(CF_FROM_CFI(ctx->param.chroma_format_idc), ctx->param.bit_depth, 0);
            *imgb = evey_imgb_create(ctx->param.w, ctx->param.h, cs, opt, pad, align);
            evey_assert_rv(*imgb != NULL, EVEY_ERR_OUT_OF_MEMORY);

            ctx->inbuf[i] = *imgb;

            (*imgb)->addref(*imgb);
            return EVEY_OK;
        }
        else if(ctx->inbuf[i]->getref(ctx->inbuf[i]) == 1)
        {
            *imgb = ctx->inbuf[i];

            (*imgb)->addref(*imgb);
            return EVEY_OK;
        }
    }

    return EVEY_ERR_UNEXPECTED;
}

// XXNN initializing the picture buffer used to store the intra predictor context
// FIXME the allocated buffers are never deallocated
void allocate_intra_buffer(EVEYE_CTX * ctx)
{
	EVEY_PIC**	   pi_ctx = &ctx->pintra.recon_fig;
	EVEY_PIC**	   ctx_pred = &ctx->pintra.pred_fig;
	int * ret = NULL;

	(*pi_ctx) = evey_picbuf_alloc(ctx->w, ctx->h, 0, 0, 0, 10, ret);

	(*ctx_pred) = evey_picbuf_alloc(ctx->w, ctx->h, 0, 0, 0, 10, ret);
	
}

static void decide_normal_gop(EVEYE_CTX * ctx, u32 pic_imcnt)
{
    int i_period, gop_size, pos;
    u32        pic_icnt_b;

    i_period = ctx->param.i_period;
    gop_size = ctx->param.gop_size;

    if(i_period == 0 && pic_imcnt == 0)
    {
        ctx->sh.slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else if((i_period != 0) && pic_imcnt % i_period == 0)
    {
        ctx->sh.slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else if(pic_imcnt % gop_size == 0)
    {
        ctx->sh.slice_type = ctx->cdsc.inter_slice_type;
        ctx->slice_ref_flag = 1;
        ctx->slice_depth = FRM_DEPTH_1;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else
    {
        ctx->sh.slice_type = ctx->cdsc.inter_slice_type;
        if(ctx->param.use_hgop)
        {
            pos = (pic_imcnt % gop_size) - 1;
            ctx->slice_depth = tbl_slice_depth[gop_size >> 2][pos];
            int tid = ctx->slice_depth - (ctx->slice_depth > 0);
            evey_poc_derivation(&ctx->sps, tid, &ctx->poc);
            ctx->poc.poc_val = ctx->poc.poc_val;

            if(gop_size >= 2)
            {
                ctx->slice_ref_flag = (ctx->slice_depth == tbl_slice_depth[gop_size >> 2][gop_size - 2] ? 0 : 1);
            }
            else
            {
                ctx->slice_ref_flag = 1;
            }
        }
        else
        {
            pos = (pic_imcnt % gop_size) - 1;
            ctx->slice_depth = FRM_DEPTH_2;
            ctx->poc.poc_val = ((pic_imcnt / gop_size) * gop_size) - gop_size + pos + 1;
            ctx->slice_ref_flag = 0;
        }
        /* find current encoding picture's(B picture) pic_icnt */
        pic_icnt_b = ctx->poc.poc_val;

        /* find pico again here */
        ctx->pico_idx = (u8)(pic_icnt_b % ctx->pico_max_cnt);
        ctx->pico = ctx->pico_buf[ctx->pico_idx];

        PIC_ORIG(ctx) = &ctx->pico->pic;
    }
}

/* slice_type / slice_depth / poc / PIC_ORIG setting */
static void decide_slice_type(EVEYE_CTX * ctx)
{
    u32 pic_imcnt, pic_icnt;
    int i_period, gop_size;
    int force_cnt = 0;

    i_period = ctx->param.i_period;
    gop_size = ctx->param.gop_size;
    pic_icnt = (ctx->pic_cnt + ctx->param.max_b_frames);
    pic_imcnt = pic_icnt;
    ctx->pico_idx = pic_icnt % ctx->pico_max_cnt;
    ctx->pico = ctx->pico_buf[ctx->pico_idx];
    PIC_ORIG(ctx) = &ctx->pico->pic;

    if(gop_size == 1) 
    {
        if (i_period == 1) /* IIII... */
        {
            ctx->sh.slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = pic_icnt;
            ctx->slice_ref_flag = 0;
        }
        else /* IPPP... */
        {
            pic_imcnt = (i_period > 0) ? pic_icnt % i_period : pic_icnt;
            if (pic_imcnt == 0)
            {
                ctx->sh.slice_type = SLICE_I;
                ctx->slice_depth = FRM_DEPTH_0;
                ctx->poc.poc_val = 0;
                ctx->slice_ref_flag = 1;
            }
            else
            {
                ctx->sh.slice_type = ctx->cdsc.inter_slice_type;

                if (ctx->param.use_hgop)
                {
                    ctx->slice_depth = tbl_slice_depth_P[ctx->param.ref_pic_gap_length >> 2][(pic_imcnt - 1) % ctx->param.ref_pic_gap_length];
                }
                else
                {
                    ctx->slice_depth = FRM_DEPTH_1;
                }
                ctx->poc.poc_val = (i_period > 0) ? ctx->pic_cnt % i_period : ctx->pic_cnt;
                ctx->slice_ref_flag = 1;
            }
        }
    }
    else /* include B Picture (gop_size = 2 or 4 or 8 or 16) */
    {
        if(pic_icnt == gop_size - 1) /* special case when sequence start */
        {
            ctx->sh.slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = 0;
            ctx->poc.prev_doc_offset = 0;
            ctx->poc.prev_poc_val = ctx->poc.poc_val;
            ctx->slice_ref_flag = 1;

            /* flush the first IDR picture */
            PIC_ORIG(ctx) = &ctx->pico_buf[0]->pic;
            ctx->pico = ctx->pico_buf[0];
        }
        else if(ctx->force_slice)
        {
            for(force_cnt = ctx->force_ignored_cnt; force_cnt < gop_size; force_cnt++)
            {
                pic_icnt = (ctx->pic_cnt + ctx->param.max_b_frames + force_cnt);
                pic_imcnt = pic_icnt;

                decide_normal_gop(ctx, pic_imcnt);

                if(ctx->poc.poc_val <= (int)ctx->pic_ticnt)
                {
                    break;
                }
            }
            ctx->force_ignored_cnt = force_cnt;
        }
        else /* normal GOP case */
        {
            decide_normal_gop(ctx, pic_imcnt);
        }
    }
    if (ctx->param.use_hgop && gop_size > 1)
    {
        ctx->nalu.nuh_temporal_id = ctx->slice_depth - (ctx->slice_depth > 0);
    }
    else
    {
        ctx->nalu.nuh_temporal_id = 0;
    }
}

static int eveye_encode_sps(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    /* update BSB */
    bitb->err = 0;

    EVEYE_BSW * bs = &ctx->bs;
    EVEY_SPS * sps = &ctx->sps;
    EVEY_NALU  nalu;

    bs->pdata[1] = &ctx->sbac_enc;

    /* nalu header */
    set_nalu(&nalu, EVEY_SPS_NUT, 0);

    int* size_field = (int*)(*(&bs->cur));
    u8* cur_tmp = bs->cur;

    eveye_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    set_sps(ctx, sps);
    evey_assert_rv(eveye_eco_sps(bs, sps) == EVEY_OK, EVEY_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    eveye_bsw_deinit(bs);

    /* write the bitstream size */
    *size_field = (int)(bs->cur - cur_tmp) - 4;

    return EVEY_OK;
}

static int eveye_generate_pps_array(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    set_pps(ctx, &(ctx->pps));

    int num_pps = 1;
    for(int i = 0; i < num_pps; i++)
    {
        ctx->pps.pps_pic_parameter_set_id = i;
        evey_mcpy(&(ctx->pps_array[i]), &(ctx->pps), sizeof(EVEY_PPS));
    }
    evey_mcpy(&(ctx->pps), &(ctx->pps_array[0]), sizeof(EVEY_PPS));

    return EVEY_OK;
}

static int eveye_encode_pps(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    /* update BSB */
    bitb->err = 0;

    EVEYE_BSW * bs = &ctx->bs;
    EVEY_SPS  * sps = &ctx->sps;
    EVEY_PPS  * pps = &ctx->pps;
    EVEY_NALU   nalu;

    bs->pdata[1] = &ctx->sbac_enc;

    /* nalu header */
    set_nalu(&nalu, EVEY_PPS_NUT, ctx->nalu.nuh_temporal_id);

    int* size_field = (int*)(*(&bs->cur));
    u8* cur_tmp = bs->cur;

    eveye_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    set_pps(ctx, pps);
    evey_assert_rv(eveye_eco_pps(bs, sps, pps) == EVEY_OK, EVEY_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    eveye_bsw_deinit(bs);

    /* write the bitstream size */
    *size_field = (int)(bs->cur - cur_tmp) - 4;

    return EVEY_OK;
}

static int eveye_enc_pic_prepare(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    EVEYE_PARAM * param;
    int           ret;
    int           size;

    evey_assert_rv(PIC_ORIG(ctx) != NULL, EVEY_ERR_UNEXPECTED);

    param = &ctx->param;
    ret = set_enc_param(ctx, param);
    evey_assert_rv(ret == EVEY_OK, ret);

    PIC_CURR(ctx) = evey_picman_get_empty_pic(&ctx->dpbm, &ret);
    evey_assert_rv(PIC_CURR(ctx) != NULL, ret);

    ctx->pic = PIC_CURR(ctx);
    ctx->map_refi = PIC_CURR(ctx)->map_refi;
    ctx->map_mv = PIC_CURR(ctx)->map_mv;

    if(ctx->cdsc.rdo_dbk_switch && ctx->pic_dbk == NULL)
    {
        ctx->pic_dbk = evey_pic_alloc(&ctx->dpbm.pa, &ret);
        evey_assert_rv(ctx->pic_dbk != NULL, ret);
    }

    decide_slice_type(ctx);

    ctx->ctu_cnt = ctx->f_ctu;
    ctx->slice_num = 0;

    if(ctx->sh.slice_type == SLICE_I)
    {
        ctx->last_intra_poc = ctx->poc.poc_val;
    }

    size = sizeof(s8) * ctx->f_scu * LIST_NUM;
    evey_mset_x64a(ctx->map_refi, -1, size);

    size = sizeof(s16) * ctx->f_scu * LIST_NUM * MV_D;
    evey_mset_x64a(ctx->map_mv, 0, size);

    /* initialize bitstream container */
    eveye_bsw_init(&ctx->bs, bitb->addr, bitb->bsize, NULL);

    /* clear map */
    evey_mset_x64a(ctx->map_scu, 0, sizeof(u32) * ctx->f_scu);

    /* encode parameter sets (SPS and PPS) */
    if(ctx->pic_cnt == 0 || (ctx->sh.slice_type == SLICE_I && ctx->param.use_closed_gop)) /* if nalu_type is IDR */
    {
        ret = eveye_encode_sps(ctx, bitb, stat);
        evey_assert_rv(ret == EVEY_OK, ret);

        ret = eveye_generate_pps_array(ctx, bitb, stat);
        evey_assert_rv(ret == EVEY_OK, ret);

        int number_pps = 1;
        for(int i = 0; i < number_pps; i++)
        {
            ret = eveye_encode_pps(ctx, bitb, stat);
            evey_assert_rv(ret == EVEY_OK, ret);
        }
    }

    ctx->sh.slice_pic_parameter_set_id = ctx->pico->pic.imgb->imgb_active_pps_id;
    set_active_pps_info(ctx);
    PIC_CURR(ctx)->imgb->imgb_active_pps_id = ctx->pico->pic.imgb->imgb_active_pps_id;

    return EVEY_OK;
}

static int eveye_enc_pic_finish(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    EVEY_IMGB * imgb_o, * imgb_c;
    int         ret;
    int         i, j;

    evey_mset(stat, 0, sizeof(EVEYE_STAT));

    /* adding picture sign */
    if (ctx->param.use_pic_sign) /* This is a non-normative sei. EVEY decoder should ignore this. */
    {
        EVEYE_BSW * bs = &ctx->bs;
        EVEY_NALU   sei_nalu;

        set_nalu(&sei_nalu, EVEY_SEI_NUT, ctx->nalu.nuh_temporal_id);

        int* size_field = (int*)(*(&bs->cur));
        u8* cur_tmp = bs->cur;

        eveye_eco_nalu(bs, &sei_nalu);

        ret = eveye_eco_sei(ctx, bs);
        evey_assert_rv(ret == EVEY_OK, ret);

        eveye_bsw_deinit(bs);
        stat->sei_size = (int)(bs->cur - cur_tmp);
        *size_field = stat->sei_size - 4;
    }

    /* expand current encoding picture, if needs */
    ctx->dpbm.pa.fn_expand(PIC_CURR(ctx));

    /* picture buffer management */
    ret = evey_picman_put_pic(ctx, PIC_CURR(ctx), 0);

    evey_assert_rv(ret == EVEY_OK, ret);

    imgb_o = PIC_ORIG(ctx)->imgb;
    evey_assert(imgb_o != NULL);

    imgb_c = PIC_CURR(ctx)->imgb;
    evey_assert(imgb_c != NULL);

    /* set stat */
    stat->write = EVEYE_BSW_GET_WRITE_BYTE(&ctx->bs);
    stat->nalu_type = (ctx->pic_cnt == 0 || (ctx->sh.slice_type == SLICE_I && ctx->param.use_closed_gop)) ? EVEY_IDR_NUT : EVEY_NONIDR_NUT;
    stat->stype = ctx->sh.slice_type;
    stat->fnum = ctx->pic_cnt;
    stat->qp = ctx->sh.qp;
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

    ctx->pic_cnt++; /* increase picture count */
    ctx->param.f_ifrm = 0; /* clear force-IDR flag */
    ctx->pico->is_used = 0;

    imgb_c->ts[0] = bitb->ts[0] = imgb_o->ts[0];
    imgb_c->ts[1] = bitb->ts[1] = imgb_o->ts[1];
    imgb_c->ts[2] = bitb->ts[2] = imgb_o->ts[2];
    imgb_c->ts[3] = bitb->ts[3] = imgb_o->ts[3];

    if(imgb_o)
    {
        imgb_o->release(imgb_o);
    }

    return EVEY_OK;
}

/* encode one picture */
static int eveye_enc_pic(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    EVEYE_CORE * core;
    EVEYE_BSW  * bs;
    EVEY_SH    * sh;
    int          ret;
    u8         * curr_temp = ctx->bs.cur;

    /* initialize reference pictures */
    ret = evey_picman_refp_init(ctx);
    evey_assert_rv(ret == EVEY_OK, ret);

    /* initialize mode decision for frame encoding */
    ret = ctx->fn_mode_init_frame(ctx);
    evey_assert_rv(ret == EVEY_OK, ret);

    /* frame level processing */
    ret = ctx->fn_mode_analyze_frame(ctx);
    evey_assert_rv(ret == EVEY_OK, ret);

    bs = &ctx->bs;
    core = ctx->core;
    sh = &ctx->sh;

    if((int)ctx->poc.poc_val > last_intra_poc)
    {
        last_intra_poc = EVEY_INT_MAX;
    }
    if(ctx->sh.slice_type == SLICE_I)
    {
        last_intra_poc = ctx->poc.poc_val;
    }

    core->x_ctu = core->y_ctu = 0;
    core->x_pel = core->y_pel = 0;
    core->ctu_num = 0;
    ctx->ctu_cnt = ctx->f_ctu;

    /* set nalu header */
    set_nalu(&ctx->nalu, (ctx->pic_cnt == 0 || (ctx->sh.slice_type == SLICE_I && ctx->param.use_closed_gop)) ? EVEY_IDR_NUT : EVEY_NONIDR_NUT, ctx->nalu.nuh_temporal_id);

    /* set slice header */
    set_sh(ctx, sh);

    core->qp_y = ctx->sh.qp + 6 * ctx->sps.bit_depth_luma_minus8;
    core->qp_u = p_evey_tbl_qp_chroma_dynamic[0][sh->qp_u] + 6 * ctx->sps.bit_depth_chroma_minus8;
    core->qp_v = p_evey_tbl_qp_chroma_dynamic[1][sh->qp_v] + 6 * ctx->sps.bit_depth_chroma_minus8;
    core->bs_temp.pdata[1] = &core->s_temp_run;

#if TRACE_RDO_EXCLUDE_I
    if(ctx->sh.slice_type != SLICE_I)
    {
#endif
        EVEY_TRACE_SET(0);
#if TRACE_RDO_EXCLUDE_I
    }
#endif
    ctx->sh.qp_prev_eco = ctx->sh.qp;
    ctx->sh.qp_prev_mode = ctx->sh.qp;
    core->dqp_data[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2].prev_qp = ctx->sh.qp_prev_mode;
    core->dqp_curr_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2].curr_qp = ctx->sh.qp;
    core->dqp_curr_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2].prev_qp = ctx->sh.qp;

    if(ctx->slice_num == 0 && !(ctx->pic_cnt == 0 || (ctx->sh.slice_type == SLICE_I && ctx->param.use_closed_gop))) /* first slice and not IDR picture */
    {
        eveye_bsw_init(&ctx->bs, (u8*)bitb->addr, bitb->bsize, NULL);
    }
    else
    {
        eveye_bsw_init_slice(&ctx->bs, (u8*)curr_temp, bitb->bsize, NULL);
    }
    
    int* size_field = (int*)(*(&bs->cur));
    u8* cur_tmp = bs->cur;

    /* encode nalu header */
    ret = eveye_eco_nalu(bs, &ctx->nalu);
    evey_assert_rv(ret == EVEY_OK, ret);

    /* encode slice header */
    EVEYE_BSW bs_sh;
    evey_mcpy(&bs_sh, bs, sizeof(EVEYE_BSW));
#if TRACE_HLS
    s32 tmp_fp_point = ftell(fp_trace);
#endif
    ret = eveye_eco_sh(bs, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
    evey_assert_rv(ret == EVEY_OK, ret);

    ctx->sh.qp_prev_eco = ctx->sh.qp;

    /* Initialization of arithmetic encoder  */
    eveye_sbac_reset(ctx, GET_SBAC_ENC(bs));
    eveye_sbac_reset(ctx, &core->s_curr_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2]);

    /* set entry point */
    core->x_ctu = 0;
    core->y_ctu = 0;

    int bef_cu_qp = ctx->sh.qp_prev_eco;

    /* CTU encoding loop */
    while(ctx->ctu_cnt > 0)
    {
        evey_update_core_loc_param(ctx, core);

        /* initialize structures for mode decision */
        ret = ctx->fn_mode_init_ctu(ctx, core);
        evey_assert_rv(ret == EVEY_OK, ret);

        SBAC_LOAD(core->s_curr_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2], *GET_SBAC_ENC(bs));
        core->s_curr_best[ctx->log2_ctu_size - 2][ctx->log2_ctu_size - 2].is_bit_count = 1;

        /* mode decision for a CTU */
        ret = ctx->fn_mode_analyze_ctu(ctx, core);
        evey_assert_rv(ret == EVEY_OK, ret);

        ctx->sh.qp_prev_eco = bef_cu_qp;

        /* entropy coding for a CTU */
        ret = eveye_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->ctu_size, ctx->ctu_size, 0);
        evey_assert_rv(ret == EVEY_OK, ret);

        bef_cu_qp = ctx->sh.qp_prev_eco;
        core->x_ctu++;
        if(core->x_ctu >= ctx->w_ctu)
        {
            core->x_ctu = 0;
            core->y_ctu++;
        }
        ctx->ctu_cnt--;
    } /* end of CTU processing loop */

    /* write tile_end_flag */
    eveye_eco_tile_end_flag(bs, 1);
    eveye_sbac_finish(bs);

    /* cabac_zero_word coding */
    {
        unsigned int bin_counts_in_units = 0;
        unsigned int num_bytes_in_units = (int)(bs->cur - cur_tmp) - 4;
        int log2_sub_widthC_subHeightC = 2;
        int min_cu_w = ctx->min_cu_size;
        int min_cu_h = ctx->min_cu_size;
        int padded_w = ((ctx->w + min_cu_w - 1) / min_cu_w) * min_cu_w;
        int padded_h = ((ctx->h + min_cu_h - 1) / min_cu_h) * min_cu_h;
        int raw_bits = padded_w * padded_h * ((ctx->sps.bit_depth_luma_minus8 + 8) + (ctx->sps.chroma_format_idc != 0 ? 2 * ((ctx->sps.bit_depth_chroma_minus8 + 8) >> log2_sub_widthC_subHeightC) : 0));
        unsigned int threshold = (CABAC_ZERO_PARAM / 3) * num_bytes_in_units + (raw_bits / 32);
        if(bin_counts_in_units >= threshold)
        {
            unsigned int target_num_bytes_in_units = ((bin_counts_in_units - (raw_bits / 32)) * 3 + (CABAC_ZERO_PARAM - 1)) / CABAC_ZERO_PARAM;
            if(target_num_bytes_in_units > num_bytes_in_units)
            {
                unsigned int num_add_bytes_needed = target_num_bytes_in_units - num_bytes_in_units;
                unsigned int num_add_cabac_zero_words = (num_add_bytes_needed + 2) / 3;
                unsigned int num_add_cabac_zero_bytes = num_add_cabac_zero_words * 3;
                for(unsigned int i = 0; i < num_add_cabac_zero_words; i++)
                {
                    eveye_bsw_write(bs, 0, 16);
                }
            }
        }
    }

    eveye_bsw_deinit(bs);
    *size_field = (int)(bs->cur - cur_tmp) - 4; /* set nal_unit_size field */
    curr_temp = bs->cur;

    /* deblocking filter */
    if(sh->slice_deblocking_filter_flag)
    {
#if TRACE_DBF
        EVEY_TRACE_SET(1);
#endif
        ret = ctx->fn_deblock(ctx);
        evey_assert_rv(ret == EVEY_OK, ret);
#if TRACE_DBF
        EVEY_TRACE_SET(0);
#endif
    }

    return EVEY_OK;
}

static int eveye_enc(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    int ret;
    int gop_size, pic_cnt;

    pic_cnt = ctx->pic_icnt - ctx->frm_rnum;
    gop_size = ctx->param.gop_size;

    ctx->force_slice = ((ctx->pic_ticnt % gop_size >= ctx->pic_ticnt - pic_cnt + 1) && FORCE_OUT(ctx)) ? 1 : 0;

    evey_assert_rv(bitb->addr && bitb->bsize > 0, EVEY_ERR_INVALID_ARGUMENT);

    /* initialize variables for a picture encoding */
    ret = ctx->fn_enc_pic_prepare(ctx, bitb, stat);
    evey_assert_rv(ret == EVEY_OK, ret);

    /* encode one picture */
    ret = ctx->fn_enc_pic(ctx, bitb, stat);
    evey_assert_rv(ret == EVEY_OK, ret);

    /* finishing of encoding a picture */
    ctx->fn_enc_pic_finish(ctx, bitb, stat);
    evey_assert_rv(ret == EVEY_OK, ret);

    return EVEY_OK;
}

static int eveye_push_frm(EVEYE_CTX * ctx, EVEY_IMGB * img)
{
    EVEY_PIC  * pic;
    EVEY_IMGB * imgb;
    int         ret;

    ret = ctx->fn_get_inbuf(ctx, &imgb);
    evey_assert_rv(EVEY_OK == ret, ret);

    imgb->cs = EVEY_CS_SET(CF_FROM_CFI(ctx->cdsc.chroma_format_idc), ctx->cdsc.codec_bit_depth, 0);
    evey_imgb_cpy(imgb, img);

    ctx->pic_icnt++;
    ctx->pico_idx = ctx->pic_icnt % ctx->pico_max_cnt;
    ctx->pico = ctx->pico_buf[ctx->pico_idx];
    ctx->pico->pic_icnt = ctx->pic_icnt;
    ctx->pico->is_used = 1;
    pic = &ctx->pico->pic;
    PIC_ORIG(ctx) = pic;

    /* set pushed image to current input (original) image */
    evey_mset(pic, 0, sizeof(EVEY_PIC));

    pic->buf_y = imgb->baddr[0];
    pic->buf_u = imgb->baddr[1];
    pic->buf_v = imgb->baddr[2];

    pic->y = imgb->a[0];
    pic->u = imgb->a[1];
    pic->v = imgb->a[2];

    pic->w_l = imgb->w[0];
    pic->h_l = imgb->h[0];
    pic->w_c = imgb->w[1];
    pic->h_c = imgb->h[1];

    pic->s_l = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->imgb = imgb;

    return EVEY_OK;
}

static int eveye_platform_init(EVEYE_CTX * ctx)
{
    int ret = EVEY_ERR_UNKNOWN;

    /* create mode decision */
    ret = eveye_mode_create(ctx, 0);
    evey_assert_rv(EVEY_OK == ret, ret);

    /* create intra prediction analyzer */
    ret = eveye_pintra_create(ctx, 0);
    evey_assert_rv(EVEY_OK == ret, ret);

    /* create inter prediction analyzer */
    ret = eveye_pinter_create(ctx, 0);
    evey_assert_rv(EVEY_OK == ret, ret);
    
    ctx->pic_dbk = NULL;
    ctx->fn_ready = eveye_ready;
    ctx->fn_flush = eveye_flush;
    ctx->fn_enc = eveye_enc;
    ctx->fn_enc_pic = eveye_enc_pic;
    ctx->fn_enc_pic_prepare = eveye_enc_pic_prepare;
    ctx->fn_enc_pic_finish = eveye_enc_pic_finish;
    ctx->fn_push = eveye_push_frm;
    ctx->fn_deblock = evey_deblock;
    ctx->fn_get_inbuf = eveye_picbuf_get_inbuf;
    ctx->pf = NULL;

    ret = evey_scan_tbl_init();
    evey_assert_rv(ret == EVEY_OK, ret);

    eveye_init_err_scale(ctx->cdsc.codec_bit_depth);

    return EVEY_OK;
}

static void eveye_platform_deinit(EVEYE_CTX * ctx)
{
    evey_assert(ctx->pf == NULL);

    ctx->fn_ready = NULL;
    ctx->fn_flush = NULL;
    ctx->fn_enc = NULL;
    ctx->fn_enc_pic = NULL;
    ctx->fn_enc_pic_prepare = NULL;
    ctx->fn_enc_pic_finish = NULL;
    ctx->fn_push = NULL;
    ctx->fn_deblock = NULL;
    ctx->fn_get_inbuf = NULL;

    evey_scan_tbl_delete();
}

static int check_frame_delay(EVEYE_CTX * ctx)
{
    if(ctx->pic_icnt < ctx->frm_rnum)
    {
        return EVEY_OK_OUT_NOT_AVAILABLE;
    }
    return EVEY_OK;
}

static int check_more_frames(EVEYE_CTX * ctx)
{
    EVEYE_PICO * pico;
    int          i;

    if(FORCE_OUT(ctx))
    {
        /* pseudo eveye_push() for bumping process ****************/
        ctx->pic_icnt++;
        /**********************************************************/

        for(i = 0; i < ctx->pico_max_cnt; i++)
        {
            pico = ctx->pico_buf[i];
            if(pico != NULL)
            {
                if(pico->is_used == 1)
                {
                    return EVEY_OK;
                }
            }
        }

        return EVEY_OK_NO_MORE_FRM;
    }

    return EVEY_OK;
}

EVEYE eveye_create(EVEYE_CDSC * cdsc, int * err)
{
    EVEYE_CTX * ctx;
    int         ret;
    
#if ENC_DEC_TRACE
#if TRACE_DBF
    fp_trace = fopen("enc_trace_dbf.txt", "w+");
#else
    fp_trace = fopen("enc_trace.txt", "w+");
#endif
#if TRACE_HLS
    EVEY_TRACE_SET(1);
#endif
#endif

    ctx = NULL;

    /* memory allocation for ctx and core structure */
    ctx = ctx_alloc();
    evey_assert_gv(ctx != NULL, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);
    evey_mcpy(&ctx->cdsc, cdsc, sizeof(EVEYE_CDSC));

    /* set default value for encoding parameter */
    ret = set_init_param(cdsc, &ctx->param);
    evey_assert_g(ret == EVEY_OK, ERR);

    ret = eveye_platform_init(ctx);
    evey_assert_g(ret == EVEY_OK, ERR);

    if(ctx->fn_ready != NULL)
    {
        ret = ctx->fn_ready(ctx);
        evey_assert_g(ret == EVEY_OK, ERR);
    }

    /* set default value for ctx */
    ctx->magic = EVEYE_MAGIC_CODE;
    ctx->id = (EVEYE)ctx;
    //XXNN
    allocate_intra_buffer(ctx);
    return (ctx->id);
ERR:
    if(ctx)
    {
        eveye_platform_deinit(ctx);
        ctx_free(ctx);
    }
    if(err) *err = ret;
    return NULL;
}

void eveye_delete(EVEYE id)
{
    EVEYE_CTX * ctx;

    EVEYE_ID_TO_CTX_R(id, ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif

    if(ctx->fn_flush != NULL)
    {
        ctx->fn_flush(ctx);
    }
    eveye_platform_deinit(ctx);

    ctx_free(ctx);
}

int eveye_encode(EVEYE id, EVEY_BITB * bitb, EVEYE_STAT * stat)
{
    EVEYE_CTX * ctx;

    EVEYE_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv(ctx->fn_enc, EVEY_ERR_UNEXPECTED);

    /* bumping - check whether input pictures are remaining or not in pico_buf[] */
    if(EVEY_OK_NO_MORE_FRM == check_more_frames(ctx))
    {
        return EVEY_OK_NO_MORE_FRM;
    }

    /* store input picture and return if needed */
    if(EVEY_OK_OUT_NOT_AVAILABLE == check_frame_delay(ctx))
    {
        return EVEY_OK_OUT_NOT_AVAILABLE;
    }

    /* update BSB */
    bitb->err = 0;

    return ctx->fn_enc(ctx, bitb, stat);
}

int eveye_push(EVEYE id, EVEY_IMGB * img)
{
    EVEYE_CTX * ctx;

    EVEYE_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv(ctx->fn_push, EVEY_ERR_UNEXPECTED);

    return ctx->fn_push(ctx, img);
}

int eveye_config(EVEYE id, int cfg, void * buf, int * size)
{
    EVEYE_CTX * ctx;
    int         t0;
    EVEY_IMGB * imgb;

    EVEYE_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);

    switch(cfg)
    {
        /* set config **********************************************************/
        case EVEYE_CFG_SET_FORCE_OUT:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.force_output = (t0) ? 1 : 0;
            /* store total input picture count at this time */
            ctx->pic_ticnt = ctx->pic_icnt;
            break;
        case EVEYE_CFG_SET_FINTRA:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.f_ifrm = t0;
            break;
        case EVEYE_CFG_SET_QP:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            evey_assert_rv(t0 >= MIN_QUANT && t0 <= MAX_QUANT, \
                           EVEY_ERR_INVALID_ARGUMENT);
            ctx->param.qp = t0;
            break;
        case EVEYE_CFG_SET_FPS:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            evey_assert_rv(t0 > 0, EVEY_ERR_INVALID_ARGUMENT);
            ctx->param.fps = t0;
            break;
        case EVEYE_CFG_SET_I_PERIOD:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            evey_assert_rv(t0 >= 0, EVEY_ERR_INVALID_ARGUMENT);
            ctx->param.i_period = t0;
            break;
        case EVEYE_CFG_SET_QP_MIN:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            evey_assert_rv(t0 >= MIN_QUANT, EVEY_ERR_INVALID_ARGUMENT);
            ctx->param.qp_min = t0;
            break;
        case EVEYE_CFG_SET_QP_MAX:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            evey_assert_rv(t0 <= MAX_QUANT, EVEY_ERR_INVALID_ARGUMENT);
            ctx->param.qp_max = t0;
            break;
        case EVEYE_CFG_SET_USE_DEBLOCK:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.use_deblock = t0;
            break;
        case EVEYE_CFG_SET_USE_PIC_SIGNATURE:
            ctx->param.use_pic_sign = (*((int *)buf)) ? 1 : 0;
            break;
            /* get config *******************************************************/
        case EVEYE_CFG_GET_QP:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.qp;
            break;
        case EVEYE_CFG_GET_WIDTH:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.w;
            break;
        case EVEYE_CFG_GET_HEIGHT:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.h;
            break;
        case EVEYE_CFG_GET_FPS:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.fps;
            break;
        case EVEYE_CFG_GET_I_PERIOD:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.i_period;
            break;
        case EVEYE_CFG_GET_RECON:
            evey_assert_rv(*size == sizeof(EVEY_IMGB**), EVEY_ERR_INVALID_ARGUMENT);
            imgb = PIC_CURR(ctx)->imgb;
            if(ctx->sps.picture_cropping_flag)
            {
                for(int i = 0; i < N_C; i++)
                {
                    int cs_offset = i == Y_C ? 2 : 1;
                    imgb->x[i] = ctx->sps.picture_crop_left_offset * cs_offset;
                    imgb->y[i] = ctx->sps.picture_crop_top_offset * cs_offset;
                    imgb->h[i] = imgb->ah[i] - (ctx->sps.picture_crop_top_offset + ctx->sps.picture_crop_bottom_offset) * cs_offset;
                    imgb->w[i] = imgb->aw[i] - (ctx->sps.picture_crop_left_offset + ctx->sps.picture_crop_left_offset) * cs_offset;
                }
            }
            *((EVEY_IMGB **)buf) = imgb;
            imgb->addref(imgb);
            break;
        case EVEYE_CFG_GET_USE_DEBLOCK:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_deblock;
            break;
        case EVEYE_CFG_GET_CLOSED_GOP:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_closed_gop;
            break;
        case EVEYE_CFG_GET_HIERARCHICAL_GOP:
            evey_assert_rv(*size == sizeof(int), EVEY_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_hgop;
            break;
        default:
            evey_trace("unknown config value (%d)\n", cfg);
            evey_assert_rv(0, EVEY_ERR_UNSUPPORTED);
    }

    return EVEY_OK;
}

int eveye_get_inbuf(EVEYE id, EVEY_IMGB ** img)
{
    EVEYE_CTX * ctx;

    EVEYE_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);
    evey_assert_rv(ctx->fn_get_inbuf, EVEY_ERR_UNEXPECTED);
    evey_assert_rv(img != NULL, EVEY_ERR_INVALID_ARGUMENT);

    return ctx->fn_get_inbuf(ctx, img);
}
