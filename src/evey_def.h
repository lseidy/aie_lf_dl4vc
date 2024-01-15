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

#ifndef _EVEY_DEF_H_
#define _EVEY_DEF_H_

#include "evey.h"
#include "evey_port.h"

/* SIMD Optimizations */
#if X86_SSE
#define OPT_SIMD_MC_L                      1
#define OPT_SIMD_MC_C                      1
#define OPT_SIMD_SAD                       1
#define OPT_SIMD_HAD_SAD                   1
#else
#define OPT_SIMD_MC_L                      0
#define OPT_SIMD_MC_C                      0 
#define OPT_SIMD_SAD                       0
#define OPT_SIMD_HAD_SAD                   0
#endif

#define MAX_NUM_PPS                        64

/* Multiple Referene */
#define MAX_NUM_ACTIVE_REF_FRAME_B         2  /* Maximum number of active reference frames for RA condition */
#define MAX_NUM_ACTIVE_REF_FRAME_LDB       4  /* Maximum number of active reference frames for LDB condition */
#define MVP_SCALING_PRECISION              5  /* Scaling precision for motion vector prediction (2^MVP_SCALING_PRECISION) */

/* Residual coding */
#define COEF_SCAN_ZIGZAG                   0
#define COEF_SCAN_TYPE_NUM                 1

/* QP related */
#define GET_QP(qp,dqp)                     ((qp + dqp + 52) % 52)
#define GET_LUMA_QP(qp, qp_bd_offset)      (qp + 6 * qp_bd_offset)
#define MAX_QP_TABLE_SIZE                  58
#define MAX_QP_TABLE_SIZE_EXT              94

/* imgb related */
#define STRIDE_IMGB2PIC(s_imgb)            ((s_imgb)>>1)

/* encoder fast algorithms */
#define ENC_ECU_SKIP                       1
#if ENC_ECU_SKIP
#define ENC_ECU_DEPTH                      8 /* for early CU termination */
#endif
#define ENC_FAST_SKIP_INTRA                0 /* Note: this might be turned off for the intra method currently implemented */

/* Profiles definitions */
#define PROFILE_VIDEO                      0
#define PROFILE_STILL_PICTURE              2

/* For debugging */
#define USE_DRAW_PARTITION_DEC             0
#define ENC_DEC_TRACE                      0
#if ENC_DEC_TRACE
#define TRACE_ENC_CU_DATA                  0 ///< Trace CU index on encoder
#define TRACE_ENC_CU_DATA_CHECK            0 ///< Trace CU index on encoder
#define MVF_TRACE                          0 ///< use for tracing MVF
#define TRACE_COEFFS                       0 ///< Trace coefficients
#define TRACE_RDO                          0 //!< Trace only encode stream (0), only RDO (1) or all of them (2)
#define TRACE_BIN                          0 //!< trace each bin
#define TRACE_START_POC                    0 //!< POC of frame from which we start to write output tracing information 
#define TRACE_COSTS                        0 //!< Trace cost information
#define TRACE_REMOVE_COUNTER               0 //!< Remove trace counter
#define TRACE_DBF                          0 //!< Trace only DBF
#define TRACE_HLS                          0 //!< Trace SPS, PPS, APS, Slice Header, etc.
#if TRACE_RDO
#define TRACE_RDO_EXCLUDE_I                0 //!< Exclude I frames
#endif
extern FILE *fp_trace;
extern int fp_trace_print;
extern int fp_trace_counter;
#if TRACE_START_POC
extern int fp_trace_started;
#endif
#if TRACE_RDO == 1
#define EVEY_TRACE_SET(A) fp_trace_print=!A
#elif TRACE_RDO == 2
#define EVEY_TRACE_SET(A)
#else
#define EVEY_TRACE_SET(A) fp_trace_print=A
#endif
#define EVEY_TRACE_STR(STR) if(fp_trace_print) { fprintf(fp_trace, STR); fflush(fp_trace); }
#define EVEY_TRACE_DOUBLE(DOU) if(fp_trace_print) { fprintf(fp_trace, "%g", DOU); fflush(fp_trace); }
#define EVEY_TRACE_INT(INT) if(fp_trace_print) { fprintf(fp_trace, "%d ", INT); fflush(fp_trace); }
#define EVEY_TRACE_INT_HEX(INT) if(fp_trace_print) { fprintf(fp_trace, "0x%x ", INT); fflush(fp_trace); }
#if TRACE_REMOVE_COUNTER
#define EVEY_TRACE_COUNTER 
#else
#define EVEY_TRACE_COUNTER  EVEY_TRACE_INT(fp_trace_counter++); EVEY_TRACE_STR("\t")
#endif
#define EVEY_TRACE_MV(X, Y) if(fp_trace_print) { fprintf(fp_trace, "(%d, %d) ", X, Y); fflush(fp_trace); }
#define EVEY_TRACE_FLUSH    if(fp_trace_print) fflush(fp_trace)
#else
#define EVEY_TRACE_SET(A)
#define EVEY_TRACE_STR(str)
#define EVEY_TRACE_DOUBLE(DOU)
#define EVEY_TRACE_INT(INT)
#define EVEY_TRACE_INT_HEX(INT)
#define EVEY_TRACE_COUNTER 
#define EVEY_TRACE_MV(X, Y)
#define EVEY_TRACE_FLUSH
#endif

