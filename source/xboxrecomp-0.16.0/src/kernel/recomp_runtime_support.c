/*
 * Small host-side diagnostics used by generated Xbox code.
 *
 * These functions report unresolved indirect calls and explicit
 * recompilation gaps. They do not fabricate game behavior or return fake
 * translated functions.
 */
#include <stdint.h>
#include <stdio.h>

void recomp_icall_fail_log(uint32_t va)
{
    fprintf(stderr, "[recomp] indirect call target 0x%08X has no translation\n",
            (unsigned)va);
    fflush(stderr);
}

void recomp_icall_not_code_log(uint32_t va)
{
    fprintf(stderr, "[recomp] indirect call target 0x%08X is not known game code\n",
            (unsigned)va);
    fflush(stderr);
}

void recomp_unimpl(const char *text, uint32_t va)
{
    fprintf(stderr, "[recomp] unimplemented operation at Xbox VA 0x%08X: %s\n",
            (unsigned)va, text ? text : "(no description)");
    fflush(stderr);
}
