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
#include "eveye_eco.h"

#pragma warning(disable:4018)


int eveye_eco_nalu(EVEYE_BSW * bs, EVEY_NALU * nalu)
{
#if TRACE_HLS
    eveye_bsw_write_trace(bs, nalu->nal_unit_size, 0, 32);
#else
    eveye_bsw_write(bs, nalu->nal_unit_size, 32);
#endif
    eveye_bsw_write(bs, nalu->forbidden_zero_bit, 1);
    eveye_bsw_write(bs, nalu->nal_unit_type_plus1, 6);
    eveye_bsw_write(bs, nalu->nuh_temporal_id, 3);
    eveye_bsw_write(bs, nalu->nuh_reserved_zero_5bits, 5);
    eveye_bsw_write(bs, nalu->nuh_extension_flag, 1);
    return EVEY_OK;
}

int eveye_eco_hrd_parameters(EVEYE_BSW * bs, EVEY_HRD * hrd)
{
    eveye_bsw_write_ue(bs, hrd->cpb_cnt_minus1);
    eveye_bsw_write(bs, hrd->bit_rate_scale, 4);
    eveye_bsw_write(bs, hrd->cpb_size_scale, 4);
    for(int sched_sel_idx = 0; sched_sel_idx <= hrd->cpb_cnt_minus1; sched_sel_idx++)
    {
        eveye_bsw_write_ue(bs, hrd->bit_rate_value_minus1[sched_sel_idx]);
        eveye_bsw_write_ue(bs, hrd->cpb_size_value_minus1[sched_sel_idx]);
        eveye_bsw_write1(bs, hrd->cbr_flag[sched_sel_idx]);
    }
    eveye_bsw_write(bs, hrd->initial_cpb_removal_delay_length_minus1, 5);
    eveye_bsw_write(bs, hrd->cpb_removal_delay_length_minus1, 5);
    eveye_bsw_write(bs, hrd->dpb_output_delay_length_minus1, 5);
    eveye_bsw_write(bs, hrd->time_offset_length, 5);

    return EVEY_OK;
}

int eveye_eco_vui(EVEYE_BSW * bs, EVEY_VUI * vui)
{
    eveye_bsw_write1(bs, vui->aspect_ratio_info_present_flag);
    if(vui->aspect_ratio_info_present_flag)
    {
        eveye_bsw_write(bs, vui->aspect_ratio_idc, 8);
        if(vui->aspect_ratio_idc == EXTENDED_SAR)
        {
            eveye_bsw_write(bs, vui->sar_width, 16);
            eveye_bsw_write(bs, vui->sar_height, 16);
        }
    }
    eveye_bsw_write1(bs, vui->overscan_info_present_flag);
    if(vui->overscan_info_present_flag)
    {
        eveye_bsw_write1(bs, vui->overscan_appropriate_flag);
    }
    eveye_bsw_write1(bs, vui->video_signal_type_present_flag);
    if(vui->video_signal_type_present_flag)
    {
        eveye_bsw_write(bs, vui->video_format, 3);
        eveye_bsw_write1(bs, vui->video_full_range_flag);
        eveye_bsw_write1(bs, vui->colour_description_present_flag);
        if(vui->colour_description_present_flag)
        {
            eveye_bsw_write(bs, vui->colour_primaries, 8);
            eveye_bsw_write(bs, vui->transfer_characteristics, 8);
            eveye_bsw_write(bs, vui->matrix_coefficients, 8);
        }
    }
    eveye_bsw_write1(bs, vui->chroma_loc_info_present_flag);
    if(vui->chroma_loc_info_present_flag)
    {
        eveye_bsw_write_ue(bs, vui->chroma_sample_loc_type_top_field);
        eveye_bsw_write_ue(bs, vui->chroma_sample_loc_type_bottom_field);
    }
    eveye_bsw_write1(bs, vui->neutral_chroma_indication_flag);
    eveye_bsw_write1(bs, vui->field_seq_flag);
    eveye_bsw_write1(bs, vui->timing_info_present_flag);
    if(vui->timing_info_present_flag)
    {
        eveye_bsw_write(bs, vui->num_units_in_tick, 32);
        eveye_bsw_write(bs, vui->time_scale, 32);
        eveye_bsw_write1(bs, vui->fixed_pic_rate_flag);
    }
    eveye_bsw_write1(bs, vui->nal_hrd_parameters_present_flag);
    if(vui->nal_hrd_parameters_present_flag)
    {
        eveye_eco_hrd_parameters(bs, &(vui->hrd_parameters));
    }
    eveye_bsw_write1(bs, vui->vcl_hrd_parameters_present_flag);
    if(vui->vcl_hrd_parameters_present_flag)
    {
        eveye_eco_hrd_parameters(bs, &(vui->hrd_parameters));
    }
    if(vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
    {
        eveye_bsw_write1(bs, vui->low_delay_hrd_flag);
    }
    eveye_bsw_write1(bs, vui->pic_struct_present_flag);
    eveye_bsw_write1(bs, vui->bitstream_restriction_flag);
    if(vui->bitstream_restriction_flag)
    {
        eveye_bsw_write1(bs, vui->motion_vectors_over_pic_boundaries_flag);
        eveye_bsw_write_ue(bs, vui->max_bytes_per_pic_denom);
        eveye_bsw_write_ue(bs, vui->max_bits_per_mb_denom);
        eveye_bsw_write_ue(bs, vui->log2_max_mv_length_horizontal);
        eveye_bsw_write_ue(bs, vui->log2_max_mv_length_vertical);
        eveye_bsw_write_ue(bs, vui->num_reorder_pics);
        eveye_bsw_write_ue(bs, vui->max_dec_pic_buffering);
    }

    return EVEY_OK;
}

int eveye_eco_sps(EVEYE_BSW * bs, EVEY_SPS * sps)
{
#if TRACE_HLS
    EVEY_TRACE_STR("***********************************\n");
    EVEY_TRACE_STR("************ SPS Start ************\n");
#endif
    eveye_bsw_write_ue(bs, sps->sps_seq_parameter_set_id);
    eveye_bsw_write(bs, sps->profile_idc, 8);
    eveye_bsw_write(bs, sps->level_idc, 8);
    eveye_bsw_write(bs, sps->toolset_idc_h, 32);
    eveye_bsw_write(bs, sps->toolset_idc_l, 32);
    eveye_bsw_write_ue(bs, sps->chroma_format_idc);
    eveye_bsw_write_ue(bs, sps->pic_width_in_luma_samples);
    eveye_bsw_write_ue(bs, sps->pic_height_in_luma_samples);
    eveye_bsw_write_ue(bs, sps->bit_depth_luma_minus8);
    eveye_bsw_write_ue(bs, sps->bit_depth_chroma_minus8);
    eveye_bsw_write1(bs, sps->sps_btt_flag);     /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_suco_flag);    /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_admvp_flag);   /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_eipd_flag);    /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_cm_init_flag); /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_iqt_flag);     /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_addb_flag);    /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_alf_flag);     /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_htdf_flag);    /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_rpl_flag);     /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_pocs_flag);    /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_dquant_flag);  /* Should be 0 */
    eveye_bsw_write1(bs, sps->sps_dra_flag);     /* Should be 0 */
    if(!sps->sps_rpl_flag || !sps->sps_pocs_flag)
    {
        eveye_bsw_write_ue(bs, sps->log2_sub_gop_length);
        if(sps->log2_sub_gop_length == 0)
        {
            eveye_bsw_write_ue(bs, sps->log2_ref_pic_gap_length);
        }
    }
    if(!sps->sps_rpl_flag)
    {
        eveye_bsw_write_ue(bs, sps->max_num_ref_pics);
    }

    eveye_bsw_write1(bs, sps->picture_cropping_flag);
    if (sps->picture_cropping_flag)
    {
        eveye_bsw_write_ue(bs, sps->picture_crop_left_offset);
        eveye_bsw_write_ue(bs, sps->picture_crop_right_offset);
        eveye_bsw_write_ue(bs, sps->picture_crop_top_offset);
        eveye_bsw_write_ue(bs, sps->picture_crop_bottom_offset);
    }

    if(sps->chroma_format_idc != 0)
    {
        eveye_bsw_write1(bs, sps->chroma_qp_table_struct.chroma_qp_table_present_flag);
        if(sps->chroma_qp_table_struct.chroma_qp_table_present_flag)
        {
            eveye_bsw_write1(bs, sps->chroma_qp_table_struct.same_qp_table_for_chroma);
            eveye_bsw_write1(bs, sps->chroma_qp_table_struct.global_offset_flag);
            for(int i = 0; i < (sps->chroma_qp_table_struct.same_qp_table_for_chroma ? 1 : 2); i++)
            {
                eveye_bsw_write_ue(bs, (u32)sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]);
                for(int j = 0; j <= sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]; j++)
                {
                    eveye_bsw_write(bs, sps->chroma_qp_table_struct.delta_qp_in_val_minus1[i][j], 6);
                    eveye_bsw_write_se(bs, (u32)sps->chroma_qp_table_struct.delta_qp_out_val[i][j]);
                }
            }
        }
    }

    eveye_bsw_write1(bs, sps->vui_parameters_present_flag);
    if(sps->vui_parameters_present_flag)
    {
        eveye_eco_vui(bs, &(sps->vui_parameters));
    }

    u32 t0 = 0;
    while(!EVEYE_BSW_IS_BYTE_ALIGN(bs))
    {
        eveye_bsw_write1(bs, t0);
    }

