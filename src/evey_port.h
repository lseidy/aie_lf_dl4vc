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

#ifndef _EVEY_PORT_H_
#define _EVEY_PORT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

/*****************************************************************************
 * types
 *****************************************************************************/
typedef int8_t                   s8;
typedef uint8_t                  u8;
typedef int16_t                  s16;
typedef uint16_t                 u16;
typedef int32_t                  s32;
typedef uint32_t                 u32;
typedef int64_t                  s64;
typedef uint64_t                 u64;
typedef s16                      pel;
typedef s32                      double_pel; 
/*****************************************************************************
 * limit constant
 *****************************************************************************/
#define EVEY_UINT16_MAX          ((u16)0xFFFF)
#define EVEY_UINT16_MIN          ((u16)0x0)
#define EVEY_INT16_MAX           ((s16)0x7FFF)
#define EVEY_INT16_MIN           ((s16)0x8000)

#define EVEY_UINT_MAX            ((u32)0xFFFFFFFF)
#define EVEY_UINT_MIN            ((u32)0x0)
#define EVEY_INT_MAX             ((int)0x7FFFFFFF)
#define EVEY_INT_MIN             ((int)0x80000000)

#define EVEY_UINT32_MAX          ((u32)0xFFFFFFFF)
#define EVEY_UINT32_MIN          ((u32)0x0)
#define EVEY_INT32_MAX           ((s32)0x7FFFFFFF)
#define EVEY_INT32_MIN           ((s32)0x80000000)

#define EVEY_UINT64_MAX          ((u64)0xFFFFFFFFFFFFFFFFL)
#define EVEY_UINT64_MIN          ((u64)0x0L)
#define EVEY_INT64_MAX           ((s64)0x7FFFFFFFFFFFFFFFL)
#define EVEY_INT64_MIN           ((s64)0x8000000000000000L)

/*****************************************************************************
 * memory operations
 *****************************************************************************/
#define evey_malloc(size)          malloc((size))
#define evey_malloc_fast(size)     evey_malloc((size))

#define evey_mfree(m)              if(m){free(m);}
#define evey_mfree_fast(m)         if(m){evey_mfree(m);}

#define evey_mcpy(dst,src,size)    memcpy((dst), (src), (size))
#define evey_mset(dst,v,size)      memset((dst), (v), (size))
#define evey_mset_x64a(dst,v,size) memset((dst), (v), (size))
#define evey_mset_x128(dst,v,size) memset((dst), (v), (size))
#define evey_mcmp(dst,src,size)    memcmp((dst), (src), (size))
static __inline void evey_mset_16b(s16 * dst, s16 v, int cnt)
{
    int i;
    for(i=0; i<cnt; i++)
        dst[i] = v;
}


/*****************************************************************************
 * trace and assert
 *****************************************************************************/
void evey_trace0(char * filename, int line, const char *fmt, ...);
void evey_trace_line(char * pre);
#ifndef EVEY_TRACE
#define EVEY_TRACE               0
#endif

/* trace function */
#if EVEY_TRACE
#if defined(__GNUC__)
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define evey_trace(args...) evey_trace0(__FILENAME__, __LINE__, args)
#else
#define __FILENAME__ \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define evey_trace(args,...) evey_trace0(__FILENAME__,__LINE__, args,__VA_ARGS__)
#endif
#else
#define evey_trace(args,...) {}
#endif
#if defined(__GNUC__)
#define evey_print(args,...) evey_trace0(NULL, -1, args)
#else
#define evey_print(args,...) evey_trace0(NULL, -1, args,__VA_ARGS__)
#endif

/* assert function */
#include <assert.h>
#define evey_assert(x) \
    {if(!(x)){assert(x);}}
#define evey_assert_r(x) \
    {if(!(x)){assert(x); return;}}
#define evey_assert_rv(x,r) \
    {if(!(x)){assert(x); return (r);}}
#define evey_assert_g(x,g) \
    {if(!(x)){assert(x); goto g;}}
#define evey_assert_gv(x,r,v,g) \
    {if(!(x)){assert(x); (r)=(v); goto g;}}

#define X86_SSE                 1

#if X86_SSE
#ifdef _WIN32
#include <emmintrin.h>
#include <xmmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#else
#include <x86intrin.h>
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EVEY_PORT_H_ */
