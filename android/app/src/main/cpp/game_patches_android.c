/*
 * Android-portable manual game patches.
 *
 * Keep these lifted guest functions separate from desktop-only additions in
 * source/mygame/src/game_patches.c. The generated runtime supplies the guest
 * register, memory, x87 and dispatcher contracts through these headers.
 */
#include "recomp_types.h"
#include "recomp_funcs.h"
#include <time.h>

/* Android monotonic clock and cooperative sleep used by the stream-flush
 * patch. Wall-clock time is only a last-resort fallback if CLOCK_MONOTONIC
 * unexpectedly fails. */
static uint64_t android_patch_monotonic_ms(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)
        return (uint64_t)now.tv_sec * 1000u
             + (uint64_t)now.tv_nsec / 1000000u;
    {
        time_t fallback = time(NULL);
        return fallback > 0 ? (uint64_t)fallback * 1000u : 0;
    }
}

static void android_patch_sleep_ms(uint32_t milliseconds)
{
    struct timespec delay;
    delay.tv_sec = (time_t)(milliseconds / 1000u);
    delay.tv_nsec = (long)(milliseconds % 1000u) * 1000000L;
    (void)nanosleep(&delay, NULL);
}



/* 0x0017B210: original lifted guest behavior. This function has no Win32,
 * desktop input, diagnostic, or host-clock dependencies. */
void game_patch_0017B210(void)
{
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)

loc_0017B210: ;
    PUSH32(esp, esi);
    esi = ecx;
    SET_LO8(eax, MEM8(esi + 5));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017B273; /* je: equal / zero */

loc_0017B21A: ;
    SET_LO8(eax, MEM8(esi + 6));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017B234; /* je: equal / zero */

loc_0017B221: ;
    eax = MEM32(esi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017B234; /* je: equal / zero */

loc_0017B228: ;
    ecx = esi + 8;
    PUSH32(esp, 0x0017B230u); RECOMP_ABI_CALL(0x0010B220u, sub_0010B220); /* call 0x0010B220 */

loc_0017B230: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017B273; /* je: equal / zero */

loc_0017B234: ;
    ecx = esi;
    PUSH32(esp, 0x0017B23Bu); RECOMP_ABI_CALL(0x0017AF80u, sub_0017AF80); /* call 0x0017AF80 */

loc_0017B23B: ;
    eax = MEM32(0x51FB7C);
    PUSH32(esp, 0x322E08);
    MEM32(0x51FB74) = eax;
    PUSH32(esp, 0x0017B24Fu); RECOMP_ABI_CALL(0x00151EF0u, sub_00151EF0); /* call 0x00151EF0 */

loc_0017B24F: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    PUSH32(esp, 0x322DF4);
    if (CMP_EQ(_fa, _fb)) goto loc_0017B262; /* je: equal / zero */

loc_0017B25B: ;
    PUSH32(esp, 0x322DFC);
    goto loc_0017B267;

loc_0017B262: ;
    PUSH32(esp, 0x307A28);

loc_0017B267: ;
    PUSH32(esp, 0x0017B26Cu); RECOMP_ABI_CALL(0x00170760u, sub_00170760); /* call 0x00170760 */

loc_0017B26C: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM8(esi + 5) = 0;

loc_0017B273: ;
    SET_LO8(eax, MEM8(esi + 4));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017B302; /* je: equal / zero */

loc_0017B27E: ;
    _fa = (uint32_t)(MEM8(0x51FBBC)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(0x51FBBC), 1 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_0017B2AA; /* jne: not equal / not zero */

loc_0017B287: ;
    fp_push(MEMF(0x2FEFB0)); /* fld float */
    eax = MEM32(0x51FBBC);
    fp_top() = RECOMP_FP_PC(fp_top() / MEMF(0x3727C4)); /* fdiv dword ptr [0x3727c4] */
    eax = eax | 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x51FBBC) = eax;
    PUSH32(esp, 0x0017B2A5u); RECOMP_ABI_CALL(0x002BE320u, sub_002BE320); /* call 0x002BE320 */

loc_0017B2A5: ;
    MEM32(0x51FBB8) = eax;

loc_0017B2AA: ;
    fp_push(MEMF(esp + 8)); /* fld float */
    PUSH32(esp, edi);
    PUSH32(esp, 0x0017B2B4u); RECOMP_ABI_CALL(0x002BE320u, sub_002BE320); /* call 0x002BE320 */

loc_0017B2B4: ;
    ecx = MEM32(esp + 0xC);
    edi = eax;
    /* PATCH: (frame time * ms per frame), rounded -- was (int)frame time
     * * ms per frame, 0 below one 60 Hz frame (RECOMP_FPS). */
    edi = (uint32_t)(int32_t)(MEMF(esp + 0xC) * (float)(int32_t)MEM32(0x51FBB8) + 0.5f);
    PUSH32(esp, ecx);
    ecx = esi;
    PUSH32(esp, 0x0017B2C9u); RECOMP_ABI_CALL(0x001789F0u, sub_001789F0); /* call 0x001789F0 */

loc_0017B2C9: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_0017B2E3; /* jle: less or equal (signed <=) */

loc_0017B2CF: ;
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - edi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(esi) = eax;
    if (CMP_G(_fas, _fbs)) goto loc_0017B301; /* jg: greater (signed >) */

loc_0017B2D7: ;
    POP32(esp, edi);
    MEM8(0x3C250F) = 1;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_0017B2E3: ;
    _fa = (uint32_t)(MEM32(0x388876)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x192) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(0x388876), 0x192 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0017B2F8; /* je: equal / zero */

loc_0017B2EF: ;
    _fa = (uint32_t)(MEM32(0x51FB74)) & 0xFFFFFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x51FB74), 0xC (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B301; /* jne: not equal / not zero */

loc_0017B2F8: ;
    PUSH32(esp, edi);
    PUSH32(esp, 0x0017B2FEu); RECOMP_ABI_CALL(0x00170730u, sub_00170730); /* call 0x00170730 */

loc_0017B2FE: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0017B301: ;
    POP32(esp, edi);

loc_0017B302: ;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}