#if TRACE_HLS
    EVEY_TRACE_STR("************ SPS End   ************\n");
    EVEY_TRACE_STR("***********************************\n");
#endif

    return EVEY_OK;
}

int eveye_eco_pps(EVEYE_BSW * bs, EVEY_SPS * sps, EVEY_PPS * pps)
{
#if TRACE_HLS
    EVEY_TRACE_STR("***********************************\n");
    EVEY_TRACE_STR("************ PPS Start ************\n");
#endif
    eveye_bsw_write_ue(bs, pps->pps_pic_parameter_set_id);
    eveye_bsw_write_ue(bs, pps->pps_seq_parameter_set_id);
    eveye_bsw_write_ue(bs, 0);                               /* Should be 0 (pps->num_ref_idx_default_active_minus1[0]) */
    eveye_bsw_write_ue(bs, 0);                               /* Should be 0 (pps->num_ref_idx_default_active_minus1[1]) */
    eveye_bsw_write_ue(bs, 0);                               /* Should be 0 (pps->additional_lt_poc_lsb_len) */
    eveye_bsw_write1(bs, 0);                                 /* Should be 0 (pps->rpl1_idx_present_flag) */
    eveye_bsw_write1(bs, 1);                                 /* Should be 1 (pps->single_tile_in_pic_flag) */
    eveye_bsw_write_ue(bs, 0);                               /* Should be sent, but not used (pps->tile_id_len_minus1) */
    eveye_bsw_write1(bs, 0);                                 /* Should be sent, but not used (pps->explicit_tile_id_flag) */
    eveye_bsw_write1(bs, 0);                                 /* Should be 0 (pps->pic_dra_enabled_flag) */
    eveye_bsw_write1(bs, 0);                                 /* Should be 0 (pps->arbitrary_slice_present_flag) */
    eveye_bsw_write1(bs, pps->constrained_intra_pred_flag);

    eveye_bsw_write1(bs, pps->cu_qp_delta_enabled_flag);
    if(pps->cu_qp_delta_enabled_flag)
    {
        eveye_bsw_write_ue(bs, 0); /* Should be sent, but not used (pps->cu_qp_delta_area - 6) */
    }

    u32 t0 = 0;
    while(!EVEYE_BSW_IS_BYTE_ALIGN(bs))
    {
        eveye_bsw_write1(bs, t0);
    }

#if TRACE_HLS    
    EVEY_TRACE_STR("************ PPS End   ************\n");
    EVEY_TRACE_STR("***********************************\n");
#endif

    return EVEY_OK;
}

int eveye_eco_sh(EVEYE_BSW * bs, EVEY_SPS * sps, EVEY_PPS * pps, EVEY_SH * sh, int nut)
{
#if TRACE_HLS    
    EVEY_TRACE_STR("***********************************\n");
    EVEY_TRACE_STR("************ SH  Start ************\n");
#endif

    eveye_bsw_write_ue(bs, sh->slice_pic_parameter_set_id);
    eveye_bsw_write_ue(bs, sh->slice_type);

    if(nut == EVEY_IDR_NUT)
    {
        eveye_bsw_write1(bs, sh->no_output_of_prior_pics_flag);
    }

    if(sh->slice_type != SLICE_I)
    {
        eveye_bsw_write1(bs, 0);       /* Currently it is set to 0, (sh->num_ref_idx_active_override_flag) */
    }

    eveye_bsw_write1(bs, sh->slice_deblocking_filter_flag);
    eveye_bsw_write(bs, sh->qp, 6);
    eveye_bsw_write_se(bs, sh->qp_u_offset);
    eveye_bsw_write_se(bs, sh->qp_v_offset);

    /* byte align */
    u32 t0 = 0;
    while(!EVEYE_BSW_IS_BYTE_ALIGN(bs))
    {
        eveye_bsw_write1(bs, t0);
    }

#if TRACE_HLS    
    EVEY_TRACE_STR("************ SH  End   ************\n");
    EVEY_TRACE_STR("***********************************\n");
#endif

    return EVEY_OK;
}

