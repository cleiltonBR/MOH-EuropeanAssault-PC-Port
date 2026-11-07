#include "recomp_sse_compat.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    __m128 a={1.0f,2.0f,3.0f,4.0f}, b={4.0f,3.0f,2.0f,1.0f};
    __m128 r=_mm_add_ps(a,b);
    assert(r[0]==5.0f && r[1]==5.0f && r[2]==5.0f && r[3]==5.0f);
    r=_mm_shuffle_ps(a,b,0x1B);
    assert(r[0]==4.0f && r[1]==3.0f && r[2]==1.0f && r[3]==2.0f);
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
    return 0;
}
