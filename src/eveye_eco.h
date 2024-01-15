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

#ifndef _EVEYE_ECO_H_
#define _EVEYE_ECO_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "eveye_def.h"

#define GET_SBAC_ENC(bs)   ((EVEYE_SBAC *)(bs)->pdata[1])

void eveye_sbac_reset(EVEYE_CTX * ctx, EVEYE_SBAC * sbac);
void eveye_sbac_finish(EVEYE_BSW * bs);
void eveye_sbac_encode_bin(u32 bin, EVEYE_SBAC *sbac, SBAC_CTX_MODEL *ctx_model, EVEYE_BSW *bs);

int eveye_eco_nalu(EVEYE_BSW * bs, EVEY_NALU * nalu);
int eveye_eco_sps(EVEYE_BSW * bs, EVEY_SPS * sps);
int eveye_eco_pps(EVEYE_BSW * bs, EVEY_SPS * sps, EVEY_PPS * pps);
int eveye_eco_sh(EVEYE_BSW * bs, EVEY_SPS * sps, EVEY_PPS * pps, EVEY_SH * sh, int nut);
int eveye_eco_signature(EVEYE_CTX * ctx, EVEYE_BSW * bs);
int eveye_eco_sei(EVEYE_CTX * ctx, EVEYE_BSW * bs);

int eveye_eco_cu(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, int cup, int cuw, int cuh);
int eveye_eco_split_mode(EVEYE_BSW * bs, EVEYE_CTX * ctx, EVEYE_CORE * core, int cud, int cup, int cuw, int cuh, int ctu_s);
int eveye_eco_pred_mode(EVEYE_BSW * bs, u8 pred_mode, int ctx);
int eveye_eco_intra_dir(EVEYE_BSW *bs, u8 ipm, u8 * mpm);
void eveye_eco_direct_mode_flag(EVEYE_BSW * bs, int direct_mode_flag);
void eveye_eco_inter_pred_idc(EVEYE_BSW * bs, s8 refi[LIST_NUM], int slice_type);
int eveye_eco_mvp_idx(EVEYE_BSW * bs, int mvp_idx);
int eveye_eco_mvd(EVEYE_BSW * bs, s16 mvd[MV_D]);
int eveye_eco_refi(EVEYE_BSW * bs, int num_refp, int refi);
int eveye_eco_dqp(EVEYE_BSW * bs, int ref_qp, int cur_qp);
int eveye_eco_coef(EVEYE_CTX * ctx, EVEYE_CORE * core, EVEYE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], u8 pred_mode, int nnz_sub[N_C][MAX_SUB_TB_NUM], int b_no_cbf, int run_stats, int enc_dqp, u8 cur_qp);
void eveye_eco_tile_end_flag(EVEYE_BSW * bs, int flag);

#ifdef __cplusplus
}
#endif
#endif /* _EVEYE_ECO_H_ */
