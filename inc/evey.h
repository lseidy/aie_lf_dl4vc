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

#ifndef _EVEY_H_
#define _EVEY_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
 * return values and error code
 *****************************************************************************/
/* no more frames, but it is OK */
#define EVEY_OK_NO_MORE_FRM              (205)
/* progress success, but output is not available temporarily */
#define EVEY_OK_OUT_NOT_AVAILABLE        (204)
/* frame dimension (width or height) has been changed */
#define EVEY_OK_DIM_CHANGED              (203)
/* decoding success, but output frame has been delayed */
#define EVEY_OK_FRM_DELAYED              (202)
/* not matched CRC value */
#define EVEY_ERR_BAD_CRC                 (201) 
/* CRC value presented but ignored at decoder*/
#define EVEY_WARN_CRC_IGNORED            (200)
#define EVEY_OK                          (0)
#define EVEY_ERR                         (-1) /* generic error */
#define EVEY_ERR_INVALID_ARGUMENT        (-101)
#define EVEY_ERR_OUT_OF_MEMORY           (-102)
#define EVEY_ERR_REACHED_MAX             (-103)
#define EVEY_ERR_UNSUPPORTED             (-104)
#define EVEY_ERR_UNEXPECTED              (-105)
#define EVEY_ERR_UNSUPPORTED_COLORSPACE  (-201)
#define EVEY_ERR_MALFORMED_BITSTREAM     (-202)
#define EVEY_ERR_UNKNOWN                 (-32767) /* unknown error */

/* return value checking */
#define EVEY_SUCCEEDED(ret)              ((ret) >= EVEY_OK)
#define EVEY_FAILED(ret)                 ((ret) < EVEY_OK)


/*****************************************************************************
 * color spaces
 * - value format = (endian << 14) | (bit-depth << 8) | (color format)
 * - endian (1bit): little endian = 0, big endian = 1
 * - bit-depth (6bit): 0~63
 * - color format (8bit): 0~255
 *****************************************************************************/
/* color formats */
#define EVEY_CF_UNKNOWN                  0 /* unknown color format */
#define EVEY_CF_YCBCR400                 10 /* Y only */
#define EVEY_CF_YCBCR420                 11 /* YCbCr 420 */
#define EVEY_CF_YCBCR422                 12 /* YCBCR 422 narrow chroma*/
#define EVEY_CF_YCBCR444                 13 /* YCBCR 444*/
#define EVEY_CF_YCBCR422N                EVEY_CF_YCBCR422
#define EVEY_CF_YCBCR422W                18 /* YCBCR422 wide chroma */

/* macro for color space */
#define EVEY_CS_GET_FORMAT(cs)           (((cs) >> 0) & 0xFF)
#define EVEY_CS_GET_BIT_DEPTH(cs)        (((cs) >> 8) & 0x3F)
#define EVEY_CS_GET_BYTE_DEPTH(cs)       ((EVEY_CS_GET_BIT_DEPTH(cs)+7)>>3)
#define EVEY_CS_GET_ENDIAN(cs)           (((cs) >> 14) & 0x1)
#define EVEY_CS_SET(f, bit, e)           (((e) << 14) | ((bit) << 8) | (f))
#define EVEY_CS_SET_FORMAT(cs, v)        (((cs) & ~0xFF) | ((v) << 0))
#define EVEY_CS_SET_BIT_DEPTH(cs, v)     (((cs) & ~(0x3F<<8)) | ((v) << 8))
#define EVEY_CS_SET_ENDIAN(cs, v)        (((cs) & ~(0x1<<14)) | ((v) << 14))

/* pre-defined color spaces */
#define EVEY_CS_UNKNOWN                  EVEY_CS_SET(0,0,0)
#define EVEY_CS_YCBCR400                 EVEY_CS_SET(EVEY_CF_YCBCR400, 8, 0)
#define EVEY_CS_YCBCR420                 EVEY_CS_SET(EVEY_CF_YCBCR420, 8, 0)
#define EVEY_CS_YCBCR422                 EVEY_CS_SET(EVEY_CF_YCBCR422, 8, 0)
#define EVEY_CS_YCBCR444                 EVEY_CS_SET(EVEY_CF_YCBCR444, 8, 0)
#define EVEY_CS_YCBCR400_10LE            EVEY_CS_SET(EVEY_CF_YCBCR400, 10, 0)
#define EVEY_CS_YCBCR420_10LE            EVEY_CS_SET(EVEY_CF_YCBCR420, 10, 0)
#define EVEY_CS_YCBCR422_10LE            EVEY_CS_SET(EVEY_CF_YCBCR422, 10, 0)
#define EVEY_CS_YCBCR444_10LE            EVEY_CS_SET(EVEY_CF_YCBCR444, 10, 0)
#define EVEY_CS_YCBCR400_12LE            EVEY_CS_SET(EVEY_CF_YCBCR400, 12, 0)
#define EVEY_CS_YCBCR420_12LE            EVEY_CS_SET(EVEY_CF_YCBCR420, 12, 0)
#define EVEY_CS_YCBCR444_12LE            EVEY_CS_SET(EVEY_CF_YCBCR444, 12, 0)
#define EVEY_CS_YCBCR400_14LE            EVEY_CS_SET(EVEY_CF_YCBCR400, 14, 0)
#define EVEY_CS_YCBCR420_14LE            EVEY_CS_SET(EVEY_CF_YCBCR420, 14, 0)
#define EVEY_CS_YCBCR444_14LE            EVEY_CS_SET(EVEY_CF_YCBCR444, 14, 0)