int eveye_eco_signature(EVEYE_CTX * ctx, EVEYE_BSW * bs)
{
    if(ctx->param.use_pic_sign)
    {
        int ret;
        u8  pic_sign[N_C][16] = {{0}};

        /* get picture signature */
        ret = evey_picbuf_signature(PIC_CURR(ctx), pic_sign);
        evey_assert_rv(ret == EVEY_OK, ret);

        u32 payload_type = EVEY_UD_PIC_SIGNATURE;
        u32 payload_size = 16;

        eveye_bsw_write(bs, payload_type, 8);
        eveye_bsw_write(bs, payload_size, 8);

        for(int i = 0; i < ctx->pic_e[PIC_IDX_CURR]->imgb->np; ++i)
        {
            for(int j = 0; j < payload_size; j++)
            {
                eveye_bsw_write(bs, pic_sign[i][j], 8);
            }
        }
    }

    return EVEY_OK;
}

int eveye_eco_sei(EVEYE_CTX * ctx, EVEYE_BSW * bs)
{
    evey_assert_rv(EVEYE_BSW_IS_BYTE_ALIGN(bs), EVEY_ERR_UNKNOWN);

    eveye_eco_signature(ctx, bs);

    while(!EVEYE_BSW_IS_BYTE_ALIGN(bs))
    {
        eveye_bsw_write1(bs, 0);
    }

    return EVEY_OK;
}

static void eveye_bsw_write_est(EVEYE_SBAC * sbac, u32 byte, int len)
{
    sbac->bit_counter += len;
}

static void sbac_put_byte(u8 writing_byte, EVEYE_SBAC * sbac, EVEYE_BSW * bs)
{
    if(sbac->is_pending_byte)
    {
        if(sbac->pending_byte == 0)
        {
            sbac->stacked_zero++;
        }
        else
        {
            while(sbac->stacked_zero > 0)
            {
                if(sbac->is_bit_count)
                {
                    eveye_bsw_write_est(sbac, 0x00, 8);
                }
                else
                {
#if TRACE_HLS
                    eveye_bsw_write_trace(bs, 0x00, 0, 8);
#else
                    eveye_bsw_write(bs, 0x00, 8);
#endif
                }
                sbac->stacked_zero--;
            }

            if(sbac->is_bit_count)
            {
                eveye_bsw_write_est(sbac, sbac->pending_byte, 8);
            }
            else
            {
#if TRACE_HLS
                eveye_bsw_write_trace(bs, sbac->pending_byte, 0, 8);
#else
                eveye_bsw_write(bs, sbac->pending_byte, 8);
#endif
            }
        }
    }
    sbac->pending_byte = writing_byte;
    sbac->is_pending_byte = 1;
}

static void sbac_carry_propagate(EVEYE_SBAC * sbac, EVEYE_BSW * bs)
{
    u32 out_bits = sbac->code >> 17;

    sbac->code &= (1 << 17) - 1;

    if(out_bits < 0xFF)
    {
        while(sbac->stacked_ff != 0)
        {
            sbac_put_byte(0xFF, sbac, bs);
            sbac->stacked_ff--;
        }
        sbac_put_byte(out_bits, sbac, bs);
    }
    else if(out_bits > 0xFF)
    {
        sbac->pending_byte++;
        while(sbac->stacked_ff != 0)
        {
            sbac_put_byte(0x00, sbac, bs);
            sbac->stacked_ff--;
        }
        sbac_put_byte(out_bits & 0xFF, sbac, bs);
    }
    else
    {
        sbac->stacked_ff++;
    }
}

static void sbac_encode_bin_ep(u32 bin, EVEYE_SBAC * sbac, EVEYE_BSW * bs)
{
    sbac->bin_counter++;

    (sbac->range) >>= 1;

    if(bin != 0)
    {
        (sbac->code) += (sbac->range);
    }

    (sbac->range) <<= 1;
    (sbac->code) <<= 1;

    if(--(sbac->code_bits) == 0)
    {
        sbac_carry_propagate(sbac, bs);
        sbac->code_bits = 8;
    }
}

static void sbac_write_unary_sym_ep(u32 sym, EVEYE_SBAC * sbac, EVEYE_BSW * bs, u32 max_val)
{
    u32 icounter = 0;

    sbac_encode_bin_ep(sym ? 1 : 0, sbac, bs); icounter++;

    if(sym == 0)
    {
        return;
    }

    while(sym--)
    {
        if(icounter < max_val)
        {
            sbac_encode_bin_ep(sym ? 1 : 0, sbac, bs); icounter++;
        }
    }
}

static void sbac_write_unary_sym(u32 sym, u32 num_ctx, EVEYE_SBAC * sbac, SBAC_CTX_MODEL * model, EVEYE_BSW * bs)
{
    u32 ctx_idx = 0;

    eveye_sbac_encode_bin(sym ? 1 : 0, sbac, model, bs);

    if(sym == 0)
    {
        return;
    }

    while(sym--)
    {
        if(ctx_idx < num_ctx - 1)
        {
            ctx_idx++;
        }
        eveye_sbac_encode_bin(sym ? 1 : 0, sbac, &model[ctx_idx], bs);
    }
}

static void sbac_write_truncate_unary_sym(u32 sym, u32 num_ctx, u32 max_num, EVEYE_SBAC * sbac, SBAC_CTX_MODEL * model, EVEYE_BSW * bs)
{
    u32 ctx_idx = 0;
    int symbol = 0;

    if(max_num > 1)
    {
        for(ctx_idx = 0; ctx_idx < max_num - 1; ++ctx_idx)
        {
            symbol = (ctx_idx == sym) ? 0 : 1;
            eveye_sbac_encode_bin(symbol, sbac, model + (ctx_idx > max_num - 1 ? max_num - 1 : ctx_idx), bs);

            if(symbol == 0)
                break;
        }
    }
}

static void sbac_encode_bins_ep(u32 value, int num_bin, EVEYE_SBAC * sbac, EVEYE_BSW * bs)
{
    int bin = 0;
    for(bin = num_bin - 1; bin >= 0; bin--)
    {
        sbac_encode_bin_ep(value & (1 << (u32)bin), sbac, bs);
    }
}

