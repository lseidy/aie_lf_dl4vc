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

#ifndef _EVEYE_UTIL_H_
#define _EVEYE_UTIL_H_

#include "eveye_def.h"

#define PIC_CURR(ctx)             ((ctx)->pic_e[PIC_IDX_CURR])
#define PIC_ORIG(ctx)             ((ctx)->pic_e[PIC_IDX_ORIG])

void eveye_bsw_skip_slice_size(EVEYE_BSW * bs);
int eveye_bsw_write_nalu_size(EVEYE_BSW * bs);

void eveye_diff_pred(EVEYE_CTX * ctx, int log2_cuw, int log2_cuh, pel org[N_C][MAX_CU_DIM], pel pred[N_C][MAX_CU_DIM], s16 diff[N_C][MAX_CU_DIM]);

typedef void(*EVEYE_FN_DIFF)(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth);
extern const EVEYE_FN_DIFF eveye_tbl_diff_16b[8][8];
#define eveye_diff_16b(log2w, log2h, src1, src2, s_src1, s_src2, s_diff, diff, bit_depth) \
    eveye_tbl_diff_16b[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, s_diff, diff, bit_depth)

#define SBAC_STORE(dst, src) evey_mcpy(&dst, &src, sizeof(EVEYE_SBAC))
#define SBAC_LOAD(dst, src)  evey_mcpy(&dst, &src, sizeof(EVEYE_SBAC))
#define DQP_STORE(dst, src) evey_mcpy(&dst, &src, sizeof(EVEYE_DQP))
#define DQP_LOAD(dst, src)  evey_mcpy(&dst, &src, sizeof(EVEYE_DQP))

void eveye_set_qp(EVEYE_CTX * ctx, EVEYE_CORE * core, u8 qp);

#define RATE_TO_COST_LAMBDA(l, r)       ((double)r * l)
#define RATE_TO_COST_SQRT_LAMBDA(l, r)  ((double)r * l)


#endif /* _EVEYE_UTIL_H_ */
