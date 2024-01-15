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

#ifndef _EVEY_INTER_H_
#define _EVEY_INTER_H_

#ifdef __cplusplus

extern "C"
{
#endif

typedef void(*EVEY_MC_L) (pel * ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel * pred, int w, int h, int bit_depth);
typedef void(*EVEY_MC_C) (pel * ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel * pred, int w, int h, int bit_depth);

extern EVEY_MC_L evey_tbl_mc_l[2][2];
extern EVEY_MC_C evey_tbl_mc_c[2][2];

#define evey_mc_l(ori_mv_x, ori_mv_y, ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
        (evey_tbl_mc_l[((ori_mv_x) | ((ori_mv_x)>>1) | ((ori_mv_x)>>2) | ((ori_mv_x)>>3)) & 0x1]) \
                      [((ori_mv_y) | ((ori_mv_y)>>1) | ((ori_mv_y)>>2) | ((ori_mv_y)>>3)) & 0x1] \
                      (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)
#define evey_mc_c(ori_mv_x, ori_mv_y, ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
        (evey_tbl_mc_c[((ori_mv_x) | ((ori_mv_x)>>1) | ((ori_mv_x)>>2)| ((ori_mv_x)>>3) | ((ori_mv_x)>>4)) & 0x1] \
                      [((ori_mv_y) | ((ori_mv_y)>>1) | ((ori_mv_y)>>2) | ((ori_mv_y)>>3) | ((ori_mv_y)>>4)) & 0x1]) \
                      (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)

void evey_inter_pred(void * ctx, int x, int y, int w, int h, s8 refi[LIST_NUM], s16 (* mv)[MV_D], EVEY_REFP (* refp)[LIST_NUM], pel pred[LIST_NUM][N_C][MAX_CU_DIM]);

#ifdef __cplusplus
}
#endif

#endif /* _EVEY_INTER_H_ */