void eveye_sbac_encode_bin(u32 bin, EVEYE_SBAC * sbac, SBAC_CTX_MODEL * model, EVEYE_BSW * bs)
{
    u32 lps;
    u16 mps, state;

    sbac->bin_counter++;

    state = (*model) >> 1;
    mps = (*model) & 1;

    lps = (state * (sbac->range)) >> 9;
    lps = lps < 437 ? 437 : lps;    

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

    if(bin != mps)
    {
        if(sbac->range >= lps)
        {
            sbac->code += sbac->range;
            sbac->range = lps;
        }

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
        state = state - ((state + 16) >> 5);
        *model = (state << 1) + mps;
    }

    while(sbac->range < 8192)
    {
        sbac->range <<= 1;
        sbac->code <<= 1;
        sbac->code_bits--;

        if(sbac->code_bits == 0)
        {
            sbac_carry_propagate(sbac, bs);
            sbac->code_bits = 8;
        }
    }
}

static void eveye_sbac_encode_bin_trm(u32 bin, EVEYE_SBAC * sbac, EVEYE_BSW * bs)
{
    sbac->bin_counter++;

    sbac->range--;

    if(bin)
    {
        sbac->code += sbac->range;
        sbac->range = 1;
    }
   
    while(sbac->range < 8192)
    {
        sbac->range <<= 1;
        sbac->code <<= 1;
        if(--(sbac->code_bits) == 0)
        {
            sbac_carry_propagate(sbac, bs);
            sbac->code_bits = 8;
        }
    }
}

void eveye_sbac_reset(EVEYE_CTX * ctx, EVEYE_SBAC * sbac)
{
    u8 slice_type = ctx->sh.slice_type;
    u8 slice_qp = ctx->sh.qp;

    /* Initialization of the context models */
    evey_eco_init_ctx_model(&sbac->ctx);

    /* Initialization of the internal variables */
    sbac->range = 16384;
    sbac->code = 0;
    sbac->code_bits = 11;
    sbac->pending_byte = 0;
    sbac->is_pending_byte = 0;
    sbac->stacked_ff = 0;
    sbac->stacked_zero = 0;
    sbac->bin_counter = 0;    
}

void eveye_sbac_finish(EVEYE_BSW * bs)
{
    EVEYE_SBAC * sbac;
    u32 tmp;

    sbac = GET_SBAC_ENC(bs);

    tmp = (sbac->code + sbac->range - 1) & (0xFFFFFFFF << 14);
    if(tmp < sbac->code)
    {
        tmp += 8192;
    }

    sbac->code = tmp << sbac->code_bits;
    sbac_carry_propagate(sbac, bs);

    sbac->code <<= 8;
    sbac_carry_propagate(sbac, bs);

    while(sbac->stacked_zero > 0)
    {
#if TRACE_HLS
        eveye_bsw_write_trace(bs, 0x00, 0, 8);
#else
        eveye_bsw_write(bs, 0x00, 8);
#endif
        sbac->stacked_zero--;
    }
    if(sbac->pending_byte != 0)
    {
#if TRACE_HLS
        eveye_bsw_write_trace(bs, sbac->pending_byte, 0, 8);
#else
        eveye_bsw_write(bs, sbac->pending_byte, 8);
#endif
    }
    else
    {
        if(sbac->code_bits < 4)
        {
#if TRACE_HLS
            eveye_bsw_write_trace(bs, 0, 0, 4 - sbac->code_bits);
#else
            eveye_bsw_write(bs, 0, 4 - sbac->code_bits);
#endif

            while(!EVEYE_BSW_IS_BYTE_ALIGN(bs))
            {
#if TRACE_HLS
                eveye_bsw_write1_trace(bs, 0, 0);
#else
                eveye_bsw_write1(bs, 0);
#endif
            }
        }
    }
}

static void eveye_eco_skip_flag(EVEYE_BSW * bs, int flag, int ctx)
{
    EVEYE_SBAC * sbac = GET_SBAC_ENC(bs);
    eveye_sbac_encode_bin(flag, sbac, sbac->ctx.skip_flag + ctx, bs);

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("skip flag ");
    EVEY_TRACE_INT(flag);
    EVEY_TRACE_STR("ctx ");
    EVEY_TRACE_INT(ctx);
    EVEY_TRACE_STR("\n");
}

void eveye_eco_direct_mode_flag(EVEYE_BSW * bs, int direct_mode_flag)
{
    EVEYE_SBAC * sbac = GET_SBAC_ENC(bs);
    eveye_sbac_encode_bin(direct_mode_flag, sbac, sbac->ctx.direct_mode_flag, bs);

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("direct_mode_flag ");
    EVEY_TRACE_INT(direct_mode_flag ? PRED_DIR : 0);
    EVEY_TRACE_STR("\n");
}

void eveye_eco_tile_end_flag(EVEYE_BSW * bs, int flag)
{
    EVEYE_SBAC * sbac = GET_SBAC_ENC(bs);
    eveye_sbac_encode_bin_trm(flag, sbac, bs);
}

void eveye_eco_run_length_cc(EVEYE_BSW * bs, s16 * coef, int log2_w, int log2_h, int num_sig, int ch_type)
{
    EVEYE_SBAC    * sbac;
    EVEY_SBAC_CTX * sbac_ctx;
    u32            num_coeff, scan_pos;
    u32            sign, level, prev_level, run, last_flag;
    s32            t0;
    const u16    * scanp;
    s16            coef_cur;
    int            ctx_last = 0;

    sbac = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;
    scanp = evey_scan_tbl[COEF_SCAN_ZIGZAG][log2_w - 1][log2_h - 1];
    num_coeff = 1 << (log2_w + log2_h);
    run = 0;
    prev_level = 6;

    for(scan_pos = 0; scan_pos < num_coeff; scan_pos++)
    {
        coef_cur = coef[scanp[scan_pos]];
        if(coef_cur)
        {
            level = EVEY_ABS16(coef_cur);
            sign = (coef_cur > 0) ? 0 : 1;
            t0 = ch_type == Y_C ? 0 : 2;

            /* Run coding */
            sbac_write_unary_sym(run, 2, sbac, sbac_ctx->run + t0, bs);

            /* Level coding */
            sbac_write_unary_sym(level - 1, 2, sbac, sbac_ctx->level + t0, bs);

            /* Sign coding */
            sbac_encode_bin_ep(sign, sbac, bs);

            if(scan_pos == num_coeff - 1)
            {
                break;
            }

            run = 0;
            prev_level = level;
            num_sig--;

            /* Last flag coding */
            last_flag = (num_sig == 0) ? 1 : 0;
            ctx_last = (ch_type == Y_C) ? 0 : 1;
            eveye_sbac_encode_bin(last_flag, sbac, sbac_ctx->last + ctx_last, bs);

            if(last_flag)
            {
                break;
            }
        }
        else
        {
            run++;
        }
    }

#if ENC_DEC_TRACE
    EVEY_TRACE_STR("coef luma ");
    for (scan_pos = 0; scan_pos < num_coeff; scan_pos++)
    {
        EVEY_TRACE_INT(coef[scan_pos]);
    }
    EVEY_TRACE_STR("\n");
#endif
}