#define CFI_FROM_CF(cf)     \
    ((cf == EVEY_CF_YCBCR400) ? 0 : \
     (cf == EVEY_CF_YCBCR420) ? 1 : \
     (cf == EVEY_CF_YCBCR422) ? 2 : 3)
#define CF_FROM_CFI(chroma_format_idc)     \
    ((chroma_format_idc == 0) ? EVEY_CF_YCBCR400 : \
     (chroma_format_idc == 1) ? EVEY_CF_YCBCR420 : \
     (chroma_format_idc == 2) ? EVEY_CF_YCBCR422 : EVEY_CF_YCBCR444)
#define GET_CHROMA_W_SHIFT(chroma_format_idc)    \
    ((chroma_format_idc == 0) ? 1 : \
     (chroma_format_idc == 1) ? 1 : \
     (chroma_format_idc == 2) ? 1 : 0)
#define GET_CHROMA_H_SHIFT(chroma_format_idc)    \
    ((chroma_format_idc == 0) ? 1 : \
     (chroma_format_idc == 1) ? 1 : 0)

/*****************************************************************************
 * config types for decoder
 *****************************************************************************/
#define EVEYD_CFG_SET_USE_PIC_SIGNATURE  (301)
#define EVEYD_CFG_SET_USE_OPL_OUTPUT     (302)

/*****************************************************************************
 * config types for encoder
 *****************************************************************************/
#define EVEYE_CFG_SET_COMPLEXITY         (100)
#define EVEYE_CFG_SET_SPEED              (101)
#define EVEYE_CFG_SET_FORCE_OUT          (102)
#define EVEYE_CFG_SET_FINTRA             (200)
#define EVEYE_CFG_SET_QP                 (201)
#define EVEYE_CFG_SET_BPS                (202)
#define EVEYE_CFG_SET_VBV_SIZE           (203)
#define EVEYE_CFG_SET_FPS                (204)
#define EVEYE_CFG_SET_I_PERIOD           (207)
#define EVEYE_CFG_SET_QP_MIN             (208)
#define EVEYE_CFG_SET_QP_MAX             (209)
#define EVEYE_CFG_SET_BU_SIZE            (210)
#define EVEYE_CFG_SET_USE_DEBLOCK        (211)
#define EVEYE_CFG_SET_USE_PIC_SIGNATURE  (301)
#define EVEYE_CFG_GET_COMPLEXITY         (500)
#define EVEYE_CFG_GET_SPEED              (501)
#define EVEYE_CFG_GET_QP_MIN             (600)
#define EVEYE_CFG_GET_QP_MAX             (601)
#define EVEYE_CFG_GET_QP                 (602)
#define EVEYE_CFG_GET_RCT                (603)
#define EVEYE_CFG_GET_BPS                (604)
#define EVEYE_CFG_GET_FPS                (605)
#define EVEYE_CFG_GET_I_PERIOD           (608)
#define EVEYE_CFG_GET_BU_SIZE            (609)
#define EVEYE_CFG_GET_USE_DEBLOCK        (610)
#define EVEYE_CFG_GET_CLOSED_GOP         (611)
#define EVEYE_CFG_GET_HIERARCHICAL_GOP   (612)
#define EVEYE_CFG_GET_WIDTH              (701)
#define EVEYE_CFG_GET_HEIGHT             (702)
#define EVEYE_CFG_GET_RECON              (703)

/*****************************************************************************
 * NALU types
 *****************************************************************************/
#define EVEY_NONIDR_NUT                  (0)
#define EVEY_IDR_NUT                     (1)
#define EVEY_SPS_NUT                     (24)
#define EVEY_PPS_NUT                     (25)
#define EVEY_FD_NUT                      (27)
#define EVEY_SEI_NUT                     (28)

/*****************************************************************************
 * slice type
 *****************************************************************************/
#define EVEY_ST_UNKNOWN                  (-1)
#define EVEY_ST_B                        (0)
#define EVEY_ST_P                        (1)
#define EVEY_ST_I                        (2)