typedef enum _EVEY_COMPONENT
{
    Y_C, /* Y luma */
    U_C, /* Cb Chroma */
    V_C, /* Cr Chroma */
    N_C  /* number of color component */

} EVEY_COMPONENT;

typedef enum _EVEY_REF_PIC_LIST
{
    LIST_0,    /* reference picture list 0 */
    LIST_1,    /* reference picture list 1 */
    LIST_NUM   /* number of list */

} EVEY_REF_PIC_LIST;

typedef enum _EVEY_MV_COMPONENT
{
    MV_X, /* X direction motion vector indicator */
    MV_Y, /* Y direction motion vector indicator */
    MV_D  /* Maximum count (dimension) of motion */

} EVEY_MV_COMPONENT;

#define MAX_CU_LOG2                        6  /* CTU size: 64 */
#define MIN_CU_LOG2                        2  /* Minimum CU size: 4 */
#define MAX_CU_SIZE                        (1 << MAX_CU_LOG2)
#define MIN_CU_SIZE                        (1 << MIN_CU_LOG2)
#define MAX_CU_DIM                         (MAX_CU_SIZE * MAX_CU_SIZE)
#define MIN_CU_DIM                         (MIN_CU_SIZE * MIN_CU_SIZE)
#define MAX_CU_DEPTH                       8  /* 64x64 ~ 4x4 */
#define NUM_CU_DEPTH                       (MAX_CU_DEPTH + 1)

#define MAX_TR_LOG2                        6  /* 64x64 */
#define MIN_TR_LOG2                        1  /* 2x2 */
#define MAX_TR_SIZE                        (1 << MAX_TR_LOG2)
#define MIN_TR_SIZE                        (1 << MIN_TR_LOG2)
#define MAX_TR_DIM                         (MAX_TR_SIZE * MAX_TR_SIZE)
#define MIN_TR_DIM                         (MIN_TR_SIZE * MIN_TR_SIZE)

#define INTRA_REF_NUM                      2  /* left, up */
#define INTRA_REF_SIZE                     (MAX_CU_SIZE * 2 + 1)

#define MAX_SUB_TB_NUM                     4

/* maximum CB count in a LCB */
#define MAX_CU_CNT_IN_CTU                  (MAX_CU_DIM / MIN_CU_DIM)

/* pixel position to SCB position */
#define PEL2SCU(pel)                       ((pel) >> MIN_CU_LOG2)

/* padding size of reference pictures */
#define PIC_PAD_SIZE_L                     (128 + 16)                /* TBD: need to check whether padding size is appropriate */
#define PIC_PAD_SIZE_C                     (PIC_PAD_SIZE_L >> 1)

/* number of MVP candidates */
#define EVEY_MVP_NUM                       4

/* maximum picture buffer size */
#define MAX_PB_SIZE                        (MAX_NUM_REF_PICS + 5) /* TBD: Should be checked */

/* maximum tiles in row or col */
#define MAX_NUM_TILES_ROW                  1
#define MAX_NUM_TILES_COL                  1

/* Neighboring block availability flag bits */
#define AVAIL_BIT_UP                       0
#define AVAIL_BIT_LE                       1
#define AVAIL_BIT_UP_LE                    2
#define AVAIL_BIT_UP_RI                    3

