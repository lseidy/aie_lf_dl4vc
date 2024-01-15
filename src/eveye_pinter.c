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

#include "eveye_def.h"
#include "eveye_pinter.h"
#include <math.h>


#define MV_COST(pi, mv_bits) (u32)(((pi)->lambda_mv * mv_bits + (1 << 15)) >> 16)
#define SWAP(a, b, t) { (t) = (a); (a) = (b); (b) = (t); }

#define BI_ITER                            4
#define MAX_FIRST_SEARCH_STEP              3
#define MAX_REFINE_SEARCH_STEP             2
#define RASTER_SEARCH_STEP                 5
#define RASTER_SEARCH_THD                  5
#define REFINE_SEARCH_THD                  0
#define BI_STEP                            5

/* Define the Search Range for int-pel */
#define SEARCH_RANGE_IPEL_RA               384
#define SEARCH_RANGE_IPEL_LD               64

/* quarter-pel search pattern */
#define QUARTER_PEL_SEARCH_PATTERN_CNT        8
static s8 tbl_search_pattern_qpel_8point[QUARTER_PEL_SEARCH_PATTERN_CNT][MV_D] =
{
    {-1,  0},
    { 0,  1},
    { 1,  0},
    { 0, -1},
    {-1,  1},
    { 1,  1},
    {-1, -1},
    { 1, -1}
};

static const s8 tbl_diapos_partial[2][16][2] =
{
    {
        {-2, 0}, {-1, 1}, {0, 2}, {1, 1}, {2, 0}, {1, -1}, {0, -2}, {-1, -1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
    },
    {
        {-4, 0}, {-3, 1}, {-2, 2}, {-1, 3}, {0, 4}, {1, 3}, {2, 2}, {3, 1}, {4, 0}, {3, -1}, {2, -2}, {1, -3}, {0, -4}, {-1, -3}, {-2, -2}, {-3, -1}
    }
};

/* half-pel search pattern */
#define HALF_PEL_SEARCH_PATTERN_CNT        8
static s8 tbl_search_pattern_hpel_partial[HALF_PEL_SEARCH_PATTERN_CNT][MV_D] =
{
    {-2,  0},
    {-2,  2},
    { 0,  2},
    { 2,  2},
    { 2,  0},
    { 2, -2},
    { 0, -2},
    {-2, -2}
};

__inline static u32 get_exp_golomb_bits(u32 abs_mvd)
{
    int bits = 0;
    int len_i, len_c, nn;

    /* abs(mvd) */
    nn = ((abs_mvd + 1) >> 1);
    for(len_i = 0; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }
    len_c = (len_i << 1) + 1;

    bits += len_c;

    /* sign */
    if(abs_mvd)
    {
        bits++;
    }

    return bits;
}

static int get_mv_bits(int mvd_x, int mvd_y, int num_refp, int refi)
{
    int bits = 0;
    bits  = (mvd_x > 2048 || mvd_x <= -2048) ? get_exp_golomb_bits(EVEY_ABS(mvd_x)) : eveye_tbl_mv_bits[mvd_x];
    bits += (mvd_y > 2048 || mvd_y <= -2048) ? get_exp_golomb_bits(EVEY_ABS(mvd_y)) : eveye_tbl_mv_bits[mvd_y];
    bits += eveye_tbl_refi_bits[num_refp][refi];
    return bits;
}

static void get_range_ipel(EVEYE_PINTER * pi, s16 mvc[MV_D], s16 range[MV_RANGE_DIM][MV_D], int bi, int ri, int lidx)
{
    int offset = pi->gop_size >> 1;
    int max_search_range = EVEY_CLIP3(pi->max_search_range >> 2, pi->max_search_range, (pi->max_search_range * EVEY_ABS(pi->poc - (int)pi->refp[ri][lidx].poc) + offset) / pi->gop_size);
    int search_range_x = bi ? BI_STEP : max_search_range;
    int search_range_y = bi ? BI_STEP : max_search_range;

    /* define search range for int-pel search and clip it if needs */
    range[MV_RANGE_MIN][MV_X] = EVEY_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X] - search_range_x);
    range[MV_RANGE_MAX][MV_X] = EVEY_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X] + search_range_x);
    range[MV_RANGE_MIN][MV_Y] = EVEY_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y] - search_range_y);
    range[MV_RANGE_MAX][MV_Y] = EVEY_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y] + search_range_y);

    evey_assert(range[MV_RANGE_MIN][MV_X] <= range[MV_RANGE_MAX][MV_X]);
    evey_assert(range[MV_RANGE_MIN][MV_Y] <= range[MV_RANGE_MAX][MV_Y]);
}

