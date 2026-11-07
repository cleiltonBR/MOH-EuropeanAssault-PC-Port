#ifndef MOH_RECOMP_SSE_COMPAT_H
#define MOH_RECOMP_SSE_COMPAT_H

/*
 * Minimal ARM64/portable SSE surface used by the generated recomp_types.h.
 * Correctness-first; reciprocal estimates and x86 FP exceptions are not bit-exact.
 */
#include <math.h>
#include <stdint.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
typedef float __m128 __attribute__((vector_size(16), aligned(16)));
#else
#error "The ARM64 SSE compatibility layer requires GCC or Clang vector extensions."
#endif

static inline void moh_sse_to_array(float out[4], __m128 v) { memcpy(out, &v, 16); }
static inline __m128 moh_sse_from_array(const float in[4]) { __m128 v; memcpy(&v, in, 16); return v; }

static inline __m128 _mm_loadu_ps(const float *p) { __m128 r; memcpy(&r,p,16); return r; }
static inline void _mm_storeu_ps(float *p, __m128 a) { memcpy(p,&a,16); }
static inline __m128 _mm_movelh_ps(__m128 a,__m128 b) { return (__m128){a[0],a[1],b[0],b[1]}; }
static inline __m128 _mm_movehl_ps(__m128 a,__m128 b) { return (__m128){b[2],b[3],a[2],a[3]}; }

#define MOH_SSE_BIN(name, expr) static inline __m128 name(__m128 a,__m128 b) { __m128 r; int i; for(i=0;i<4;i++) r[i]=(expr); return r; }
MOH_SSE_BIN(_mm_add_ps, a[i]+b[i])
MOH_SSE_BIN(_mm_sub_ps, a[i]-b[i])
MOH_SSE_BIN(_mm_mul_ps, a[i]*b[i])
MOH_SSE_BIN(_mm_div_ps, a[i]/b[i])
MOH_SSE_BIN(_mm_min_ps, (isnan(a[i]) || isnan(b[i]) || a[i]==b[i]) ? b[i] : (a[i]<b[i]?a[i]:b[i]))
MOH_SSE_BIN(_mm_max_ps, (isnan(a[i]) || isnan(b[i]) || a[i]==b[i]) ? b[i] : (a[i]>b[i]?a[i]:b[i]))
#undef MOH_SSE_BIN

#define MOH_SSE_BIT(name, op) static inline __m128 name(__m128 a,__m128 b){float av[4],bv[4],rv[4];int i; moh_sse_to_array(av,a);moh_sse_to_array(bv,b);for(i=0;i<4;i++){uint32_t x,y,z;memcpy(&x,&av[i],4);memcpy(&y,&bv[i],4);z=(op);memcpy(&rv[i],&z,4);}return moh_sse_from_array(rv);}
MOH_SSE_BIT(_mm_and_ps, x&y)
MOH_SSE_BIT(_mm_or_ps, x|y)
MOH_SSE_BIT(_mm_xor_ps, x^y)
MOH_SSE_BIT(_mm_andnot_ps, (~x)&y)
#undef MOH_SSE_BIT

#define MOH_SSE_CMP(name, cond) static inline __m128 name(__m128 a,__m128 b){float av[4],bv[4],rv[4];int i;moh_sse_to_array(av,a);moh_sse_to_array(bv,b);for(i=0;i<4;i++){uint32_t u=(cond)?UINT32_MAX:0;memcpy(&rv[i],&u,4);}return moh_sse_from_array(rv);}
MOH_SSE_CMP(_mm_cmpeq_ps, av[i]==bv[i])
MOH_SSE_CMP(_mm_cmplt_ps, av[i]<bv[i])
MOH_SSE_CMP(_mm_cmple_ps, av[i]<=bv[i])
MOH_SSE_CMP(_mm_cmpneq_ps, av[i]!=bv[i])
MOH_SSE_CMP(_mm_cmpunord_ps, isnan(av[i])||isnan(bv[i]))
MOH_SSE_CMP(_mm_cmpnlt_ps, !(av[i]<bv[i]))
MOH_SSE_CMP(_mm_cmpnle_ps, !(av[i]<=bv[i]))
MOH_SSE_CMP(_mm_cmpord_ps, !isnan(av[i])&&!isnan(bv[i]))
#undef MOH_SSE_CMP

static inline __m128 _mm_sqrt_ps(__m128 a){__m128 r;int i;for(i=0;i<4;i++)r[i]=sqrtf(a[i]);return r;}
static inline __m128 _mm_set_ss(float x){return (__m128){x,0,0,0};}
static inline __m128 _mm_set_sd(double x){uint64_t bits[2]={0,0};__m128 r;memcpy(&bits[0],&x,sizeof(x));memcpy(&r,bits,sizeof(r));return r;}
static inline float _mm_cvtss_f32(__m128 a){return a[0];}
static inline __m128 _mm_rcp_ss(__m128 a){__m128 r=a;r[0]=1.0f/a[0];return r;}
static inline __m128 _mm_rsqrt_ss(__m128 a){__m128 r=a;r[0]=1.0f/sqrtf(a[0]);return r;}
static inline __m128 _mm_rcp_ps(__m128 a){__m128 r;int i;for(i=0;i<4;i++)r[i]=1.0f/a[i];return r;}
static inline __m128 _mm_rsqrt_ps(__m128 a){__m128 r;int i;for(i=0;i<4;i++)r[i]=1.0f/sqrtf(a[i]);return r;}
static inline int _mm_movemask_ps(__m128 a){float av[4];int i,m=0;moh_sse_to_array(av,a);for(i=0;i<4;i++){uint32_t u;memcpy(&u,&av[i],4);m|=(int)((u>>31)&1u)<<i;}return m;}

#define _mm_shuffle_ps(a,b,imm) ((__m128){(a)[((imm)&3)],(a)[(((imm)>>2)&3)],(b)[(((imm)>>4)&3)],(b)[(((imm)>>6)&3)]})
static inline __m128 _mm_unpacklo_ps(__m128 a,__m128 b){return (__m128){a[0],b[0],a[1],b[1]};}
static inline __m128 _mm_unpackhi_ps(__m128 a,__m128 b){return (__m128){a[2],b[2],a[3],b[3]};}

static inline int32_t _mm_cvttss_si32(__m128 a){double x=trunc((double)a[0]);return isfinite(x)&&x>=-2147483648.0&&x<2147483648.0?(int32_t)x:INT32_MIN;}
static inline int32_t _mm_cvtss_si32(__m128 a){double x=nearbyint((double)a[0]);return isfinite(x)&&x>=-2147483648.0&&x<2147483648.0?(int32_t)x:INT32_MIN;}
static inline double moh_sse_low_double(__m128 a){double x;memcpy(&x,&a,sizeof(x));return x;}
static inline int32_t moh_sse_double_to_i32(double x,int rounded){x=rounded?nearbyint(x):trunc(x);return isfinite(x)&&x>=-2147483648.0&&x<2147483648.0?(int32_t)x:INT32_MIN;}
static inline int32_t _mm_cvttsd_si32(__m128 a){return moh_sse_double_to_i32(moh_sse_low_double(a),0);}
static inline int32_t _mm_cvtsd_si32(__m128 a){return moh_sse_double_to_i32(moh_sse_low_double(a),1);}

#endif