/* Neighboring block availability flags */
#define AVAIL_UP                           (1 << AVAIL_BIT_UP)
#define AVAIL_LE                           (1 << AVAIL_BIT_LE)
#define AVAIL_UP_LE                        (1 << AVAIL_BIT_UP_LE)
#define AVAIL_UP_RI                        (1 << AVAIL_BIT_UP_RI)

/* MB availability check macro */
#define IS_AVAIL(avail, pos)               (((avail)&(pos)) == (pos))
/* MB availability set macro */
#define SET_AVAIL(avail, pos)              (avail) |= (pos)
/* MB availability remove macro */
#define REM_AVAIL(avail, pos)              (avail) &= (~(pos))
/* MB availability into bit flag */
#define GET_AVAIL_FLAG(avail, bit)         (((avail)>>(bit)) & 0x1)

/*****************************************************************************
 * slice type
 *****************************************************************************/
#define SLICE_I                            EVEY_ST_I
#define SLICE_P                            EVEY_ST_P
#define SLICE_B                            EVEY_ST_B

#define IS_INTRA_SLICE(slice_type)         ((slice_type) == SLICE_I))
#define IS_INTER_SLICE(slice_type)         (((slice_type) == SLICE_P) || ((slice_type) == SLICE_B))

/*****************************************************************************
 * prediction mode
 *****************************************************************************/
typedef enum _EVEY_CU_MODE
{
    MODE_INTRA,
    MODE_INTER,
    MODE_SKIP,
    MODE_DIR,
    MODE_NUM

} EVEY_CU_MODE;

/*****************************************************************************
 * inter prediction
 *****************************************************************************/
typedef enum _EVEY_INTER_PRED_MODE
{
    PRED_L0,   /* inter pred direction, refering to list0 */
    PRED_L1,   /* inter pred direction, refering to list1 */
    PRED_BI,   /* inter pred direction, refering to both list0 and list1 */
    PRED_SKIP, /* inter skip mode */
    PRED_DIR,  /* inter direct mode, refering to both list0 and list1 */
    PRED_NUM   /* number of inter prediction modes */

} EVEY_INTER_PRED_MODE;

/*****************************************************************************
 * intra prediction
 *****************************************************************************/
typedef enum _EVEY_INTRA_PRED_MODE
{
    IPD_DC,  /* DC */
    IPD_HOR, /* Horizontal */
    IPD_VER, /* Vertical */
    IPD_UL,
    IPD_UR,
    IPD_CNT

} EVEY_INTRA_PRED_MODE;

#define IPD_RDO_CNT                        5
#define IPD_INVALID                        (-1)

/*****************************************************************************
 * reference index
 *****************************************************************************/
#define REFI_INVALID                       (-1)
#define REFI_IS_VALID(refi)                ((refi) >= 0)
#define SET_REFI(refi, idx0, idx1)         (refi)[LIST_0] = (idx0); (refi)[LIST_1] = (idx1)

 /*****************************************************************************
 * macros for CU map

 - [ 0: 6] : slice number (0 ~ 128)
 - [ 7:14] : reserved
 - [15:15] : 1 -> intra CU, 0 -> inter CU
 - [16:22] : QP
 - [23:23] : reserved
 - [24:24] : cbf for y
 - [25:25] : cbf for cb
 - [26:26] : cbf for cr
 - [27:30] : reserved
 - [31:31] : 0 -> no encoded/decoded CU, 1 -> encoded/decoded CU
 *****************************************************************************/
/* set slice number to map */
#define MCU_SET_SN(m, sn)       (m)=(((m) & 0xFFFFFF80)|((sn) & 0x7F))
/* get slice number from map */
#define MCU_GET_SN(m)           (int)((m) & 0x7F)

/* set intra CU flag to map */
#define MCU_SET_IF(m)           (m)=((m)|(1<<15))
/* get intra CU flag from map */
#define MCU_GET_IF(m)           (int)(((m)>>15) & 1)
/* clear intra CU flag in map */
#define MCU_CLR_IF(m)           (m)=((m) & 0xFFFF7FFF)

/* set QP to map */
#define MCU_SET_QP(m, qp)       (m)=((m)|((qp)&0x7F)<<16)
/* get QP from map */
#define MCU_GET_QP(m)           (int)(((m)>>16)&0x7F)
#define MCU_RESET_QP(m)         (m)=((m) & (~((127)<<16)))