static u32 me_raster(EVEYE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D], s16 gmvp[MV_D], s16 mv[MV_D], int bit_depth_luma)
{
    EVEY_PIC * ref_pic;
    pel      * org, *ref;
    u8         mv_bits, best_mv_bits;
    u32        cost_best, cost;
    int        i, j;
    s16        mv_x, mv_y;
    s32        search_step_x = EVEY_MAX(RASTER_SEARCH_STEP, (1 << (log2_cuw - 1))); /* Adaptive step size : Half of CU dimension */
    s32        search_step_y = EVEY_MAX(RASTER_SEARCH_STEP, (1 << (log2_cuh - 1))); /* Adaptive step size : Half of CU dimension */
    s16        center_mv[MV_D];
    s32        search_step;

    search_step_x = search_step_y = EVEY_MAX(RASTER_SEARCH_STEP, (1 << (EVEY_MIN(log2_cuh, log2_cuw) - 1)));

    org = pi->o_y;
    ref_pic = pi->refp[refi][lidx].pic;
    best_mv_bits = 0;
    cost_best = EVEY_UINT32_MAX;

    for(i = range[MV_RANGE_MIN][MV_Y]; i <= range[MV_RANGE_MAX][MV_Y]; i += (search_step_y * (refi + 1)))
    {
        for(j = range[MV_RANGE_MIN][MV_X]; j <= range[MV_RANGE_MAX][MV_X]; j += (search_step_x * (refi + 1)))
        {
            mv_x = j;
            mv_y = i;

            /* get MVD bits */
            mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

            /* get MVD cost_best */
            cost = MV_COST(pi, mv_bits);

            ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;
            
            /* get sad */
            cost += eveye_sad_16b(log2_cuw, log2_cuh, org, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);

            /* check if motion cost_best is less than minimum cost_best */
            if(cost < cost_best)
            {
                mv[MV_X] = ((mv_x - x) << 2);
                mv[MV_Y] = ((mv_y - y) << 2);
                cost_best = cost;
                best_mv_bits = mv_bits;
            }
        }
    }

    /* Grid search around best mv for all dyadic step sizes till integer pel */
    search_step = (refi + 1) * EVEY_MAX(search_step_x, search_step_y) >> 1;

    while(search_step > 0)
    {
        center_mv[MV_X] = mv[MV_X];
        center_mv[MV_Y] = mv[MV_Y];

        for(i = -search_step; i <= search_step; i += search_step)
        {
            for(j = -search_step; j <= search_step; j += search_step)
            {
                mv_x = (center_mv[MV_X] >> 2) + x + j;
                mv_y = (center_mv[MV_Y] >> 2) + y + i;

                if((mv_x < range[MV_RANGE_MIN][MV_X]) || (mv_x > range[MV_RANGE_MAX][MV_X]))
                    continue;
                if((mv_y < range[MV_RANGE_MIN][MV_Y]) || (mv_y > range[MV_RANGE_MAX][MV_Y]))
                    continue;

                /* get MVD bits */
                mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

                /* get MVD cost_best */
                cost = MV_COST(pi, mv_bits);

                ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;

                /* get sad */
                cost += eveye_sad_16b(log2_cuw, log2_cuh, org, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);

                /* check if motion cost_best is less than minimum cost_best */
                if(cost < cost_best)
                {
                    mv[MV_X] = ((mv_x - x) << 2);
                    mv[MV_Y] = ((mv_y - y) << 2);
                    cost_best = cost;
                    best_mv_bits = mv_bits;
                }
            }
        }

        /* Halve the step size */
        search_step >>= 1;
    }

    if(best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 me_ipel_refinement(EVEYE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D], s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int * beststep, int faststep, int bit_depth_luma)
{
    EVEY_PIC * ref_pic;
    pel      * org, *ref;
    u32        cost, cost_best = EVEY_UINT32_MAX;
    int        mv_bits, best_mv_bits;
    s16        mv_x, mv_y, mv_best_x, mv_best_y;
    int        lidx_r = (lidx == LIST_0) ? LIST_1 : LIST_0;
    s16      * org_bi = pi->org_bi;
    int        step, i;
    s16        imv_x, imv_y;
    int        mvsize = 1;

    org = pi->o_y;
    ref_pic = pi->refp[refi][lidx].pic;
    mv_best_x = (mvi[MV_X] >> 2);
    mv_best_y = (mvi[MV_Y] >> 2);
    best_mv_bits = 0;
    step = 1;
    mv_best_x = EVEY_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mv_best_x);
    mv_best_y = EVEY_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mv_best_y);

    imv_x = mv_best_x;
    imv_y = mv_best_y;

    int test_pos[9][2] = { { 0, 0}, { -1, -1},{ -1, 0},{ -1, 1},{ 0, -1},{ 0, 1},{ 1, -1},{ 1, 0},{ 1, 1} };

    for (i = 0; i <= 8; i++)
    {
        mv_x = imv_x + (step * test_pos[i][MV_X]);
        mv_y = imv_y + (step * test_pos[i][MV_Y]);

        if (mv_x > range[MV_RANGE_MAX][MV_X] ||
            mv_x < range[MV_RANGE_MIN][MV_X] ||
            mv_y > range[MV_RANGE_MAX][MV_Y] ||
            mv_y < range[MV_RANGE_MIN][MV_Y])
        {
            cost = EVEY_UINT32_MAX;
        }
        else
        {
            /* get MVD bits */
            mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

            if (bi)
            {
                mv_bits += pi->mot_bits[lidx_r];
            }

            /* get MVD cost_best */
            cost = MV_COST(pi, mv_bits);

            ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;
            if (bi)
            {
                /* get sad */
                cost += eveye_sad_bi_16b(log2_cuw, log2_cuh, org_bi, ref, 1 << log2_cuw, ref_pic->s_l,bit_depth_luma);
            }
            else
            {
                /* get sad */
                cost += eveye_sad_16b(log2_cuw, log2_cuh, org, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
            }

            /* check if motion cost_best is less than minimum cost_best */
            if (cost < cost_best)
            {
                mv_best_x = mv_x;
                mv_best_y = mv_y;
                cost_best = cost;
                best_mv_bits = mv_bits;
            }
        }
    }

    /* set best MV */
    mv[MV_X] = ((mv_best_x - x) << 2);
    mv[MV_Y] = ((mv_best_y - y) << 2);

    if (!bi && best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 me_ipel_diamond(EVEYE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D], s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int * beststep, int faststep, int bit_depth_luma)
{
    EVEY_PIC * ref_pic;
    pel      * org, *ref;
    u32        cost, cost_best = EVEY_UINT32_MAX;
    int        mv_bits, best_mv_bits;
    s16        mv_x, mv_y, mv_best_x, mv_best_y;
    int        lidx_r = (lidx == LIST_0) ? LIST_1 : LIST_0;
    s16      * org_bi = pi->org_bi;
    s16        mvc[MV_D];
    int        step, i, j;
    int        min_cmv_x, min_cmv_y, max_cmv_x, max_cmv_y;
    s16        imv_x, imv_y;
    int        mvsize = 1;
    int        not_found_best = 0;

    org = pi->o_y;
    ref_pic = pi->refp[refi][lidx].pic;
    mv_best_x = (mvi[MV_X] >> 2);
    mv_best_y = (mvi[MV_Y] >> 2);
    best_mv_bits = 0;
    step = 0;
    mv_best_x = EVEY_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mv_best_x);
    mv_best_y = EVEY_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mv_best_y);

    imv_x = mv_best_x;
    imv_y = mv_best_y;

    while(1)
    {
        not_found_best++;

        if(step <= 2)
        {
            min_cmv_x = (mv_best_x <= range[MV_RANGE_MIN][MV_X]) ? mv_best_x : mv_best_x - (bi ? BI_STEP : 2);
            min_cmv_y = (mv_best_y <= range[MV_RANGE_MIN][MV_Y]) ? mv_best_y : mv_best_y - (bi ? BI_STEP : 2);
            max_cmv_x = (mv_best_x >= range[MV_RANGE_MAX][MV_X]) ? mv_best_x : mv_best_x + (bi ? BI_STEP : 2);
            max_cmv_y = (mv_best_y >= range[MV_RANGE_MAX][MV_Y]) ? mv_best_y : mv_best_y + (bi ? BI_STEP : 2);
            mvsize = 1;

            for(i = min_cmv_y; i <= max_cmv_y; i += mvsize)
            {
                for(j = min_cmv_x; j <= max_cmv_x; j += mvsize)
                {
                    mv_x = j;
                    mv_y = i;

                    if(mv_x > range[MV_RANGE_MAX][MV_X] ||
                       mv_x < range[MV_RANGE_MIN][MV_X] ||
                       mv_y > range[MV_RANGE_MAX][MV_Y] ||
                       mv_y < range[MV_RANGE_MIN][MV_Y])
                    {
                        cost = EVEY_UINT32_MAX;
                    }
                    else
                    {
                        /* get MVD bits */
                        mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

                        if(bi)
                        {
                            mv_bits += pi->mot_bits[lidx_r];
                        }

                        /* get MVD cost_best */
                        cost = MV_COST(pi, mv_bits);

                        ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;

                        if(bi)
                        {
                            /* get sad */
                            cost += eveye_sad_bi_16b(log2_cuw, log2_cuh, org_bi, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
                        }
                        else
                        {
                            /* get sad */
                            cost += eveye_sad_16b(log2_cuw, log2_cuh, org, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
                        }

                        /* check if motion cost_best is less than minimum cost_best */
                        if(cost < cost_best)
                        {
                            mv_best_x = mv_x;
                            mv_best_y = mv_y;
                            *beststep = 2;
                            not_found_best = 0;
                            cost_best = cost;
                            best_mv_bits = mv_bits;
                        }
                    }
                }
            }

            mvc[MV_X] = mv_best_x;
            mvc[MV_Y] = mv_best_y;

            get_range_ipel(pi, mvc, range, bi, refi, lidx);

            step += 2;
        }
        else
        {
            int meidx = step > 8 ? 2 : 1;
            int multi = step;

            for(i = 0; i < 16; i++)
            {
                if(meidx == 1 && i > 8)
                {
                    continue;
                }
                if((step == 4) && (i == 1 || i == 3 || i == 5 || i == 7))
                {
                    continue;
                }

                mv_x = imv_x + ((multi >> meidx) * tbl_diapos_partial[meidx - 1][i][MV_X]);
                mv_y = imv_y + ((multi >> meidx) * tbl_diapos_partial[meidx - 1][i][MV_Y]);

                if(mv_x > range[MV_RANGE_MAX][MV_X] ||
                   mv_x < range[MV_RANGE_MIN][MV_X] ||
                   mv_y > range[MV_RANGE_MAX][MV_Y] ||
                   mv_y < range[MV_RANGE_MIN][MV_Y])
                {
                    cost = EVEY_UINT32_MAX;
                }
                else
                {
                    /* get MVD bits */
                    mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

                    if(bi)
                    {
                        mv_bits += pi->mot_bits[lidx_r];
                    }

                    /* get MVD cost_best */
                    cost = MV_COST(pi, mv_bits);

                    ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;
                    if(bi)
                    {
                        /* get sad */
                        cost += eveye_sad_bi_16b(log2_cuw, log2_cuh, org_bi, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
                    }
                    else
                    {
                        /* get sad */
                        cost += eveye_sad_16b(log2_cuw, log2_cuh, org, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
                    }

                    /* check if motion cost_best is less than minimum cost_best */
                    if(cost < cost_best)
                    {
                        mv_best_x = mv_x;
                        mv_best_y = mv_y;
                        *beststep = step;
                        cost_best = cost;
                        best_mv_bits = mv_bits;
                        not_found_best = 0;
                    }
                }
            }
        }

        if (not_found_best == faststep)
        {
            break;
        }

        if(bi)
        {
            break;
        }

        step <<= 1;

        if (step > pi->max_search_range)
        {
            break;
        }
    }

    /* set best MV */
    mv[MV_X] = ((mv_best_x - x) << 2);
    mv[MV_Y] = ((mv_best_y - y) << 2);

    if(!bi && best_mv_bits > 0 )
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 me_spel_pattern(EVEYE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int bit_depth_luma)
{
    pel  * org, * ref, * pred;
    s16  * org_bi;
    u32    cost, cost_best = EVEY_UINT32_MAX;
    s16    mv_x, mv_y, cx, cy;
    int    lidx_r = (lidx == LIST_0) ? LIST_1 : LIST_0;
    int    i, mv_bits, cuw, cuh, s_ref, best_mv_bits;

    org = pi->o_y;
    s_ref = pi->refp[refi][lidx].pic->s_l;
    ref = pi->refp[refi][lidx].pic->y;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    org_bi = pi->org_bi;
    pred = pi->pred_buf;
    best_mv_bits = 0;

    /* make MV to be global coordinate */
    cx = mvi[MV_X] + (x << 2);
    cy = mvi[MV_Y] + (y << 2);

    /* intial value */
    mv[MV_X] = mvi[MV_X];
    mv[MV_Y] = mvi[MV_Y];

    /* search upto hpel-level from here */
    /* search of large diamond pattern */
    for(i = 0; i < HALF_PEL_SEARCH_PATTERN_CNT; i++)
    {
        mv_x = cx + tbl_search_pattern_hpel_partial[i][MV_X];
        mv_y = cy + tbl_search_pattern_hpel_partial[i][MV_Y];

        /* get MVD bits */
        mv_bits = get_mv_bits(mv_x - gmvp[MV_X], mv_y - gmvp[MV_Y], pi->num_refp, refi);

        if(bi)
        {
            mv_bits += pi->mot_bits[lidx_r];
        }

        /* get MVD cost_best */
        cost = MV_COST(pi, mv_bits);

        /* get the interpolated(predicted) image */
        evey_mc_l((mv_x << 2), (mv_y << 2), ref, (mv_x << 2), (mv_y << 2), s_ref, cuw, pred, cuw, cuh, bit_depth_luma);

        if(bi)
        {
            /* get sad */
            cost += eveye_sad_bi_16b(log2_cuw, log2_cuh, org_bi, pred, cuw, cuw, bit_depth_luma);
        }
        else
        {
            /* get sad */
            cost += eveye_sad_16b(log2_cuw, log2_cuh, org, pred, cuw, cuw, bit_depth_luma);
        }

        /* check if motion cost_best is less than minimum cost_best */
        if(cost < cost_best)
        {
            mv[MV_X] = mv_x - (x << 2);
            mv[MV_Y] = mv_y - (y << 2);
            cost_best = cost;
        }
    }

    /* search upto qpel-level from here*/
    /* search of small diamond pattern */
    if(pi->me_level > ME_LEV_HPEL)
    {
        /* make MV to be absolute coordinate */
        cx = mv[MV_X] + (x << 2);
        cy = mv[MV_Y] + (y << 2);

        for(i = 0; i < QUARTER_PEL_SEARCH_PATTERN_CNT; i++)
        {
            mv_x = cx + tbl_search_pattern_qpel_8point[i][MV_X];
            mv_y = cy + tbl_search_pattern_qpel_8point[i][MV_Y];

            /* get MVD bits */
            mv_bits = get_mv_bits(mv_x - gmvp[MV_X], mv_y - gmvp[MV_Y], pi->num_refp, refi);

            if(bi)
            {
                mv_bits += pi->mot_bits[lidx_r];
            }

            /* get MVD cost_best */
            cost = MV_COST(pi, mv_bits);

            /* get the interpolated(predicted) image */
            evey_mc_l((mv_x << 2), (mv_y << 2), ref, (mv_x << 2), (mv_y << 2), s_ref, cuw, pred, cuw, cuh, bit_depth_luma);

            if(bi)
            {
                /* get sad */
                cost += eveye_sad_bi_16b(log2_cuw, log2_cuh, org_bi, pred, cuw, cuw, bit_depth_luma);
            }
            else
            {
                /* get sad */
                cost += eveye_sad_16b(log2_cuw, log2_cuh, org, pred, cuw, cuw, bit_depth_luma);
            }

            /* check if motion cost_best is less than minimum cost_best */
            if(cost < cost_best)
            {
                mv[MV_X] = mv_x - (x << 2);
                mv[MV_Y] = mv_y - (y << 2);
                cost_best = cost;
                best_mv_bits = mv_bits;
            }
        }
    }

    if(!bi && best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 pinter_me_epzs(EVEYE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 * refi, int lidx, s16 mvp[MV_D], s16 mv[MV_D], int bi, int bit_depth_luma)
{
    s16 mvc[MV_D];  /* MV center for search */
    s16 gmvp[MV_D]; /* MVP in frame cordinate */
    s16 range[MV_RANGE_DIM][MV_D]; /* search range after clipping */
    s16 mvi[MV_D];
    s16 mvt[MV_D];
    u32 cost, cost_best = EVEY_UINT32_MAX;
    s8  ri = 0;     /* reference buffer index */
    int tmpstep = 0;
    int beststep = 0;

    gmvp[MV_X] = mvp[MV_X] + (x << 2);
    gmvp[MV_Y] = mvp[MV_Y] + (y << 2);

    if(bi)
    {
        mvi[MV_X] = mv[MV_X] + (x << 2);
        mvi[MV_Y] = mv[MV_Y] + (y << 2);
        mvc[MV_X] = x + (mv[MV_X] >> 2);
        mvc[MV_Y] = y + (mv[MV_Y] >> 2);
    }
    else
    {
        mvi[MV_X] = mvp[MV_X] + (x << 2);
        mvi[MV_Y] = mvp[MV_Y] + (y << 2);
        mvc[MV_X] = x + (mvp[MV_X] >> 2);
        mvc[MV_Y] = y + (mvp[MV_Y] >> 2);
    }

    ri = *refi;

    mvc[MV_X] = EVEY_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X]);
    mvc[MV_Y] = EVEY_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y]);

    get_range_ipel(pi, mvc, range, bi, ri, lidx);

    cost = me_ipel_diamond(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_FIRST_SEARCH_STEP, bit_depth_luma);
    if(cost < cost_best)
    {
        cost_best = cost;
        mv[MV_X] = mvt[MV_X];
        mv[MV_Y] = mvt[MV_Y];
        if(abs(mvp[MV_X] - mv[MV_X]) < 2 && abs(mvp[MV_Y] - mv[MV_Y]) < 2)
        {
            beststep = 0;
        }
        else
        {
            beststep = tmpstep;
        }
    }

    if(!bi && beststep > RASTER_SEARCH_THD)
    {
        cost = me_raster(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvt, bit_depth_luma);
        if(cost < cost_best)
        {
            beststep = RASTER_SEARCH_THD;

            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];
        }
    }

    while (!bi && beststep > REFINE_SEARCH_THD)
    {
        mvc[MV_X] = x + (mv[MV_X] >> 2);
        mvc[MV_Y] = y + (mv[MV_Y] >> 2);

        get_range_ipel(pi, mvc, range, bi, ri, lidx);

        mvi[MV_X] = mv[MV_X] + (x << 2);
        mvi[MV_Y] = mv[MV_Y] + (y << 2);

        beststep = 0;
        cost = me_ipel_diamond(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_REFINE_SEARCH_STEP, bit_depth_luma);
        if(cost < cost_best)
        {
            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];

            if (abs(mvp[MV_X] - mv[MV_X]) < 2 && abs(mvp[MV_Y] - mv[MV_Y]) < 2)
            {
                beststep = 0;
            }
            else
            {
                beststep = tmpstep;
            }

        }
    }

    if(pi->me_level > ME_LEV_IPEL)
    {
        /* sub-pel ME */
        cost = me_spel_pattern(pi, x, y, log2_cuw, log2_cuh, ri, lidx, gmvp, mv, mvt, bi, bit_depth_luma);

        if(cost < cost_best)
        {
            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];
        }
    }
    else
    {
        mvc[MV_X] = x + (mv[MV_X] >> 2);
        mvc[MV_Y] = y + (mv[MV_Y] >> 2);

        get_range_ipel(pi, mvc, range, bi, ri, lidx);

        mvi[MV_X] = mv[MV_X] + (x << 2);
        mvi[MV_Y] = mv[MV_Y] + (y << 2);
        cost = me_ipel_refinement(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_REFINE_SEARCH_STEP, bit_depth_luma);
        if (cost < cost_best)
        {
            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];
        }
    }

    return cost_best;
}