/* Original lifted guest patches with desktop-only input, FPS and sampler
 * injections removed. Guest register/memory behavior is otherwise retained. */

void game_patch_000137C0(void)
{
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs; (void)_flags;


loc_000137C0: ;
    SET_LO8(eax, MEM8(0x38DC70));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    PUSH32(esp, esi);
    esi = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_0001380D; /* je: equal / zero */

loc_000137CC: ;
    PUSH32(esp, 0x000137D1u); RECOMP_ABI_CALL(0x00013500u, sub_00013500); /* call 0x00013500 */

loc_000137D1: ;
    
    eax = MEM32(0x38DC6C);
    ecx = MEM32(0x3727D4);

loc_000137E0: ;
    edx = MEM32(0x38DC8C);
    edx = edx - eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    if (CMP_B(_fa, _fb)) { RECOMP_BACK_EDGE(0x000137EAu); goto loc_000137E0; } /* jb: below (unsigned <) */

loc_000137EC: ;
    ecx = MEM32(esi + 0x84);
    eax = MEM32(ecx);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 4); PUSH32(esp, 0x000137F7u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x000137F4u); } /* indirect call */
    }

loc_000137F7: ;
    MEM32(0x38DCA8) = MEM32(0x38DCA8) - 1;
    ecx = MEM32(esi + 0x84);
    edx = MEM32(ecx);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx); PUSH32(esp, 0x00013807u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00013805u); } /* indirect call */
    }

loc_00013807: ;
    MEM32(0x38DCA8) = MEM32(0x38DCA8) + 1;

loc_0001380D: ;
    
    eax = MEM32(0x38DC8C);
    edx = MEM32(0x38DC6C);
    ecx = MEM32(0x38DC8C);
    eax = eax - edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    MEM32(0x38DC6C) = ecx;
    POP32(esp, esi);
    if (CMP_GE(_fas, _fbs)) goto loc_00013831; /* jge: greater or equal (signed >=) */

loc_0001382C: ;
    eax = 1;

loc_00013831: ;
    esp += 4; return; /* ret */
}

void game_patch_001149E0(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;

loc_001149E0: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x5C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x5C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    xmm1 = XMM_SCALAR(MEMF(ebp + 8)); /* movss */
    PUSH32(esp, esi);
    esi = ecx;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x1C8)); /* movss */
    SET_LO8(eax, MEM8(esi + 0x128));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x2FF020); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x110); /* addss */
    MEMF(esi + 0x10C) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x110) = xmm0.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_00114A4B; /* je: equal / zero */

loc_00114A27: ;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x12C)); /* movss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    xmm2 = XMM_SCALAR(MEMF(esi + 0x130)); /* movss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114A41; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_00114A3C: ;
    xmm0 = xmm1; /* movaps */
    goto loc_00114A6F;

loc_00114A41: ;
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114A6F; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm2.f[0]) */

loc_00114A46: ;
    xmm0 = xmm2; /* movaps */
    goto loc_00114A6F;

loc_00114A4B: ;
    xmm1 = XMM_SCALAR(MEMF(0x301054)); /* movss */
    _fca = xmm0.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114A5E; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm1.f[0]) */

loc_00114A58: ;
    xmm0.f[0] = xmm0.f[0] - xmm1.f[0]; /* subss */
    goto loc_00114A6F;

loc_00114A5E: ;
    xmm2 = XMM_SCALAR(MEMF(0x319298)); /* movss */
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114A77; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm0.f[0]) */

loc_00114A6B: ;
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */

loc_00114A6F: ;
    MEMF(esi + 0x110) = xmm0.f[0]; /* movss */

loc_00114A77: ;
    eax = MEM32(esi + 0x1C0);
    eax = eax >> 0xD;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_00114AD4; /* je: equal / zero */

