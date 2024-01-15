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

#ifndef _EVEYD_ECO_H_
#define _EVEYD_ECO_H_

#include "eveyd_def.h"

#define GET_SBAC_DEC(bs)   ((EVEYD_SBAC *)((bs)->pdata[1]))
#define SET_SBAC_DEC(bs, sbac) ((bs)->pdata[1] = (sbac))

void eveyd_sbac_reset(EVEYD_CTX * ctx);

int eveyd_eco_nalu(EVEYD_BSR * bs, EVEY_NALU * nalu);
int eveyd_eco_sps(EVEYD_BSR * bs, EVEY_SPS * sps);
int eveyd_eco_pps(EVEYD_BSR * bs, EVEY_SPS * sps, EVEY_PPS * pps);
int eveyd_eco_sh(EVEYD_BSR * bs, EVEY_SPS * sps, EVEY_PPS * pps, EVEY_SH * sh, int nut);
int eveyd_eco_sei(EVEYD_CTX * ctx, EVEYD_BSR * bs);

int eveyd_eco_cu(EVEYD_CTX * ctx, EVEYD_CORE * core);
s8  eveyd_eco_split_mode(EVEYD_BSR * bs);
int eveyd_eco_tile_end_flag(EVEYD_BSR * bs);

#endif /* _EVEYD_ECO_H_ */
