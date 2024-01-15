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

#ifndef _EVEYE_DEF_H_
#define _EVEYE_DEF_H_

#include "evey_def.h"
#include "eveye_bsw.h"
#include "eveye_sad.h"


/* support RDOQ */
#define SCALE_BITS               15    /* Inherited from TMuC, pressumably for fractional bit estimates in RDOQ */
#define ERR_SCALE_PRECISION_BITS 20

/* EVEY encoder magic code */
#define EVEYE_MAGIC_CODE         0x45565945 /* EVYE */

/* Max. and min. Quantization parameter */
#define MAX_QUANT                51
#define MIN_QUANT                0
#define GOP_P                    8

/* picture buffers for encoding
   0 (PIC_IDX_CURR): current picture buffer
   1 (PIC_IDX_ORIG): original (input) picture buffer
*/
#define PIC_D                    2
/* current encoding picture buffer index */
#define PIC_IDX_CURR             0
/* original (input) picture buffer index */
#define PIC_IDX_ORIG             1

/* check whether bumping is progress or not */
#define FORCE_OUT(ctx)           (ctx->param.force_output == 1)

/* motion vector accuracy level for inter-mode decision */
#define ME_LEV_IPEL              1
#define ME_LEV_HPEL              2
#define ME_LEV_QPEL              3

/* maximum inbuf count */
#define EVEYE_MAX_INBUF_CNT      33

/* maximum cost value */
#define MAX_COST                 (1.7e+308)

/*****************************************************************************
 * mode decision structure
 *****************************************************************************/
typedef struct _EVEYE_MODE
{
    /* address of best coefficient for current CU */
    s16                  (* coef)[MAX_TR_DIM];
    /* address of best nnz for current CU */
    int                   * nnz;
    /* address of best nnz_sub for current CU */
    int                  (* nnz_sub)[MAX_SUB_TB_NUM];
    /* address of best reconstructions for current CU */
    pel                  (* rec)[MAX_CU_DIM];
    /* address of best reference indices */
    s8                    * refi;
    /* address of best MVP indices */
    u8                    * mvp_idx;
    /* address of best mv difference */
    s16                  (* mvd)[MV_D];    
    /* address of best mv */
    s16                  (* mv)[MV_D];
#if TRACE_ENC_CU_DATA
    u64                     trace_cu_idx;
#endif
    void                  * pdata[4];
    int                   * ndata[4];

} EVEYE_MODE;

/* frame depth for B pictures */
#define FRM_DEPTH_0              0
#define FRM_DEPTH_1              1
#define FRM_DEPTH_2              2
#define FRM_DEPTH_3              3
#define FRM_DEPTH_4              4
#define FRM_DEPTH_5              5
#define FRM_DEPTH_6              6
#define FRM_DEPTH_MAX            7


/*****************************************************************************
 * original picture buffer structure
 *****************************************************************************/
typedef struct _EVEYE_PICO
{
    /* original picture store */
    EVEY_PIC                pic;
    /* input picture count */
    u32                     pic_icnt;
    /* be used for encoding input */
    u8                      is_used;
    /* address of sub-picture */
    EVEY_PIC              * spic;

} EVEYE_PICO;


/*****************************************************************************
 * intra prediction structure
 *****************************************************************************/
typedef struct _EVEYE_PINTRA
{
    /* prediction buffer */
    pel                     pred_cache[IPD_CNT][MAX_CU_DIM]; /* only for luma */
    // XXNN picture buffer used to store the intra predictor context
    EVEY_PIC*              recon_fig; //recon context picture
    EVEY_PIC*              pred_fig; //pred context picture
    /* reconstruction buffer */
    pel                     rec[N_C][MAX_CU_DIM];
    pel                     rec_best[N_C][MAX_CU_DIM];
    /* coefficient buffer */
    s16                     coef_tmp[N_C][MAX_CU_DIM];
    s16                     coef_best[N_C][MAX_CU_DIM];
    int                     nnz_best[N_C];
    int                     nnz_sub_best[N_C][MAX_SUB_TB_NUM];

    int                     complexity;
    void                  * pdata[4];
    int                   * ndata[4];    

} EVEYE_PINTRA;


/*****************************************************************************
 * inter prediction structure
 *****************************************************************************/
#define MV_RANGE_MIN             0
#define MV_RANGE_MAX             1
#define MV_RANGE_DIM             2

