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

#include "evey_def.h"
#include "evey_picman.h"

/* macros for reference picture flag */
#define IS_REF(pic)          (((pic)->is_ref) != 0)
#define SET_REF_UNMARK(pic)  (((pic)->is_ref) = 0)
#define SET_REF_MARK(pic)    (((pic)->is_ref) = 1)

#define PRINT_DPB(pm)\
    evey_print("%s: current num_ref = %d, dpb_size = %d\n", __FUNCTION__, \
    pm->cur_num_ref_pics, picman_get_num_allocated_pics(pm));

static int picman_get_num_allocated_pics(EVEY_PM * pm)
{
    int i, cnt = 0;
    for(i = 0; i < MAX_PB_SIZE; i++) /* this is coding order */
    {
        if(pm->pic[i]) cnt++;
    }
    return cnt;
}

static int picman_move_pic(EVEY_PM * pm, int from, int to)
{
    int        i;
    EVEY_PIC * pic;

    pic = pm->pic[from];

    for(i = from; i < to; i++)
    {
        pm->pic[i] = pm->pic[i + 1];
    }
    pm->pic[to] = pic;

    return 0;
}

static void pic_marking_no_rpl(EVEY_PM * pm, int ref_pic_gap_length)
{
    int        i;
    EVEY_PIC * pic;

    /* mark all pics with layer id > 0 as unused for reference */
    for(i = 0; i < MAX_PB_SIZE; i++) /* this is coding order */
    {
        if(pm->pic[i] && IS_REF(pm->pic[i]) &&
            (pm->pic[i]->temporal_id > 0 || (i > 0 && ref_pic_gap_length > 0 && pm->pic[i]->poc % ref_pic_gap_length != 0)))
        {
            pic = pm->pic[i];

            /* unmark for reference */
            SET_REF_UNMARK(pic);
            picman_move_pic(pm, i, MAX_PB_SIZE - 1);

            if(pm->cur_num_ref_pics > 0)
            {
                pm->cur_num_ref_pics--;
            }
            i--;
        }
    }
    while(pm->cur_num_ref_pics >= MAX_NUM_ACTIVE_REF_FRAME)
    {
        for(i = 0; i < MAX_PB_SIZE; i++) /* this is coding order */
        {
            if(pm->pic[i] && IS_REF(pm->pic[i]) )
            {
                pic = pm->pic[i];

                /* unmark for reference */
                SET_REF_UNMARK(pic);
                picman_move_pic(pm, i, MAX_PB_SIZE - 1);

                pm->cur_num_ref_pics--;

                break;
            }
        }
    }
}

static void picman_flush_pb(EVEY_PM * pm)
{
    int i;

    /* mark all frames unused */
    for(i = 0; i < MAX_PB_SIZE; i++)
    {
        if(pm->pic[i]) SET_REF_UNMARK(pm->pic[i]);
    }
    pm->cur_num_ref_pics = 0;
}

static void picman_update_pic_ref(EVEY_PM * pm)
{
    EVEY_PIC ** pic;
    EVEY_PIC ** pic_ref;
    EVEY_PIC  * pic_t;
    int         i, j, cnt;

    pic = pm->pic;
    pic_ref = pm->pic_ref;

    for(i = 0, j = 0; i < MAX_PB_SIZE; i++)
    {
        if(pic[i] && IS_REF(pic[i]))
        {
            pic_ref[j++] = pic[i];
        }
    }
    cnt = j;
    while(j < MAX_NUM_REF_PICS) pic_ref[j++] = NULL;

    /* descending order sort based on POC */
    for(i = 0; i < cnt - 1; i++)
    {
        for(j = i + 1; j < cnt; j++)
        {
            if(pic_ref[i]->poc < pic_ref[j]->poc)
            {
                pic_t = pic_ref[i];
                pic_ref[i] = pic_ref[j];
                pic_ref[j] = pic_t;
            }
        }
    }
}

static EVEY_PIC * picman_remove_pic_from_pb(EVEY_PM * pm, int pos)
{
    int         i;
    EVEY_PIC  * pic_rem;

    pic_rem = pm->pic[pos];
    pm->pic[pos] = NULL;

    /* fill empty pic buffer */
    for(i = pos; i < MAX_PB_SIZE - 1; i++)
    {
        pm->pic[i] = pm->pic[i + 1];
    }
    pm->pic[MAX_PB_SIZE - 1] = NULL;

    pm->cur_pb_size--;

    return pic_rem;
}

