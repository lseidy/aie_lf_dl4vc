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

#ifndef _EVEYE_SAD_H_
#define _EVEYE_SAD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "eveye_def.h"

typedef int(*EVEYE_FN_SAD) (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
extern const EVEYE_FN_SAD eveye_tbl_sad_16b[8][8];
#define eveye_sad_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
    eveye_tbl_sad_16b[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)
#define eveye_sad_bi_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
    (eveye_tbl_sad_16b[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth) >> 1)

typedef int(*EVEYE_FN_SATD)(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
extern const EVEYE_FN_SATD eveye_tbl_satd_16b[8][8];
#define eveye_satd_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
    eveye_tbl_satd_16b[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)
#define eveye_satd_bi_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
    (eveye_tbl_satd_16b[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth) >> 1)

typedef s64(*EVEYE_FN_SSD) (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
extern const EVEYE_FN_SSD eveye_tbl_ssd_16b[8][8];
#define eveye_ssd_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
    eveye_tbl_ssd_16b[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)

#ifdef __cplusplus
}
#endif

#endif /* _EVEYE_SAD_H_ */
