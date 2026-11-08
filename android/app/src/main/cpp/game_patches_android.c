/*
 * Android-portable manual game patches.
 *
 * Keep these lifted guest functions separate from desktop-only additions in
 * source/mygame/src/game_patches.c. The generated runtime supplies the guest
 * register, memory, x87 and dispatcher contracts through these headers.
 */
#include "recomp_types.h"
#include "recomp_funcs.h"

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