static void picman_set_pic_to_pb(EVEY_PM * pm, EVEY_PIC * pic, EVEY_REFP (* refp)[LIST_NUM], int pos)
{
    int i;

    for(i = 0; i < pm->num_refp[LIST_0]; i++)
    {
        pic->list_poc[i] = refp[i][LIST_0].poc;
    }

    if(pos >= 0)
    {
        evey_assert(pm->pic[pos] == NULL);
        pm->pic[pos] = pic;
    }
    else /* pos < 0 */
    {
        /* search empty pic buffer position */
        for(i = (MAX_PB_SIZE - 1); i >= 0; i--)
        {
            if(pm->pic[i] == NULL)
            {
                pm->pic[i] = pic;
                break;
            }
        }
        if(i < 0)
        {
            evey_trace("i=%d\n", i);
            evey_assert(i >= 0);
        }
    }
    pm->cur_pb_size++;
}

static int picman_get_empty_pic_from_list(EVEY_PM * pm)
{
    EVEY_IMGB * imgb;
    EVEY_PIC  * pic;
    int         i;

    for(i = 0; i < MAX_PB_SIZE; i++)
    {
        pic = pm->pic[i];

        if(pic != NULL && !IS_REF(pic) && pic->need_for_out == 0)
        {
            imgb = pic->imgb;
            evey_assert(imgb != NULL);

            /* check reference count */
            if (1 == imgb->getref(imgb))
            {
                return i; /* this is empty buffer */
            }
        }
    }
    return -1;
}

void set_refp(EVEY_REFP * refp, EVEY_PIC  * pic_ref)
{
    refp->pic      = pic_ref;
    refp->poc      = pic_ref->poc;
    refp->map_mv   = pic_ref->map_mv;
    refp->map_refi = pic_ref->map_refi;
    refp->list_poc = pic_ref->list_poc;
}

void copy_refp(EVEY_REFP * refp_dst, EVEY_REFP * refp_src)
{
    refp_dst->pic      = refp_src->pic;
    refp_dst->poc      = refp_src->poc;
    refp_dst->map_mv   = refp_src->map_mv;
    refp_dst->map_refi = refp_src->map_refi;
    refp_dst->list_poc = refp_src->list_poc;
}

int check_copy_refp(EVEY_REFP(*refp)[LIST_NUM], int cnt, int lidx, EVEY_REFP  * refp_src)
{
    int i;

    for(i = 0; i < cnt; i++)
    {
        if(refp[i][lidx].poc == refp_src->poc)
        {
            return -1;
        }
    }
    copy_refp(&refp[cnt][lidx], refp_src);

    return EVEY_OK;
}