static void eveye_eco_xcoef(EVEYE_BSW * bs, s16 * coef, int log2_w, int log2_h, int num_sig, int ch_type)
{
    eveye_eco_run_length_cc(bs, coef, log2_w, log2_h, num_sig, (ch_type == Y_C ? 0 : 1));

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
}

int eveye_eco_cbf(EVEYE_BSW * bs, int cbf_y, int cbf_u, int cbf_v, u8 pred_mode, int b_no_cbf, int is_sub, int sub_pos, int cbf_all, int run[N_C], int chroma_format_idc)
{
    EVEYE_SBAC    * sbac;
    EVEY_SBAC_CTX * sbac_ctx;

    sbac = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;

    /* code allcbf */
    if(pred_mode != MODE_INTRA)
    {
        if (!cbf_all && sub_pos)
        {
            return EVEY_OK;
        }            
        if(b_no_cbf == 1)
        {
            evey_assert(cbf_all != 0);
        }
        else if(sub_pos == 0 && (run[Y_C] + run[U_C] + run[V_C]) == 3) // not count bits of root_cbf when checking each component
        {
            if(cbf_all == 0)
            {
                eveye_sbac_encode_bin(0, sbac, sbac_ctx->cbf_all, bs);

                EVEY_TRACE_COUNTER;
                EVEY_TRACE_STR("all_cbf ");
                EVEY_TRACE_INT(0);
                EVEY_TRACE_STR("\n");

                return EVEY_OK;
            }
            else
            {
                eveye_sbac_encode_bin(1, sbac, sbac_ctx->cbf_all, bs);

                EVEY_TRACE_COUNTER;
                EVEY_TRACE_STR("all_cbf ");
                EVEY_TRACE_INT(1);
                EVEY_TRACE_STR("\n");
            }
        }

        if(run[U_C] && chroma_format_idc != 0)
        {
            eveye_sbac_encode_bin(cbf_u, sbac, sbac_ctx->cbf_cb, bs);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf U ");
            EVEY_TRACE_INT(cbf_u);
            EVEY_TRACE_STR("\n");
        }

        if(run[V_C] && chroma_format_idc != 0)
        {
            eveye_sbac_encode_bin(cbf_v, sbac, sbac_ctx->cbf_cr, bs);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf V ");
            EVEY_TRACE_INT(cbf_v);
            EVEY_TRACE_STR("\n");
        }

        if (run[Y_C] && (cbf_u + cbf_v != 0 || is_sub))
        {
            eveye_sbac_encode_bin(cbf_y, sbac, sbac_ctx->cbf_luma, bs);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf Y ");
            EVEY_TRACE_INT(cbf_y);
            EVEY_TRACE_STR("\n");
        }       
    }
    else
    {
        if(run[U_C] && chroma_format_idc != 0)
        {
            eveye_sbac_encode_bin(cbf_u, sbac, sbac_ctx->cbf_cb, bs);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf U ");
            EVEY_TRACE_INT(cbf_u);
            EVEY_TRACE_STR("\n");
        }

        if(run[V_C] && chroma_format_idc != 0)
        {
            eveye_sbac_encode_bin(cbf_v, sbac, sbac_ctx->cbf_cr, bs);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf V ");
            EVEY_TRACE_INT(cbf_v);
            EVEY_TRACE_STR("\n");
        }

        if (run[Y_C])
        {
            eveye_sbac_encode_bin(cbf_y, sbac, sbac_ctx->cbf_luma, bs);
            EVEY_TRACE_COUNTER;
            EVEY_TRACE_STR("cbf Y ");
            EVEY_TRACE_INT(cbf_y);
            EVEY_TRACE_STR("\n");
        }
    }

    return EVEY_OK;
}

int eveye_eco_dqp(EVEYE_BSW * bs, int ref_qp, int cur_qp)
{
    int             abs_dqp, dqp, t0;
    u32             sign;
    EVEYE_SBAC    * sbac;
    EVEY_SBAC_CTX * sbac_ctx;

    sbac = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;

    dqp = cur_qp - ref_qp;
    abs_dqp = EVEY_ABS(dqp);
    t0 = abs_dqp;

    sbac_write_unary_sym(t0, NUM_CTX_DELTA_QP, sbac, sbac_ctx->delta_qp, bs);

    if(abs_dqp > 0)
    {
        sign = dqp > 0 ? 0 : 1;
        sbac_encode_bin_ep(sign, sbac, bs);
    }

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("dqp ");
    EVEY_TRACE_INT(dqp);
    EVEY_TRACE_STR("\n");

    return EVEY_OK;
}