void copy_tu_from_cu(s16 tu_resi[N_C][MAX_CU_DIM], s16 cu_resi[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, int chroma_format_idc)
{
    int j;
    int cuw = 1 << log2_cuw;
    int log2_tuw, log2_tuh;
    int tuw, tuh;

    log2_tuw = log2_cuw;
    log2_tuh = log2_cuh;
    tuw = 1 << log2_tuw;
    tuh = 1 << log2_tuh;

    /* Y */
    for (j = 0; j < tuh; j++)
    {
        evey_mcpy(tu_resi[Y_C] + j * tuw, cu_resi[Y_C] + j * cuw, sizeof(s16) * tuw);
    }

    /* UV */
    tuw >>= (GET_CHROMA_W_SHIFT(chroma_format_idc));;
    tuh >>= (GET_CHROMA_H_SHIFT(chroma_format_idc));;
    cuw >>= (GET_CHROMA_W_SHIFT(chroma_format_idc));;

    if(chroma_format_idc)
    {
        for(j = 0; j < tuh; j++)
        {
            evey_mcpy(tu_resi[U_C] + j * tuw, cu_resi[U_C] + j * cuw, sizeof(s16) * tuw);
            evey_mcpy(tu_resi[V_C] + j * tuw, cu_resi[V_C] + j * cuw, sizeof(s16) * tuw);
        }
    }
}

/* get original dummy buffer for bi prediction */
static void get_org_bi(pel * org, pel * pred, int s_o, int cuw, int cuh, s16 * org_bi)
{
    int i, j;

    for(j = 0; j < cuh; j++)
    {
        for(i = 0; i < cuw; i++)
        {
            org_bi[i] = ((s16)(org[i]) << 1) - (s16)pred[i];
        }

        org += s_o;
        pred += cuw;
        org_bi += cuw;
    }
}

static void check_best_mvp(EVEYE_CTX * ctx, EVEYE_CORE * core, s32 slice_type, s8 refi[LIST_NUM],
                           int lidx, int pidx, s16(*mvp)[2], s16 * mv, s16 * mvd, u8 * mvp_idx)
{
    double cost, best_cost = MAX_COST;
    int    idx, best_idx;
    u32    bit_cnt;
    s16    mvd_tmp[LIST_NUM][MV_D];

    best_idx = *mvp_idx;

    for(idx = 0; idx < EVEY_MVP_NUM; idx++)
    {
        if(idx)
        {
            /* encoder side pruning */
            if(mvp[idx][MV_X] == mvp[best_idx][MV_X] &&
               mvp[idx][MV_Y] == mvp[best_idx][MV_Y])
            {
                continue;
            }
        }

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);
        eveye_sbac_bit_reset(&core->s_temp_run);

        mvd_tmp[lidx][MV_X] = mv[MV_X] - mvp[idx][MV_X];
        mvd_tmp[lidx][MV_Y] = mv[MV_Y] - mvp[idx][MV_Y];

        eveye_rdo_bit_cnt_mvp(ctx, core, refi, mvd_tmp, pidx, idx);
        bit_cnt = eveye_get_bit_number(&core->s_temp_run);
        cost = RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);
        if(cost < best_cost)
        {
            best_idx = idx;
            best_cost = cost;
        }
    }

    *mvp_idx = best_idx;
    mvd[MV_X] = mv[MV_X] - mvp[*mvp_idx][MV_X];
    mvd[MV_Y] = mv[MV_Y] - mvp[*mvp_idx][MV_Y];
}