int evey_picman_refp_init(void * ctx)
{
    EVEY_CTX   * c_ctx = (EVEY_CTX*)ctx;
    EVEY_PM    * pm = &c_ctx->dpbm;
    int          max_num_ref_pics = c_ctx->sps.max_num_ref_pics;
    int          slice_type = c_ctx->sh.slice_type;
    u32          poc = c_ctx->poc.poc_val;
    u8           layer_id = c_ctx->nalu.nuh_temporal_id;
    int          last_intra = c_ctx->last_intra_poc;
    EVEY_REFP (* refp)[LIST_NUM] = c_ctx->refp;

    int i, cnt;
    if(slice_type == SLICE_I)
    {
        return EVEY_OK;
    }

    picman_update_pic_ref(pm);
    evey_assert_rv(pm->cur_num_ref_pics > 0, EVEY_ERR_UNEXPECTED);

    for(i = 0; i < MAX_NUM_REF_PICS; i++)
    {
        refp[i][LIST_0].pic = refp[i][LIST_1].pic = NULL;
    }
    pm->num_refp[LIST_0] = pm->num_refp[LIST_1] = 0;

    /* forward */
    if(slice_type == SLICE_P)
    {
        if(layer_id > 0)
        {
            for(i = 0, cnt = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
            {
                /* if(poc >= last_intra && pm->pic_ref[i]->poc < last_intra) continue; */
                if(layer_id == 1)
                {
                    if(pm->pic_ref[i]->poc < poc && pm->pic_ref[i]->temporal_id <= layer_id)
                    {
                        set_refp(&refp[cnt][LIST_0], pm->pic_ref[i]);
                        cnt++;
                    }
                }
                else if(pm->pic_ref[i]->poc < poc && cnt == 0)
                {
                    set_refp(&refp[cnt][LIST_0], pm->pic_ref[i]);
                    cnt++;
                }
                else if(cnt != 0 && pm->pic_ref[i]->poc < poc && \
                          pm->pic_ref[i]->temporal_id <= 1)
                {
                    set_refp(&refp[cnt][LIST_0], pm->pic_ref[i]);
                    cnt++;
                }
            }
        }
        else /* layer_id == 0, non-scalable  */
        {
            for(i = 0, cnt = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
            {
                if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
                if(pm->pic_ref[i]->poc < poc)
                {
                    set_refp(&refp[cnt][LIST_0], pm->pic_ref[i]);
                    cnt++;
                }
            }
        }
    }
    else /* SLICE_B */
    {
        int next_layer_id = EVEY_MAX(layer_id - 1, 0);
        for(i = 0, cnt = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
        {
            if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
            if(pm->pic_ref[i]->poc < poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
            {
                set_refp(&refp[cnt][LIST_0], pm->pic_ref[i]);
                cnt++;
                next_layer_id = EVEY_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
            }
        }
    }

    if(cnt < max_num_ref_pics && slice_type == SLICE_B)
    {
        int next_layer_id = EVEY_MAX(layer_id - 1, 0);
        for(i = pm->cur_num_ref_pics - 1; i >= 0 && cnt < max_num_ref_pics; i--)
        {
            if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
            if(pm->pic_ref[i]->poc > poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
            {
                set_refp(&refp[cnt][LIST_0], pm->pic_ref[i]);
                cnt++;
                next_layer_id = EVEY_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
            }
        }
    }

    evey_assert_rv(cnt > 0, EVEY_ERR_UNEXPECTED);
    pm->num_refp[LIST_0] = cnt;

    /* backward */
    if(slice_type == SLICE_B)
    {
        int next_layer_id = EVEY_MAX(layer_id - 1, 0);
        for(i = pm->cur_num_ref_pics - 1, cnt = 0; i >= 0 && cnt < max_num_ref_pics; i--)
        {
            if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
            if(pm->pic_ref[i]->poc > poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
            {
                set_refp(&refp[cnt][LIST_1], pm->pic_ref[i]);
                cnt++;
                next_layer_id = EVEY_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
            }
        }

        if(cnt < max_num_ref_pics)
        {
            next_layer_id = EVEY_MAX(layer_id - 1, 0);
            for(i = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
            {

                if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
                if(pm->pic_ref[i]->poc < poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
                {
                    set_refp(&refp[cnt][LIST_1], pm->pic_ref[i]);
                    cnt++;
                    next_layer_id = EVEY_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
                }
            }
        }

        evey_assert_rv(cnt > 0, EVEY_ERR_UNEXPECTED);
        pm->num_refp[LIST_1] = cnt;
    }

    if(slice_type == SLICE_B)
    {
        pm->num_refp[LIST_0] = EVEY_MIN(pm->num_refp[LIST_0], max_num_ref_pics);
        pm->num_refp[LIST_1] = EVEY_MIN(pm->num_refp[LIST_1], max_num_ref_pics);
    }

    return EVEY_OK;
}

EVEY_PIC * evey_picman_get_empty_pic(EVEY_PM * pm, int * err)
{
    int        ret;
    EVEY_PIC * pic = NULL;

    /* try to find empty picture buffer in list */
    ret = picman_get_empty_pic_from_list(pm);
    if(ret >= 0)
    {
        pic = picman_remove_pic_from_pb(pm, ret);
        goto END;
    }
    /* else if available, allocate picture buffer */
    pm->cur_pb_size = picman_get_num_allocated_pics(pm);

    if(pm->cur_pb_size < pm->max_pb_size)
    {
        /* create picture buffer */
        pic = pm->pa.fn_alloc(&pm->pa, &ret);
        evey_assert_gv(pic != NULL, ret, EVEY_ERR_OUT_OF_MEMORY, ERR);

        goto END;
    }
    evey_assert_gv(0, ret, EVEY_ERR_UNKNOWN, ERR);

END:
    pm->pic_lease = pic;
    if(err) *err = EVEY_OK;
    return pic;

ERR:
    if(err) *err = ret;
    if(pic) pm->pa.fn_free(pic);
    return NULL;
}

int evey_picman_put_pic(void * ctx, EVEY_PIC * pic, int need_for_output)
{
    EVEY_CTX   * c_ctx = (EVEY_CTX*)ctx;
    EVEY_PM    * pm = &c_ctx->dpbm;
    int          is_idr = c_ctx->nalu.nal_unit_type_plus1 - 1 == EVEY_IDR_NUT;
    u32          poc = c_ctx->poc.poc_val;
    u8           temporal_id = c_ctx->nalu.nuh_temporal_id;
    EVEY_REFP (* refp)[LIST_NUM] = c_ctx->refp;
    int          ref_pic = c_ctx->slice_ref_flag;
    int          ref_pic_gap_length = c_ctx->ref_pic_gap_length;

    /* manage DPB */
    if(is_idr)
    {
        picman_flush_pb(pm);
    }    
    else /* Perform picture marking */
    {
        if (temporal_id == 0)
        {
            pic_marking_no_rpl(pm, ref_pic_gap_length);
        }
    }

    SET_REF_MARK(pic);

    if(!ref_pic)
    {
        SET_REF_UNMARK(pic);
    }

    pic->temporal_id = temporal_id;
    pic->poc = poc;
    pic->need_for_out = need_for_output;

    /* put picture into listed DPB */
    if(IS_REF(pic))
    {
        picman_set_pic_to_pb(pm, pic, refp, pm->cur_num_ref_pics);
        pm->cur_num_ref_pics++;
    }
    else
    {
        picman_set_pic_to_pb(pm, pic, refp, -1);
    }

    if(pm->pic_lease == pic)
    {
        pm->pic_lease = NULL;
    }

    return EVEY_OK;
}

EVEY_PIC * evey_picman_out_pic(EVEY_PM * pm, int * err)
{
    EVEY_PIC ** ps;
    int         i, ret, any_need_for_out = 0;

    ps = pm->pic;

    for(i = 0; i < MAX_PB_SIZE; i++)
    {
        if(ps[i] != NULL && ps[i]->need_for_out)
        {
            any_need_for_out = 1;

            if((ps[i]->poc <= pm->poc_next_output))
            {
                ps[i]->need_for_out = 0;
                pm->poc_next_output = ps[i]->poc + pm->poc_increase;

                if(err) *err = EVEY_OK;
                return ps[i];
            }
        }
    }
    if(any_need_for_out == 0)
    {
        ret = EVEY_ERR_UNEXPECTED;
    }
    else
    {
        ret = EVEY_OK_FRM_DELAYED;
    }

    if(err) *err = ret;
    return NULL;
}

int evey_picman_deinit(EVEY_PM * pm)
{
    int i;

    /* remove allocated picture and picture store buffer */
    for(i = 0; i < MAX_PB_SIZE; i++)
    {
        if(pm->pic[i])
        {
            pm->pa.fn_free(pm->pic[i]);
            pm->pic[i] = NULL;
        }
    }
    if(pm->pic_lease)
    {
        pm->pa.fn_free(pm->pic_lease);
        pm->pic_lease = NULL;
    }
    return EVEY_OK;
}

int evey_picman_init(EVEY_PM * pm, int max_pb_size, int max_num_ref_pics, EVEY_PICBUF_ALLOCATOR * pa)
{
    if(max_num_ref_pics > MAX_NUM_REF_PICS || max_pb_size > MAX_PB_SIZE)
    {
        return EVEY_ERR_UNSUPPORTED;
    }
    pm->max_num_ref_pics = max_num_ref_pics;
    pm->max_pb_size = max_pb_size;
    pm->poc_increase = 1;
    pm->pic_lease = NULL;

    evey_mcpy(&pm->pa, pa, sizeof(EVEY_PICBUF_ALLOCATOR));

    return EVEY_OK;
}