/* set luma cbf flag */
#define MCU_SET_CBFL(m)         (m)=((m)|(1<<24))
/* get luma cbf flag */
#define MCU_GET_CBFL(m)         (int)(((m)>>24) & 1)
/* clear luma cbf flag */
#define MCU_CLR_CBFL(m)         (m)=((m) & (~(1<<24)))

/* set cb cbf flag */
#define MCU_SET_CBFCB(m)        (m)=((m)|(1<<25))
/* get cb cbf flag */
#define MCU_GET_CBFCB(m)        (int)(((m)>>25) & 1)
/* clear cb cbf flag */
#define MCU_CLR_CBFCB(m)        (m)=((m) & (~(1<<25)))

/* set cr cbf flag */
#define MCU_SET_CBFCR(m)        (m)=((m)|(1<<26))
/* get cr cbf flag */
#define MCU_GET_CBFCR(m)        (int)(((m)>>26) & 1)
/* clear cr cbf flag */
#define MCU_CLR_CBFCR(m)        (m)=((m) & (~(1<<26)))

/* set encoded/decoded CU to map */
#define MCU_SET_COD(m)          (m)=((m)|(1<<31))
/* get encoded/decoded CU flag from map */
#define MCU_GET_COD(m)          (int)(((m)>>31) & 1)
/* clear encoded/decoded CU flag to map */
#define MCU_CLR_COD(m)          (m)=((m) & 0x7FFFFFFF)

/* multi bit setting: intra flag, encoded/decoded flag, slice number */
#define MCU_SET_IF_COD_SN_QP(m, i, sn, qp) \
    (m) = (((m)&0xFF807F80)|((sn)&0x7F)|((qp)<<16)|((i)<<15)|(1<<31))

#define MCU_IS_COD_NIF(m)      ((((m)>>15) & 0x10001) == 0x10000)

/* context models for arithemetic coding */
typedef u16 SBAC_CTX_MODEL;
#define NUM_CTX_SKIP_FLAG                  1
#define NUM_CTX_CBF_LUMA                   1
#define NUM_CTX_CBF_CB                     1
#define NUM_CTX_CBF_CR                     1
#define NUM_CTX_CBF_ALL                    1
#define NUM_CTX_PRED_MODE                  1
#define NUM_CTX_INTER_PRED_IDC             2  /* number of context models for inter prediction direction */
#define NUM_CTX_DIRECT_MODE_FLAG           1
#define NUM_CTX_REF_IDX                    2
#define NUM_CTX_MVP_IDX                    3
#define NUM_CTX_MVD                        1  /* number of context models for motion vector difference */
#define NUM_CTX_INTRA_PRED_MODE            2
#define NUM_CTX_CC_RUN                     4
#define NUM_CTX_CC_LAST                    2
#define NUM_CTX_CC_LEVEL                   4
#define NUM_CTX_SPLIT_CU_FLAG              1
#define NUM_CTX_DELTA_QP                   1
typedef struct _EVEY_SBAC_CTX
{
    SBAC_CTX_MODEL   skip_flag            [NUM_CTX_SKIP_FLAG];
    SBAC_CTX_MODEL   direct_mode_flag     [NUM_CTX_DIRECT_MODE_FLAG];
    SBAC_CTX_MODEL   inter_dir            [NUM_CTX_INTER_PRED_IDC];
    SBAC_CTX_MODEL   intra_dir            [NUM_CTX_INTRA_PRED_MODE];
    SBAC_CTX_MODEL   pred_mode            [NUM_CTX_PRED_MODE];
    SBAC_CTX_MODEL   refi                 [NUM_CTX_REF_IDX];
    SBAC_CTX_MODEL   mvp_idx              [NUM_CTX_MVP_IDX];
    SBAC_CTX_MODEL   mvd                  [NUM_CTX_MVD];
    SBAC_CTX_MODEL   cbf_all              [NUM_CTX_CBF_ALL];
    SBAC_CTX_MODEL   cbf_luma             [NUM_CTX_CBF_LUMA];
    SBAC_CTX_MODEL   cbf_cb               [NUM_CTX_CBF_CB];
    SBAC_CTX_MODEL   cbf_cr               [NUM_CTX_CBF_CR];
    SBAC_CTX_MODEL   run                  [NUM_CTX_CC_RUN];
    SBAC_CTX_MODEL   last                 [NUM_CTX_CC_LAST];
    SBAC_CTX_MODEL   level                [NUM_CTX_CC_LEVEL];
    SBAC_CTX_MODEL   split_cu_flag        [NUM_CTX_SPLIT_CU_FLAG];
    SBAC_CTX_MODEL   delta_qp             [NUM_CTX_DELTA_QP];
    
} EVEY_SBAC_CTX;