loc_00114A84: ;
    xmm1 = XMM_SCALAR(MEMF(0x375F84)); /* movss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x2FF084); /* mulss */
    xmm0 = XMM_SCALAR(MEMF(esi + 0x110)); /* movss */
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    xmm1 = XMM_SCALAR(MEMF(0x319294)); /* movss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    MEMF(esi + 0x464) = xmm0.f[0]; /* movss */
    if ((_fca > _fcb)) goto loc_00114AC2; /* ja: above (unsigned >) (xmm1.f[0] vs xmm0.f[0]) */

loc_00114AB5: ;
    xmm1 = XMM_SCALAR(MEMF(0x303CFC)); /* movss */
    _fca = xmm0.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114AC5; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm1.f[0]) */

loc_00114AC2: ;
    xmm0 = xmm1; /* movaps */

loc_00114AC5: ;
    MEMF(esi + 0x464) = xmm0.f[0]; /* movss */
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

loc_00114AD4: ;
    xmm0 = XMM_MEM(esi + 0x60); /* movaps */
    ecx = MEM32(esi + 0x110);
    XMM_STORE(esp + 0x20, xmm0); /* movaps */
    xmm0 = XMM_MEM(esi + 0x70); /* movaps */
    XMM_STORE(esp + 0x30, xmm0); /* movaps */
    xmm0 = XMM_MEM(esi + 0x80); /* movaps */
    XMM_STORE(esp + 0x40, xmm0); /* movaps */
    xmm0 = XMM_MEM(esi + 0x90); /* movaps */
    XMM_STORE(esp + 0x50, xmm0); /* movaps */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x50)); /* movss */
    MEMF(esp + 0x10) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    MEMF(esp + 0x14) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x58)); /* movss */
    PUSH32(esp, ecx);
    ecx = esp + 0x24;
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00114B32u); RECOMP_ABI_CALL(0x00265DA0u, sub_00265DA0); /* call 0x00265DA0 */

loc_00114B32: ;
    edx = esp + 0x10;
    PUSH32(esp, edx);
    ecx = esp + 0x24;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00114B40u); RECOMP_ABI_CALL(0x00264FA0u, sub_00264FA0); /* call 0x00264FA0 */

loc_00114B40: ;
    eax = MEM32(esi + 0x30);
    ecx = esi + 0x30;
    edx = esp + 0x20;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edx);
    { uint32_t _icall_target = MEM32(eax + 0xEC); PUSH32(esp, 0x00114B51u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00114B4Bu); } /* indirect call */
    }

loc_00114B51: ;
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

}

void game_patch_00114B60(void)
{
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;

loc_00114B60: ;
    xmm1 = XMM_SCALAR(MEMF(esp + 4)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(ecx + 0x14C)); /* movss */
    xmm2 = XMM_ZERO(); /* xorps self = zero */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    xmm1.f[0] = xmm1.f[0] * MEMF(ecx + 0x1C8); /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x2FF020); /* mulss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114C01; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm2.f[0]) */

loc_00114B86: ;
    _fca = xmm2.f[0]; _fcb = xmm1.f[0]; /* comiss */
    xmm4 = xmm1; /* movaps */
    if ((_fca > _fcb)) goto loc_00114B91; /* ja: above (unsigned >) (xmm2.f[0] vs xmm1.f[0]) */

loc_00114B8E: ;
    xmm4 = xmm2; /* movaps */

loc_00114B91: ;
    xmm3 = XMM_SCALAR(MEMF(ecx + 0x14C)); /* movss */
    xmm0 = xmm3; /* movaps */
    xmm0.f[0] = xmm0.f[0] + xmm4.f[0]; /* addss */
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114BAA; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm0.f[0]) */

loc_00114BA5: ;
    xmm3 = xmm2; /* movaps */
    goto loc_00114BB2;

loc_00114BAA: ;
    _fca = xmm0.f[0]; _fcb = xmm3.f[0]; /* comiss */
    if ((_fca > _fcb)) goto loc_00114BB2; /* ja: above (unsigned >) (xmm0.f[0] vs xmm3.f[0]) */

loc_00114BAF: ;
    xmm3 = xmm0; /* movaps */

loc_00114BB2: ;
    xmm0 = XMM_SCALAR(MEMF(ecx + 0x13C)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ecx + 0x1C8); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x31929C); /* mulss */
    xmm4 = xmm2; /* movaps */
    xmm4.f[0] = xmm4.f[0] - xmm3.f[0]; /* subss */
    _fca = xmm4.f[0]; _fcb = xmm0.f[0]; /* comiss */
    MEMF(ecx + 0x14C) = xmm3.f[0]; /* movss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114BE3; /* jbe: below or equal (unsigned <=) (xmm4.f[0] vs xmm0.f[0]) */

loc_00114BDE: ;
    xmm0 = xmm4; /* movaps */
    goto loc_00114BEB;

