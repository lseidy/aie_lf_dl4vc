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

#define VERBOSE_NONE               VERBOSE_0
#define VERBOSE_FRAME              VERBOSE_1
#define VERBOSE_ALL                VERBOSE_2

#define MAX_BS_BUF                 (16*1024*1024) /* in byte */

static char op_fname_inp[256] = "\0";
static char op_fname_out[256] = "\0";
static char op_fname_opl[256] = "\0";
static int  op_max_frm_num = 0;
static int  op_use_pic_signature = 0;
static int  op_out_bit_depth = 0;
static int  op_out_chroma_format = 1;

typedef enum _STATES
{
    STATE_DECODING,
    STATE_BUMPING

} STATES;

typedef enum _OP_FLAGS
{
    OP_FLAG_FNAME_INP,
    OP_FLAG_FNAME_OUT,
    OP_FLAG_FNAME_OPL,
    OP_FLAG_MAX_FRM_NUM,
    OP_FLAG_USE_PIC_SIGN,
    OP_FLAG_OUT_BIT_DEPTH,
    OP_FLAG_VERBOSE,
    OP_FLAG_MAX

} OP_FLAGS;

static int op_flag[OP_FLAG_MAX] = {0};

static EVEY_ARGS_OPTION options[] = \
{
    {
        'i', "input", EVEY_ARGS_VAL_TYPE_STRING|EVEY_ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FNAME_INP], op_fname_inp,
        "file name of input bitstream"
    },
    {
        'o', "output", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_OUT], op_fname_out,
        "file name of decoded output"
    },
    {
        EVEY_ARGS_NO_KEY, "opl", EVEY_ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_OPL], op_fname_opl,
        "file name of opl file"
    },
    {
        'f',  "frames", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_FRM_NUM], &op_max_frm_num,
        "maximum number of frames to be decoded"
    },
    {
        's',  "signature", EVEY_ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_USE_PIC_SIGN], &op_use_pic_signature,
        "conformance check using picture signature (HASH)"
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
        EVEY_ARGS_NO_KEY,  "output_bit_depth", EVEY_ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], &op_out_bit_depth,
        "output bitdepth (8(default), 10) "
    },
    { 0, "", EVEY_ARGS_VAL_TYPE_NONE, NULL, NULL, ""} /* termination */
};

#define NUM_ARG_OPTION   ((int)(sizeof(options)/sizeof(options[0]))-1)
static void print_usage(void)
{
    int i;
    char str[1024];

    logv0("< Usage >\n");

    for(i=0; i<NUM_ARG_OPTION; i++)
    {
        if(evey_args_get_help(options, i, str) < 0) return;
        logv0("%s\n", str);
    }
}