typedef struct _EVEYE_PINTER EVEYE_PINTER;
struct _EVEYE_PINTER
{
    /* temporary prediction buffer (only used for ME)*/
    pel                     pred_buf[MAX_CU_DIM];
    /* temporary buffer for analyze_cu */
    s8                      refi[PRED_NUM][LIST_NUM];
    /* Ref idx predictor */
    s8                      refi_pred[LIST_NUM][EVEY_MVP_NUM];
    u8                      mvp_idx[PRED_NUM][LIST_NUM];
    s16                     mvp_scale[LIST_NUM][MAX_NUM_ACTIVE_REF_FRAME][EVEY_MVP_NUM][MV_D];
    s16                     mv_scale[LIST_NUM][MAX_NUM_ACTIVE_REF_FRAME][MV_D];
    int                     max_search_range;
    s16                     resi[N_C][MAX_CU_DIM];
    /* MV predictor */
    s16                     mvp[LIST_NUM][EVEY_MVP_NUM][MV_D];
    s16                     mv[PRED_NUM][LIST_NUM][MV_D];
    s16                     mvd[PRED_NUM][LIST_NUM][MV_D];
    s16                     org_bi[MAX_CU_DIM];
    s32                     mot_bits[LIST_NUM];
    /* temporary prediction buffer (only used for ME)*/
    pel                     pred[PRED_NUM + 1][2][N_C][MAX_CU_DIM];
    /* reconstruction buffer */
    pel                     rec[PRED_NUM][N_C][MAX_CU_DIM];
    /* last one buffer used for RDO */
    s16                     coef[PRED_NUM + 1][N_C][MAX_CU_DIM];
    int                     nnz_best[PRED_NUM][N_C];
    int                     nnz_sub_best[PRED_NUM][N_C][MAX_SUB_TB_NUM];
    u8                      num_refp;
    /* minimum clip value */
    s16                     min_clip[MV_D];
    /* maximum clip value */
    s16                     max_clip[MV_D];
    /* original (input) block buffer pointer */
    pel                   * o_y;
    /* motion vector map */
    s16                  (* map_mv)[LIST_NUM][MV_D];
    /* picture width in SCU unit */
    u16                     w_scu;
    /* labda for mv */
    u32                     lambda_mv;
    /* reference pictures */
    EVEY_REFP            (* refp)[LIST_NUM];
    /* search level for motion estimation */
    int                     me_level;    
    /* current picture order count */
    int                     poc;
    /* gop size */
    int                     gop_size;
    /* best inter pred mode index */
    int                     best_idx;
    /* address of best pred for y */
    pel                   * pred_y_best;
    /* ME function (Full-ME or Fast-ME) */
    u32  (*fn_me)(EVEYE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 *refi, int lidx, s16 mvp[MV_D], s16 mv[MV_D], int bi, int bit_depth_luma);

    int                     complexity;
    void                  * pdata[4];
    int                   * ndata[4];
};

/* EVEY encoder parameter */
typedef struct _EVEYE_PARAM
{
    /* picture size of input sequence (width) */
    int                     w;
    /* picture size of input sequence (height) */
    int                     h;
    /* picture bit depth*/
    int                     bit_depth;
    /* qp value for I- and P- slice */
    int                     qp;
    /* frame per second */
    int                     fps;
    /* Enable deblocking filter or not
       - 0: Disable deblocking filter
       - 1: Enable deblocking filter
    */
    int                     use_deblock;
    /* I-frame period */
    int                     i_period;
    /* force I-frame */
    int                     f_ifrm;
    /* Maximum qp value */
    int                     qp_max;
    /* Minimum qp value */
    int                     qp_min;
    /*indicates set of constraints*/
    int                     toolset_idc_h; /* should be 0 */
    int                     toolset_idc_l; /* should be 0 */
    /* use picture signature embedding */
    int                     use_pic_sign;
    int                     max_b_frames;
    int                     max_num_ref_pics;
    int                     ref_pic_gap_length;
    /* start bumping process if force_output is on */
    int                     force_output;
    int                     gop_size;
    int                     use_dqp;
    int                     use_closed_gop;
    int                     use_hgop;
    int                     chroma_format_idc;
    int                     qp_incread_frame;           /* 10 bits */
    EVEY_CHROMA_TABLE       chroma_qp_table_struct;

} EVEYE_PARAM;

typedef struct _EVEYE_SBAC
{
    u32                     range;
    u32                     code;
    u32                     code_bits;
    u32                     stacked_ff;
    u32                     stacked_zero;
    u32                     pending_byte;
    u32                     is_pending_byte;
    EVEY_SBAC_CTX           ctx;
    u32                     bit_counter;
    u8                      is_bit_count;
    u32                     bin_counter;

} EVEYE_SBAC;