loc_00114BE3: ;
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114BEB; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm2.f[0]) */

loc_00114BE8: ;
    xmm0 = xmm2; /* movaps */

loc_00114BEB: ;
    xmm4 = xmm0; /* movaps */
    xmm4.f[0] = xmm4.f[0] + xmm1.f[0]; /* addss */
    xmm3.f[0] = xmm3.f[0] + xmm0.f[0]; /* addss */
    xmm1 = xmm4; /* movaps */
    MEMF(ecx + 0x14C) = xmm3.f[0]; /* movss */

loc_00114C01: ;
    xmm5 = XMM_SCALAR(MEMF(ecx + 0x144)); /* movss */
    xmm0 = xmm5; /* movaps */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114C20; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm2.f[0]) */

loc_00114C11: ;
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    xmm1 = xmm0; /* movaps */
    MEMF(ecx + 0x144) = xmm2.f[0]; /* movss */

loc_00114C20: ;
    SET_LO8(eax, MEM8(ecx + 0x11C));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    xmm0 = xmm1; /* movaps */
    xmm0.f[0] = xmm0.f[0] + MEMF(ecx + 0x108); /* addss */
    if (CMP_EQ(_fa, _fb)) goto loc_00114C57; /* je: equal / zero */

loc_00114C35: ;
    xmm3 = XMM_SCALAR(MEMF(ecx + 0x120)); /* movss */
    _fca = xmm3.f[0]; _fcb = xmm0.f[0]; /* comiss */
    xmm4 = XMM_SCALAR(MEMF(ecx + 0x124)); /* movss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114C4F; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */

loc_00114C4A: ;
    xmm0 = xmm3; /* movaps */
    goto loc_00114C57;

loc_00114C4F: ;
    _fca = xmm0.f[0]; _fcb = xmm4.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114C57; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm4.f[0]) */

loc_00114C54: ;
    xmm0 = xmm4; /* movaps */

loc_00114C57: ;
    _fca = xmm1.f[0]; _fcb = xmm2.f[0]; /* comiss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ecx + 0x108); /* subss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114C6D; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm2.f[0]) */

loc_00114C64: ;
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    xmm3 = xmm1; /* movaps */
    goto loc_00114C74;

loc_00114C6D: ;
    xmm3 = xmm0; /* movaps */
    xmm3.f[0] = xmm3.f[0] - xmm1.f[0]; /* subss */

loc_00114C74: ;
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    xmm1 = xmm5; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm3.f[0]; /* subss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114CA6; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm2.f[0]) */

loc_00114C80: ;
    _fca = xmm2.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114C8A; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm1.f[0]) */

loc_00114C85: ;
    xmm1 = xmm2; /* movaps */
    goto loc_00114C92;

loc_00114C8A: ;
    _fca = xmm1.f[0]; _fcb = xmm5.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114C92; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm5.f[0]) */

loc_00114C8F: ;
    xmm1 = xmm5; /* movaps */

loc_00114C92: ;
    xmm2 = XMM_SCALAR(MEMF(ecx + 0x14C)); /* movss */
    xmm2.f[0] = xmm2.f[0] + xmm1.f[0]; /* addss */
    MEMF(ecx + 0x14C) = xmm2.f[0]; /* movss */

loc_00114CA6: ;
    eax = MEM32(ecx + 0x1C0);
    eax = eax >> 0xD;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 1 (8-bit) */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] + MEMF(ecx + 0x108); /* addss */
    MEMF(ecx + 0x108) = xmm1.f[0]; /* movss */
    MEMF(ecx + 0x104) = xmm0.f[0]; /* movss */
    if (TEST_Z(_fa, _fb)) goto loc_00114D0F; /* je: equal / zero */

loc_00114CCE: ;
    xmm0 = XMM_SCALAR(MEMF(0x375F88)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x2FF084); /* mulss */
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */
    xmm0 = XMM_SCALAR(MEMF(0x319294)); /* movss */
    _fca = xmm0.f[0]; _fcb = xmm1.f[0]; /* comiss */
    MEMF(ecx + 0x460) = xmm1.f[0]; /* movss */
    if ((_fca > _fcb)) goto loc_00114D04; /* ja: above (unsigned >) (xmm0.f[0] vs xmm1.f[0]) */

loc_00114CF7: ;
    xmm0 = XMM_SCALAR(MEMF(0x303CFC)); /* movss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00114D07; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_00114D04: ;
    xmm1 = xmm0; /* movaps */

loc_00114D07: ;
    MEMF(ecx + 0x460) = xmm1.f[0]; /* movss */

loc_00114D0F: ;
    esp += 8; return; /* ret 4 */

}

void game_patch_0011AC60(void)
{
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)

loc_0011AC60: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0xA8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xA8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = ecx;
    eax = MEM32(esi + 0x1C0);
    ecx = eax;
    ecx = ecx >> 0xD;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 1 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_0011B184; /* jne: not equal / not zero */