/*****************************************************************************
 * type and macro for media time
 *****************************************************************************/
/* media time in 100-nanosec unit */
typedef long long                    EVEY_MTIME;

/*****************************************************************************
 * image buffer format
 *****************************************************************************
 baddr
    +---------------------------------------------------+ ---
    |                                                   |  ^
    |                                              |    |  |
    |    a                                         v    |  |
    |   --- +-----------------------------------+ ---   |  |
    |    ^  |  (x, y)                           |  y    |  |
    |    |  |   +---------------------------+   + ---   |  |
    |    |  |   |                           |   |  ^    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |       |   |                           |   |       |
    |    ah |   |                           |   |  h    |  e
    |       |   |                           |   |       |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  v    |  |
    |    |  |   +---------------------------+   | ---   |  |
    |    v  |                                   |       |  |
    |   --- +---+-------------------------------+       |  |
    |     ->| x |<----------- w ----------->|           |  |
    |       |<--------------- aw -------------->|       |  |
    |                                                   |  v
    +---------------------------------------------------+ ---

    |<---------------------- s ------------------------>|

 *****************************************************************************/

#define EVEY_IMGB_MAX_PLANE              (4)

typedef struct _EVEY_IMGB EVEY_IMGB;
struct _EVEY_IMGB
{
    int            cs; /* color space */
    int            np; /* number of plane */
    /* width (in unit of pixel) */
    int            w[EVEY_IMGB_MAX_PLANE];
    /* height (in unit of pixel) */
    int            h[EVEY_IMGB_MAX_PLANE];
    /* X position of left top (in unit of pixel) */
    int            x[EVEY_IMGB_MAX_PLANE];
    /* Y postion of left top (in unit of pixel) */
    int            y[EVEY_IMGB_MAX_PLANE];
    /* buffer stride (in unit of byte) */
    int            s[EVEY_IMGB_MAX_PLANE];
    /* buffer elevation (in unit of byte) */
    int            e[EVEY_IMGB_MAX_PLANE];
    /* address of each plane */
    void         * a[EVEY_IMGB_MAX_PLANE];

    /* time-stamps */
    EVEY_MTIME      ts[4];

    int            ndata[4]; /* arbitrary data, if needs */
    void         * pdata[4]; /* arbitrary adedress if needs */

    /* aligned width (in unit of pixel) */
    int            aw[EVEY_IMGB_MAX_PLANE];
    /* aligned height (in unit of pixel) */
    int            ah[EVEY_IMGB_MAX_PLANE];

    /* left padding size (in unit of pixel) */
    int            padl[EVEY_IMGB_MAX_PLANE];
    /* right padding size (in unit of pixel) */
    int            padr[EVEY_IMGB_MAX_PLANE];
    /* up padding size (in unit of pixel) */
    int            padu[EVEY_IMGB_MAX_PLANE];
    /* bottom padding size (in unit of pixel) */
    int            padb[EVEY_IMGB_MAX_PLANE];

    /* address of actual allocated buffer */
    void         * baddr[EVEY_IMGB_MAX_PLANE];
    /* actual allocated buffer size */
    int            bsize[EVEY_IMGB_MAX_PLANE];

    /* life cycle management */
    int            refcnt;
    int            (*addref)(EVEY_IMGB * imgb);
    int            (*getref)(EVEY_IMGB * imgb);
    int            (*release)(EVEY_IMGB * imgb);

    int            imgb_active_pps_id;
};

/*****************************************************************************
 * Bitstream buffer
 *****************************************************************************/
typedef struct _EVEY_BITB
{
    /* user space address indicating buffer */
    void         * addr;
    /* physical address indicating buffer, if any */
    void         * pddr;
    /* byte size of buffer memory */
    int            bsize;
    /* byte size of bitstream in buffer */
    int            ssize;
    /* bitstream has an error? */
    int            err;
    /* arbitrary data, if needs */
    int            ndata[4];
    /* arbitrary address, if needs */
    void         * pdata[4];
    /* time-stamps */
    EVEY_MTIME     ts[4];

} EVEY_BITB;

/*****************************************************************************
 * description for creating of decoder
 *****************************************************************************/
typedef struct _EVEYD_CDSC
{
    int            __na; /* nothing */

} EVEYD_CDSC;

/*****************************************************************************
 * status after decoder operation
 *****************************************************************************/
typedef struct _EVEYD_STAT
{
    /* byte size of decoded bitstream (read size of bitstream) */
    int            read;
    /* nalu type */
    int            nalu_type;
    /* slice type */
    int            stype;
    /* frame number monotonically increased whenever decoding a frame.
    note that it has negative value if the decoded data is not frame */
    int            fnum;
    /* picture order count */
    int            poc;
    /* layer id */
    int            tid;

    /* number of reference pictures */
    unsigned char  refpic_num[2];
    /* list of reference pictures */
    int            refpic[2][16];
} EVEYD_STAT;