typedef struct _EVEYE_DQP
{
    s8                      prev_qp;
    s8                      curr_qp;

} EVEYE_DQP;

typedef struct _EVEYE_CU_DATA
{
    s8                      split_mode[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_CTU];
    u8                    * qp_y;
    u8                    * qp_u;
    u8                    * qp_v;
    u8                    * pred_mode;
    s8                   ** ipm;
    s8                   ** refi;    
    u8                   ** mvp_idx; 
    s16                     mv[MAX_CU_CNT_IN_CTU][LIST_NUM][MV_D];
    s16                     mvd[MAX_CU_CNT_IN_CTU][LIST_NUM][MV_D];
    int                   * nnz[N_C];
    int                   * nnz_sub[N_C][4];
    u32                   * map_scu;
    s16                   * coef[N_C]; 
    pel                   * reco[N_C]; 
#if TRACE_ENC_CU_DATA
    u64                     trace_idx[MAX_CU_CNT_IN_CTU];
#endif

} EVEYE_CU_DATA;


/*****************************************************************************
 * CORE information used for encoding process.
 *
 * The variables in this structure are very often used in encoding process.
 *****************************************************************************/
typedef struct _EVEYE_CORE
{
    EVEY_CORE; /* should be first */
       
    /* CU data for RDO */
    EVEYE_CU_DATA           cu_data_best[MAX_CU_DEPTH][MAX_CU_DEPTH];
    EVEYE_CU_DATA           cu_data_temp[MAX_CU_DEPTH][MAX_CU_DEPTH];
    EVEYE_DQP               dqp_data[MAX_CU_DEPTH][MAX_CU_DEPTH];
    /* temporary coefficient buffer */
    EVEYE_DQP               dqp_curr_best[MAX_CU_DEPTH][MAX_CU_DEPTH];
    EVEYE_DQP               dqp_next_best[MAX_CU_DEPTH][MAX_CU_DEPTH];
    EVEYE_DQP               dqp_temp_best;
    EVEYE_DQP               dqp_temp_run;
    /* bitstream structure for RDO */
    EVEYE_BSW               bs_temp;
    /* SBAC structure for full RDO */
    EVEYE_SBAC              s_curr_best[MAX_CU_DEPTH][MAX_CU_DEPTH];
    EVEYE_SBAC              s_next_best[MAX_CU_DEPTH][MAX_CU_DEPTH];
    EVEYE_SBAC              s_temp_best;
    EVEYE_SBAC              s_temp_run;
    EVEYE_SBAC              s_temp_prev_comp_best;
    EVEYE_SBAC              s_temp_prev_comp_run;
    EVEYE_SBAC              s_curr_before_split[MAX_CU_DEPTH][MAX_CU_DEPTH];
    double                  cost_best;
    s32                     dist_cu;
    s32                     dist_cu_best; /* dist of the best intra mode (note: only updated in intra coding now) */
    /* one picture that arranges cu pixels and neighboring pixels for deblocking (just to match the interface of deblocking functions) */
    s64                     delta_dist[N_C];  /* delta distortion from filtering (negative values mean distortion reduced) */
    s64                     dist_nofilt[N_C]; /* distortion of not filtered samples */
    s64                     dist_filter[N_C]; /* distortion of filtered samples */
    /* RDOQ related variables*/
    int                     rdoq_est_cbf_all[2];
    int                     rdoq_est_cbf_luma[2];
    int                     rdoq_est_cbf_cb[2];
    int                     rdoq_est_cbf_cr[2];
    s32                     rdoq_est_run[NUM_CTX_CC_RUN][2];
    s32                     rdoq_est_level[NUM_CTX_CC_LEVEL][2];
    s32                     rdoq_est_last[NUM_CTX_CC_LAST][2];
    /* original buffer */
    pel                     org[N_C][MAX_CU_DIM];
#if TRACE_ENC_CU_DATA
    u64                     trace_idx;
#endif
    /* platform specific data, if needed */
    void                  * pf;

} EVEYE_CORE;

/******************************************************************************
 * CONTEXT used for encoding process.
 *
 * All have to be stored are in this structure.
 *****************************************************************************/
 typedef struct _EVEYE_CTX EVEYE_CTX;

struct _EVEYE_CTX
{
    EVEY_CTX; /* should be first */

