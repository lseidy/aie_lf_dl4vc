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

#include "../src/eveyd_def.h"
#include "../src/eveye_eco.h"
#include "../src/eveye_bsw.h"
#include "eveya_util.h"
#include "eveya_args.h"

#define VERBOSE_NONE               VERBOSE_0
#define VERBOSE_FRAME              VERBOSE_1
#define VERBOSE_ALL                VERBOSE_2

#define MAX_BS_BUF                (16*1024*1024) /* byte */

static char op_fname_inp[256] = "\0";
static char op_fname_out[256] = "\0";
static int  op_max_frm_num = 0;
static int  op_use_pic_signature = 0;
static int  op_out_bit_depth = 8;

typedef enum _STATES
{
    STATE_DECODING,
    STATE_BUMPING
} STATES;

typedef enum _OP_FLAGS
{
    OP_FLAG_FNAME_INP,
    OP_FLAG_FNAME_OUT,
    OP_FLAG_MAX_FRM_NUM,
    OP_FLAG_USE_PIC_SIGN,
    OP_FLAG_OUT_BIT_DEPTH,
    OP_FLAG_VERBOSE,
    OP_FLAG_MAX
} OP_FLAGS;

static int op_flag[OP_FLAG_MAX] = { 0 };

static EVEY_ARGS_OPTION options[] = \
{
    {
        'i', "input", EVEY_ARGS_VAL_TYPE_STRING | EVEY_ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FNAME_INP], op_fname_inp,
        "file name of input bitstream"
    },
    {
        'o', "output", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_OUT], op_fname_out,
        "file name of decoded output"
    },
    {
        'f', "frames", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_FRM_NUM], &op_max_frm_num,
        "maximum number of frames to be decoded"
    },
    {
        's', "signature", EVEY_ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_USE_PIC_SIGN], &op_use_pic_signature,
        "conformance check using picture signature (HASH)"
    },
    {
        'v', "verbose", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_VERBOSE], &op_verbose,
        "verbose level\n"
        "\t 0: no message\n"
        "\t 1: frame-level messages (default)\n"
        "\t 2: all messages\n"
    },
    {
        EVEY_ARGS_NO_KEY, "output_bit_depth", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], &op_out_bit_depth,
        "output bitdepth (8(default), 10) "
    },
    { 0, "", EVEY_ARGS_VAL_TYPE_NONE, NULL, NULL, "" } /* termination */
};

#define NUM_ARG_OPTION   ((int)(sizeof(options)/sizeof(options[0]))-1)
static void print_usage(void)
{
    int i;
    char str[1024];

    logv0("< Usage >\n");

    for (i = 0; i<NUM_ARG_OPTION; i++)
    {
        if (evey_args_get_help(options, i, str) < 0) return;
        logv0("%s\n", str);
    }
}

static int read_nalu(FILE * fp, int * pos, unsigned char * bs_buf)
{
    int read_size, bs_size;
    unsigned char b = 0;

    bs_size = 0;
    read_size = 0;

    if (!fseek(fp, *pos, SEEK_SET))
    {
        /* read size first */
        if (4 == fread(&bs_size, 1, 4, fp))
        {
            if (bs_size <= 0)
            {
                logv0("Invalid bitstream size![%d]\n", bs_size);
                return -1;
            }

            while (bs_size)
            {
                /* read byte */
                if (1 != fread(&b, 1, 1, fp))
                {
                    logv0("Cannot read bitstream!\n");
                    return -1;
                }
                bs_buf[read_size] = b;
                read_size++;
                bs_size--;
            }
        }
        else
        {
            if(feof(fp))
            {
                logv2("End of file\n");
            }
            else
            {
                logv0("Cannot read bitstream size!\n");
            }

            return -1;
        }
    }
    else
    {
        logv0("Cannot seek bitstream!\n");
        return -1;
    }

    return read_size;
}