static double pinter_residue_rdo(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, pel pred[2][N_C][MAX_CU_DIM], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 * mvp_idx)
{
    EVEYE_PINTER * pi = &ctx->pinter;
    int            chroma_format_idc = ctx->sps.chroma_format_idc;
    int            w_shift = (GET_CHROMA_W_SHIFT(chroma_format_idc));
    int            h_shift = (GET_CHROMA_H_SHIFT(chroma_format_idc));
    int            log2_cuw = core->log2_cuw;
    int            log2_cuh = core->log2_cuh;
    int          * nnz, tnnz, w[N_C], h[N_C], log2_w[N_C], log2_h[N_C];
    int            cuw;
    int            cuh;
    pel         (* rec)[MAX_CU_DIM];
    s64            dist[2][N_C];
    double         cost, cost_best = MAX_COST;
    int            cbf_idx[N_C], nnz_store[N_C];
    int            nnz_sub_store[N_C][MAX_SUB_TB_NUM] = {{0},};
    int            bit_cnt;
    int            i, idx_y, idx_u, idx_v;
    double         cost_comp_best = MAX_COST;
    int            idx_best[N_C] = {0, };
    int            j;
    u8             is_from_mv_field = 0;
    s64            dist_no_resi[N_C];
    int            nnz_best[N_C] = { -1, -1, -1 };
    s64            dist_idx = -1;

    rec = pi->rec[pidx];
    nnz = core->nnz;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    w[Y_C] = 1 << log2_cuw;
    h[Y_C] = 1 << log2_cuh;
    log2_w[Y_C] = log2_cuw;
    log2_h[Y_C] = log2_cuh;    
    w[U_C] = w[V_C] = 1 << (log2_cuw - w_shift);
    h[U_C] = h[V_C] = 1 << (log2_cuh - h_shift);
    log2_w[U_C] = log2_w[V_C] = log2_cuw - w_shift;
    log2_h[U_C] = log2_h[V_C] = log2_cuh - h_shift;

    /* motion compensation */
    evey_inter_pred(ctx, x, y, w[0], h[0], pi->refi[pidx], pi->mv[pidx], pi->refp, pred);

    int bit_depth_tbl[3] = {ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8};

    /* get residual */
    eveye_diff_pred(ctx, log2_cuw, log2_cuh, core->org, pred[0], pi->resi);

    for (i = 0; i < N_C; i++)
    {
        if(!chroma_format_idc && i != 0)
        {
            dist[0][i] = 0;
        }
        else
        {
            dist[0][i] = eveye_ssd_16b(log2_w[i], log2_h[i], pred[0][i], core->org[i], w[i], w[i], bit_depth_tbl[i]);
        }
        dist_no_resi[i] = dist[0][i];
    }

    /* prepare tu residual */
    copy_tu_from_cu(coef, pi->resi, log2_cuw, log2_cuh, chroma_format_idc);

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        eveye_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp);
    }

    /* transform and quantization */
    tnnz = eveye_sub_block_tq(ctx, core, coef, 0, RUN_L | RUN_CB | RUN_CR);

    if(tnnz)
    {
        for(i = 0; i < N_C; i++)
        {
            if(i != 0 && !chroma_format_idc)
            {
                cbf_idx[i] = 0;
                nnz_store[i] = nnz[i];
                evey_mcpy(nnz_sub_store[i], core->nnz_sub[i], sizeof(int) * MAX_SUB_TB_NUM);
                continue;
            }
            int size = (cuw * cuh) >> (i == 0 ? 0 : (h_shift + w_shift));
            evey_mcpy(core->coef[i], coef[i], sizeof(s16) * size);

            cbf_idx[i] = 0;
            nnz_store[i] = nnz[i];
            evey_mcpy(nnz_sub_store[i], core->nnz_sub[i], sizeof(int) * MAX_SUB_TB_NUM);
        }

        evey_sub_block_itdq(ctx, core, core->coef, core->nnz_sub);

        if(ctx->cdsc.rdo_dbk_switch)
        {
            eveye_calc_delta_dist_filter_boundary(ctx, core, pred[0], cuw, x, y, 0, 0, pi->refi[pidx], pi->mv[pidx], is_from_mv_field);
        }

        for(i = 0; i < N_C; i++)
        {
            evey_recon(core->coef[i], pred[0][i], nnz[i], w[i], h[i], w[i], rec[i], ctx->sps.bit_depth_luma_minus8 + 8);
            if(nnz[i])
            {                
                if(!chroma_format_idc && i != 0)
                {
                    dist[1][i] = 0;
                }
                else
                {
                    dist[1][i] = eveye_ssd_16b(log2_w[i], log2_h[i], rec[i], core->org[i], w[i], w[i], bit_depth_tbl[i]);
                }
            }
            else
            {
                dist[1][i] = dist_no_resi[i];
            }

            if(ctx->cdsc.rdo_dbk_switch)
            {
                dist[0][i] += core->delta_dist[i];
            }
        }

        if(ctx->cdsc.rdo_dbk_switch)
        {
            /* filter rec and calculate ssd */
            eveye_calc_delta_dist_filter_boundary(ctx, core, rec, cuw, x, y, 0, nnz[Y_C] != 0, pi->refi[pidx], pi->mv[pidx], is_from_mv_field);
            for(i = 0; i < N_C; i++)
            {
                dist[1][i] += core->delta_dist[i];
                if(i != 0 && !chroma_format_idc)
                {
                    dist[1][i] = 0;
                }
            }
        }

        if(pidx != PRED_DIR)
        {
            /* test all zero case */
            idx_y = 0;
            idx_u = 0;
            idx_v = 0;
            nnz[Y_C] = 0;
            nnz[U_C] = 0;
            nnz[V_C] = 0;
            evey_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);

            cost = (double)dist[idx_y][Y_C] + (((double)dist[idx_u][U_C] * ctx->dist_chroma_weight[0]) + ((double)dist[idx_v][V_C] * ctx->dist_chroma_weight[1]));

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
            eveye_sbac_bit_reset(&core->s_temp_run);

            eveye_rdo_bit_cnt_cu_inter(ctx, core, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx);

            bit_cnt = eveye_get_bit_number(&core->s_temp_run);
            cost += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);

            if(cost < cost_best)
            {
                cost_best = cost;
                cbf_idx[Y_C] = idx_y;
                cbf_idx[U_C] = idx_u;
                cbf_idx[V_C] = idx_v;
                SBAC_STORE(core->s_temp_best, core->s_temp_run);
                DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
            }
        } /* forced zero */

        /* test as it is */
        idx_y = nnz_store[Y_C] > 0 ? 1 : 0;
        idx_u = nnz_store[U_C] > 0 ? 1 : 0;
        idx_v = nnz_store[V_C] > 0 ? 1 : 0;
        nnz[Y_C] = nnz_store[Y_C];
        nnz[U_C] = nnz_store[U_C];
        nnz[V_C] = nnz_store[V_C];
        evey_mcpy(core->nnz_sub, nnz_sub_store, sizeof(int) * N_C * MAX_SUB_TB_NUM);

        cost = (double)dist[idx_y][Y_C] + (((double)dist[idx_u][U_C] * ctx->dist_chroma_weight[0]) + ((double)dist[idx_v][V_C] * ctx->dist_chroma_weight[1]));

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

        eveye_sbac_bit_reset(&core->s_temp_run);

        eveye_rdo_bit_cnt_cu_inter(ctx, core, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx);

        bit_cnt = eveye_get_bit_number(&core->s_temp_run);
        cost += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);

        if(cost < cost_best)
        {
            cost_best = cost;
            cbf_idx[Y_C] = idx_y;
            cbf_idx[U_C] = idx_u;
            cbf_idx[V_C] = idx_v;
            SBAC_STORE(core->s_temp_best, core->s_temp_run);
            DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
        }

        SBAC_LOAD(core->s_temp_prev_comp_best, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        /* cbf test for each component */
        for(i = 0; i < N_C; i++)
        {
            if(nnz_store[i] > 0)
            {
                cost_comp_best = MAX_COST;
                SBAC_LOAD(core->s_temp_prev_comp_run, core->s_temp_prev_comp_best);
                for(j = 0; j < 2; j++)
                {
                    cost = dist[j][i] * (i == 0 ? 1 : ctx->dist_chroma_weight[i - 1]);
                    nnz[i] = j ? nnz_store[i] : 0;
                    if(j)
                    {
                        evey_mcpy(core->nnz_sub[i], nnz_sub_store[i], sizeof(int) * MAX_SUB_TB_NUM);
                    }
                    else
                    {
                        evey_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                    }

                    SBAC_LOAD(core->s_temp_run, core->s_temp_prev_comp_run);
                    eveye_sbac_bit_reset(&core->s_temp_run);
                    eveye_rdo_bit_cnt_cu_inter_comp(ctx, core, coef, i, pidx);
                    bit_cnt = eveye_get_bit_number(&core->s_temp_run);
                    cost += RATE_TO_COST_LAMBDA(ctx->lambda[i], bit_cnt);
                    if(cost < cost_comp_best)
                    {
                        cost_comp_best = cost;
                        idx_best[i] = j;
                        SBAC_STORE(core->s_temp_prev_comp_best, core->s_temp_run);
                    }
                }
            }
            else
            {
                idx_best[i] = 0;
            }
        }

        if(idx_best[Y_C] != 0 || idx_best[U_C] != 0 || idx_best[V_C] != 0)
        {
            idx_y = idx_best[Y_C];
            idx_u = idx_best[U_C];
            idx_v = idx_best[V_C];
            nnz[Y_C] = idx_y ? nnz_store[Y_C] : 0;
            nnz[U_C] = idx_u ? nnz_store[U_C] : 0;
            nnz[V_C] = idx_v ? nnz_store[V_C] : 0;
            for(i = 0; i < N_C; i++)
            {
                if(idx_best[i])
                {
                    evey_mcpy(core->nnz_sub[i], nnz_sub_store[i], sizeof(int) * MAX_SUB_TB_NUM);
                }
                else
                {
                    evey_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                }
            }
        }

        if(nnz[Y_C] != nnz_store[Y_C] || nnz[U_C] != nnz_store[U_C] || nnz[V_C] != nnz_store[V_C])
        {
            cost = (double)dist[idx_y][Y_C] + (((double)dist[idx_u][U_C] * ctx->dist_chroma_weight[0]) + ((double)dist[idx_v][V_C] * ctx->dist_chroma_weight[1]));

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

            eveye_sbac_bit_reset(&core->s_temp_run);

            eveye_rdo_bit_cnt_cu_inter(ctx, core, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx);

            bit_cnt = eveye_get_bit_number(&core->s_temp_run);
            cost += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);

            if(cost < cost_best)
            {
                cost_best = cost;
                cbf_idx[Y_C] = idx_y;
                cbf_idx[U_C] = idx_u;
                cbf_idx[V_C] = idx_v;
                SBAC_STORE(core->s_temp_best, core->s_temp_run);
                DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
            }
        }

        for(i = 0; i < N_C; i++)
        {
            nnz[i] = (cbf_idx[i] ? nnz_store[i] : 0);
            if(cbf_idx[i])
            {
                evey_mcpy(core->nnz_sub[i], nnz_sub_store[i], sizeof(int) * MAX_SUB_TB_NUM);
            }
            else
            {
                evey_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
            }
            if(nnz[i] == 0 && nnz_store[i] != 0)
            {
                evey_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                evey_mset(coef[i], 0, sizeof(s16) * ((cuw * cuh) >> (i == 0 ? 0 : (h_shift + w_shift))));
            }
        }
    }
    else /*  No residual case */
    {
        if(ctx->pps.cu_qp_delta_enabled_flag)
        {
            eveye_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
        }

        for(i = 0; i < N_C; i++)
        {
            nnz[i] = 0;
            evey_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
        }

        if(ctx->cdsc.rdo_dbk_switch)
        {
            eveye_calc_delta_dist_filter_boundary(ctx, core, pred[0], cuw, x, y, 0, 0, pi->refi[pidx], pi->mv[pidx], is_from_mv_field);
        }

        for(i = 0; i < N_C; i++)
        {
            dist[0][i] = dist_no_resi[i];

            if(ctx->cdsc.rdo_dbk_switch)
            {
                dist[0][i] += core->delta_dist[i];
            }

            if(i != 0 && !chroma_format_idc)
            {
                dist[0][i] = 0;
            }
        }
        cost_best = (double)dist[0][Y_C] + (ctx->dist_chroma_weight[0] * (double)dist[0][U_C]) + (ctx->dist_chroma_weight[1] * (double)dist[0][V_C]);

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

        eveye_sbac_bit_reset(&core->s_temp_run);
        eveye_rdo_bit_cnt_cu_inter(ctx, core, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx);

        bit_cnt = eveye_get_bit_number(&core->s_temp_run);
        cost_best += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);
        SBAC_STORE(core->s_temp_best, core->s_temp_run);
        DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);

        nnz_best[Y_C] = nnz_best[U_C] = nnz_best[V_C] = 0;
    }

    for(i = 0; i < N_C; i++)
    {
        evey_recon(core->coef[i], pred[0][i], nnz[i], w[i], h[i], w[i], rec[i], ctx->sps.bit_depth_luma_minus8 + 8);
    }

    return cost_best;
}