static int read_nalu(FILE * fp, int * pos, unsigned char * bs_buf)
{
    int read_size, bs_size;
    unsigned char b = 0;

    bs_size = 0;
    read_size = 0;

    if(!fseek(fp, *pos, SEEK_SET))
    {
        /* read size first */
        if(4 == fread(&bs_size, 1, 4, fp)) //@TBC(Chernyak): is it ok from endianness perspective?
        {
            if(bs_size <= 0)
            {
                logv0("Invalid bitstream size![%d]\n", bs_size);
                return -1;
            }

            while(bs_size)
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

static void print_stat(EVEYD_STAT * stat, int ret)
{
    int i, j;

    if(EVEY_SUCCEEDED(ret))
    {
        if(stat->nalu_type < EVEY_SPS_NUT)
        {
            logv1("%c-slice", stat->stype == EVEY_ST_I ? 'I' : stat->stype == EVEY_ST_P ? 'P' : 'B');

            logv1(" (%d bytes", stat->read);
            logv1(", poc=%d, tid=%d, ", (int)stat->poc, (int)stat->tid);

            for (i = 0; i < 2; i++)
            {
                logv1("[L%d ", i);
                for (j = 0; j < stat->refpic_num[i]; j++) logv1("%d ", stat->refpic[i][j]);
                logv1("] ");
            }
        }
        else if(stat->nalu_type == EVEY_SPS_NUT)
        {
            logv1("Sequence Parameter Set (%d bytes)", stat->read);
        }
        else if (stat->nalu_type == EVEY_PPS_NUT)
        {
            logv1("Picture Parameter Set (%d bytes)", stat->read);
        }
        else if (stat->nalu_type == EVEY_SEI_NUT)
        {
            logv1("SEI message: ");
            if (ret == EVEY_OK)
            {
                logv1("MD5 check OK");
            }
            else if (ret == EVEY_ERR_BAD_CRC)
            {
                logv1("MD5 check mismatch!");
            }
            else if (ret == EVEY_WARN_CRC_IGNORED)
            {
                logv1("MD5 check ignored!");
            }
        }
        else
        {
            logv0("Unknown bitstream");
        }

        logv1("\n");
    }
    else
    {
        logv0("Decoding error = %d\n", ret);
    }
}

static int set_extra_config(EVEYD id)
{
    int  ret, size, value;

    if(op_use_pic_signature)
    {
        value = 1;
        size = 4;
        ret = eveyd_config(id, EVEYD_CFG_SET_USE_PIC_SIGNATURE, &value, &size);
        if(EVEY_FAILED(ret))
        {
            logv0("failed to set config for picture signature\n");
            return -1;
        }
    }

    if (op_fname_opl[0])
    {
        value = 1;
        size = 4;
        ret = eveyd_config(id, EVEYD_CFG_SET_USE_OPL_OUTPUT, &value, &size);
        if (EVEY_FAILED(ret))
        {
            logv0("failed to set config for picture signature\n");
            return -1;
        }
    }

    return 0;
}

static int write_dec_img(EVEYD id, char * fname, EVEY_IMGB * img, EVEY_IMGB * imgb_t)
{
    imgb_cpy(imgb_t, img);
    if(imgb_write(op_fname_out, imgb_t)) return -1;
    return EVEY_OK;
}

int main(int argc, const char **argv)
{
    STATES             state = STATE_DECODING;
    unsigned char    * bs_buf = NULL;
    EVEYD              id = NULL;
    EVEYD_CDSC         cdsc;
    EVEY_BITB          bitb;
    EVEY_IMGB        * imgb;
    /*temporal buffer for video bit depth less than 10bit */
    EVEY_IMGB        * imgb_t = NULL;
    EVEYD_STAT         stat;
    EVEYD_OPL          opl;
    int               ret;
    EVEY_CLK           clk_beg, clk_tot;
    int                bs_cnt, pic_cnt;
    int                bs_size, bs_read_pos = 0;
    int                w, h;
    FILE             * fp_bs = NULL;
       
    clk_beg = evey_clk_get();

    /* parse options */
    ret = evey_args_parse_all(argc, argv, options);
    if(ret != 0)
    {
        if(ret > 0) logv0("-%c argument should be set\n", ret);
        print_usage();
        return -1;
    }

    /* open input bitstream */
    fp_bs = fopen(op_fname_inp, "rb");
    if(fp_bs == NULL)
    {
        logv0("ERROR: cannot open bitstream file = %s\n", op_fname_inp);
        print_usage();
        return -1;
    }

    if(op_flag[OP_FLAG_FNAME_OUT])
    {
        /* remove decoded file contents if exists */
        FILE * fp;
        fp = fopen(op_fname_out, "wb");
        if(fp == NULL)
        {
            logv0("ERROR: cannot create a decoded file\n");
            print_usage();
            return -1;
        }
        fclose(fp);
    }

    if (op_flag[OP_FLAG_FNAME_OPL])
    {
        /* remove opl file contents if exists */
        FILE * fp;
        fp = fopen(op_fname_opl, "wb");
        if (fp == NULL)
        {
            logv0("ERROR: cannot create an opl file\n");
            print_usage();
            return -1;
        }
        fclose(fp);
    }

    bs_buf = malloc(MAX_BS_BUF);
    if(bs_buf == NULL)
    {
        logv0("ERROR: cannot allocate bit buffer, size=%d\n", MAX_BS_BUF);
        return -1;
    }
    id = eveyd_create(&cdsc, NULL);
    if(id == NULL)
    {
        logv0("ERROR: cannot create EVEY decoder\n");
        return -1;
    }
    if(set_extra_config(id))
    {
        logv0("ERROR: cannot set extra configurations\n");
        return -1;
    }

    pic_cnt = 0;
    clk_tot = 0;
    bs_cnt  = 0;
    w = h   = 0;

    int process_status = EVEY_OK;

    while(1)
    {
        if (state == STATE_DECODING)
        {
            memset(&stat, 0, sizeof(EVEYD_STAT));

            bs_size = read_nalu(fp_bs, &bs_read_pos, bs_buf);
            int nalu_size_field_in_bytes = 4;

            if (bs_size <= 0)
            {
                state = STATE_BUMPING;
                logv1("bumping process starting...\n");
                continue;
            }

            bs_read_pos += (nalu_size_field_in_bytes + bs_size);
            stat.read += nalu_size_field_in_bytes;
            bitb.addr = bs_buf;
            bitb.ssize = bs_size;
            bitb.bsize = MAX_BS_BUF;

            logv1("[%4d] NALU --> ", bs_cnt++);

            /* main decoding block */
            ret = eveyd_decode(id, &bitb, &stat);

            if(EVEY_FAILED(ret))
            {
                logv0("failed to decode bitstream\n");
                goto END;
            }

            print_stat(&stat, ret);

            if(stat.read - nalu_size_field_in_bytes != bs_size)
            {
                logv0("\t=> different reading of bitstream (in:%d, read:%d)\n",
                    bs_size, stat.read);
            }

            process_status = ret;
        }

        if(stat.fnum >= 0 || state == STATE_BUMPING)
        {
            ret = eveyd_pull(id, &imgb, &opl);
            if(ret == EVEY_ERR_UNEXPECTED)
            {
                logv1("bumping process completed\n");
                goto END;
            }
            else if(EVEY_FAILED(ret))
            {
                logv0("failed to pull the decoded image\n");
                return -1;
            }
        }
        else
        {
            imgb = NULL;
        }

        if(imgb)
        {
            w = imgb->w[0];
            h = imgb->h[0];

            op_out_bit_depth = op_out_bit_depth == 0 ? EVEY_CS_GET_BIT_DEPTH(imgb->cs) : op_out_bit_depth;

            if(op_flag[OP_FLAG_FNAME_OUT])
            {
                if(imgb_t == NULL)
                {
                    
                    imgb_t = imgb_alloc(w, h, EVEY_CS_SET(EVEY_CS_GET_FORMAT(imgb->cs), op_out_bit_depth, 0));

                    if(imgb_t == NULL)
                    {
                        logv0("failed to allocate temporay image buffer\n");
                        return -1;
                    }
                }

                write_dec_img(id, op_fname_out, imgb, imgb_t);
            }

            if (op_flag[OP_FLAG_FNAME_OPL])
            {
                FILE* fp_opl = fopen(op_fname_opl, "a");
                if (fp_opl == NULL)
                {
                    logv0("ERROR: cannot create an opl file\n");
                    print_usage();
                    return -1;
                }

                fprintf(fp_opl, "%d %d %d ", opl.poc, w, h);
                for (int i = 0; i < 3; ++i) /* number of compononets */
                {
                    for (int j = 0; j < 16; ++j)
                    {
                        unsigned int byte = (unsigned char) opl.digest[i][j];
                        fprintf(fp_opl, "%02x", byte);
                    }
                    fprintf(fp_opl, " ");
                }

                fprintf(fp_opl, "\n");

                fclose(fp_opl);
            }

            imgb->release(imgb);
            pic_cnt++;
        }
        fflush(stdout);
        fflush(stderr);
    }

END:

    clk_tot += evey_clk_from(clk_beg);

    logv1("=======================================================================================\n");
    logv1("Resolution                        = %d x %d\n", w, h);
    logv1("Processed NALUs                   = %d\n", bs_cnt);
    logv1("Decoded frame count               = %d\n", pic_cnt);
    if(pic_cnt > 0)
    {
        logv1("total decoding time               = %d msec,",
                (int)evey_clk_msec(clk_tot));
        logv1(" %.3f sec\n",
            (float)(evey_clk_msec(clk_tot) /1000.0));

        logv1("Average decoding time for a frame = %d msec\n",
                (int)evey_clk_msec(clk_tot)/pic_cnt);
        logv1("Average decoding speed            = %.3f frames/sec\n",
                ((float)pic_cnt*1000)/((float)evey_clk_msec(clk_tot)));
    }
    logv1("=======================================================================================\n");

    if(id) eveyd_delete(id);
    if(imgb_t) imgb_free(imgb_t);
    if(fp_bs) fclose(fp_bs);
    if(bs_buf) free(bs_buf);

    return process_status;
}