int print_stat(EVEYD_STAT * stat, int ret)
{
    char stype;
    int i, j;

    if (EVEY_SUCCEEDED(ret))
    {
        if (stat->nalu_type < EVEY_SPS_NUT)
        {
            switch (stat->stype)
            {
            case EVEY_ST_I:
                stype = 'I';
                break;

            case EVEY_ST_P:
                stype = 'P';
                break;

            case EVEY_ST_B:
                stype = 'B';
                break;

            case EVEY_ST_UNKNOWN:
            default:
                stype = 'U';
                break;
            }
            logv1("%c-slice", stype);
        }
        else if (stat->nalu_type == EVEY_SPS_NUT)
        {
            logv1("Sequence Parameter Set");
        }
        else
        {
            logv0("Unknown bitstream");
        }
        logv1(" (read=%d, poc=%d) ", stat->read, (int)stat->poc);
        for (i = 0; i < 2; i++)
        {
            logv1("[L%d ", i);
            for (j = 0; j < stat->refpic_num[i]; j++) logv1("%d ", stat->refpic[i][j]);
            logv1("] ");
        }

        if (ret == EVEY_OK)
        {
            logv1("\n");
        }
        else if (ret == EVEY_OK_FRM_DELAYED)
        {
            logv1("->Frame delayed\n");
        }
        else if (ret == EVEY_OK_DIM_CHANGED)
        {
            logv1("->Resolution changed\n");
        }
        else
        {
            logv1("->Unknown OK code = %d\n", ret);
        }
    }
    else
    {
        logv0("Decoding error = %d\n", ret);
    }
    return 0;
}

static int set_extra_config(EVEYD id)
{
    int  ret, size, value;

    if (op_use_pic_signature)
    {
        value = 1;
        size = 4;
        ret = eveyd_config(id, EVEYD_CFG_SET_USE_PIC_SIGNATURE, &value, &size);
        if (EVEY_FAILED(ret))
        {
            logv0("failed to set config for picture signature\n");
            return -1;
        }
    }
    return 0;
}

/* convert EVEYD into EVEYD_CTX with return value if assert on */
#define EVEYD_ID_TO_CTX_RV(id, ctx, ret) \
    evey_assert_rv((id), (ret)); \
    (ctx) = (EVEYD_CTX *)id; \
    evey_assert_rv((ctx)->magic == EVEYD_MAGIC_CODE, (ret));