loc_0011AC84: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    edx = eax;
    edx = edx >> 0xB;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0011ACA0; /* je: equal / zero */

loc_0011AC91: ;
    MEMF(ebp + 0x10) = xmm0.f[0]; /* movss */
    MEMF(ebp + 8) = xmm0.f[0]; /* movss */
    MEMF(ebp + 0xC) = xmm0.f[0]; /* movss */

loc_0011ACA0: ;
    eax = eax >> 0x12;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0011ACE5; /* je: equal / zero */

loc_0011ACA7: ;
    xmm1 = XMM_SCALAR(MEMF(ebp + 0xC)); /* movss */
    eax = MEM32(esi);
    MEMF(esi + 0x430) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(ebp + 8)); /* movss */
    ecx = MEM32(esi + 0x430);
    MEMF(esi + 0x434) = xmm1.f[0]; /* movss */
    edx = MEM32(esi + 0x434);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    ecx = esi;
    MEMF(ebp + 0xC) = xmm0.f[0]; /* movss */
    MEMF(ebp + 8) = xmm0.f[0]; /* movss */
    { uint32_t _icall_target = MEM32(eax + 0x150); PUSH32(esp, 0x0011ACE3u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0011ACDDu); } /* indirect call */
    }

loc_0011ACE3: ;
    goto loc_0011ACF5;

loc_0011ACE5: ;
    MEMF(esi + 0x430) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x434) = xmm0.f[0]; /* movss */

loc_0011ACF5: ;
    eax = MEM32(esi);
    ecx = esi;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 0x10C); PUSH32(esp, 0x0011ACFFu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0011ACF9u); } /* indirect call */
    }

loc_0011ACFF: ;
    MEMF(esp + 0xC) = (float)fp_top(); fp_pop(); /* fstp */
    _fa = (uint32_t)(MEM8(esi + 0x1C0)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(esi + 0x1C0), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0011AD4E; /* je: equal / zero */

loc_0011AD0C: ;
    xmm3 = XMM_ZERO(); /* xorps self = zero */
    xmm2 = XMM_SCALAR(MEMF(ebp + 8)); /* movss */
    _fca = xmm3.f[0]; _fcb = xmm2.f[0]; /* comiss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x376168)); /* movss */
    if ((_fca > _fcb)) goto loc_0011AD2F; /* ja: above (unsigned >) (xmm3.f[0] vs xmm2.f[0]) */

loc_0011AD27: ;
    xmm0 = XMM_SCALAR(MEMF(0x376164)); /* movss */

loc_0011AD2F: ;
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * xmm2.f[0]; /* mulss */
    MEMF(ebp + 8) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x37616C)); /* movss */
    MEMF(ebp + 0x10) = xmm3.f[0]; /* movss */
    goto loc_0011ADE6;

loc_0011AD4E: ;
    ecx = MEM32(esi + 0x1C0);
    ecx = ecx >> 0x1F;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0011AD9B; /* je: equal / zero */

loc_0011AD5C: ;
    xmm3 = XMM_ZERO(); /* xorps self = zero */
    xmm2 = XMM_SCALAR(MEMF(ebp + 8)); /* movss */
    _fca = xmm3.f[0]; _fcb = xmm2.f[0]; /* comiss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x376178)); /* movss */
    if ((_fca > _fcb)) goto loc_0011AD7F; /* ja: above (unsigned >) (xmm3.f[0] vs xmm2.f[0]) */

loc_0011AD77: ;
    xmm0 = XMM_SCALAR(MEMF(0x376174)); /* movss */

loc_0011AD7F: ;
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * xmm2.f[0]; /* mulss */
    MEMF(ebp + 8) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x37617C)); /* movss */
    MEMF(ebp + 0x10) = xmm3.f[0]; /* movss */
    goto loc_0011ADE6;

loc_0011AD9B: ;
    ecx = esi + 0x13D4;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011ADA6u); RECOMP_ABI_CALL(0x00127A40u, sub_00127A40); /* call 0x00127A40 */

loc_0011ADA6: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0xC)); /* fmul dword ptr [esp + 0xc] */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    xmm2 = XMM_SCALAR(MEMF(ebp + 8)); /* movss */
    MEMF(esp + 0xC) = (float)fp_top(); fp_pop(); /* fstp */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x37614C)); /* movss */
    if ((_fca > _fcb)) goto loc_0011ADD1; /* ja: above (unsigned >) (xmm0.f[0] vs xmm2.f[0]) */

loc_0011ADC9: ;
    xmm0 = XMM_SCALAR(MEMF(0x376148)); /* movss */

loc_0011ADD1: ;
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * xmm2.f[0]; /* mulss */
    MEMF(ebp + 8) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x376150)); /* movss */