/* Maximum transform dynamic range (excluding sign bit) */
#define MAX_TX_DYNAMIC_RANGE               15
#define MAX_TX_VAL                         ((1 << MAX_TX_DYNAMIC_RANGE) - 1)
#define MIN_TX_VAL                         (-(1 << MAX_TX_DYNAMIC_RANGE))

#define QUANT_SHIFT                        14
#define QUANT_IQUANT_SHIFT                 20

/* neighbor CUs
   neighbor position:

   D     B     C

   A     X,<G>

   E          <F>
*/
#define MAX_NEB                            5
#define NEB_A                              0  /* left */
#define NEB_B                              1  /* up */
#define NEB_C                              2  /* up-right */
#define NEB_D                              3  /* up-left */
#define NEB_E                              4  /* low-left */

#define NEB_F                              5  /* co-located of low-right */
#define NEB_G                              6  /* co-located of X */
#define NEB_X                              7  /* center (current block) */
#define NEB_H                              8  /* right */  
#define NEB_I                              9  /* low-right */  
#define MAX_NEB2                           10

/* picture store structure */
typedef struct _EVEY_PIC
{
    /* Address of Y buffer (include padding) */
    pel            * buf_y;
    /* Address of U buffer (include padding) */
    pel            * buf_u;
    /* Address of V buffer (include padding) */
    pel            * buf_v;
    /* Start address of Y component (except padding) */
    pel            * y;
    /* Start address of U component (except padding)  */
    pel            * u;
    /* Start address of V component (except padding)  */
    pel            * v;
    /* Stride of luma picture */
    int              s_l;
    /* Stride of chroma picture */
    int              s_c;
    /* Width of luma picture */
    int              w_l;
    /* Height of luma picture */
    int              h_l;
    /* Width of chroma picture */
    int              w_c;
    /* Height of chroma picture */
    int              h_c;
    /* padding size of luma */
    int              pad_l;
    /* padding size of chroma */
    int              pad_c;
    /* image buffer */
    EVEY_IMGB      * imgb;
    /* presentation temporal reference of this picture */
    u32              poc;
    /* 0: not used for reference buffer, reference picture type */
    u8               is_ref;
    /* needed for output? */
    u8               need_for_out;
    /* scalable layer id */
    u8               temporal_id;
    s16           (* map_mv)[LIST_NUM][MV_D];
    s8            (* map_refi)[LIST_NUM];
    u32              list_poc[MAX_NUM_REF_PICS];
    int              pic_qp_u_offset;
    int              pic_qp_v_offset;
    u8               digest[N_C][16];

} EVEY_PIC;

/*****************************************************************************
 * picture buffer allocator
 *****************************************************************************/
typedef struct _EVEY_PICBUF_ALLOCATOR EVEY_PICBUF_ALLOCATOR;
struct _EVEY_PICBUF_ALLOCATOR
{
    /* address of picture buffer allocation function */
    EVEY_PIC      *(*fn_alloc)(EVEY_PICBUF_ALLOCATOR * pa, int * ret);
    /* address of picture buffer free function */
    void           (*fn_free)(EVEY_PIC * pic);
    /* address of picture buffer expand */
    void           (*fn_expand)(EVEY_PIC * pic);
    /* width */
    int              w;
    /* height */
    int              h;
    /* pad size for luma */
    int              pad_l;
    /* pad size for chroma */
    int              pad_c;
    /* arbitrary data, if needs */
    int              ndata[4];
    /* arbitrary address, if needs */
    void           * pdata[4];
    int              bit_depth;
    int              chroma_format_idc;
};

/*****************************************************************************
 * picture manager for DPB in decoder and RPB in encoder
 *****************************************************************************/
typedef struct _EVEY_PM
{
    /* picture store (including reference and non-reference) */
    EVEY_PIC       * pic[MAX_PB_SIZE];
    /* address of reference pictures */
    EVEY_PIC       * pic_ref[MAX_NUM_REF_PICS];
    /* maximum reference picture count */
    u8               max_num_ref_pics;
    /* current count of available reference pictures in PB */
    u8               cur_num_ref_pics;
    /* number of reference pictures */
    u8               num_refp[LIST_NUM];
    /* next output POC */
    u32              poc_next_output;
    /* POC increment */
    u8               poc_increase;
    /* max number of picture buffer */
    u8               max_pb_size;
    /* current picture buffer size */
    u8               cur_pb_size;
    /* address of leased picture for current decoding/encoding buffer */
    EVEY_PIC       * pic_lease;
    /* picture buffer allocator */
    EVEY_PICBUF_ALLOCATOR pa;

} EVEY_PM;