int eveye_eco_coef(EVEYE_CTX * ctx, EVEYE_CORE * core, EVEYE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], u8 pred_mode, int nnz_sub[N_C][MAX_SUB_TB_NUM], int b_no_cbf, int run_stats, int enc_dqp, u8 cur_qp)
{
    int    w_shift = (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int    h_shift = (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));
    int    log2_cuw = core->log2_cuw;
    int    log2_cuh = core->log2_cuh;
    int    run[N_C] = { run_stats & 1, (run_stats >> 1) & 1, (run_stats >> 2) & 1 };
    s16  * coef_temp[N_C];
    s16 (* coef_temp_buf)[MAX_TR_DIM] = core->coef_temp;
    int    i, j, c;
    int    log2_w_sub = (log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuw;
    int    log2_h_sub = (log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuh;
    int    loop_w = (log2_cuw > MAX_TR_LOG2) ? (1 << (log2_cuw - MAX_TR_LOG2)) : 1;
    int    loop_h = (log2_cuh > MAX_TR_LOG2) ? (1 << (log2_cuh - MAX_TR_LOG2)) : 1;
    int    stride = (1 << log2_cuw);
    int    sub_stride = (1 << log2_w_sub);
    int    is_sub = loop_h + loop_w > 2 ? 1 : 0;

    evey_assert(run_stats != 0);

    int cbf_all = 0;
    u8 is_intra = (pred_mode == MODE_INTRA) ? 1 : 0;
    EVEYE_SBAC    * sbac = GET_SBAC_ENC(bs);

    for (j = 0; j < loop_h; j++)
    {
        for (i = 0; i < loop_w; i++)
        {
            for (c = 0; c < N_C; c++)
            {
                if (run[c])
                {
                    cbf_all += !!nnz_sub[c][(j << 1) | i];
                }
            }
        }
    }

    for(j = 0; j < loop_h; j++)
    {
        for(i = 0; i < loop_w; i++)
        {
            eveye_eco_cbf(bs, !!nnz_sub[Y_C][(j << 1) | i], !!nnz_sub[U_C][(j << 1) | i], !!nnz_sub[V_C][(j << 1) | i], pred_mode, b_no_cbf, is_sub, j + i, cbf_all, run, ctx->sps.chroma_format_idc);

            if(ctx->pps.cu_qp_delta_enabled_flag)
            {
                if(enc_dqp == 1)
                {
                    int cbf_for_dqp = (!!nnz_sub[Y_C][(j << 1) | i]) || (!!nnz_sub[U_C][(j << 1) | i]) || (!!nnz_sub[V_C][(j << 1) | i]);
                    if(cbf_for_dqp)
                    {
                        eveye_eco_dqp(bs, ctx->sh.qp_prev_eco, cur_qp);
                        ctx->sh.qp_prev_eco = cur_qp;
                    }
                }
            }

            for(c = 0; c < N_C; c++)
            {
                if(nnz_sub[c][(j << 1) | i] && run[c])
                {
                    int pos_sub_x = c == 0 ? (i * (1 << (log2_w_sub))) : (i * (1 << (log2_w_sub - w_shift)));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - h_shift)) * (stride >> w_shift);

                    if(is_sub)
                    {
                        if(c == 0)
                        {
                            evey_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride, coef_temp_buf[c], sub_stride, log2_w_sub, log2_h_sub);
                        }
                        else
                        {
                            evey_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, coef_temp_buf[c], sub_stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        }
                        coef_temp[c] = coef_temp_buf[c];
                    }
                    else
                    {
                        coef_temp[c] = coef[c];
                    }

                    if(c == 0)
                    {
                        eveye_eco_xcoef(bs, coef_temp[c], log2_w_sub, log2_h_sub, nnz_sub[c][(j << 1) | i], c);
                    }
                    else
                    {
                        eveye_eco_xcoef(bs, coef_temp[c], log2_w_sub - w_shift, log2_h_sub - h_shift, nnz_sub[c][(j << 1) | i], c);
                    }

                    if(is_sub)
                    {
                        if(c == 0)
                        {
                            evey_block_copy(coef_temp_buf[c], sub_stride, coef[c] + pos_sub_x + pos_sub_y, stride, log2_w_sub, log2_h_sub);
                        }
                        else
                        {
                            evey_block_copy(coef_temp_buf[c], sub_stride >> w_shift, coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        }
                    }
                }
            }
        }
    }
    return EVEY_OK;
}

int eveye_eco_pred_mode(EVEYE_BSW * bs, u8 pred_mode, int ctx)
{
    EVEYE_SBAC * sbac = GET_SBAC_ENC(bs);       
    eveye_sbac_encode_bin(pred_mode == MODE_INTRA, sbac, sbac->ctx.pred_mode + ctx, bs);

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("pred mode ");
    EVEY_TRACE_INT(pred_mode == MODE_INTRA ? MODE_INTRA : MODE_INTER);
    EVEY_TRACE_STR("\n");

    return EVEY_OK;
}

int eveye_eco_intra_dir(EVEYE_BSW * bs, u8 ipm, u8 *mpm)
{
    EVEYE_SBAC * sbac;
    sbac = GET_SBAC_ENC(bs);
    sbac_write_unary_sym(mpm[ipm], 2, sbac, sbac->ctx.intra_dir, bs);

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("ipm Y ");
    EVEY_TRACE_INT(ipm);
    EVEY_TRACE_STR("\n");

    return EVEY_OK;
}

void eveye_eco_inter_pred_idc(EVEYE_BSW * bs, s8 refi[LIST_NUM], int slice_type)
{
    EVEYE_SBAC * sbac;
    sbac = GET_SBAC_ENC(bs);

    if(REFI_IS_VALID(refi[LIST_0]) && REFI_IS_VALID(refi[LIST_1])) /* PRED_BI */
    {
        assert(slice_type == SLICE_B);
        eveye_sbac_encode_bin(0, sbac, sbac->ctx.inter_dir, bs);
    }
    else
    {
        if(slice_type == SLICE_B)
        {
            eveye_sbac_encode_bin(1, sbac, sbac->ctx.inter_dir, bs);
        }

        if(REFI_IS_VALID(refi[LIST_0])) /* PRED_L0 */
        {
            eveye_sbac_encode_bin(0, sbac, sbac->ctx.inter_dir + 1, bs);
        }
        else /* PRED_L1 */
        {
            eveye_sbac_encode_bin(1, sbac, sbac->ctx.inter_dir + 1, bs);
        }
    }

#if ENC_DEC_TRACE
    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("inter dir ");
    EVEY_TRACE_INT(REFI_IS_VALID(refi[LIST_0]) && REFI_IS_VALID(refi[LIST_1])? PRED_BI : (REFI_IS_VALID(refi[LIST_0]) ? PRED_L0 : PRED_L1));
    EVEY_TRACE_STR("\n");
#endif

    return;
}

int eveye_eco_refi(EVEYE_BSW * bs, int num_refp, int refi)
{
    EVEYE_SBAC    * sbac = GET_SBAC_ENC(bs);
    EVEY_SBAC_CTX * sbac_ctx = &sbac->ctx;
    int             i, bin;

    if(num_refp > 1)
    {
        if(refi == 0)
        {
            eveye_sbac_encode_bin(0, sbac, sbac_ctx->refi, bs);
        }
        else
        {
            eveye_sbac_encode_bin(1, sbac, sbac_ctx->refi, bs);
            if(num_refp > 2)
            {
                for(i = 2; i < num_refp; i++)
                {
                    bin = (i == refi + 1) ? 0 : 1;
                    if(i == 2)
                    {
                        eveye_sbac_encode_bin(bin, sbac, sbac_ctx->refi + 1, bs);
                    }
                    else
                    {
                        sbac_encode_bin_ep(bin, sbac, bs);
                    }
                    if(bin == 0)
                    {
                        break;
                    }
                }
            }
        }
    }

    return EVEY_OK;
}