static double analyze_skip_mode(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, double * cost_inter)
{
    EVEYE_PINTER * pi = &ctx->pinter;
    int            chroma_format_idc = ctx->sps.chroma_format_idc;
    int            w_shift = (GET_CHROMA_W_SHIFT(chroma_format_idc));
    int            h_shift = (GET_CHROMA_H_SHIFT(chroma_format_idc));
    int            log2_cuw = core->log2_cuw;
    int            log2_cuh = core->log2_cuh;
    int            cuw, cuh, idx0, idx1, cnt, bit_cnt;
    s16            mvp[LIST_NUM][MV_D];
    s8             refi[LIST_NUM];
    double         cost, cost_best = MAX_COST;    
    s64            cy, cu, cv;   

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        eveye_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
    }

    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);

    cu = cv = cy = 0;    

    evey_get_motion(ctx, core, LIST_0, pi->refi_pred[LIST_0], pi->mvp[LIST_0]);
    if(ctx->sh.slice_type == SLICE_B)
    {
        evey_get_motion(ctx, core, LIST_1, pi->refi_pred[LIST_1], pi->mvp[LIST_1]);
    }

    pi->mvp_idx[PRED_SKIP][LIST_0] = 0;
    pi->mvp_idx[PRED_SKIP][LIST_1] = 0;

    for(idx0 = 0; idx0 < 4; idx0++)
    {
        if(idx0)
        {
            /* encoder side pruning */
            if(pi->mv[PRED_SKIP][LIST_0][MV_X] == pi->mvp[LIST_0][idx0][MV_X] &&
               pi->mv[PRED_SKIP][LIST_0][MV_Y] == pi->mvp[LIST_0][idx0][MV_Y])
            {
                continue;
            }
        }
        cnt = (ctx->sh.slice_type == SLICE_B ? 4 : 1);
        for(idx1 = 0; idx1 < cnt; idx1++)
        {
            if(idx1)
            {
                /* encoder side pruning */
                if(pi->mv[PRED_SKIP][LIST_1][MV_X] == pi->mvp[LIST_1][idx1][MV_X] &&
                   pi->mv[PRED_SKIP][LIST_1][MV_Y] == pi->mvp[LIST_1][idx1][MV_Y])
                {
                    continue;
                }
            }
            mvp[LIST_0][MV_X] = pi->mvp[LIST_0][idx0][MV_X];
            mvp[LIST_0][MV_Y] = pi->mvp[LIST_0][idx0][MV_Y];
            mvp[LIST_1][MV_X] = pi->mvp[LIST_1][idx1][MV_X];
            mvp[LIST_1][MV_Y] = pi->mvp[LIST_1][idx1][MV_Y];

            SET_REFI(refi, pi->refi_pred[LIST_0][idx0], ctx->sh.slice_type == SLICE_B ? pi->refi_pred[LIST_1][idx1] : REFI_INVALID);
            if(!REFI_IS_VALID(refi[LIST_0]) && !REFI_IS_VALID(refi[LIST_1]))
            {
                continue;
            }

            /* motion compensation */
            evey_inter_pred(ctx, x, y, cuw, cuh, refi, mvp, pi->refp, pi->pred[PRED_NUM]);

            cy = eveye_ssd_16b(log2_cuw, log2_cuh, pi->pred[PRED_NUM][0][Y_C], core->org[Y_C], cuw, cuw, ctx->sps.bit_depth_luma_minus8 + 8);

            if(chroma_format_idc)
            {
                cu = eveye_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][U_C], core->org[U_C], cuw >> w_shift
                                  , cuw >> w_shift, ctx->sps.bit_depth_chroma_minus8 + 8);

                cv = eveye_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][V_C], core->org[V_C], cuw >> w_shift
                                  , cuw >> w_shift, ctx->sps.bit_depth_chroma_minus8 + 8);
            }

            if(ctx->cdsc.rdo_dbk_switch)
            {
                eveye_calc_delta_dist_filter_boundary(ctx, core, pi->pred[PRED_NUM][0], cuw, x, y, 0, 0, refi, mvp, 0);
                cy += core->delta_dist[Y_C];
                if(chroma_format_idc)
                {
                    cu += core->delta_dist[U_C];
                    cv += core->delta_dist[V_C];
                }
            }

            cost = (double)cy + (ctx->dist_chroma_weight[0] * (double)cu) + (ctx->dist_chroma_weight[1] * (double)cv);

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
            eveye_sbac_bit_reset(&core->s_temp_run);

            eveye_rdo_bit_cnt_cu_skip(ctx, core, idx0, idx1, 0);

            bit_cnt = eveye_get_bit_number(&core->s_temp_run);
            cost += RATE_TO_COST_LAMBDA(ctx->lambda[0], bit_cnt);

            if(cost < cost_best)
            {
                int j;
                cost_best = cost;
                pi->mvp_idx[PRED_SKIP][LIST_0] = idx0;
                pi->mvp_idx[PRED_SKIP][LIST_1] = idx1;
                pi->mv[PRED_SKIP][LIST_0][MV_X] = mvp[LIST_0][MV_X];
                pi->mv[PRED_SKIP][LIST_0][MV_Y] = mvp[LIST_0][MV_Y];
                pi->mv[PRED_SKIP][LIST_1][MV_X] = mvp[LIST_1][MV_X];
                pi->mv[PRED_SKIP][LIST_1][MV_Y] = mvp[LIST_1][MV_Y];
                pi->mvd[PRED_SKIP][LIST_0][MV_X] = 0;
                pi->mvd[PRED_SKIP][LIST_0][MV_Y] = 0;
                pi->mvd[PRED_SKIP][LIST_1][MV_X] = 0;
                pi->mvd[PRED_SKIP][LIST_1][MV_Y] = 0;
                pi->refi[PRED_SKIP][LIST_0] = refi[LIST_0];
                pi->refi[PRED_SKIP][LIST_1] = refi[LIST_1];

                for(j = 0; j < N_C; j++)
                {
                    if(j != 0 && !ctx->sps.chroma_format_idc)
                    {
                        continue;
                    }
                    int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (h_shift + w_shift));
                    evey_mcpy(pi->pred[PRED_SKIP][0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                    evey_mcpy(pi->rec[PRED_SKIP][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                }

                SBAC_STORE(core->s_temp_best, core->s_temp_run);
                DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
            }
        }
    }

    if(cost_best < core->cost_best)
    {
        core->pred_mode = MODE_SKIP;
        pi->best_idx = PRED_SKIP;
        core->cost_best = cost_best;
        cost_inter[pi->best_idx] = cost_best;
        SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
        DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
        evey_mset(pi->nnz_best[PRED_SKIP], 0, sizeof(int) * N_C);
        evey_mcpy(pi->nnz_sub_best[PRED_SKIP], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);

        /* save all CU inforamtion of the best inter mode */
        EVEYE_MODE * mi = &ctx->mode;
        mi->rec = pi->rec[pi->best_idx];
        mi->coef = pi->coef[pi->best_idx];
        mi->nnz = pi->nnz_best[pi->best_idx];
        mi->nnz_sub = pi->nnz_sub_best[pi->best_idx];
        mi->refi = pi->refi[pi->best_idx];
        mi->mvp_idx = pi->mvp_idx[pi->best_idx];
        mi->mv = pi->mv[pi->best_idx];
        mi->mvd = pi->mvd[pi->best_idx];
        pi->pred_y_best = pi->pred[pi->best_idx][0][Y_C];
    }

    return cost_best;
}

