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

#ifndef _EVEY_TBL_H_
#define _EVEY_TBL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "evey_def.h"

extern const u8 evey_tbl_log2[257];
extern const s8 evey_tbl_tm2[2][2];
extern const s8 evey_tbl_tm4[4][4];
extern const s8 evey_tbl_tm8[8][8];
extern const s8 evey_tbl_tm16[16][16];
extern const s8 evey_tbl_tm32[32][32];
extern const s8 evey_tbl_tm64[64][64];
extern u16 * evey_inv_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_TR_LOG2][MAX_TR_LOG2];
extern u16 * evey_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_TR_LOG2][MAX_TR_LOG2];
extern const u8 evey_tbl_mpm[6][6][5];
extern const int evey_tbl_dq_scale[6];
extern const u8 evey_tbl_df_st[4][52];
extern int evey_tbl_qp_chroma_ajudst[MAX_QP_TABLE_SIZE];
extern int evey_tbl_qp_chroma_dynamic_ext[2][MAX_QP_TABLE_SIZE_EXT];
extern int * p_evey_tbl_qp_chroma_dynamic_ext[2]; /* pointer to [0th position in evey_tbl_qp_chroma_dynamic_ext] */
extern int * p_evey_tbl_qp_chroma_dynamic[2];     /* pointer to [12th position in evey_tbl_qp_chroma_dynamic_ext] */

void evey_derived_chroma_qp_mapping_tables(EVEY_CHROMA_TABLE * structChromaQP, int bit_depth);
void evey_set_chroma_qp_tbl_loc(int codec_bit_depth);

#ifdef __cplusplus
}
#endif

#endif /* _EVEY_TBL_H_ */