int eveye_eco_mvp_idx(EVEYE_BSW * bs, int mvp_idx)
{
    EVEYE_SBAC    * sbac = GET_SBAC_ENC(bs);
    EVEY_SBAC_CTX * sbac_ctx = &sbac->ctx;

    sbac_write_truncate_unary_sym(mvp_idx, 3, 4, sbac, sbac_ctx->mvp_idx, bs);

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("mvp idx ");
    EVEY_TRACE_INT(mvp_idx);
    EVEY_TRACE_STR("\n");

    return EVEY_OK;
}

static int eveye_eco_abs_mvd(u32 sym, EVEYE_SBAC * sbac, SBAC_CTX_MODEL * model, EVEYE_BSW * bs)
{
    u32 val = sym;
    s32 len_i, len_c, info, nn;
    u32 code;
    s32 i;

    nn = ((val + 1) >> 1);
    for(len_i = 0; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }

    info = val + 1 - (1 << len_i);
    code = (1 << len_i) | ((info)& ((1 << len_i) - 1));

    len_c = (len_i << 1) + 1;

    for(i = 0; i < len_c; i++)
    {
        int bin = (code >> (len_c - 1 - i)) & 0x01;
        if(i <= 1)
        {
            eveye_sbac_encode_bin(bin, sbac, model, bs); /* use one context model for two bins */
        }
        else
        {
            sbac_encode_bin_ep(bin, sbac, bs);
        }
    }

    return EVEY_OK;
}

int eveye_eco_mvd(EVEYE_BSW * bs, s16 mvd[MV_D])
{
    EVEYE_SBAC    * sbac;
    EVEY_SBAC_CTX * sbac_ctx;
    int             t0;
    u32             mv;

    sbac     = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;
    t0 = 0;
    mv = mvd[MV_X];
    if(mvd[MV_X] < 0)
    {
        t0 = 1;
        mv = -mvd[MV_X];
    }
    eveye_eco_abs_mvd(mv, sbac, sbac_ctx->mvd, bs);

    if(mv)
    {
        sbac_encode_bin_ep(t0, sbac, bs);
    }

    t0 = 0;
    mv = mvd[MV_Y];
    if(mvd[MV_Y] < 0)
    {
        t0 = 1;
        mv = -mvd[MV_Y];
    }
    eveye_eco_abs_mvd(mv, sbac, sbac_ctx->mvd, bs);

    if(mv)
    {
        sbac_encode_bin_ep(t0, sbac, bs);
    }

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("mvd x ");
    EVEY_TRACE_INT(mvd[MV_X]);
    EVEY_TRACE_STR("mvd y ");
    EVEY_TRACE_INT(mvd[MV_Y]);
    EVEY_TRACE_STR("\n");

    return EVEY_OK;
}

