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

#ifndef _EVEYD_UTIL_H_
#define _EVEYD_UTIL_H_

#include "eveyd_def.h"

int eveyd_picbuf_check_signature(EVEYD_CTX * ctx, int do_compare);

/* set motion information */
void eveyd_set_mv_info(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y);

/* set decoded information */
void eveyd_set_dec_info(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y
#if ENC_DEC_TRACE
                        , u8 write_trace
#endif
);

/* get decoded information */
void eveyd_get_dec_info(EVEYD_CTX * ctx, EVEYD_CORE * core, int x, int y);

#if USE_DRAW_PARTITION_DEC
void eveyd_draw_partition(EVEYD_CTX * ctx, EVEY_PIC * pic);
#endif

#endif /* _EVEYD_UTIL_H_ */