static double analyze_direct_mode(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, double * cost_inter)
{
    if(ctx->sh.slice_type != SLICE_B)
    {
        return MAX_COST;
    }

    EVEYE_PINTER * pi = &ctx->pinter;
    int            log2_cuw = core->log2_cuw;
    int            log2_cuh = core->log2_cuh;
    double         cost;
    int            pidx;
    s8             refidx = 0;

    pidx = PRED_DIR;
    evey_get_mv_dir(ctx, core, pi->mv[pidx]);

    pi->mvd[pidx][LIST_0][MV_X] = 0;
    pi->mvd[pidx][LIST_0][MV_Y] = 0;
    pi->mvd[pidx][LIST_1][MV_X] = 0;
    pi->mvd[pidx][LIST_1][MV_Y] = 0;

    SET_REFI(pi->refi[pidx], 0, 0);

    cost = pinter_residue_rdo(ctx, core, x, y, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx]);

    evey_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);

    if(cost < core->cost_best)
    {
        core->pred_mode = MODE_DIR;
        pi->best_idx = PRED_DIR;
        core->cost_best = cost;
        cost_inter[pi->best_idx] = cost;
        SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
        DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
        evey_mcpy(pi->nnz_sub_best[PRED_DIR], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);

        /* save all CU inforamtion of the best inter mode */
        EVEYE_MODE * mi = &ctx->mode;
        mi->rec = pi->rec[pi->best_idx];
        mi->coef = pi->coef[pi->best_idx];
        mi->nnz = pi->nnz_best[pi->best_idx];
        mi->nnz_sub = pi->nnz_sub_best[pi->best_idx];
        mi->refi = pi->refi[pi->best_idx];
        mi->mvp_idx = pi->mvp_idx[pi->best_idx];
        mi->mv = pi->mv[pi->best_idx];
        mi->mvd = pi->mvd[pi->best_idx];
        pi->pred_y_best = pi->pred[pi->best_idx][0][Y_C];
    }

    return cost;
}

