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

#include "evey.h"
#include "eveya_util.h"
#include "eveya_args.h"
// XXNN
#include "eveye_networking.h"

#define VERBOSE_NONE               VERBOSE_0
#define VERBOSE_FRAME              VERBOSE_1
#define VERBOSE_ALL                VERBOSE_2

#define MAX_BS_BUF                 (16*1024*1024)

typedef enum _STATES
{
    STATE_ENCODING,
    STATE_BUMPING,
    STATE_SKIPPING

} STATES;

static char op_fname_cfg[256]                     = "\0"; /* config file path name */
static char op_fname_inp[256]                     = "\0"; /* input original video */
static char op_fname_out[256]                     = "\0"; /* output bitstream */
static char op_fname_rec[256]                     = "\0"; /* reconstructed video */
static int  op_max_frm_num                        = 0;
static int  op_use_pic_signature                  = 0;
static int  op_w                                  = 0;
static int  op_h                                  = 0;
static int  op_qp                                 = 0;
static int  op_fps                                = 0;
static int  op_iperiod                            = 0;
static int  op_max_b_frames                       = 0;
static int  op_max_num_ref_pics                   = 0;
static int  op_ref_pic_gap_length                 = 0;
static int  op_closed_gop                         = 0;
static int  op_disable_hgop                       = 0;
static int  op_in_bit_depth                       = 8;
static int  op_skip_frames                        = 0;
static int  op_out_bit_depth                      = 0; /* same as input bit depth */
static int  op_codec_bit_depth                    = 10;
static int  op_chroma_format_idc                  = 1; /* 0: 4:0:0 format, 1: 4:2:0 format */
static int  op_profile                            = 0; /* 0: Video profile, 2: Still picture profile */
static int  op_level                              = 0;
static int  op_toolset_idc_h                      = 0;
static int  op_toolset_idc_l                      = 0;
static int  op_add_qp_frames                      = 0;
static int  op_cb_qp_offset                       = 0;
static int  op_cr_qp_offset                       = 0;
static int  op_constrained_intra_pred             = 0; /* default off */
static int  op_tool_deblocking                    = 1; /* default on */
static int  op_chroma_qp_table_present_flag       = 0;
static char op_chroma_qp_num_points_in_table[256] = { 0 };
static char op_chroma_qp_delta_in_val_cb[256]     = { 0 };
static char op_chroma_qp_delta_out_val_cb[256]    = { 0 };
static char op_chroma_qp_delta_in_val_cr[256]     = { 0 };
static char op_chroma_qp_delta_out_val_cr[256]    = { 0 };
static int  op_use_dqp                            = 0;  /* default cu_delta_qp is off */
static int  op_inter_slice_type                   = 0;
static int  op_picture_cropping_flag              = 0;
static int  op_picture_crop_left_offset           = 0;
static int  op_picture_crop_right_offset          = 0;
static int  op_picture_crop_top_offset            = 0;
static int  op_picture_crop_bottom_offset         = 0;
static int  op_rdo_dbk_switch                     = 1;
static int  op_use_rdoq                           = 1;
static int  op_nn_base_port                       = 0;

typedef enum _OP_FLAGS
{
    OP_FLAG_FNAME_CFG,
    OP_FLAG_FNAME_INP,
    OP_FLAG_FNAME_OUT,
    OP_FLAG_FNAME_REC,
    OP_FLAG_WIDTH_INP,
    OP_FLAG_HEIGHT_INP,
    OP_FLAG_QP,
    OP_FLAG_USE_DQP,
    OP_FLAG_FPS,
    OP_FLAG_IPERIOD,
    OP_FLAG_MAX_FRM_NUM,
    OP_FLAG_USE_PIC_SIGN,
    OP_FLAG_VERBOSE,
    OP_FLAG_MAX_B_FRAMES,
    OP_FLAG_MAX_NUM_REF_PICS,
    OP_FLAG_CLOSED_GOP,
    OP_FLAG_DISABLE_HGOP,
    OP_FLAG_OUT_BIT_DEPTH,
    OP_FLAG_IN_BIT_DEPTH,
    OP_FLAG_CODEC_BIT_DEPTH,
    OP_FLAG_CHROMA_FORMAT,
    OP_FLAG_SKIP_FRAMES,
    OP_PROFILE,
    OP_LEVEL,
    OP_TOOLSET_IDC_H,
    OP_TOOLSET_IDC_L,
    OP_FLAG_ADD_QP_FRAME,
    OP_CB_QP_OFFSET,
    OP_CR_QP_OFFSET,
    OP_CONSTRAINED_INTRA_PRED,
    OP_TOOL_DBF,
    OP_CHROMA_QP_TABLE_PRESENT_FLAG,
    OP_CHROMA_QP_NUM_POINTS_IN_TABLE,
    OP_CHROMA_QP_DELTA_IN_VAL_CB,
    OP_CHROMA_QP_DELTA_OUT_VAL_CB,
    OP_CHROMA_QP_DELTA_IN_VAL_CR,
    OP_CHROMA_QP_DELTA_OUT_VAL_CR,
    OP_INTER_SLICE_TYPE,
    OP_PIC_CROP_FLAG,
    OP_PIC_CROP_LEFT,
    OP_PIC_CROP_RIGHT,
    OP_PIC_CROP_TOP,
    OP_PIC_CROP_BOTTOM,
    OP_FLAG_RDO_DBK_SWITCH,
    OP_FLAG_USE_RDOQ,
    OP_FLAG_MAX,
    OP_NN_BASE_PORT

} OP_FLAGS;