/* reference picture structure */
typedef struct _EVEY_REFP
{
    /* address of reference picture */
    EVEY_PIC       * pic;
    /* POC of reference picture */
    u32              poc;
    s16           (* map_mv)[LIST_NUM][MV_D];
    s8            (* map_refi)[LIST_NUM];
    u32            * list_poc;

} EVEY_REFP;

/*****************************************************************************
 * NALU header
 *****************************************************************************/
typedef struct _EVEY_NALU
{
    int              nal_unit_size;
    int              forbidden_zero_bit;
    int              nal_unit_type_plus1;
    int              nuh_temporal_id;
    int              nuh_reserved_zero_5bits;
    int              nuh_extension_flag;

} EVEY_NALU;


#define EXTENDED_SAR                       255
#define NUM_CPB                            32

/*****************************************************************************
* Hypothetical Reference Decoder (HRD) parameters, part of VUI
*****************************************************************************/
typedef struct _EVEY_HRD
{
    int              cpb_cnt_minus1;
    int              bit_rate_scale;
    int              cpb_size_scale;
    int              bit_rate_value_minus1[NUM_CPB];
    int              cpb_size_value_minus1[NUM_CPB];
    int              cbr_flag[NUM_CPB];
    int              initial_cpb_removal_delay_length_minus1;
    int              cpb_removal_delay_length_minus1;
    int              dpb_output_delay_length_minus1;
    int              time_offset_length;

} EVEY_HRD;

/*****************************************************************************
* video usability information (VUI) part of SPS
*****************************************************************************/
typedef struct _EVEY_VUI
{
    int              aspect_ratio_info_present_flag;
    int              aspect_ratio_idc;
    int              sar_width;
    int              sar_height;
    int              overscan_info_present_flag;
    int              overscan_appropriate_flag;
    int              video_signal_type_present_flag;
    int              video_format;
    int              video_full_range_flag;
    int              colour_description_present_flag;
    int              colour_primaries;
    int              transfer_characteristics;
    int              matrix_coefficients;
    int              chroma_loc_info_present_flag;
    int              chroma_sample_loc_type_top_field;
    int              chroma_sample_loc_type_bottom_field;
    int              neutral_chroma_indication_flag;
    int              field_seq_flag;
    int              timing_info_present_flag;
    int              num_units_in_tick;
    int              time_scale;
    int              fixed_pic_rate_flag;
    int              nal_hrd_parameters_present_flag;
    int              vcl_hrd_parameters_present_flag;
    int              low_delay_hrd_flag;
    int              pic_struct_present_flag;
    int              bitstream_restriction_flag;
    int              motion_vectors_over_pic_boundaries_flag;
    int              max_bytes_per_pic_denom;
    int              max_bits_per_mb_denom;
    int              log2_max_mv_length_horizontal;
    int              log2_max_mv_length_vertical;
    int              num_reorder_pics;
    int              max_dec_pic_buffering;
    EVEY_HRD         hrd_parameters;

} EVEY_VUI;

/*****************************************************************************
 * sequence parameter set
 *****************************************************************************/
 /* chromaQP table structure to be signalled in SPS*/
typedef struct _EVEY_CHROMA_TABLE
{
    int              chroma_qp_table_present_flag;
    int              same_qp_table_for_chroma;
    int              global_offset_flag;
    int              num_points_in_qp_table_minus1[2];
    int              delta_qp_in_val_minus1[2][MAX_QP_TABLE_SIZE];
    int              delta_qp_out_val[2][MAX_QP_TABLE_SIZE];

} EVEY_CHROMA_TABLE;