loc_0011ADE6: ;
    edx = MEM32(ebp + 0x10);
    eax = MEM32(ebp + 8);
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebp + 0xC); /* mulss */
    PUSH32(esp, edx);
    MEMF(ebp + 0xC) = xmm0.f[0]; /* movss */
    ecx = MEM32(ebp + 0xC);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    ecx = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011AE07u); RECOMP_ABI_CALL(0x00113E40u, sub_00113E40); /* call 0x00113E40 */

loc_0011AE07: ;
    eax = MEM32(esi + 0x1C0);
    edx = eax;
    edx = edx >> 0x12;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0011AE3E; /* je: equal / zero */

loc_0011AE17: ;
    xmm0 = XMM_SCALAR(MEMF(0x376188)); /* movss */
    xmm1 = xmm0; /* movaps */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebp + 8); /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(ebp + 0xC); /* mulss */
    MEMF(ebp + 8) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(ebp + 0xC) = xmm1.f[0]; /* movss */
    MEMF(ebp + 0x10) = xmm0.f[0]; /* movss */

loc_0011AE3E: ;
    eax = eax >> 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 1 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_0011AFBF; /* jne: not equal / not zero */

loc_0011AE4B: ;
    xmm0 = XMM_SCALAR(MEMF(ebp + 0x10)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x3210B0); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0011AFBF; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x3210B0)) */

loc_0011AE5D: ;
    eax = MEM32(esi);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 4);
    ecx = esi;
    { uint32_t _icall_target = MEM32(eax + 0x5C); PUSH32(esp, 0x0011AE66u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0011AE63u); } /* indirect call */
    }

loc_0011AE66: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0011AFB7; /* je: equal / zero */

loc_0011AE6E: ;
    edx = MEM32(esi);
    ecx = esi;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0xE8); PUSH32(esp, 0x0011AE78u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0011AE72u); } /* indirect call */
    }

loc_0011AE78: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0011AFB7; /* je: equal / zero */

loc_0011AE80: ;
    eax = MEM32(esi);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 2);
    ecx = esi;
    { uint32_t _icall_target = MEM32(eax + 0x60); PUSH32(esp, 0x0011AE89u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0011AE86u); } /* indirect call */
    }

loc_0011AE89: ;
    ecx = (uint32_t)(int32_t)SMEM16(esi + 0x330);
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    xmm0 = XMM_SCALAR(MEMF(0x2FE99C)); /* movss */
    MEM32(esp + 0x68) = ecx;
    ecx = esp + 0x40;
    MEM32(esp + 0x44) = ebx;
    MEM32(esp + 0x48) = ebx;
    MEM32(esp + 0x5C) = ebx;
    MEM32(esp + 0x50) = ebx;
    MEM32(esp + 0x54) = ebx;
    MEM32(esp + 0x58) = ebx;
    MEMF(esp + 0x4C) = xmm1.f[0]; /* movss */
    MEMF(esp + 0x70) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x78) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x40) = 0x30BC64;
    MEM32(esp + 0x88) = ebx;
    MEM32(esp + 0x8C) = ebx;
    MEM32(esp + 0xA0) = ebx;
    MEM32(esp + 0x94) = ebx;
    MEM32(esp + 0x98) = ebx;
    MEM32(esp + 0x9C) = ebx;
    MEMF(esp + 0x90) = xmm1.f[0]; /* movss */
    MEM32(esp + 0x84) = 0x2FF9EC;
    MEM32(esp + 0xA4) = ebx;
    MEMF(esp + 0xA8) = xmm0.f[0]; /* movss */
    MEM8(esp + 0xAC) = LO8(ebx);
    MEM8(esp + 0x2C) = LO8(ebx);
    MEM32(esp + 0x28) = 0x2FFA54;
    MEM32(esp + 0x34) = ebx;
    MEM32(esp + 0x38) = ebx;
    MEM32(esp + 0x3C) = ebx;
    MEM32(esp + 0x30) = 0x2FF02C;
    MEM32(esp + 0x74) = 1;
    MEM32(esp + 0x60) = ebx;
    MEMF(esp + 0x64) = xmm1.f[0]; /* movss */
    MEM32(esp + 0x6C) = 0x11;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011AF69u); RECOMP_ABI_CALL(0x00143580u, sub_00143580); /* call 0x00143580 */

loc_0011AF69: ;
    ecx = MEM32(esp + 0x34);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    MEM32(esp + 0x28) = 0x2FFA54;
    MEM32(esp + 0x30) = 0x2FEC40;
    if (CMP_EQ(_fa, _fb)) goto loc_0011AF8B; /* je: equal / zero */

loc_0011AF81: ;
    edx = esp + 0x30;
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011AF8Bu); RECOMP_ABI_CALL(0x0012F650u, sub_0012F650); /* call 0x0012F650 */

loc_0011AF8B: ;
    eax = MEM32(esi + 0x168);
    ecx = esi + 0x13D4;
    MEM32(esi + 0x178) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011AFA2u); RECOMP_ABI_CALL(0x00127A50u, sub_00127A50); /* call 0x00127A50 */