static int op_flag[OP_FLAG_MAX] = {0};

static EVEY_ARGS_OPTION options[] = \
{
    {
        EVEY_ARGS_NO_KEY, EVEY_ARGS_KEY_LONG_CONFIG, EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_CFG], op_fname_cfg,
        "file name of configuration"
    },
    {
        'i', "input", EVEY_ARGS_VAL_TYPE_STRING|EVEY_ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FNAME_INP], op_fname_inp,
        "file name of input video"
    },
    {
        'o', "output", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_OUT], op_fname_out,
        "file name of output bitstream"
    },
    {
        'r', "recon", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_REC], op_fname_rec,
        "file name of reconstructed video"
    },
    {
        'w',  "width", EVEY_ARGS_VAL_TYPE_INTEGER|EVEY_ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_WIDTH_INP], &op_w,
        "pixel width of input video"
    },
    {
        'h',  "height", EVEY_ARGS_VAL_TYPE_INTEGER|EVEY_ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_HEIGHT_INP], &op_h,
        "pixel height of input video"
    },
    {
        'q',  "op_qp", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_QP], &op_qp,
        "QP value (0~51)"
    },
    {
         EVEY_ARGS_NO_KEY,  "use_dqp", EVEY_ARGS_VAL_TYPE_INTEGER,
         &op_flag[OP_FLAG_USE_DQP], &op_use_dqp,
         "use_dqp ({0,..,25})(default: 0) "
    },
    {
        'z',  "hz", EVEY_ARGS_VAL_TYPE_INTEGER|EVEY_ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FPS], &op_fps,
        "frame rate (Hz)"
    },
    {
        'p',  "iperiod", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IPERIOD], &op_iperiod,
        "I-picture period"
    },
    {
        'g',  "max_b_frames", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_B_FRAMES], &op_max_b_frames,
        "Number of maximum B frames (1,3,7,15)\n"
    },
    {
        EVEY_ARGS_NO_KEY,  "max_num_ref_pics", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_NUM_REF_PICS], &op_max_num_ref_pics,
        "Number of maximum reference picture\n"
    },
    {
        'f',  "frames", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_FRM_NUM], &op_max_frm_num,
        "maximum number of frames to be encoded"
    },
    {
        's',  "signature", EVEY_ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_USE_PIC_SIGN], &op_use_pic_signature,
        "embed picture signature (HASH) for conformance checking in decoding"
    },
    {
        'v',  "verbose", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_VERBOSE], &op_verbose,
        "verbose level\n"
        "\t 0: no message\n"
        "\t 1: frame-level messages (default)\n"
        "\t 2: all messages\n"
    },
    {
        'd',  "input_bit_depth", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IN_BIT_DEPTH], &op_in_bit_depth,
        "input bitdepth (8(default), 10) "
    },
    {
        EVEY_ARGS_NO_KEY,  "codec_bit_depth", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CODEC_BIT_DEPTH], &op_codec_bit_depth,
        "codec internal bitdepth (10(default), 8, 12, 14) "
    },
    {
        EVEY_ARGS_NO_KEY,  "chroma_format", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CHROMA_FORMAT], &op_chroma_format_idc,
        "chroma format (1 (default), 0(400), 1(420), 2(422), 3(444)) "
    },
    {
        EVEY_ARGS_NO_KEY,  "output_bit_depth", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], &op_out_bit_depth,
        "output bitdepth (8, 10)(default: same as input bitdpeth) "
    },
    {
        EVEY_ARGS_NO_KEY,  "ref_pic_gap_length", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], &op_ref_pic_gap_length,
        "reference picture gap length (1, 2, 4, 8, 16) only available when -g is 0"
    },
    {
        EVEY_ARGS_NO_KEY,  "closed_gop", EVEY_ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_CLOSED_GOP], &op_closed_gop,
        "use closed GOP structure. if not set, open GOP is used"
    },
    {
        EVEY_ARGS_NO_KEY,  "disable_hgop", EVEY_ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_DISABLE_HGOP], &op_disable_hgop,
        "disable hierarchical GOP. if not set, hierarchical GOP is used"
    },
    {
        EVEY_ARGS_NO_KEY,  "skip_frames", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_SKIP_FRAMES], &op_skip_frames,
        "number of skipped frames before encoding. default 0"
    },
    {
        EVEY_ARGS_NO_KEY,  "profile", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PROFILE], &op_profile,
        "profile setting flag  0: video profile, 2: still picture profile  (default 0) "
    },
    {
        EVEY_ARGS_NO_KEY,  "level", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_LEVEL], &op_level,
        "level setting "
    },
    {
        'a',  "qp_add_frm", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_ADD_QP_FRAME], &op_add_qp_frames,
        "one more qp are added after this number of frames, disable:0 (default)"
    },
    {
        EVEY_ARGS_NO_KEY,  "cb_qp_offset", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CB_QP_OFFSET], &op_cb_qp_offset,
        "cb qp offset"
    },
    {
        EVEY_ARGS_NO_KEY,  "cr_qp_offset", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CR_QP_OFFSET], &op_cr_qp_offset,
        "cr qp offset"
    },
    {
        EVEY_ARGS_NO_KEY,  "toolset_idc_h", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOLSET_IDC_H], &op_toolset_idc_h,
        "toolset idc h"
    },
    {
        EVEY_ARGS_NO_KEY,  "toolset_idc_l", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOLSET_IDC_L], &op_toolset_idc_l,
        "toolset idc l"
    },
    {
        EVEY_ARGS_NO_KEY,  "constrained_intra_pred", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CONSTRAINED_INTRA_PRED], &op_constrained_intra_pred,
        "constrained intra pred"
    },
    {
        EVEY_ARGS_NO_KEY,  "dbf", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DBF], &op_tool_deblocking,
        "Deblocking filter on/off flag"
    },
    {
        EVEY_ARGS_NO_KEY,  "chroma_qp_table_present_flag", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CHROMA_QP_TABLE_PRESENT_FLAG], &op_chroma_qp_table_present_flag,
        "chroma_qp_table_present_flag"
    },
    {
        EVEY_ARGS_NO_KEY,  "chroma_qp_num_points_in_table", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_NUM_POINTS_IN_TABLE], &op_chroma_qp_num_points_in_table,
        "Number of pivot points for Cb and Cr channels"
    },
    {
        EVEY_ARGS_NO_KEY,  "chroma_qp_delta_in_val_cb", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_IN_VAL_CB], &op_chroma_qp_delta_in_val_cb,
        "Array of input pivot points for Cb"
    },
    {
        EVEY_ARGS_NO_KEY,  "chroma_qp_delta_out_val_cb", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_OUT_VAL_CB], &op_chroma_qp_delta_out_val_cb,
        "Array of input pivot points for Cb"
    },
    {
        EVEY_ARGS_NO_KEY,  "chroma_qp_delta_in_val_cr", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_IN_VAL_CR], &op_chroma_qp_delta_in_val_cr,
        "Array of input pivot points for Cr"
    },
    {
        EVEY_ARGS_NO_KEY,  "chroma_qp_delta_out_val_cr", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_OUT_VAL_CR], &op_chroma_qp_delta_out_val_cr,
        "Array of input pivot points for Cr"
    },
    {
        EVEY_ARGS_NO_KEY,  "inter_slice_type", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_INTER_SLICE_TYPE], &op_inter_slice_type,
        "INTER_SLICE_TYPE"
    },
    {
        EVEY_ARGS_NO_KEY,  "picture_cropping_flag", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_FLAG], &op_picture_cropping_flag,
        "INTER_SLICE_TYPE"
    },
    {
        EVEY_ARGS_NO_KEY,  "picture_crop_left", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_LEFT], &op_picture_crop_left_offset,
        "INTER_SLICE_TYPE"
    },
    {
        EVEY_ARGS_NO_KEY,  "picture_crop_right", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_RIGHT], &op_picture_crop_right_offset,
        "INTER_SLICE_TYPE"
    },
    {
        EVEY_ARGS_NO_KEY,  "picture_crop_top", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_TOP], &op_picture_crop_top_offset,
        "INTER_SLICE_TYPE"
    },
    {
        EVEY_ARGS_NO_KEY,  "picture_crop_bottom", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_BOTTOM], &op_picture_crop_bottom_offset,
        "INTER_SLICE_TYPE"
    },
    {
        EVEY_ARGS_NO_KEY,  "rdo_dbk_switch", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_RDO_DBK_SWITCH], &op_rdo_dbk_switch,
        "switch to on/off rdo_dbk (1(default), 0) "
    },
    {
        EVEY_ARGS_NO_KEY,  "use_rdoq", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_USE_RDOQ], &op_use_rdoq,
        "switch to on/off RDOQ (1(default), 0) "
    },
    {
        EVEY_ARGS_NO_KEY,  "nn_base_port", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NN_BASE_PORT], &op_nn_base_port,
        "base port at 127.0.0.1 where the NN server is listening (0(default) means no server is listening) "
    },
    {0, "", EVEY_ARGS_VAL_TYPE_NONE, NULL, NULL, ""} /* termination */
};