typedef struct _EVEY_SPS
{
    int              sps_seq_parameter_set_id;
    int              profile_idc;
    int              level_idc;
    int              toolset_idc_h;
    int              toolset_idc_l;
    int              chroma_format_idc;
    u32              pic_width_in_luma_samples;  
    u32              pic_height_in_luma_samples; 
    int              bit_depth_luma_minus8;
    int              bit_depth_chroma_minus8;
    int              sps_btt_flag;
    int              sps_suco_flag;
    int              sps_addb_flag;
    int              sps_alf_flag;
    int              sps_htdf_flag;
    int              sps_admvp_flag;
    int              sps_eipd_flag;
    int              sps_iqt_flag;
    int              sps_cm_init_flag;
    int              sps_rpl_flag;
    int              sps_pocs_flag;
    int              log2_sub_gop_length;
    int              log2_ref_pic_gap_length;
    int              max_num_ref_pics;
    int              picture_cropping_flag;
    int              picture_crop_left_offset;
    int              picture_crop_right_offset;
    int              picture_crop_top_offset;
    int              picture_crop_bottom_offset;
    int              sps_dquant_flag;
    EVEY_CHROMA_TABLE chroma_qp_table_struct;
    u32              ibc_flag;
    int              vui_parameters_present_flag;
    int              sps_dra_flag;
    EVEY_VUI         vui_parameters;

} EVEY_SPS;

/*****************************************************************************
* picture parameter set
*****************************************************************************/
typedef struct _EVEY_PPS
{
    int              pps_pic_parameter_set_id;
    int              pps_seq_parameter_set_id;
    int              num_ref_idx_default_active_minus1[2];
    int              additional_lt_poc_lsb_len;
    int              rpl1_idx_present_flag;
    int              single_tile_in_pic_flag;
    int              num_tile_columns_minus1;
    int              num_tile_rows_minus1;
    int              uniform_tile_spacing_flag;
    int              tile_column_width_minus1[MAX_NUM_TILES_ROW];
    int              tile_row_height_minus1[MAX_NUM_TILES_COL];
    int              tile_offset_lens_minus1;
    int              tile_id_len_minus1;
    int              explicit_tile_id_flag;
    int              tile_id_val[MAX_NUM_TILES_ROW][MAX_NUM_TILES_COL];
    int              arbitrary_slice_present_flag;
    int              constrained_intra_pred_flag;
    int              cu_qp_delta_enabled_flag;
    int              cu_qp_delta_area;
    int              pic_dra_enabled_flag;

} EVEY_PPS;

/*****************************************************************************
 * slice header
 *****************************************************************************/
typedef struct _EVEY_SH
{
    int              slice_pic_parameter_set_id;
    int              first_tile_id;
    int              slice_type;
    int              no_output_of_prior_pics_flag;
    s32              poc_lsb;
    u32              num_ref_idx_active_override_flag;
    int              slice_deblocking_filter_flag;
    int              sh_deblock_alpha_offset;
    int              sh_deblock_beta_offset;
    int              qp;
    int              qp_u;
    int              qp_v;
    int              qp_u_offset;
    int              qp_v_offset;
    u8               qp_prev_eco; /*QP of previous cu in decoding order (used for dqp)*/
    u8               dqp;
    u8               qp_prev_mode;

} EVEY_SH;

/*****************************************************************************
* POC struct
******************************************************************************/
typedef struct _EVEY_POC
{
    /* current picture order count value */
    int              poc_val;
    /* the picture order count of the previous Tid0 picture */
    u32              prev_poc_val;
    /* the decoding order count of the previous picture */
    int              prev_doc_offset;

} EVEY_POC;

/*****************************************************************************
 * user data types
 *****************************************************************************/
#define EVEY_UD_PIC_SIGNATURE              0x10
#define EVEY_UD_END                        0xFF

/* split mode */
typedef enum _EVEY_SPLIT_MODE
{
    NO_SPLIT,
    SPLIT_QUAD,
    NUM_SPLIT_MODE

} EVEY_SPLIT_MODE;

/* block shape */
typedef enum _EVEY_BLOCK_SHAPE
{
    SQUARE,
    NUM_BLOCK_SHAPE,

} EVEY_BLOCK_SHAPE;

enum EVEY_TQC_RUN {
    RUN_L  = 1,
    RUN_CB = 2,
    RUN_CR = 4
};

