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

#ifndef _EVEYE_MODE_H_
#define _EVEYE_MODE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "eveye_def.h"

int  eveye_mode_create(EVEYE_CTX * ctx, int complexity);
void eveye_rdo_bit_cnt_cu_intra(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM]);
void eveye_rdo_bit_cnt_cu_intra_luma(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM]);
void eveye_rdo_bit_cnt_cu_intra_chroma(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM]);
void eveye_rdo_bit_cnt_cu_inter(EVEYE_CTX * ctx, EVEYE_CORE * core, s8 refi[LIST_NUM], s16 mvd[LIST_NUM][MV_D], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 * mvp_idx);
void eveye_rdo_bit_cnt_cu_inter_comp(EVEYE_CTX * ctx, EVEYE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int ch_type, int pidx);
void eveye_rdo_bit_cnt_cu_skip(EVEYE_CTX * ctx, EVEYE_CORE * core, int mvp_idx0, int mvp_idx1, int c_num);
void eveye_rdo_bit_cnt_mvp(EVEYE_CTX * ctx, EVEYE_CORE * core, s8 refi[LIST_NUM], s16 mvd[LIST_NUM][MV_D], int pidx, int mvp_idx);
void eveye_sbac_bit_reset(EVEYE_SBAC * sbac);
u32  eveye_get_bit_number(EVEYE_SBAC * sbac);
void eveye_init_bits_est();
void eveye_calc_delta_dist_filter_boundary(EVEYE_CTX * ctx, EVEYE_CORE * core, pel (*src)[MAX_CU_DIM], int s_src, int x, int y, u8 intra_flag, u8 cbf_l, s8 * refi, s16 (*mv)[MV_D], u8 is_mv_from_mvf);

#ifdef __cplusplus
}
#endif

#endif /* _EVEYE_MODE_H_ */