static double analyze_inter_mode(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y, double * cost_inter)
{
    EVEYE_PINTER * pi = &ctx->pinter;
    int            log2_cuw = core->log2_cuw;
    int            log2_cuh = core->log2_cuh;
    s8             refi[LIST_NUM] = {REFI_INVALID, REFI_INVALID};
    s8             refi_cur;
    s8             refi_best = 0;
    s8             t0, t1;
    u8             mvp_idx = 0;
    int            cuw = 1 << log2_cuw;
    int            cuh = 1 << log2_cuh;
    int            lidx_ref, lidx_cnd;
    int            changed = 0;    
    int            pidx, pidx_ref, pidx_cnd, i;
    u32            best_mecost = EVEY_UINT32_MAX;
    u32            mecost;
    double         cost;
    double         cost_best = MAX_COST;

    /* uni-directional prediction (PRED_L0 or PRED_L1) */
    for(int lidx = 0; lidx <= ((ctx->sh.slice_type == SLICE_P) ? PRED_L0 : PRED_L1); lidx++)
    {
        s8 refi_temp = 0;
        pidx = lidx;

        pi->num_refp = ctx->dpbm.num_refp[lidx];

        best_mecost = EVEY_UINT32_MAX;

        for(refi_cur = 0; refi_cur < pi->num_refp; refi_cur++)
        {
            evey_get_motion(ctx, core, lidx, pi->refi_pred[lidx], pi->mvp_scale[lidx][refi_cur]);
            mvp_idx = pi->mvp_idx[PRED_SKIP][lidx];

            /* motion estimation */
            mecost = pi->fn_me(pi, x, y, log2_cuw, log2_cuh, &refi_cur, lidx, pi->mvp_scale[lidx][refi_cur][mvp_idx], pi->mv[pidx][lidx], 0, ctx->sps.bit_depth_luma_minus8 + 8);

            pi->mv_scale[lidx][refi_cur][MV_X] = pi->mv[pidx][lidx][MV_X];
            pi->mv_scale[lidx][refi_cur][MV_Y] = pi->mv[pidx][lidx][MV_Y];
            if(mecost < best_mecost)
            {
                best_mecost = mecost;
                refi_temp = refi_cur;
            }
        }

        refi_cur = refi_temp;
        pi->mv[pidx][lidx][MV_X] = pi->mv_scale[lidx][refi_cur][MV_X];
        pi->mv[pidx][lidx][MV_Y] = pi->mv_scale[lidx][refi_cur][MV_Y];

        t0 = (lidx == 0) ? refi_cur : REFI_INVALID;
        t1 = (lidx == 1) ? refi_cur : REFI_INVALID;
        SET_REFI(pi->refi[pidx], t0, t1);

        pi->mvd[pidx][lidx][MV_X] = pi->mv[pidx][lidx][MV_X] - pi->mvp_scale[lidx][refi_cur][mvp_idx][MV_X];
        pi->mvd[pidx][lidx][MV_Y] = pi->mv[pidx][lidx][MV_Y] - pi->mvp_scale[lidx][refi_cur][mvp_idx][MV_Y];

        check_best_mvp(ctx, core, ctx->sh.slice_type, pi->refi[pidx], lidx, pidx, pi->mvp_scale[lidx][refi_cur], pi->mv[pidx][lidx], pi->mvd[pidx][lidx], &mvp_idx);

        pi->mvp_idx[pidx][lidx] = mvp_idx;

        cost = cost_inter[pidx] = pinter_residue_rdo(ctx, core, x, y, pi->pred[PRED_NUM], pi->coef[PRED_NUM], pidx, pi->mvp_idx[pidx]);

        if(cost < core->cost_best)
        {
            core->pred_mode = MODE_INTER;
            pi->best_idx = pidx;
            pi->mvp_idx[pi->best_idx][lidx] = mvp_idx;
            cost_best = cost;
            core->cost_best = cost;            
            cost_inter[pi->best_idx] = cost;
            SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
            DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);

            for(int j = 0; j < N_C; j++)
            {
                if(j != 0 && !ctx->sps.chroma_format_idc)
                {
                    continue;
                }
                int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc) + GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc)));
                pi->nnz_best[pidx][j] = core->nnz[j];
                evey_mcpy(pi->nnz_sub_best[pidx][j], core->nnz_sub[j], sizeof(int) * MAX_SUB_TB_NUM);
                evey_mcpy(pi->pred[pidx][0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                evey_mcpy(pi->coef[pidx][j], pi->coef[PRED_NUM][j], size_tmp * sizeof(s16));
            }

            /* save all CU inforamtion of the best inter mode */
            EVEYE_MODE * mi = &ctx->mode;
            mi->rec = pi->rec[pi->best_idx];
            mi->coef = pi->coef[pi->best_idx];
            mi->nnz = pi->nnz_best[pi->best_idx];
            mi->nnz_sub = pi->nnz_sub_best[pi->best_idx];
            mi->refi = pi->refi[pi->best_idx];
            mi->mvp_idx = pi->mvp_idx[pi->best_idx];
            mi->mv = pi->mv[pi->best_idx];
            mi->mvd = pi->mvd[pi->best_idx];
            pi->pred_y_best = pi->pred[pi->best_idx][0][Y_C];
        }
    }

    /* bi-directional prediction (PRED_BI) */
    if(ctx->sh.slice_type == SLICE_B)
    {        
        best_mecost = EVEY_UINT32_MAX;
        pidx = PRED_BI;

        if(cost_inter[PRED_L0] <= cost_inter[PRED_L1])
        {
            lidx_ref = LIST_0;
            lidx_cnd = LIST_1;
            pidx_ref = PRED_L0;
            pidx_cnd = PRED_L1;
        }
        else
        {
            lidx_ref = LIST_1;
            lidx_cnd = LIST_0;
            pidx_ref = PRED_L1;
            pidx_cnd = PRED_L0;
        }

        pi->mvp_idx[pidx][LIST_0] = pi->mvp_idx[PRED_L0][LIST_0];
        pi->mvp_idx[pidx][LIST_1] = pi->mvp_idx[PRED_L1][LIST_1];

        pi->refi[pidx][LIST_0] = pi->refi[PRED_L0][LIST_0];
        pi->refi[pidx][LIST_1] = pi->refi[PRED_L1][LIST_1];

        pi->mv[pidx][lidx_ref][MV_X] = pi->mv[pidx_ref][lidx_ref][MV_X];
        pi->mv[pidx][lidx_ref][MV_Y] = pi->mv[pidx_ref][lidx_ref][MV_Y];
        pi->mv[pidx][lidx_cnd][MV_X] = pi->mv[pidx_cnd][lidx_cnd][MV_X];
        pi->mv[pidx][lidx_cnd][MV_Y] = pi->mv[pidx_cnd][lidx_cnd][MV_Y];

        t0 = (lidx_ref == LIST_0) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
        t1 = (lidx_ref == LIST_1) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
        SET_REFI(refi, t0, t1);

        for(i = 0; i < BI_ITER; i++)
        {
            /* motion compensation */
            evey_inter_pred(ctx, x, y, cuw, cuh, refi, pi->mv[pidx], pi->refp, pi->pred[pidx]);

            get_org_bi(core->org[Y_C], pi->pred[pidx][0][Y_C], cuw, cuw, cuh, pi->org_bi);

            SWAP(refi[lidx_ref], refi[lidx_cnd], t0);
            SWAP(lidx_ref, lidx_cnd, t0);
            SWAP(pidx_ref, pidx_cnd, t0);

            mvp_idx = pi->mvp_idx[pidx][lidx_ref];
            changed = 0;

            for(refi_cur = 0; refi_cur < pi->num_refp; refi_cur++)
            {
                refi[lidx_ref] = refi_cur;
                mecost = pi->fn_me(pi, x, y, log2_cuw, log2_cuh, &refi[lidx_ref], lidx_ref, pi->mvp[lidx_ref][mvp_idx], pi->mv_scale[lidx_ref][refi_cur], 1, ctx->sps.bit_depth_luma_minus8 + 8);
                if(mecost < best_mecost)
                {
                    refi_best = refi_cur;
                    best_mecost = mecost;

                    changed = 1;
                    t0 = (lidx_ref == LIST_0) ? refi_best : pi->refi[pidx][lidx_cnd];
                    t1 = (lidx_ref == LIST_1) ? refi_best : pi->refi[pidx][lidx_cnd];
                    SET_REFI(pi->refi[pidx], t0, t1);

                    pi->mv[pidx][lidx_ref][MV_X] = pi->mv_scale[lidx_ref][refi_cur][MV_X];
                    pi->mv[pidx][lidx_ref][MV_Y] = pi->mv_scale[lidx_ref][refi_cur][MV_Y];
                }
            }

            t0 = (lidx_ref == LIST_0) ? refi_best : REFI_INVALID;
            t1 = (lidx_ref == LIST_1) ? refi_best : REFI_INVALID;
            SET_REFI(refi, t0, t1);

            if(!changed)
            {
                break;
            }
        }

        pi->mvd[pidx][LIST_0][MV_X] = pi->mv[pidx][LIST_0][MV_X] - pi->mvp_scale[LIST_0][pi->refi[pidx][LIST_0]][pi->mvp_idx[pidx][LIST_0]][MV_X];
        pi->mvd[pidx][LIST_0][MV_Y] = pi->mv[pidx][LIST_0][MV_Y] - pi->mvp_scale[LIST_0][pi->refi[pidx][LIST_0]][pi->mvp_idx[pidx][LIST_0]][MV_Y];
        pi->mvd[pidx][LIST_1][MV_X] = pi->mv[pidx][LIST_1][MV_X] - pi->mvp_scale[LIST_1][pi->refi[pidx][LIST_1]][pi->mvp_idx[pidx][LIST_1]][MV_X];
        pi->mvd[pidx][LIST_1][MV_Y] = pi->mv[pidx][LIST_1][MV_Y] - pi->mvp_scale[LIST_1][pi->refi[pidx][LIST_1]][pi->mvp_idx[pidx][LIST_1]][MV_Y];

        cost = pinter_residue_rdo(ctx, core, x, y, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx]);

        evey_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);
        evey_mcpy(pi->nnz_sub_best[pidx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);

        if(cost < core->cost_best)
        {
            core->pred_mode = MODE_INTER;
            pi->best_idx = pidx;
            cost_best = cost;
            core->cost_best = cost;
            cost_inter[pi->best_idx] = cost;
            SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
            DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
            evey_mcpy(pi->nnz_sub_best[pi->best_idx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);

            /* save all CU inforamtion of the best inter mode */
            EVEYE_MODE * mi = &ctx->mode;
            mi->rec = pi->rec[pi->best_idx];
            mi->coef = pi->coef[pi->best_idx];
            mi->nnz = pi->nnz_best[pi->best_idx];
            mi->nnz_sub = pi->nnz_sub_best[pi->best_idx];
            mi->refi = pi->refi[pi->best_idx];
            mi->mvp_idx = pi->mvp_idx[pi->best_idx];
            mi->mv = pi->mv[pi->best_idx];
            mi->mvd = pi->mvd[pi->best_idx];
            pi->pred_y_best = pi->pred[pi->best_idx][0][Y_C];
        }
    }

    return cost_best;
}