typedef struct _EVEY_CORE
{
    /* log2 of CU width */
    u8               log2_cuw;
    /* log2 of CU height */
    u8               log2_cuh;
    /* CU position in current frame in SCU unit */
    u32              scup;
    /* CU position X in a frame in SCU unit */
    u16              x_scu;
    /* CU position Y in a frame in SCU unit */
    u16              y_scu;
    /* prediction mode of current CU: INTRA, INTER, ... */
    u8               pred_mode;
    /* QP for current CU. Used to derive Luma and chroma qp */
    u8               qp;
    /* QP for Luma of current CU */
    u8               qp_y;
    /* QP for Chroma of current CU */
    u8               qp_u;
    u8               qp_v;
    /* coefficient buffer of current CU */
    s16              coef[N_C][MAX_CU_DIM];
    s16              coef_temp[N_C][MAX_CU_DIM];
    /* pred buffer of current CU */
    /* [1] is used for bi-pred. */
    pel              pred[2][N_C][MAX_CU_DIM];
    /* neighbor pixel buffer for intra prediction */
    pel              nb[N_C][INTRA_REF_NUM][INTRA_REF_SIZE];
    /* intra prediction direction of current CU */
    u8               ipm[2];
    /* most probable mode for intra prediction */
    u8             * mpm_b_list;
    /* neighbor CUs availability of current CU */
    u16              avail_cu;
    /* number of non-zero coefficient */
    int              nnz[N_C];
    int              nnz_sub[N_C][MAX_SUB_TB_NUM];
    /* address of current CTU  */
    u16              ctu_num;
    /* X address of current CTU */
    u16              x_ctu;
    /* Y address of current CTU */
    u16              y_ctu;
    /* left pel position of current CTU */
    u16              x_pel;
    /* top pel position of current CTU */
    u16              y_pel;

} EVEY_CORE;

typedef struct _EVEY_CTX
{
    /* magic code */
    u32              magic;
    /* current nalu header */
    EVEY_NALU        nalu;
    /* sequence parameter set */
    EVEY_SPS         sps;
    /* picture parameter set */
    EVEY_PPS         pps;
    EVEY_PPS         pps_array[64];
    /* slice header */
    EVEY_SH          sh;
    /* decoded picture buffer (DPB) management */
    EVEY_PM          dpbm;
    /* picture width */
    u16              w;
    /* picture height */
    u16              h;
    /* CTU size (i.e. maximum CU width and height) */
    u16              ctu_size;
    /* log2 of CTU size */
    u8               log2_ctu_size;
    /* minimum CU width and height */
    u16              min_cu_size;
    /* log2 of minimum CU width and height */
    u8               log2_min_cu_size;
    /* picture width in CTU unit */
    u16              w_ctu;
    /* picture height in CTU unit */
    u16              h_ctu;
    /* picture size in CTU unit (= w_ctu * h_ctu) */
    u32              f_ctu;
    /* total count of remained CTUs for one picture */
    u32              ctu_cnt;
    /* picture width in SCU unit */
    u16              w_scu;
    /* picture height in SCU unit */
    u16              h_scu;
    /* picture size in SCU unit (= w_scu * h_scu) */
    u32              f_scu;
    /* the picture order count value */
    EVEY_POC         poc;
    /* the decoding order count of the previous picture */
    u32              prev_doc_offset;
    /* number of coded pictures counted */
    u32              pic_cnt;
    /* last coded intra picture's picture order count */
    int              last_intra_poc;
    /* current slice number.
    when starting a slice for new picture, it is set to zero. */
    u16              slice_num;
    /* flag whether current picture is refecened picture or not */
    u8               slice_ref_flag;
    /* distance between ref pics in addition to closest ref ref pic in LD*/
    int              ref_pic_gap_length;
    /* reference picture (0: foward, 1: backward) */
    EVEY_REFP        refp[MAX_NUM_REF_PICS][LIST_NUM];
    /* current picture buffer */
    EVEY_PIC       * pic;
    /* map for CU information */
    u32            * map_scu;
    /* map for motion vectors in SCU */
    s16           (* map_mv)[LIST_NUM][MV_D];
    /* map for reference indices */
    s8            (* map_refi)[LIST_NUM];
    /* map for intra pred mode */
    s8             * map_ipm;
    /* map for CTU split information */
    s8            (* map_split)[NUM_CU_DEPTH][NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_CTU];
    /* map for CU mode */
    u8             * map_pred_mode;

} EVEY_CTX;


#include "evey_tbl.h"
#include "evey_util.h"
#include "evey_recon.h"
#include "evey_intra.h"
#include "evey_inter.h"
#include "evey_itdq.h"
#include "evey_picman.h"

#endif /* _EVEY_DEF_H_ */