    /* EVEYE identifier */
    EVEYE                   id;
    /* create descriptor */
    EVEYE_CDSC              cdsc;
    /* address of core structure */
    EVEYE_CORE            * core;
    /* SBAC */
    EVEYE_SBAC              sbac_enc;
    /* bitstream structure */
    EVEYE_BSW               bs;
    /* address indicating current and original pictures */
    EVEY_PIC              * pic_e[PIC_D]; /* the last one is for original */
    /* one picture that arranges cu pixels and neighboring pixels for deblocking (just to match the interface of deblocking functions) */
    EVEY_PIC              * pic_dbk;
    /* address of current input picture, ref_picture  buffer structure */
    EVEYE_PICO            * pico_buf[EVEYE_MAX_INBUF_CNT];
    /* address of current input picture buffer structure */
    EVEYE_PICO            * pico;    
    /* index of current input picture buffer in pico_buf[] */
    u8                      pico_idx;
    int                     pico_max_cnt;    
    /* current picture input count (only update when CTX0) */
    u32                     pic_icnt;
    /* total input picture count (only used for bumping process) */
    u32                     pic_ticnt;
    /* remaining pictures is encoded to p or b slice (only used for bumping process) */
    u8                      force_slice;
    /* ignored pictures for force slice count (unavailable pictures cnt in gop,\
    only used for bumping process) */
    u8                      force_ignored_cnt;
    /* initial frame return number(delayed input count) due to B picture or Forecast */
    u32                     frm_rnum;    
    /* slice depth for current picture */
    u8                      slice_depth;  
    /* address of inbufs */
    EVEY_IMGB             * inbuf[EVEYE_MAX_INBUF_CNT];    
    /* encoding parameter */
    EVEYE_PARAM             param;    
    /* mode decision structure */
    EVEYE_MODE              mode;
    /* intra prediction analysis */
    EVEYE_PINTRA            pintra;
    /* inter prediction analysis */
    EVEYE_PINTER            pinter;
    /* cu data for current CTU */
    EVEYE_CU_DATA         * map_cu_data;
    double                  lambda[3];
    double                  sqrt_lambda[3];
    double                  dist_chroma_weight[2];

    int    (*fn_ready)(EVEYE_CTX * ctx);
    void   (*fn_flush)(EVEYE_CTX * ctx);
    int    (*fn_enc)(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat);
    int    (*fn_enc_pic_prepare)(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat);
    int    (*fn_enc_pic)(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat);
    int    (*fn_enc_pic_finish)(EVEYE_CTX * ctx, EVEY_BITB * bitb, EVEYE_STAT * stat);
    int    (*fn_push)(EVEYE_CTX * ctx, EVEY_IMGB * img);
    int    (*fn_deblock)(void * ctx);
    int    (*fn_get_inbuf)(EVEYE_CTX * ctx, EVEY_IMGB ** img);
    
    /* mode decision functions */
    int    (*fn_mode_init_frame)(EVEYE_CTX * ctx);
    int    (*fn_mode_init_ctu)(EVEYE_CTX * ctx, EVEYE_CORE * core);
    int    (*fn_mode_analyze_frame)(EVEYE_CTX * ctx);
    int    (*fn_mode_analyze_ctu)(EVEYE_CTX * ctx, EVEYE_CORE * core);
    int    (*fn_mode_set_complexity)(EVEYE_CTX * ctx, int complexity);

    /* intra prediction functions */
    int    (*fn_pintra_init_frame)(EVEYE_CTX * ctx);
    int    (*fn_pintra_init_ctu)(EVEYE_CTX * ctx, EVEYE_CORE * core);
    double (*fn_pintra_analyze_cu)(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y);
    int    (*fn_pintra_set_complexity)(EVEYE_CTX * ctx, int complexity);

    /* inter prediction functions */
    int    (*fn_pinter_init_frame)(EVEYE_CTX * ctx);
    int    (*fn_pinter_init_ctu)(EVEYE_CTX * ctx, EVEYE_CORE * core);
    double (*fn_pinter_analyze_cu)(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y);
    int    (*fn_pinter_set_complexity)(EVEYE_CTX * ctx, int complexity);

    /* platform specific data, if needed */
    void                  * pf;
};

#include "eveye_util.h"
#include "eveye_eco.h"
#include "eveye_mode.h"
#include "eveye_tq.h"
#include "eveye_pintra.h"
#include "eveye_pinter.h"
#include "eveye_tbl.h"

#endif /* _EVEYE_DEF_H_ */
