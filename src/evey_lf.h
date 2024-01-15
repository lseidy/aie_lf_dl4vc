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

#ifndef _EVEY_LF_H_
#define _EVEY_LF_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "evey_def.h"
 
void evey_deblock_cu_hor(EVEY_PIC * pic, int x_pel, int y_pel, int cuw, int cuh, u32 * map_scu, s8 (* map_refi)[LIST_NUM], s16 (* map_mv)[LIST_NUM][MV_D]
                         , int w_scu, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);

void evey_deblock_cu_ver(EVEY_PIC * pic, int x_pel, int y_pel, int cuw, int cuh, u32 * map_scu, s8 (* map_refi)[LIST_NUM], s16 (* map_mv)[LIST_NUM][MV_D]
                         , int w_scu, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);

int evey_deblock(void * ctx);

#ifdef __cplusplus
}
#endif

#endif /* _EVEY_LF_H_ */