loc_0011AFA2: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x37615C)); /* fmul dword ptr [0x37615c] */
    edx = MEM32(esi);
    ecx = esi;
    MEMF(ebp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0xEC); PUSH32(esp, 0x0011AFB5u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0011AFAFu); } /* indirect call */
    }

loc_0011AFB5: ;
    goto loc_0011AFBF;

loc_0011AFB7: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(ebp + 0x10) = xmm0.f[0]; /* movss */

loc_0011AFBF: ;
    ecx = MEM32(esi + 0x1CC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0011AFDC; /* je: equal / zero */

loc_0011AFC9: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011AFCEu); RECOMP_ABI_CALL(0x000F3C30u, sub_000F3C30); /* call 0x000F3C30 */

loc_0011AFCE: ;
    fp_push(MEMF(ebp + 0xC)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * fp_st1()); /* fmul st(1) */
    MEMF(ebp + 0xC) = (float)fp_top(); fp_pop(); /* fstp */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebp + 8)); /* fmul dword ptr [ebp + 8] */
    MEMF(ebp + 8) = (float)fp_top(); fp_pop(); /* fstp */

loc_0011AFDC: ;
    xmm0 = XMM_MEM(esi + 0x60); /* movaps */
    XMM_STORE(esp + 0x40, xmm0); /* movaps */
    xmm0 = XMM_MEM(esi + 0x70); /* movaps */
    XMM_STORE(esp + 0x50, xmm0); /* movaps */
    xmm0 = XMM_MEM(esi + 0x80); /* movaps */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x50)); /* movss */
    XMM_STORE(esp + 0x60, xmm0); /* movaps */
    xmm0 = XMM_SCALAR(MEMF(esi + 0x1C8)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x2FF020); /* mulss */
    MEMF(esp + 0x10) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    PUSH32(esp, ecx);
    MEMF(esp + 0x10) = xmm0.f[0]; /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebp + 8); /* mulss */
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    ecx = esp + 0x14;
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    MEMF(esp) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011B045u); RECOMP_ABI_CALL(0x00266200u, sub_00266200); /* call 0x00266200 */

loc_0011B045: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x170)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esp + 0x10); /* addss */
    MEMF(esi + 0x170) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x174); /* addss */
    MEMF(esi + 0x174) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x178); /* addss */
    MEMF(esi + 0x178) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x40)); /* movss */
    MEMF(esp + 0x10) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x44)); /* movss */
    MEMF(esp + 0x14) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebp + 0xC); /* mulss */
    PUSH32(esp, ecx);
    ecx = esp + 0x14;
    MEMF(esp) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011B0C2u); RECOMP_ABI_CALL(0x00266200u, sub_00266200); /* call 0x00266200 */

loc_0011B0C2: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x170)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esp + 0x10); /* addss */
    MEMF(esi + 0x170) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x174); /* addss */
    MEMF(esi + 0x174) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x178); /* addss */
    MEMF(esi + 0x178) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x60)); /* movss */
    MEMF(esp + 0x10) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x64)); /* movss */
    MEMF(esp + 0x14) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x68)); /* movss */
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebp + 0x10); /* mulss */
    PUSH32(esp, ecx);
    ecx = esp + 0x14;
    MEMF(esp) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0011B142u); RECOMP_ABI_CALL(0x00266200u, sub_00266200); /* call 0x00266200 */

loc_0011B142: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x170)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esp + 0x10); /* addss */
    MEMF(esi + 0x170) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x174); /* addss */
    MEMF(esi + 0x174) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x178); /* addss */
    MEMF(esi + 0x178) = xmm0.f[0]; /* movss */

loc_0011B184: ;
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 16; return; /* ret 12 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/* Android-safe DirectSound flush wait: never release a stream that the
 * audio service still reports as active. The bounded timeout prevents a
 * missing APU stop notification from hanging campaign loading. */
void game_patch_00196467(void)
{
    uint64_t wait_start = 0;
    int still_playing = 0;
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_00196467: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    PUSH32(esp, ecx);
    PUSH32(esp, 1);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, MEM32(0x5239B8));
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0019647Cu); RECOMP_ABI_CALL(0x002ECCBFu, sub_002ECCBF); /* call 0x002ECCBF */

    wait_start = android_patch_monotonic_ms();
loc_0019647C: ;
    eax = ebp + -4;
    PUSH32(esp, eax);
    PUSH32(esp, MEM32(0x5239B8));
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0019648Bu); RECOMP_ABI_CALL(0x002ECD1Fu, sub_002ECD1F); /* call 0x002ECD1F */

loc_0019648B: ;
    _fa = (uint32_t)(MEM8(ebp + -4)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ebp + -4), 1 (8-bit) */
    if (TEST_NZ(_fa, _fb)) {
        /* PATCH: bounded. The APU model does not always report the stream
         * stopped; on the console this takes a few milliseconds. */
        if (android_patch_monotonic_ms() - wait_start < 2000u) {
            RECOMP_BACK_EDGE(0x0019648Fu);
            android_patch_sleep_ms(1u);
            goto loc_0019647C;
        }
        still_playing = 1;
    }