static int eco_cu_init(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int cup, int cuw, int cuh)
{
    EVEYE_CU_DATA * cu_data = &ctx->map_cu_data[core->ctu_num];

    core->log2_cuw = EVEY_CONV_LOG2(cuw);
    core->log2_cuh = EVEY_CONV_LOG2(cuh);
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    core->avail_cu = 0;
    core->nnz[Y_C] = core->nnz[U_C] = core->nnz[V_C] = 0;
    core->pred_mode = cu_data->pred_mode[cup];

    if(core->pred_mode == MODE_INTRA)
    {
        core->avail_cu = evey_get_avail_intra(ctx, core);
    }
    else
    {
        core->avail_cu = evey_get_avail_inter(ctx, core);
    }

#if TRACE_ENC_CU_DATA
    core->trace_idx = cu_data->trace_idx[cup];
#endif
#if TRACE_ENC_CU_DATA_CHECK
    evey_assert(core->trace_idx != 0);
#endif

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("poc: ");
    EVEY_TRACE_INT(ctx->poc.poc_val);
    EVEY_TRACE_STR("x pos ");
    EVEY_TRACE_INT(core->x_pel + ((cup % (ctx->ctu_size >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    EVEY_TRACE_STR("y pos ");
    EVEY_TRACE_INT(core->y_pel + ((cup / (ctx->ctu_size >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    EVEY_TRACE_STR("width ");
    EVEY_TRACE_INT(cuw);
    EVEY_TRACE_STR("height ");
    EVEY_TRACE_INT(cuh);
    EVEY_TRACE_STR("\n");

    return EVEY_OK;
}

static int eco_cu_deinit(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    u32 * map_scu = ctx->map_scu + core->scup;
    int   w = 1 << (core->log2_cuw - MIN_CU_LOG2);
    int   h = 1 << (core->log2_cuh - MIN_CU_LOG2);
    
    for(int i = 0; i < h; i++)
    {
        for(int j = 0; j < w; j++)
        {
            int sub_idx = ((!!(i & 32)) << 1) | (!!(j & 32));
            if(core->nnz_sub[Y_C][sub_idx] > 0)
            {
                MCU_SET_CBFL(map_scu[j]);
            }
            else
            {
                MCU_CLR_CBFL(map_scu[j]);
            }

            MCU_SET_COD(map_scu[j]);

            if(ctx->pps.cu_qp_delta_enabled_flag)
            {
                MCU_RESET_QP(map_scu[j]);
                MCU_SET_QP(map_scu[j], ctx->sh.qp_prev_eco);
            }
        }
        map_scu += ctx->w_scu;
    }

#if TRACE_ENC_CU_DATA
    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("RDO check id ");
    EVEY_TRACE_INT((int)core->trace_idx);
    EVEY_TRACE_STR("\n");
    evey_assert(core->trace_idx != 0);
#endif

#if MVF_TRACE
    // Trace MVF in encoder
    {
        s8(*map_refi)[LIST_NUM];
        s16(*map_mv)[LIST_NUM][MV_D];
        u32  *map_scu;

        map_refi = ctx->map_refi + core->scup;
        map_scu = ctx->map_scu + core->scup;
        map_mv = ctx->map_mv + core->scup;

        for(i = 0; i < h; i++)
        {
            for(j = 0; j < w; j++)
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

    return EVEY_OK;
}

static void coef_rect_to_series(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 * coef_src[N_C], int x, int y, int cuw, int cuh, s16 coef_dst[N_C][MAX_CU_DIM])
{
    int w_shift = (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int h_shift = (GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));
    int i, j, sidx, didx;

    sidx = (x&(ctx->ctu_size - 1)) + ((y&(ctx->ctu_size - 1)) << ctx->log2_ctu_size);
    didx = 0;

    for(j = 0; j < cuh; j++)
    {
        for(i = 0; i < cuw; i++)
        {
            coef_dst[Y_C][didx++] = coef_src[Y_C][sidx + i];
        }
        sidx += ctx->ctu_size;
    }

    if(ctx->sps.chroma_format_idc)
    {
        x >>= w_shift;
        y >>= h_shift;
        cuw >>= w_shift;
        cuh >>= h_shift;

        sidx = (x&((ctx->ctu_size >> w_shift) - 1)) + ((y&((ctx->ctu_size >> h_shift) - 1)) << (ctx->log2_ctu_size - w_shift));
        didx = 0;

        for(j = 0; j < cuh; j++)
        {
            for(i = 0; i < cuw; i++)
            {
                coef_dst[U_C][didx] = coef_src[U_C][sidx + i];
                coef_dst[V_C][didx] = coef_src[V_C][sidx + i];
                didx++;
            }
            sidx += (ctx->ctu_size >> w_shift);
        }
    }
}

int eveye_eco_split_mode(EVEYE_BSW * bs, EVEYE_CTX * ctx, EVEYE_CORE * core, int cud, int cup, int cuw, int cuh, int ctu_s)
{
    EVEYE_SBAC * sbac;
    int          ret = EVEY_OK;
    s8           split_mode;

    if(cuw < 8 && cuh < 8)
    {
        return ret;
    }

    sbac = GET_SBAC_ENC(bs);
       
    if(sbac->is_bit_count)
    {
        evey_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctu_s, core->cu_data_temp[EVEY_CONV_LOG2(cuw) - 2][EVEY_CONV_LOG2(cuh) - 2].split_mode);
    }
    else
    {
        evey_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctu_s, ctx->map_cu_data[core->ctu_num].split_mode);
    }

    eveye_sbac_encode_bin(split_mode != NO_SPLIT, sbac, sbac->ctx.split_cu_flag, bs); /* split_cu_flag */

    EVEY_TRACE_COUNTER;
    EVEY_TRACE_STR("x pos ");
    EVEY_TRACE_INT(core->x_pel + ((cup % (c->ctu_size >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    EVEY_TRACE_STR("y pos ");
    EVEY_TRACE_INT(core->y_pel + ((cup / (c->ctu_size >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    EVEY_TRACE_STR("width ");
    EVEY_TRACE_INT(cuw);
    EVEY_TRACE_STR("height ");
    EVEY_TRACE_INT(cuh);
    EVEY_TRACE_STR("depth ");
    EVEY_TRACE_INT(cud);
    EVEY_TRACE_STR("split mode ");
    EVEY_TRACE_INT(split_mode);
    EVEY_TRACE_STR("\n");

    return ret;
}

int eveye_eco_cu(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int cup, int cuw, int cuh)
{
    EVEYE_BSW     * bs = &ctx->bs;    
    EVEYE_CU_DATA * cu_data = &ctx->map_cu_data[core->ctu_num];
    int             slice_type = ctx->sh.slice_type;

    /* initialization */
    eco_cu_init(ctx, core, x, y, cup, cuw, cuh);
    
    if(slice_type != SLICE_I)
    {
        eveye_eco_skip_flag(bs, core->pred_mode == MODE_SKIP, 0); /* cu_skip_flag */
    }

    if(core->pred_mode == MODE_SKIP)
    {
        eveye_eco_mvp_idx(bs, cu_data->mvp_idx[cup][LIST_0]);

        if(slice_type == SLICE_B)
        {
            eveye_eco_mvp_idx(bs, cu_data->mvp_idx[cup][LIST_1]);
        }

        evey_mset(core->nnz, 0, sizeof(int) * N_C);
        evey_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    }
    else
    {
        if(slice_type != SLICE_I)
        {
            eveye_eco_pred_mode(bs, core->pred_mode, 0);
        }

        if(core->pred_mode == MODE_INTRA) /* intra mode */
        {
            evey_assert(cu_data->ipm[0][cup] != IPD_INVALID);
            evey_assert(cu_data->ipm[1][cup] != IPD_INVALID);
            evey_get_mpm(ctx, core);
            eveye_eco_intra_dir(bs, cu_data->ipm[0][cup], core->mpm_b_list);
        }
        else /* (core->pred_mode != MODE_INTRA) */ /* inter mode */
        {
            if(slice_type == SLICE_B)
            {
                eveye_eco_direct_mode_flag(bs, cu_data->pred_mode[cup] == MODE_DIR); /* direct_mode_flag */
            }

            if((cu_data->pred_mode[cup]) != MODE_DIR)
            {
                if(slice_type == SLICE_B)
                {
                    eveye_eco_inter_pred_idc(bs, cu_data->refi[cup], slice_type);
                }

                int refi0 = cu_data->refi[cup][LIST_0];
                int refi1 = cu_data->refi[cup][LIST_1];
                if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
                {
                    eveye_eco_refi(bs, ctx->dpbm.num_refp[LIST_0], refi0);
                    eveye_eco_mvp_idx(bs, cu_data->mvp_idx[cup][LIST_0]);
                    eveye_eco_mvd(bs, cu_data->mvd[cup][LIST_0]);
                }

                if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
                {
                    eveye_eco_refi(bs, ctx->dpbm.num_refp[LIST_1], refi1);
                    eveye_eco_mvp_idx(bs, cu_data->mvp_idx[cup][LIST_1]);
                    eveye_eco_mvd(bs, cu_data->mvd[cup][LIST_1]);
                }
            }
        }

        /* get coefficients */
        coef_rect_to_series(ctx, core, cu_data->coef, x, y, cuw, cuh, core->coef);

        for(int i = 0; i < N_C; i++)
        {
            core->nnz[i] = cu_data->nnz[i][cup];

            for(int j = 0; j < MAX_SUB_TB_NUM; j++)
            {
                core->nnz_sub[i][j] = cu_data->nnz_sub[i][j][cup];
            }
        }

        int b_no_cbf = 0;
        int enc_dqp = 1;
        /* residual encoding */
        eveye_eco_coef(ctx, core, bs, core->coef, core->pred_mode, core->nnz_sub, b_no_cbf, RUN_L | RUN_CB | RUN_CR, enc_dqp, cu_data->qp_y[cup] - 6 * ctx->sps.bit_depth_luma_minus8);
    }

    eco_cu_deinit(ctx, core);

    return EVEY_OK;
}