typedef struct _EVEYD_OPL
{
    int            poc;
    char           digest[3][16];
} EVEYD_OPL;

#define MAX_NUM_REF_PICS                   21
#define MAX_NUM_ACTIVE_REF_FRAME           5

/*****************************************************************************
 * description for creating of encoder
 *****************************************************************************/
typedef struct _EVEYE_CDSC
{
    /* width of input frame */
    int            w;
    /* height of input frame */
    int            h;
    /* frame rate (Hz) */
    int            fps;
    /* period of I-frame.
    - 0: only one I-frame at the first time.
    - 1: every frame is coded in I-frame
    */
    int            iperiod;
    /* quantization parameter.
    if the rate control is enabled, the value would be ignored */
    int            qp;
    /* color space of input image */
    int            cs;
    /* The maximum number of consecutive B frames (up to 7)
       - Default: Off(0)                                                      */
    int            max_b_frames;
    /* Has meaning only when max_b_frames is more than 0
       - enable(0) means use hierarchy GOP structure for B frames
               is valid only when max_b_frames is equal to 1, 3 and 7
       - disable (1) means frame type will be decided automatically
       - Default: enable(0)                                                       */
    int            max_num_ref_pics;
    /* The value depend on configuration:
        - (2), in case of RA
        - (4), in case of LDB
        - (0), in case of still pic
    */
    int            disable_hgop;
    int            ref_pic_gap_length;
    /* use closed GOP sturcture
       - 0 : use open GOP (default)
       - 1 : use closed GOP */
    int            closed_gop; 
    /* bit depth of input video */
    int            in_bit_depth;
    /* bit depth of output video */
    int            out_bit_depth;
    int            codec_bit_depth;
    int            chroma_format_idc;
    int            profile;
    int            level;
    int            toolset_idc_h;
    int            toolset_idc_l;
    int            add_qp_frame;
    int            cb_qp_offset;
    int            cr_qp_offset;
    int            use_dqp;
    int            constrained_intra_pred;
    int            use_deblock;
    int            inter_slice_type;
    int            picture_cropping_flag;
    int            picture_crop_left_offset;
    int            picture_crop_right_offset;
    int            picture_crop_top_offset;
    int            picture_crop_bottom_offset;
    /* explicit chroma QP mapping table */
    int            chroma_qp_table_present_flag;
    int            same_qp_table_for_chroma;
    int            global_offset_flag;
    int            num_points_in_qp_table_minus1[2];
    int            delta_qp_in_val_minus1[2][58];
    int            delta_qp_out_val[2][58];
    /* RDO w/ dbf */
    int            rdo_dbk_switch;
    /* RDOQ */
    int            use_rdoq;
    int            nn_base_port;

} EVEYE_CDSC;

/*****************************************************************************
 * status after encoder operation
 *****************************************************************************/
typedef struct _EVEYE_STAT
{
    /* encoded bitstream byte size */
    int            write;
    /* encoded sei messages byte size */
    int            sei_size;
    /* picture number increased whenever encoding a frame */
    unsigned long  fnum;
    /* nalu type */
    int            nalu_type;
    /* slice type */
    int            stype;
    /* quantization parameter used for encoding */
    int            qp;
    /* picture order count */
    int            poc;
    /* layer id */
    int            tid;
    /* number of reference pictures */
    int            refpic_num[2];
    /* list of reference pictures */
    int            refpic[2][16];

} EVEYE_STAT;

/*****************************************************************************
 * API for decoder
 *****************************************************************************/
/* EVEY instance identifier for decoder */
typedef void  * EVEYD;

EVEYD eveyd_create(EVEYD_CDSC * cdsc, int * err);
void eveyd_delete(EVEYD id);
int eveyd_decode(EVEYD id, EVEY_BITB * bitb, EVEYD_STAT * stat);
int eveyd_pull(EVEYD id, EVEY_IMGB ** img, EVEYD_OPL * opl);
int eveyd_config(EVEYD id, int cfg, void * buf, int * size);

/*****************************************************************************
 * API for encoder
 *****************************************************************************/
/* EVEY instance identifier for encoder */
typedef void  * EVEYE;

EVEYE eveye_create(EVEYE_CDSC * cdsc, int * err);
void eveye_delete(EVEYE id);
int eveye_push(EVEYE id, EVEY_IMGB * imgb);
int eveye_encode(EVEYE id, EVEY_BITB * bitb, EVEYE_STAT * stat);
int eveye_get_inbuf(EVEYE id, EVEY_IMGB ** imgb);
int eveye_config(EVEYE id, int cfg, void * buf, int * size);

#ifdef __cplusplus
}
#endif

#endif /* _EVEY_H_ */
