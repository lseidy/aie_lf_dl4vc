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

#ifndef _EVEY_UTIL_H_
#define _EVEY_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "evey_def.h"
#include <stdlib.h>

#define EVEY_IMGB_OPT_NONE                (0)

/* create image buffer */
EVEY_IMGB * evey_imgb_create(int w, int h, int cs, int opt, int pad[EVEY_IMGB_MAX_PLANE], int align[EVEY_IMGB_MAX_PLANE]);
void evey_imgb_cpy(EVEY_IMGB * dst, EVEY_IMGB * src);

/*! macro to determine maximum */
#define EVEY_MAX(a,b)                   (((a) > (b)) ? (a) : (b))

/*! macro to determine minimum */
#define EVEY_MIN(a,b)                   (((a) < (b)) ? (a) : (b))

/*! macro to absolute a value */
#define EVEY_ABS(a)                        abs(a)

/*! macro to absolute a 64-bit value */
#define EVEY_ABS64(a)                   (((a)^((a)>>63)) - ((a)>>63))

/*! macro to absolute a 32-bit value */
#define EVEY_ABS32(a)                   (((a)^((a)>>31)) - ((a)>>31))

/*! macro to absolute a 16-bit value */
#define EVEY_ABS16(a)                   (((a)^((a)>>15)) - ((a)>>15))

/*! macro to clipping within min and max */
#define EVEY_CLIP3(min_x, max_x, value)    EVEY_MAX((min_x), EVEY_MIN((max_x), (value)))

/*! macro to clipping within min and max */
#define EVEY_CLIP(n,min,max)             (((n)>(max))? (max) : (((n)<(min))? (min) : (n)))

#define EVEY_SIGN(x)                     (((x) < 0) ? -1 : 1)

/*! macro to get a sign from a 16-bit value.\n
operation: if(val < 0) return 1, else return 0 */
#define EVEY_SIGN_GET(val)                ((val<0)? 1: 0)

/*! macro to set sign into a value.\n
operation: if(sign == 0) return val, else if(sign == 1) return -val */
#define EVEY_SIGN_SET(val, sign)          ((sign)? -val : val)

/*! macro to get a sign from a 16-bit value.\n
operation: if(val < 0) return 1, else return 0 */
#define EVEY_SIGN_GET16(val)             (((val)>>15) & 1)

/*! macro to set sign into a 16-bit value.\n
operation: if(sign == 0) return val, else if(sign == 1) return -val */
#define EVEY_SIGN_SET16(val, sign)       (((val) ^ ((s16)((sign)<<15)>>15)) + (sign))

#define EVEY_ALIGN(val, align)          ((((val) + (align) - 1) / (align)) * (align))

#define EVEY_CONV_LOG2(v)                  (evey_tbl_log2[v])

u16 evey_get_avail_inter(void * ctx, void * core);
u16 evey_get_avail_intra(void * ctx, void * core);

EVEY_PIC * evey_pic_alloc(EVEY_PICBUF_ALLOCATOR * pa, int * ret);
void evey_pic_free(EVEY_PIC * pic);
void evey_pic_expand(EVEY_PIC * pic);

EVEY_PIC * evey_picbuf_alloc(int w, int h, int pad_l, int pad_c, int chroma_format_idc, int bit_depth, int * err);
void evey_picbuf_free(EVEY_PIC * pic);
void evey_picbuf_expand(EVEY_PIC * pic, int exp_l, int exp_c);

void evey_poc_derivation(EVEY_SPS * sps, int tid, EVEY_POC *poc);

#define EVEY_SPLIT_MAX_PART_COUNT  4

typedef struct _EVEY_SPLIT_STRUCT
{
    int       part_count;
    int       cud[EVEY_SPLIT_MAX_PART_COUNT];
    int       width[EVEY_SPLIT_MAX_PART_COUNT];
    int       height[EVEY_SPLIT_MAX_PART_COUNT];
    int       log_cuw[EVEY_SPLIT_MAX_PART_COUNT];
    int       log_cuh[EVEY_SPLIT_MAX_PART_COUNT];
    int       x_pos[EVEY_SPLIT_MAX_PART_COUNT];
    int       y_pos[EVEY_SPLIT_MAX_PART_COUNT];
    int       cup[EVEY_SPLIT_MAX_PART_COUNT];

} EVEY_SPLIT_STRUCT;

/* get partition split structure */
void evey_split_get_part_structure(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, EVEY_SPLIT_STRUCT * split_struct);

int evey_get_split_mode(s8 * split_mode, int cud, int cup, int cuw, int cuh, int ctu_s, s8(*split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_CTU]);
void evey_set_split_mode(s8  split_mode, int cud, int cup, int cuw, int cuh, int ctu_s, s8(*split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_CTU]);

void evey_get_mv_dir(void * ctx, void * core, s16 mvp[LIST_NUM][MV_D]);
void evey_get_motion(void * ctx, void * core, EVEY_REF_PIC_LIST lidx, s8 refi[EVEY_MVP_NUM], s16 mvp[EVEY_MVP_NUM][MV_D]);

int evey_scan_tbl_init();
int evey_scan_tbl_delete();

/* MD5 structure */
typedef struct _EVEY_MD5
{
    u32     h[4];    /* hash state ABCD */
    u8      msg[64]; /*input buffer (nalu message) */
    u32     bits[2]; /* number of bits, modulo 2^64 (lsb first)*/

} EVEY_MD5;

/* MD5 Functions */
void evey_md5_init(EVEY_MD5 * md5);
void evey_md5_update(EVEY_MD5 * md5, void * buf, u32 len);
void evey_md5_update_16(EVEY_MD5 * md5, void * buf, u32 len);
void evey_md5_finish(EVEY_MD5 * md5, u8 digest[16]);
int evey_md5_imgb(EVEY_IMGB * imgb, u8 digest[N_C][16]);
int evey_picbuf_signature(EVEY_PIC * pic, u8 md5_out[N_C][16]);

int evey_atomic_inc(volatile int * pcnt);
int evey_atomic_dec(volatile int * pcnt);

void evey_block_copy(s16 * src, int src_stride, s16 * dst, int dst_stride, int log2_copy_w, int log2_copy_h);

void evey_update_core_loc_param(void * ctx, void * core);

void evey_eco_init_ctx_model(EVEY_SBAC_CTX * sbac_ctx);

#ifdef __cplusplus
}
#endif

#endif /* _EVEY_UTIL_H_ */
