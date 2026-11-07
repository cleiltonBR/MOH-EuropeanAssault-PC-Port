#include "recomp_sse_compat.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    __m128 a={1.0f,2.0f,3.0f,4.0f}, b={4.0f,3.0f,2.0f,1.0f};
    __m128 r=_mm_add_ps(a,b);
    assert(r[0]==5.0f && r[1]==5.0f && r[2]==5.0f && r[3]==5.0f);
    r=_mm_shuffle_ps(a,b,0x1B);
    assert(r[0]==4.0f && r[1]==3.0f && r[2]==3.0f && r[3]==4.0f);
    r=_mm_cmplt_ps(a,b);
    assert(_mm_movemask_ps(r)==0x3);
    r=_mm_unpacklo_ps(a,b);
    assert(r[0]==1.0f && r[1]==4.0f && r[2]==2.0f && r[3]==3.0f);
    {
        uint32_t bits=0x80000000u; float negzero; memcpy(&negzero,&bits,4);
        __m128 z={negzero,1.0f,-1.0f,0.0f};
        assert(_mm_movemask_ps(z)==0x5);
    }
    assert(_mm_cvttss_si32(_mm_set_ss(-2.9f))==-2);
    {
        const double value=1.0/3.0;
        __m128 sd=_mm_set_sd(value);
        double low=0.0;
        uint64_t high=1;
        memcpy(&low,&sd,sizeof(low));
        memcpy(&high,((const unsigned char *)&sd)+sizeof(low),sizeof(high));
        assert(low==value && high==0);
        assert(_mm_cvttsd_si32(sd)==0);
        assert(_mm_cvtsd_si32(sd)==0);
        sd=_mm_set_sd(2147483647.0);
        assert(_mm_cvttsd_si32(sd)==2147483647);
        sd=_mm_set_sd(-2.9);
        assert(_mm_cvttsd_si32(sd)==-2);
        assert(_mm_cvtsd_si32(sd)==-3);
    }
    {
        float nan_value=NAN;
        __m128 n={nan_value,1.0f,nan_value,2.0f};
        __m128 one={1.0f,nan_value,nan_value,2.0f};
        assert(_mm_movemask_ps(_mm_cmpunord_ps(n,one))==0x7);
        assert(_mm_movemask_ps(_mm_cmpneq_ps(n,one))==0x7);
    }
    assert(_mm_cvtss_si32(_mm_set_ss(2.5f))==2);
    assert(_mm_cvtss_si32(_mm_set_ss(3.5f))==4);
    return 0;
}