int main(int argc, const char **argv)
{
    unsigned char * bs_buf = NULL;
    EVEYD           id = NULL;
    EVEYD_CDSC      cdsc;
    EVEY_BITB       bitb;
    /*temporal buffer for video bit depth less than 10bit */
    EVEY_IMGB     * imgb_t = NULL;
    int             ret;
    int             bs_size, bs_read_pos = 0;
    FILE          * fp_bs = NULL;
    FILE          * fp_bs_write = NULL;
    int             bs_num, max_bs_num;
    u8              tmp_size[4];
    int             bs_end_pos;
    int             intra_dist[2];
    int             intra_dist_idx = 0;
    EVEYD_BSR     * bsr;
    EVEYE_BSW       bsw;
    EVEYE_SBAC      sbac_enc;
    EVEY_SPS      * sps;
    EVEY_PPS      * pps;
    EVEY_SH       * sh;
    EVEY_NALU     * nalu;
    EVEYD_CTX     * ctx;

    // set line buffering (_IOLBF) for stdout to prevent incomplete logs when app crashed.
    setvbuf(stdout, NULL, _IOLBF, 1024);
    
    max_bs_num = argc - 2;
    fp_bs_write = fopen(argv[max_bs_num + 1], "wb");

    for (bs_num = 0; bs_num < max_bs_num; bs_num++)
    {
        fp_bs = fopen(argv[bs_num + 1], "rb");
        if (fp_bs == NULL)
        {
            logv0("ERROR: cannot open bitstream file = %s\n", op_fname_inp);
            print_usage();
            return -1;
        }

        fseek(fp_bs, 0, SEEK_END);
        bs_end_pos = ftell(fp_bs);
        intra_dist_idx = 0;
        if (bs_num) intra_dist[0] += intra_dist[1];
        
        bs_buf = malloc(MAX_BS_BUF);
        if (bs_buf == NULL)
        {
            logv0("ERROR: cannot allocate bit buffer, size=%d\n", MAX_BS_BUF);
            return -1;
        }
        id = eveyd_create(&cdsc, NULL);
        if (id == NULL)
        {
            logv0("ERROR: cannot create EVEY decoder\n");
            return -1;
        }
        if (set_extra_config(id))
        {
            logv0("ERROR: cannot set extra configurations\n");
            return -1;
        }

        bs_read_pos = 0;

        do
        {
            bs_size = read_nalu(fp_bs, &bs_read_pos, bs_buf);

            tmp_size[0] = (bs_size & 0x000000ff) >> 0;
            tmp_size[1] = (bs_size & 0x0000ff00) >> 8;
            tmp_size[2] = (bs_size & 0x00ff0000) >> 16;
            tmp_size[3] = (bs_size & 0xff000000) >> 24;

            if (bs_size <= 0)
            {
                logv1("bumping process starting...\n");
                continue;
            }
            bs_read_pos += (4 + bs_size);
            bitb.addr = bs_buf;
            bitb.ssize = bs_size;
            bitb.bsize = MAX_BS_BUF;
            EVEYD_ID_TO_CTX_RV(id, ctx, EVEY_ERR_INVALID_ARGUMENT);

            bsr = &ctx->bs;
            sps = &ctx->sps;
            pps = &ctx->pps;
            sh = &ctx->sh;
            nalu = &ctx->nalu;

            /* set error status */
            ctx->bs_err = bitb.err = 0;
            EVEY_TRACE_SET(1);
            /* bitstream reader initialization */
            eveyd_bsr_init(bsr, bitb.addr, bitb.ssize, NULL);
            SET_SBAC_DEC(bsr, &ctx->sbac_dec);
            /* bitstream writer initialization (starting from slice header) */
            eveye_bsw_init(&bsw, (u8*)bitb.addr+2, bitb.bsize, NULL);
            bsw.pdata[1] = &sbac_enc;

            /* parse NAL unit header */
            ret = eveyd_eco_nalu(bsr, nalu);
            evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

            switch(nalu->nal_unit_type_plus1 - 1)
            {
                case EVEY_SPS_NUT:
                    ret = eveyd_eco_sps(bsr, sps);
                    evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

                    if (!bs_num)
                    {
                        fwrite(tmp_size, 1, 4, fp_bs_write);
                        fwrite(bs_buf, 1, bs_size, fp_bs_write);
                    }
                    break;
                case EVEY_PPS_NUT:
                    ret = eveyd_eco_pps(bsr, sps, pps);
                    evey_assert_rv(EVEY_SUCCEEDED(ret), ret);
                    if (!bs_num)
                    {
                        fwrite(tmp_size, 1, 4, fp_bs_write);
                        fwrite(bs_buf, 1, bs_size, fp_bs_write);
                    }
                    break;
                case EVEY_NONIDR_NUT:
                case EVEY_IDR_NUT:
                    /* decode slice header */
                    ret = eveyd_eco_sh(bsr, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
                    evey_assert_rv(EVEY_SUCCEEDED(ret), ret);

                    if (bs_num == 0 && sh->slice_type == SLICE_I)
                    {
                        intra_dist[intra_dist_idx] = sh->poc_lsb;
                        intra_dist_idx++;
                    }

                    if (bs_num == 0)
                    {
                        fwrite(tmp_size, 1, 4, fp_bs_write);
                        fwrite(bs_buf, 1, bs_size, fp_bs_write);
                    }
                    else
                    {
                        if (!intra_dist_idx && sh->slice_type == SLICE_I)
                        {
                            intra_dist_idx++;
                        }
                        else
                        {
                            /* re-write slice header */
                            sh->poc_lsb += intra_dist[0];
                            ret = eveye_eco_sh(&bsw, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
                            fwrite(tmp_size, 1, 4, fp_bs_write);
                            fwrite(bs_buf, 1, bs_size, fp_bs_write);
                        }
                    }
                    break;
            }
        }while (bs_read_pos < bs_end_pos);
        
        if (id) eveyd_delete(id);
        if (imgb_t) imgb_free(imgb_t);
        if (fp_bs) fclose(fp_bs);
        if (bs_buf) free(bs_buf);
    }

    if (fp_bs_write) fclose(fp_bs_write);
    
    return 0;
}