/* entry point for inter mode decision */
static double pinter_analyze_cu(EVEYE_CTX * ctx, EVEYE_CORE * core, int x, int y)
{   
    EVEYE_PINTER * pi = &ctx->pinter;
    double         cost_inter[PRED_NUM];

    /* check availability of neighboring blocks */
    core->avail_cu = evey_get_avail_inter(ctx, core);

    pi->best_idx = PRED_SKIP;
    for(int i = 0; i < PRED_NUM; i++)
    {
        cost_inter[i] = MAX_COST;
    }

    /* skip mode decision */
    analyze_skip_mode(ctx, core, x, y, cost_inter);

    /* direct mode decision */
    analyze_direct_mode(ctx, core, x, y, cost_inter);

    /* inter mode decision */
    analyze_inter_mode(ctx, core, x, y, cost_inter);

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        eveye_set_qp(ctx, core, core->dqp_next_best[core->log2_cuw - 2][core->log2_cuh - 2].prev_qp);
    }

    /* return inter best cost */
    return cost_inter[pi->best_idx];
}

static int pinter_init_frame(EVEYE_CTX * ctx)
{
    EVEYE_PINTER * pi = &ctx->pinter;
    int            size;

    pi->o_y = ctx->core->org[Y_C];
    pi->refp = ctx->refp;
    pi->map_mv = ctx->map_mv;
    pi->w_scu = ctx->w_scu;

    size = sizeof(pel) * MAX_CU_DIM;
    evey_mset(pi->pred_buf, 0, size);

    size = sizeof(s8) * PRED_NUM * LIST_NUM;
    evey_mset(pi->refi, 0, size);

    size = sizeof(s8) * LIST_NUM * EVEY_MVP_NUM;
    evey_mset(pi->refi_pred, 0, size);

    size = sizeof(s8) * LIST_NUM * EVEY_MVP_NUM;
    evey_mset(pi->mvp_idx, 0, size);

    size = sizeof(s16) * LIST_NUM * MAX_NUM_ACTIVE_REF_FRAME * EVEY_MVP_NUM * MV_D;
    evey_mset(pi->mvp_scale, 0, size);

    size = sizeof(s16) * LIST_NUM * MAX_NUM_ACTIVE_REF_FRAME * MV_D;
    evey_mset(pi->mv_scale, 0, size);

    size = sizeof(s16) * N_C * MAX_CU_DIM;
    evey_mset(pi->resi, 0, size);

    /* MV predictor */
    size = sizeof(s16) * LIST_NUM * EVEY_MVP_NUM * MV_D;
    evey_mset(pi->mvp, 0, size);

    size = sizeof(s16) * PRED_NUM * LIST_NUM * MV_D;
    evey_mset(pi->mv, 0, size);

    size = sizeof(s16) * PRED_NUM * LIST_NUM * MV_D;
    evey_mset(pi->mvd, 0, size);

    size = sizeof(s16) * MAX_CU_DIM;
    evey_mset(pi->org_bi, 0, size);

    size = sizeof(s32) * LIST_NUM;
    evey_mset(pi->mot_bits, 0, size);

    size = sizeof(pel) * (PRED_NUM + 1) * 2 * N_C * MAX_CU_DIM;
    evey_mset(pi->pred, 0, size);

    return EVEY_OK;
}

static int pinter_init_ctu(EVEYE_CTX * ctx, EVEYE_CORE * core)
{
    EVEYE_PINTER * pi = &ctx->pinter;

    pi->lambda_mv = (u32)floor(65536.0 * ctx->sqrt_lambda[0]);
    pi->poc = ctx->poc.poc_val;
    pi->gop_size = ctx->param.gop_size;

    return EVEY_OK;
}

static int pinter_set_complexity(EVEYE_CTX * ctx, int complexity)
{
    EVEYE_PINTER * pi = &ctx->pinter;

    /* motion search range */
    pi->max_search_range = ctx->param.max_b_frames == 0 ? SEARCH_RANGE_IPEL_LD : SEARCH_RANGE_IPEL_RA;
    /* motion estimation level (?) */
    pi->me_level = ME_LEV_QPEL;
    /* motion estimation function */
    pi->fn_me = pinter_me_epzs;  

    pi->complexity = complexity;

    ctx->fn_pinter_analyze_cu = pinter_analyze_cu;

    return EVEY_OK;
}

int eveye_pinter_create(EVEYE_CTX * ctx, int complexity)
{
    /* set function addresses */
    ctx->fn_pinter_init_frame = pinter_init_frame;
    ctx->fn_pinter_init_ctu = pinter_init_ctu;
    ctx->fn_pinter_set_complexity = pinter_set_complexity;

    /* set maximum/minimum value of search range */
    ctx->pinter.min_clip[MV_X] = -128 + 1;   /* TBD: need to check whether padding size is appropriate */
    ctx->pinter.min_clip[MV_Y] = -128 + 1;   /* TBD: need to check whether padding size is appropriate */
    ctx->pinter.max_clip[MV_X] = ctx->param.w - 1;
    ctx->pinter.max_clip[MV_Y] = ctx->param.h - 1;

    return ctx->fn_pinter_set_complexity(ctx, complexity);
}