loc_00196491: ;
    /* PATCH: release the stream only once it has stopped. Freed while its
     * voices are still on DirectSound's service list, the next interrupt
     * walks freed memory. A stream left alive costs a few KB. */
    if (!still_playing) {
        PUSH32(esp, MEM32(0x5239B8));
        g_ebp = ebp; /* frame stays current across calls */
        g_seh_ebp = ebp;
        PUSH32(esp, 0x0019649Cu); RECOMP_ABI_CALL(0x002EBFE1u, sub_002EBFE1); /* call 0x002EBFE1 */
    }

loc_0019649C: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001964A1u); RECOMP_ABI_CALL(0x001950E3u, sub_001950E3); /* call 0x001950E3 */

loc_001964A1: ;
    PUSH32(esp, MEM32(0x5239D0));
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001964ACu); RECOMP_ABI_CALL(0x00197A91u, sub_00197A91); /* call 0x00197A91 */

loc_001964AC: ;
    POP32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001964B2u); RECOMP_ABI_CALL(0x001950F1u, sub_001950F1); /* call 0x001950F1 */

loc_001964B2: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001964B7u); RECOMP_ABI_CALL(0x001950E3u, sub_001950E3); /* call 0x001950E3 */

loc_001964B7: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001964BCu); RECOMP_ABI_CALL(0x00199C1Au, sub_00199C1A); /* call 0x00199C1A */

loc_001964BC: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001964C1u); RECOMP_ABI_CALL(0x001950F1u, sub_001950F1); /* call 0x001950F1 */

loc_001964C1: ;
    esp = ebp;
    POP32(esp, ebp); /* leave */
    esp += 4; return; /* ret */

}


/* Original guest emitter-update body. The desktop-only FPS accumulator and
 * emitter diagnostics/reaper are intentionally excluded until their audio
 * service ownership contract is ported for Android. */
void game_patch_001AD630(void)
{
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)

loc_001AD630: ;
    PUSH32(esp, esi);
    esi = ecx;
    SET_LO8(eax, MEM8(esi + 0x10332));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001AD6E4; /* jne: not equal / not zero */

loc_001AD641: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x100C8)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(esi + 0x100D0); /* comiss */
    if ((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001AD667; /* jb: below (unsigned <) (xmm0.f[0] vs MEMF(esi + 0x100D0)) */

loc_001AD652: ;
    eax = MEM32(esi + 0x100C0);
    MEM32(esi + 0x100BC) = eax;
    MEM8(esi + 0x100CC) = 0;
    goto loc_001AD6A0;

loc_001AD667: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 8)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(esi + 0x100C4)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] + MEMF(esi + 0x100BC); /* addss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 0x100C8); /* addss */
    MEMF(esi + 0x100BC) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x100C8) = xmm0.f[0]; /* movss */
    MEM8(esi + 0x100CC) = 1;

loc_001AD6A0: ;
    PUSH32(esp, 0x001AD6A5u); RECOMP_ABI_CALL(0x001950E3u, sub_001950E3); /* call 0x001950E3 */

loc_001AD6A5: ;
    ecx = esi;
    PUSH32(esp, 0x001AD6ACu); RECOMP_ABI_CALL(0x001AD2E0u, sub_001AD2E0); /* call 0x001AD2E0 */

loc_001AD6AC: ;
    ecx = MEM32(esp + 8);
    PUSH32(esp, ecx);
    ecx = MEM32(0x524300);
    PUSH32(esp, 0x001AD6BCu); RECOMP_ABI_CALL(0x001B1460u, sub_001B1460); /* call 0x001B1460 */

loc_001AD6BC: ;
    PUSH32(esp, 0x001AD6C1u); RECOMP_ABI_CALL(0x001950F1u, sub_001950F1); /* call 0x001950F1 */

loc_001AD6C1: ;
    eax = MEM32(esi + 0x1032C);
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    ecx = 0x14;
    { int64_t _dividend = ((int64_t)(int32_t)edx << 32) | eax;
      eax = (uint32_t)((int32_t)(_dividend / (int32_t)ecx));
      edx = (uint32_t)((int32_t)(_dividend % (int32_t)ecx)); }
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001AD6DE; /* jne: not equal / not zero */

loc_001AD6D3: ;
    PUSH32(esp, 0x001AD6D8u); RECOMP_ABI_CALL(0x002B0661u, sub_002B0661); /* call 0x002B0661 */

loc_001AD6D8: ;
    MEMF(esi + 0x10328) = (float)fp_top(); fp_pop(); /* fstp */

loc_001AD6DE: ;
    MEM32(esi + 0x1032C) = MEM32(esi + 0x1032C) + 1;
    _fa = (uint32_t)(MEM32(esi + 0x1032C)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_001AD6E4: ;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}