#define NUM_ARG_OPTION   ((int)(sizeof(options)/sizeof(options[0]))-1)
static void print_usage(void)
{
    int i;
    char str[1024];

    logv0("< Usage >\n");

    for(i = 0; i < NUM_ARG_OPTION; i++)
    {
        if(evey_args_get_help(options, i, str) < 0) return;
        logv0("%s\n", str);
    }
}

static char get_pic_type(char * in)
{
    int len = (int)strlen(in);
    char type = 0;
    for (int i = 0; i < len; i++)
    {
        if (in[i] == 'P')
        {
            type = 'P';
            break;
        }
        else if (in[i] == 'B')
        {
            type = 'B';
            break;
        }
    }

    if (type == 0)
    {
        return 0;
    }

    return type;
}

static int get_conf(EVEYE_CDSC * cdsc)
{
    int result = 0;
    memset(cdsc, 0, sizeof(EVEYE_CDSC));

    cdsc->w = op_w;
    cdsc->h = op_h;
    cdsc->qp = op_qp;
    cdsc->fps = op_fps;
    cdsc->iperiod = op_iperiod;
    cdsc->max_b_frames = op_max_b_frames;
    cdsc->max_num_ref_pics = op_max_num_ref_pics;
    cdsc->profile = op_profile;
    cdsc->level = op_level;
    cdsc->toolset_idc_h = op_toolset_idc_h;
    cdsc->toolset_idc_l = op_toolset_idc_l;
    cdsc->use_dqp = op_use_dqp;
    cdsc->ref_pic_gap_length = op_ref_pic_gap_length;
    cdsc->add_qp_frame = op_add_qp_frames;
    if(op_disable_hgop)
    {
        cdsc->disable_hgop = 1;
    }
    if(op_closed_gop)
    {
        cdsc->closed_gop = 1;
    }
    cdsc->in_bit_depth = op_in_bit_depth;
    if(op_out_bit_depth == 0)
    {
        op_out_bit_depth = op_codec_bit_depth;
    }
    cdsc->codec_bit_depth = op_codec_bit_depth;
    cdsc->out_bit_depth = op_out_bit_depth;
    cdsc->chroma_format_idc  = op_chroma_format_idc;
    cdsc->cb_qp_offset = op_cb_qp_offset;
    cdsc->cr_qp_offset = op_cr_qp_offset;
    cdsc->constrained_intra_pred = op_constrained_intra_pred;
    cdsc->use_deblock = op_tool_deblocking;
    cdsc->inter_slice_type = op_inter_slice_type == 0 ? EVEY_ST_B : EVEY_ST_P;
    cdsc->picture_cropping_flag = op_picture_cropping_flag;
    cdsc->picture_crop_left_offset = op_picture_crop_left_offset;
    cdsc->picture_crop_right_offset = op_picture_crop_right_offset;
    cdsc->picture_crop_top_offset = op_picture_crop_top_offset;
    cdsc->picture_crop_bottom_offset = op_picture_crop_bottom_offset;
    cdsc->rdo_dbk_switch = op_rdo_dbk_switch;
    cdsc->use_rdoq = op_use_rdoq;
    cdsc->nn_base_port = op_nn_base_port;
    cdsc->chroma_qp_table_present_flag = op_chroma_qp_table_present_flag;
    if (cdsc->chroma_qp_table_present_flag)
    {
        cdsc->num_points_in_qp_table_minus1[0] = atoi(strtok(op_chroma_qp_num_points_in_table, " ")) - 1;
        cdsc->num_points_in_qp_table_minus1[1] = atoi( strtok(NULL, " \r") ) - 1;

        { // input pivot points
            cdsc->delta_qp_in_val_minus1[0][0] = atoi(strtok(op_chroma_qp_delta_in_val_cb, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc->delta_qp_in_val_minus1[0][j++] = atoi(val);
            } while (1);
            assert(cdsc->num_points_in_qp_table_minus1[0] + 1 == j);

            cdsc->delta_qp_in_val_minus1[1][0] = atoi(strtok(op_chroma_qp_delta_in_val_cr, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc->delta_qp_in_val_minus1[1][j++] = atoi(val);
            } while (1);
            assert(cdsc->num_points_in_qp_table_minus1[1] + 1 == j);
        }
        { // output pivot points
            cdsc->delta_qp_out_val[0][0] = atoi(strtok(op_chroma_qp_delta_out_val_cb, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc->delta_qp_out_val[0][j++] = atoi(val);
            } while (1);
            assert(cdsc->num_points_in_qp_table_minus1[0] + 1 == j);

            cdsc->delta_qp_out_val[1][0] = atoi(strtok(op_chroma_qp_delta_out_val_cr, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc->delta_qp_out_val[1][j++] = atoi(val);
            } while (1);
            assert(cdsc->num_points_in_qp_table_minus1[1] + 1 == j);
        }
    }

    return 0;
}

static void print_enc_conf(EVEYE_CDSC * cdsc)
{
    logv1("Input bit-depth: %d \n", cdsc->in_bit_depth);
    logv1("Codec bit-depth: %d \n", cdsc->codec_bit_depth);
    logv1("Output bit-depth: %d \n", cdsc->out_bit_depth);
    logv1("Chroma format idc: %d \n", cdsc->chroma_format_idc);
    logv1("Constrained intra prediction: %d \n", cdsc->constrained_intra_pred);
    logv1("Deblocking filter: %d \n", cdsc->use_deblock);    
    logv1("Chroma QP Table: %d ", cdsc->chroma_qp_table_present_flag);    
    logv1("\n");
}

int check_conf(EVEYE_CDSC * cdsc)
{
    int success = 1;
    int min_block_size = 4;
    if(cdsc->profile == 0 /*PROFILE_VIDEO*/ || cdsc->profile == 2 /*PROFILE_STILL_PICTURE*/)
    {
    }
    else
    {
        assert(0); /* must not happen */
    }

    int pic_m = (8 > min_block_size) ? min_block_size : 8;
    if((cdsc->w & (pic_m - 1)) != 0)
    {
        logv0("Current encoder does not support picture width, not multiple of max(8, minimum CU size)\n"); success = 0;
    }
    if((cdsc->h & (pic_m - 1)) != 0)
    {
        logv0("Current encoder does not support picture height, not multiple of max(8, minimum CU size)\n"); success = 0;
    }

    return success;
}

static int set_extra_config(EVEYE id)
{
    int  ret, size, value;

    if(op_use_pic_signature)
    {
        value = 1;
        size = 4;
        ret = eveye_config(id, EVEYE_CFG_SET_USE_PIC_SIGNATURE, &value, &size);
        if(EVEY_FAILED(ret))
        {
            logv0("failed to set config for picture signature\n");
            return -1;
        }
    }
    return 0;
}

static void print_stat_init(void)
{
    if(op_verbose < VERBOSE_FRAME) return;

    logv1("---------------------------------------------------------------------------------------\n");
    logv1("  Input YUV file          : %s \n", op_fname_inp);
    if(op_flag[OP_FLAG_FNAME_OUT])
    {
        logv1("  Output EVEY bitstream    : %s \n", op_fname_out);
    }
    if(op_flag[OP_FLAG_FNAME_REC])
    {
        logv1("  Output YUV file         : %s \n", op_fname_rec);
    }
    logv1("---------------------------------------------------------------------------------------\n");
    logv1("POC   Tid   Ftype   QP   PSNR-Y    PSNR-U    PSNR-V    Bits      EncT(ms)  ");
    logv1("Ref. List\n");
    logv1("---------------------------------------------------------------------------------------\n");
}

static void print_config(EVEYE id)
{
    int s, v;

    if(op_verbose < VERBOSE_ALL) return;

    logv1("---------------------------------------------------------------------------------------\n");
    logv1("< Configurations >\n");
    if(op_flag[OP_FLAG_FNAME_CFG])
    {
        logv1("\tconfig file name         = %s\n", op_fname_cfg);
    }
    s = sizeof(int);
    eveye_config(id, EVEYE_CFG_GET_WIDTH, (void *)(&v), &s);
    logv1("\twidth                    = %d\n", v);
    eveye_config(id, EVEYE_CFG_GET_HEIGHT, (void *)(&v), &s);
    logv1("\theight                   = %d\n", v);
    eveye_config(id, EVEYE_CFG_GET_FPS, (void *)(&v), &s);
    logv1("\tFPS                      = %d\n", v);
    eveye_config(id, EVEYE_CFG_GET_I_PERIOD, (void *)(&v), &s);
    logv1("\tintra picture period     = %d\n", v);
    eveye_config(id, EVEYE_CFG_GET_QP, (void *)(&v), &s);
    logv1("\tQP                       = %d\n", v);
    logv1("\tframes                   = %d\n", op_max_frm_num);
    eveye_config(id, EVEYE_CFG_GET_USE_DEBLOCK, (void *)(&v), &s);
    logv1("\tdeblocking filter        = %s\n", v? "enabled": "disabled");
    eveye_config(id, EVEYE_CFG_GET_CLOSED_GOP, (void *)(&v), &s);
    logv1("\tGOP type                 = %s\n", v? "closed": "open");
    eveye_config(id, EVEYE_CFG_GET_HIERARCHICAL_GOP, (void *)(&v), &s);
    logv1("\thierarchical GOP         = %s\n", v? "enabled": "disabled");
}

static int write_rec(IMGB_LIST * list, EVEY_MTIME * ts)
{
    int i;

    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        if(list[i].ts == (*ts) && list[i].used == 1)
        {
            if(op_flag[OP_FLAG_FNAME_REC])
            {
                if(imgb_write(op_fname_rec, list[i].imgb))
                {
                    logv0("cannot write reconstruction image\n");
                    return -1;
                }
            }
            list[i].used = 0;
            (*ts)++;
            break;
        }
    }
    return 0;
}

void print_psnr(EVEYE_STAT * stat, double * psnr, int bitrate, EVEY_CLK clk_end)
{
    char  stype;
    int i, j;
    switch(stat->stype)
    {
    case EVEY_ST_I :
        stype = 'I';
        break;

    case EVEY_ST_P :
        stype = 'P';
        break;

    case EVEY_ST_B :
        stype = 'B';
        break;

    case EVEY_ST_UNKNOWN :
    default :
        stype = 'U';
        break;
    }

    logv1("%-7d%-5d(%c)     %-5d%-10.4f%-10.4f%-10.4f%-10d%-10d", \
            stat->poc, stat->tid, stype, stat->qp, psnr[0], psnr[1], psnr[2], \
            bitrate, evey_clk_msec(clk_end));

    for(i=0; i < 2; i++)
    {
        logv1("[L%d ", i);
        for(j=0; j < stat->refpic_num[i]; j++) logv1("%d ",stat->refpic[i][j]);
        logv1("] ");
    }

    logv1("\n");

    fflush(stdout);
    fflush(stderr);
}

int setup_bumping(EVEYE id)
{
    int val, size;

    logv1("Entering bumping process... \n");
    val  = 1;
    size = sizeof(int);
    if(EVEY_FAILED(eveye_config(id, EVEYE_CFG_SET_FORCE_OUT, (void *)(&val),
        &size)))
    {
        logv0("failed to fource output\n");
        return -1;
    }
    return 0;
}

int main(int argc, const char **argv)
{
    STATES          state = STATE_ENCODING;
    unsigned char * bs_buf = NULL;
    FILE          * fp_inp = NULL;
    EVEYE           id;
    EVEYE_CDSC      cdsc;
    EVEY_BITB       bitb;
    EVEY_IMGB     * imgb_enc = NULL;
    EVEY_IMGB     * imgb_rec = NULL;
    EVEYE_STAT      stat;
    int             i, ret, size;
    EVEY_CLK        clk_beg, clk_end, clk_tot;
    EVEY_MTIME      pic_icnt, pic_ocnt, pic_skip;
    double          bitrate;
    double          psnr[3] = { 0, };
    double          psnr_avg[3] = { 0, };
    IMGB_LIST       ilist_org[MAX_BUMP_FRM_CNT];
    IMGB_LIST       ilist_rec[MAX_BUMP_FRM_CNT];
    IMGB_LIST     * ilist_t = NULL;
    static int      is_first_enc = 1;

    /* parse options */
    ret = evey_args_parse_all(argc, argv, options);
    if(ret != 0)
    {
        if(ret > 0) logv0("-%c argument should be set\n", ret);
        if(ret < 0) logv0("config error\n");
        print_usage();
        return -1;
    }

    if(op_flag[OP_FLAG_FNAME_OUT])
    {
        /* bitstream file - remove contents and close */
        FILE * fp;
        fp = fopen(op_fname_out, "wb");
        if(fp == NULL)
        {
            logv0("cannot open bitstream file (%s)\n", op_fname_out);
            return -1;
        }
        fclose(fp);
    }

    if(op_flag[OP_FLAG_FNAME_REC])
    {
        /* reconstruction file - remove contents and close */
        FILE * fp;
        fp = fopen(op_fname_rec, "wb");
        if(fp == NULL)
        {
            logv0("cannot open reconstruction file (%s)\n", op_fname_rec);
            return -1;
        }
        fclose(fp);
    }

    /* open original file */
    fp_inp = fopen(op_fname_inp, "rb");
    if(fp_inp == NULL)
    {
        logv0("cannot open original file (%s)\n", op_fname_inp);
        print_usage();
        return -1;
    }

    /* allocate bitstream buffer */
    bs_buf = (unsigned char*)malloc(MAX_BS_BUF);
    if(bs_buf == NULL)
    {
        logv0("cannot allocate bitstream buffer, size=%d", MAX_BS_BUF);
        return -1;
    }

    /* read configurations and set values for create descriptor */
    int val;
    val = get_conf(&cdsc);
    if(val)
    {
        if(val == -1)
        {
            logv0("Number of tiles should be equal or more than number of slices\n");
            print_usage();
            return -1;
        }
        if(val == -2)
        {
            logv0("for DRA internal bit depth should be 10\n");
            print_usage();
            return -1;
        }
    }

    print_enc_conf(&cdsc);
    NN_setupServer(cdsc.nn_base_port);

    if (!check_conf(&cdsc))
    {
        logv0("invalid configuration\n");
        return -1;
    }

    /* create encoder */
    id = eveye_create(&cdsc, NULL);
    if(id == NULL)
    {
        logv0("cannot create EVEY encoder\n");
        return -1;
    }

    if(set_extra_config(id))
    {
        logv0("cannot set extra configurations\n");
        return -1;
    }

    /* create image lists */
    if(imgb_list_alloc(ilist_org, cdsc.w, cdsc.h, op_in_bit_depth, op_chroma_format_idc))
    {
        logv0("cannot allocate image list for original image\n");
        return -1;
    }
    if(imgb_list_alloc(ilist_rec, cdsc.w, cdsc.h, op_out_bit_depth, op_chroma_format_idc))
    {
        logv0("cannot allocate image list for reconstructed image\n");
        return -1;
    }

    print_config(id);
    print_stat_init();

    bitrate = 0;

    bitb.addr = bs_buf;
    bitb.bsize = MAX_BS_BUF;

    if(op_flag[OP_FLAG_SKIP_FRAMES] && op_skip_frames > 0)
    {
        state = STATE_SKIPPING;
    }

    clk_tot = 0;
    pic_icnt = 0;
    pic_ocnt = 0;
    pic_skip = 0;

    /* encode pictures *******************************************************/
    while(1)
    {
        if(state == STATE_SKIPPING)
        {
            if(pic_skip < op_skip_frames)
            {
                ilist_t = imgb_list_get_empty(ilist_org);
                if(ilist_t == NULL)
                {
                    logv0("cannot get empty orignal buffer\n");
                    goto ERR;
                }
                if(imgb_read(fp_inp, ilist_t->imgb))
                {
                    logv2("reached end of original file (or reading error)\n");
                    goto ERR;
                }
            }
            else
            {
                state = STATE_ENCODING;
            }

            pic_skip++;
            continue;
        }

        if(state == STATE_ENCODING)
        {
            ilist_t = imgb_list_get_empty(ilist_org);
            if(ilist_t == NULL)
            {
                logv0("cannot get empty orignal buffer\n");
                return -1;
            }

            /* read original image */
            if(pic_icnt >= op_max_frm_num || imgb_read(fp_inp, ilist_t->imgb))
            {
                logv2("reached end of original file (or reading error)\n");
                state = STATE_BUMPING;
                setup_bumping(id);
                continue;
            }
            imgb_list_make_used(ilist_t, pic_icnt);

            /* push image to encoder */
            ret = eveye_push(id, ilist_t->imgb);
            if(EVEY_FAILED(ret))
            {
                logv0("eveye_push() failed\n");
                return -1;
            }
            pic_icnt++;
        }

        /* encoding */
        clk_beg = evey_clk_get();

        ret = eveye_encode(id, &bitb, &stat);
        if(EVEY_FAILED(ret))
        {
            logv0("eveye_encode() failed\n");
            return -1;
        }

        clk_end = evey_clk_from(clk_beg);
        clk_tot += clk_end;

        /* store bitstream */
        if (ret == EVEY_OK_OUT_NOT_AVAILABLE)
        {
            //logv1("--> RETURN OK BUT PICTURE IS NOT AVAILABLE YET\n");
            continue;
        }
        else if(ret == EVEY_OK)
        {
            if(op_flag[OP_FLAG_FNAME_OUT] && stat.write > 0)
            {
                if(write_data(op_fname_out, bs_buf, stat.write))
                {
                    logv0("cannot write bitstream\n");
                    return -1;
                }
            }

            /* get reconstructed image */
            size = sizeof(EVEY_IMGB**);
            ret = eveye_config(id, EVEYE_CFG_GET_RECON, (void *)&imgb_rec, &size);
            if(EVEY_FAILED(ret))
            {
                logv0("failed to get reconstruction image\n");
                return -1;
            }

            ilist_t = imgb_list_put(ilist_rec, imgb_rec, imgb_rec->ts[0]);
            if(ilist_t == NULL)
            {
                logv0("cannot put reconstructed image to list\n");
                return -1;
            }

            /* calculate PSNR */
            if(cal_psnr(ilist_org, ilist_t->imgb, ilist_t->ts, op_in_bit_depth, op_out_bit_depth, op_chroma_format_idc, psnr))
            {
                logv0("cannot calculate PSNR\n");
                return -1;
            }

            /* store reconstructed image */
            if (write_rec(ilist_rec, &pic_ocnt))
            {
                logv0("cannot write reconstruction image\n");
                return -1;
            }

            if(is_first_enc)
            {
                print_psnr(&stat, psnr, (stat.write - stat.sei_size + (int)bitrate) << 3, clk_end);
                is_first_enc = 0;
            }
            else
            {
                print_psnr(&stat, psnr, (stat.write - stat.sei_size) << 3, clk_end);
            }

            bitrate += (stat.write - stat.sei_size);
            for(i = 0; i < 3; i++)
            {
                psnr_avg[i] += psnr[i];
            }

            /* release recon buffer */
            if (imgb_rec)
            {
                imgb_rec->release(imgb_rec);
            }
        }
        else if (ret == EVEY_OK_NO_MORE_FRM)
        {
            break;
        }
        else
        {
            logv2("invaild return value (%d)\n", ret);
            return -1;
        }

        if(op_flag[OP_FLAG_MAX_FRM_NUM] && pic_icnt >= op_max_frm_num
            && state == STATE_ENCODING)
        {
            state = STATE_BUMPING;
            setup_bumping(id);
        }
    }

    /* store remained reconstructed pictures in output list */
    while(pic_icnt - pic_ocnt > 0)
    {
        write_rec(ilist_rec, &pic_ocnt);
    }
    if(pic_icnt != pic_ocnt)
    {
        logv2("number of input(=%d) and output(=%d) is not matched\n", (int)pic_icnt, (int)pic_ocnt);
    }

    logv1("====================================================================\n");
    psnr_avg[0] /= pic_ocnt;
    psnr_avg[1] /= pic_ocnt;
    psnr_avg[2] /= pic_ocnt;

    logv1("  PSNR Y(dB)       : %-5.4f\n", psnr_avg[0]);
    logv1("  PSNR U(dB)       : %-5.4f\n", psnr_avg[1]);
    logv1("  PSNR V(dB)       : %-5.4f\n", psnr_avg[2]);

    logv1("  Total bits(bits) : %-.0f\n", bitrate*8);
    bitrate *= (cdsc.fps * 8);
    bitrate /= pic_ocnt;
    bitrate /= 1000;
    logv1("  bitrate(kbps)    : %-5.4f\n", bitrate);

    logv1("  Labeles:\t: br,kbps\tPSNR,Y\tPSNR,U\tPSNR,V\t\n");
    logv1("  Summary\t: %-5.4f\t%-5.4f\t%-5.4f\t%-5.4f\n", bitrate, psnr_avg[0], psnr_avg[1], psnr_avg[2]);

    logv1("====================================================================\n");
    logv1("Encoded frame count               = %d\n", (int)pic_ocnt);
    logv1("Total encoding time               = %.3f msec,",
        (float)evey_clk_msec(clk_tot));
    logv1(" %.3f sec\n", (float)(evey_clk_msec(clk_tot)/1000.0));

    logv1("Average encoding time for a frame = %.3f msec\n",
        (float)evey_clk_msec(clk_tot)/pic_ocnt);
    logv1("Average encoding speed            = %.3f frames/sec\n",
        ((float)pic_ocnt * 1000) / ((float)evey_clk_msec(clk_tot)));
    logv1("====================================================================\n");

    if (pic_ocnt != op_max_frm_num)
    {
        logv2("Wrong frames count: should be %d was %d\n", op_max_frm_num, (int)pic_ocnt);
    }

ERR:
    eveye_delete(id);

    imgb_list_free(ilist_org);
    imgb_list_free(ilist_rec);

    if(fp_inp) fclose(fp_inp);
    if(bs_buf) free(bs_buf); /* release bitstream buffer */
    return 0;
}
