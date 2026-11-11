#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "d3d8_xbox.h"
#include "kernel.h"
#include "xbox_memory_layout.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <mutex>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <cstdint>
#include <fstream>
#include <vector>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <ucontext.h>
#include <dlfcn.h>

extern "C" void (*recomp_lookup(uint32_t xbox_va))(void);
/* The recompiler models x86 registers as thread-local state. MemoryLayoutInit
 * initializes ESP to XBOX_STACK_TOP on this render/guest thread. */
#if defined(__GNUC__) || defined(__clang__)
extern __thread uint32_t g_esp, g_eax, g_ecx, g_edx;
extern __thread uint32_t g_ebx, g_esi, g_edi;
#else
extern uint32_t g_esp, g_eax, g_ecx, g_edx;
extern uint32_t g_ebx, g_esi, g_edi;
#endif
#if defined(MOH_GENERATED_RECOMP_LINKED)
extern volatile uint32_t g_icall_trace[16];
extern volatile uint32_t g_icall_trace_idx;
extern volatile uint64_t g_icall_count;
#endif
extern "C" int moh_android_is_foreground(void);

extern "C" void d3d8_android_set_egl_surface(
    EGLDisplay display, EGLSurface surface, int width, int height);

namespace {
std::atomic<bool> g_running{false};
std::thread g_thread;
std::atomic<int> g_stage{0};
std::atomic<int> g_error{0};
std::atomic<unsigned long long> g_frames{0};
std::atomic<unsigned long long> g_game_frames{0};
std::mutex g_game_dir_mutex;
std::atomic<bool> g_runtime_init_requested{false};
std::string g_game_dir;
uint32_t g_xbe_entry = 0;
uint32_t g_xbe_kernel_thunk = 0;
std::string g_xbe_validation = "XBE not inspected";
std::atomic<int> g_runtime_stage{0};
std::string g_runtime_status = "waiting for a valid XBE";
bool g_memory_started = false;
bool g_kernel_started = false;
std::vector<uint8_t> g_xbe_image;

char g_log_path[PATH_MAX] = {0};
char g_crash_marker_path[PATH_MAX] = {0};
volatile sig_atomic_t g_crash_logged = 0;

void writeHexValue(int fd, uintptr_t value) {
    char buffer[2 * sizeof(uintptr_t)];
    size_t count = 0;
    do {
        buffer[count++] = "0123456789ABCDEF"[value & 0xFu];
        value >>= 4;
    } while (value && count < sizeof(buffer));
    while (count) {
        char ch = buffer[--count];
        (void)write(fd, &ch, 1);
    }
}


void appendRuntimeLog(const char* message);

void appendGuestLayoutDiagnostics(void (*entry)(void)) {
    char message[512];
    const uintptr_t hostEntry = reinterpret_cast<uintptr_t>(entry);
    Dl_info info{};
    if (dladdr(reinterpret_cast<void*>(hostEntry), &info) && info.dli_fname) {
        snprintf(message, sizeof(message),
            "Diagnostics: dispatcher host_entry=%p module=%s module_base=%p symbol=%s symbol_addr=%p",
            reinterpret_cast<void*>(hostEntry), info.dli_fname, info.dli_fbase,
            info.dli_sname ? info.dli_sname : "(unknown)", info.dli_saddr);
    } else {
        snprintf(message, sizeof(message),
            "Diagnostics: dispatcher host_entry=%p module=(dladdr unavailable)",
            reinterpret_cast<void*>(hostEntry));
    }
    appendRuntimeLog(message);
    snprintf(message, sizeof(message),
        "Diagnostics: guest image=0x%08X..0x%08X code=0x%08X..0x%08X region=0x%08X stack=0x%08X..0x%08X size=%u ESP=0x%08X mem_offset=%lld mapped=%zu",
        g_xbox_image_lo, g_xbox_image_hi, g_xbox_code_lo, g_xbox_code_hi,
        g_xbox_region_base, g_xbox_stack_base,
        g_xbox_stack_base + g_xbox_stack_size, g_xbox_stack_size, g_esp,
        static_cast<long long>(xbox_GetMemoryOffset()), xbox_GetMappedSize());
    appendRuntimeLog(message);
}

void appendRuntimeLog(const char* message) {
    if (!g_log_path[0] || !message) return;
    int fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if (fd < 0) return;
    (void)write(fd, message, strlen(message));
    (void)write(fd, "\n", 1);
    close(fd);
}
void fatalSignalHandler(int signalNumber, siginfo_t* info, void* rawContext) {
    if (g_crash_logged) _exit(128 + signalNumber);
    g_crash_logged = 1;

    /* Persist the crash marker before attempting the optional /proc snapshot.
     * If snapshot collection stalls or fails, the app still reports the crash. */
    if (g_crash_marker_path[0]) {
        int marker = open(g_crash_marker_path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (marker >= 0) {
            static const char crashed[] = "native-crash\\n";
            (void)write(marker, crashed, sizeof(crashed) - 1);
            close(marker);
        }
    }

    if (g_log_path[0]) {
        int fd = open(g_log_path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
        if (fd >= 0) {
            static const char prefix[] = "\nFATAL NATIVE SIGNAL: ";
            (void)write(fd, prefix, sizeof(prefix) - 1);
            writeHexValue(fd, (uintptr_t)signalNumber);
            static const char addressPrefix[] = " fault_addr=0x";
            (void)write(fd, addressPrefix, sizeof(addressPrefix) - 1);
            writeHexValue(fd, info ? reinterpret_cast<uintptr_t>(info->si_addr) : 0);
            static const char codePrefix[] = " si_code=";
            (void)write(fd, codePrefix, sizeof(codePrefix) - 1);
            writeHexValue(fd, info ? (uintptr_t)info->si_code : 0);

#if defined(__aarch64__)
            const ucontext_t* context = static_cast<const ucontext_t*>(rawContext);
            if (context) {
                static const char pcPrefix[] = " pc=0x";
                static const char spPrefix[] = " sp=0x";
                static const char lrPrefix[] = " lr=0x";
                static const char x0Prefix[] = " x0=0x";
                static const char x1Prefix[] = " x1=0x";
                static const char x2Prefix[] = " x2=0x";
                static const char x3Prefix[] = " x3=0x";
                static const char x4Prefix[] = " x4=0x";
                static const char x5Prefix[] = " x5=0x";
                static const char x6Prefix[] = " x6=0x";
                static const char x7Prefix[] = " x7=0x";
                static const char x8Prefix[] = " x8=0x";
                static const char x9Prefix[] = " x9=0x";
                static const char x10Prefix[] = " x10=0x";
                static const char x11Prefix[] = " x11=0x";
                static const char x12Prefix[] = " x12=0x";
                static const char x13Prefix[] = " x13=0x";
                static const char x14Prefix[] = " x14=0x";
                static const char x15Prefix[] = " x15=0x";
                static const char x16Prefix[] = " x16=0x";
                static const char x17Prefix[] = " x17=0x";
                static const char x18Prefix[] = " x18=0x";
                static const char x19Prefix[] = " x19=0x";
                static const char x20Prefix[] = " x20=0x";
                static const char x21Prefix[] = " x21=0x";
                static const char x22Prefix[] = " x22=0x";
                static const char x23Prefix[] = " x23=0x";
                static const char x24Prefix[] = " x24=0x";
                static const char x25Prefix[] = " x25=0x";
                static const char x26Prefix[] = " x26=0x";
                static const char x27Prefix[] = " x27=0x";
                static const char x28Prefix[] = " x28=0x";
                static const char x29Prefix[] = " x29=0x";
                (void)write(fd, pcPrefix, sizeof(pcPrefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.pc);
                (void)write(fd, spPrefix, sizeof(spPrefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.sp);
                (void)write(fd, lrPrefix, sizeof(lrPrefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[30]);
                (void)write(fd, x0Prefix, sizeof(x0Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[0]);
                (void)write(fd, x1Prefix, sizeof(x1Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[1]);
                (void)write(fd, x2Prefix, sizeof(x2Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[2]);
                (void)write(fd, x3Prefix, sizeof(x3Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[3]);
                (void)write(fd, x4Prefix, sizeof(x4Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[4]);
                (void)write(fd, x5Prefix, sizeof(x5Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[5]);
                (void)write(fd, x6Prefix, sizeof(x6Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[6]);
                (void)write(fd, x7Prefix, sizeof(x7Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[7]);
                (void)write(fd, x8Prefix, sizeof(x8Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[8]);
                (void)write(fd, x9Prefix, sizeof(x9Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[9]);
                (void)write(fd, x10Prefix, sizeof(x10Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[10]);
                (void)write(fd, x11Prefix, sizeof(x11Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[11]);
                (void)write(fd, x12Prefix, sizeof(x12Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[12]);
                (void)write(fd, x13Prefix, sizeof(x13Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[13]);
                (void)write(fd, x14Prefix, sizeof(x14Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[14]);
                (void)write(fd, x15Prefix, sizeof(x15Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[15]);
                (void)write(fd, x16Prefix, sizeof(x16Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[16]);
                (void)write(fd, x17Prefix, sizeof(x17Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[17]);
                (void)write(fd, x18Prefix, sizeof(x18Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[18]);
                (void)write(fd, x19Prefix, sizeof(x19Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[19]);
                (void)write(fd, x20Prefix, sizeof(x20Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[20]);
                (void)write(fd, x21Prefix, sizeof(x21Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[21]);
                (void)write(fd, x22Prefix, sizeof(x22Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[22]);
                (void)write(fd, x23Prefix, sizeof(x23Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[23]);
                (void)write(fd, x24Prefix, sizeof(x24Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[24]);
                (void)write(fd, x25Prefix, sizeof(x25Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[25]);
                (void)write(fd, x26Prefix, sizeof(x26Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[26]);
                (void)write(fd, x27Prefix, sizeof(x27Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[27]);
                (void)write(fd, x28Prefix, sizeof(x28Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[28]);
                (void)write(fd, x29Prefix, sizeof(x29Prefix) - 1);
                writeHexValue(fd, (uintptr_t)context->uc_mcontext.regs[29]);
                /* Do not dereference the interrupted SP in a signal handler.
                 * The stack may be the cause of the fault or may no longer be
                 * mapped; a second SIGSEGV here destroys the original report. */
                static const char stackNote[] = " stack_words=(not-read; unsafe in signal handler)";
                (void)write(fd, stackNote, sizeof(stackNote) - 1);
#if defined(MOH_GENERATED_RECOMP_LINKED)
                static const char guestPrefix[] = " guest_x86{eax=0x";
                static const char ecxPrefix[] = " ecx=0x";
                static const char edxPrefix[] = " edx=0x";
                static const char ebxPrefix[] = " ebx=0x";
                static const char esiPrefix[] = " esi=0x";
                static const char ediPrefix[] = " edi=0x";
                static const char espPrefix[] = " esp=0x";
                static const char tracePrefix[] = " icall_count=0x";
                (void)write(fd, guestPrefix, sizeof(guestPrefix) - 1);
                writeHexValue(fd, g_eax);
                (void)write(fd, ecxPrefix, sizeof(ecxPrefix) - 1);
                writeHexValue(fd, g_ecx);
                (void)write(fd, edxPrefix, sizeof(edxPrefix) - 1);
                writeHexValue(fd, g_edx);
                (void)write(fd, ebxPrefix, sizeof(ebxPrefix) - 1);
                writeHexValue(fd, g_ebx);
                (void)write(fd, esiPrefix, sizeof(esiPrefix) - 1);
                writeHexValue(fd, g_esi);
                (void)write(fd, ediPrefix, sizeof(ediPrefix) - 1);
                writeHexValue(fd, g_edi);
                (void)write(fd, espPrefix, sizeof(espPrefix) - 1);
                writeHexValue(fd, g_esp);
                (void)write(fd, tracePrefix, sizeof(tracePrefix) - 1);
                writeHexValue(fd, g_icall_count);
                const uint32_t traceEnd = g_icall_trace_idx;
                for (uint32_t i = 0; i < 16; ++i) {
                    const uint32_t target = g_icall_trace[(traceEnd - 1u - i) & 15u];
                    static const char targetPrefix[] = " icall=0x";
                    (void)write(fd, targetPrefix, sizeof(targetPrefix) - 1);
                    writeHexValue(fd, target);
                }
                static const char guestEnd[] = "}";
                (void)write(fd, guestEnd, sizeof(guestEnd) - 1);
#endif
            }
#endif
            (void)write(fd, "\n", 1);

            close(fd);
        }
    }
    /*
     * SA_RESETHAND has already restored SIG_DFL for this signal. Return to
     * the interrupted instruction without changing its context: a synchronous
     * fault is raised again on sigreturn and the kernel applies the default
     * fatal disposition. This avoids _exit(139), which Android classifies as
     * REASON_EXIT_SELF and for which it omits the native tombstone.
     */
    struct sigaction defaultAction{};
    defaultAction.sa_handler = SIG_DFL;
    sigemptyset(&defaultAction.sa_mask);
    (void)sigaction(signalNumber, &defaultAction, nullptr);
    return;
}
void installCrashDiagnostics() {
    struct sigaction action{};
    action.sa_sigaction = fatalSignalHandler;
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
    sigaction(SIGILL, &action, nullptr);
    sigaction(SIGFPE, &action, nullptr);
}


uint32_t readLe32(const std::vector<uint8_t>& bytes, size_t offset) {
    return static_cast<uint32_t>(bytes[offset]) |
        (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
        (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
        (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

void inspectXbeLocked() {
    g_xbe_entry = 0;
    g_xbe_kernel_thunk = 0;
    g_xbe_validation = "default.xbe not found";
    std::ifstream input(g_game_dir + "/default.xbe", std::ios::binary | std::ios::ate);
    if (!input) return;
    const auto length = input.tellg();
    g_xbe_image.clear();
    if (length < 0x164) { g_xbe_validation = "XBE header truncated"; return; }
    // Load the complete XBE into host memory now, but do not map guest RAM or
    // invoke kernel startup from the JNI directory callback.
    if (static_cast<uint64_t>(length) > 512ull * 1024ull * 1024ull) {
        g_xbe_validation = "XBE exceeds 512 MiB safety limit"; return;
    }
    g_xbe_image.resize(static_cast<size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(g_xbe_image.data()), length)) {
        g_xbe_image.clear();
        g_xbe_validation = "Could not read complete XBE image"; return;
    }
    std::vector<uint8_t> header(g_xbe_image.begin(), g_xbe_image.begin() + 0x164);
    if (header[0] != 'X' || header[1] != 'B' || header[2] != 'E' || header[3] != 'H') {
        g_xbe_validation = "Invalid XBEH signature"; return;
    }
    // XBE stores these VAs XOR-encoded. Match the parser's retail/debug
    // selection against the image range before decoding the thunk address.
    const uint32_t base = readLe32(header, 0x104);
    const uint32_t imageSize = readLe32(header, 0x10C);
    const uint32_t entryRaw = readLe32(header, 0x128);
    const uint32_t thunkRaw = readLe32(header, 0x158);
    const uint32_t retailEntry = entryRaw ^ 0xA8FC57ABu;
    const uint32_t debugEntry = entryRaw ^ 0x94859D4Bu;
    const uint64_t imageEnd = static_cast<uint64_t>(base) + imageSize;
    const bool retail = retailEntry >= base && retailEntry < imageEnd;
    const bool debug = debugEntry >= base && debugEntry < imageEnd;
    if (!retail && !debug) {
        g_xbe_validation = "XBEH valid, but encoded entry is outside image";
        return;
    }
    const bool isDebug = !retail && debug;
    g_xbe_entry = isDebug ? debugEntry : retailEntry;
    g_xbe_kernel_thunk = thunkRaw ^ (isDebug ? 0xEFB1F152u : 0x5B6D40B6u);
    g_xbe_validation = (g_xbe_entry && g_xbe_kernel_thunk)
        ? "XBEH valid; decoded entry and kernel thunk"
        : "XBEH valid, but decoded entry point or kernel thunk is zero";
}

void shutdownXboxRuntimeLocked() {
    appendRuntimeLog("Runtime: shutdown requested");
    if (g_kernel_started) {
        appendRuntimeLog("Runtime: shutting down Xbox kernel");
        xbox_kernel_shutdown();
        g_kernel_started = false;
    }
    if (g_memory_started) {
        appendRuntimeLog("Runtime: shutting down Xbox guest memory");
        xbox_MemoryLayoutShutdown();
        g_memory_started = false;
    }
    char line[256];
    snprintf(line, sizeof(line), "Runtime: shutdown complete; retained runtime_stage=%d status=%s",
        g_runtime_stage.load(), g_runtime_status.c_str());
    appendRuntimeLog(line);
}

void logGuestAddressDiagnostics(void (*entry)(void)) {
    char line[512];
    const uintptr_t hostEntry = reinterpret_cast<uintptr_t>(entry);
    void *base = xbox_GetMemoryBase();
    const ptrdiff_t offset = xbox_GetMemoryOffset();
    snprintf(line, sizeof(line),
        "Memory map: base=%p offset=%+td mapped=%zu bytes image=0x%08X..0x%08X stack=0x%08X..0x%08X esp=0x%08X",
        base, offset, xbox_GetMappedSize(), g_xbox_image_lo, g_xbox_image_hi,
        g_xbox_stack_base, g_xbox_stack_base + g_xbox_stack_size, g_esp);
    appendRuntimeLog(line);
    snprintf(line, sizeof(line),
        "Memory translation: guest 0x00F7FC04 -> host %p; direct guest VA=%p",
        reinterpret_cast<void*>(static_cast<uintptr_t>(0x00F7FC04u) + offset),
        reinterpret_cast<void*>(static_cast<uintptr_t>(0x00F7FC04u)));
    appendRuntimeLog(line);
    Dl_info info{};
    if (entry && dladdr(reinterpret_cast<void*>(entry), &info) && info.dli_fname) {
        snprintf(line, sizeof(line), "Dispatcher entry: ptr=%p module=%s symbol=%s symbol_addr=%p",
            reinterpret_cast<void*>(entry), info.dli_fname,
            info.dli_sname ? info.dli_sname : "(unknown)", info.dli_saddr);
        appendRuntimeLog(line);
    } else {
        snprintf(line, sizeof(line), "Dispatcher entry: ptr=%p module lookup unavailable",
            reinterpret_cast<void*>(entry));
        appendRuntimeLog(line);
    }
    /* Capture mappings before entering generated guest code. Reading procfs
     * from a fatal signal handler proved unreliable on affected Android builds.
     * The Kotlin exporter later keeps only rows containing fault_addr, PC, SP or LR. */
    appendRuntimeLog("CRASH MAP SNAPSHOT BEGIN");
    std::ifstream maps("/proc/self/maps");
    std::string mapLine;
    while (std::getline(maps, mapLine))
        appendRuntimeLog(mapLine.c_str());
    appendRuntimeLog("CRASH MAP SNAPSHOT END");
}

void initializeXboxRuntimeLocked() {
    uint32_t thunk_va = 0;
    uint32_t thunk_count = 0;

    if (g_memory_started || g_xbe_image.empty() || !g_xbe_entry ||
        !g_xbe_kernel_thunk) {
        char line[256];
        snprintf(line, sizeof(line),
            "Runtime: initialization rejected; memory_started=%d xbe_bytes=%zu entry=0x%08X thunk=0x%08X",
            g_memory_started ? 1 : 0, g_xbe_image.size(), g_xbe_entry, g_xbe_kernel_thunk);
        appendRuntimeLog(line);
        g_runtime_stage.store(-10);
        g_runtime_status = "initialization prerequisites missing; see runtime log";
        return;
    }

    appendRuntimeLog("Runtime: starting Xbox guest memory initialization");
    g_runtime_stage.store(1);
    g_runtime_status = "reserving the 4 GiB Xbox guest address window";
    /* Playbook ADR 0003: Android reserves the complete 32-bit guest VA range
     * as PROT_NONE and replaces owned slices with MAP_FIXED_NOREPLACE views.
     * The POSIX memory-layout backend enables that reservation through this
     * switch; do it before mapping XBE sections or kernel apertures. */
    if (setenv("RECOMP_EXT_VMA", "1", 1) != 0) {
        char line[160];
        snprintf(line, sizeof(line), "Runtime: setenv(RECOMP_EXT_VMA) failed errno=%d", errno);
        appendRuntimeLog(line);
        g_runtime_stage.store(-1);
        g_runtime_status = "could not enable the Android 4 GiB guest address window";
        return;
    }
    appendRuntimeLog("Runtime: RECOMP_EXT_VMA=1 configured");
    g_runtime_status = "mapping Xbox guest memory and loading XBE sections";
    if (!xbox_MemoryLayoutInit(g_xbe_image.data(), g_xbe_image.size())) {
        xbox_MemoryLayoutShutdown();
        g_runtime_stage.store(-1);
        const char* memoryError = xbox_MemoryLayoutLastError();
        g_runtime_status = (memoryError && memoryError[0])
            ? memoryError : "Xbox guest memory initialization failed (no host diagnostic)";
        appendRuntimeLog("Runtime: guest memory initialization FAILED");
        appendRuntimeLog(g_runtime_status.c_str());
        return;
    }
    g_memory_started = true;
    appendRuntimeLog("Runtime: Xbox guest memory initialized");
    g_runtime_stage.store(2);
    appendRuntimeLog("Diagnostics: guest memory stage reached; capturing current process mappings");
    appendRuntimeLog("PRE-ENTRY MAP SNAPSHOT BEGIN");
    {
        std::ifstream maps("/proc/self/maps");
        std::string mapLine;
        while (std::getline(maps, mapLine)) appendRuntimeLog(mapLine.c_str());
        if (!maps.eof() && maps.fail())
            appendRuntimeLog("Diagnostics: /proc/self/maps read ended with an I/O error");
    }
    appendRuntimeLog("PRE-ENTRY MAP SNAPSHOT END");

    const std::string save_dir = g_game_dir + "/../saves";
    xbox_path_init(g_game_dir.c_str(), save_dir.c_str());
    appendRuntimeLog("Runtime: initializing Xbox kernel");
    xbox_kernel_init();
    g_kernel_started = true;
    appendRuntimeLog("Runtime: Xbox kernel initialized");
    g_runtime_stage.store(3);

    xbox_kernel_get_thunk_address(&thunk_va, &thunk_count);
    {
        char line[192];
        snprintf(line, sizeof(line),
            "Runtime: thunk query base=%p thunk_va=0x%08X thunk_count=%u",
            xbox_GetMemoryBase(), thunk_va, thunk_count);
        appendRuntimeLog(line);
    }
    if (!xbox_GetMemoryBase() || !thunk_va || !thunk_count) {
        g_runtime_status = "guest memory loaded, but XBE kernel thunk table is unavailable";
        g_runtime_stage.store(-2);
        appendRuntimeLog("Runtime: kernel thunk table validation FAILED");
        xbox_kernel_shutdown();
        g_kernel_started = false;
        xbox_MemoryLayoutShutdown();
        g_memory_started = false;
        return;
    }

    appendRuntimeLog("Runtime: installing Xbox kernel thunk bridge");
    xbox_kernel_bridge_init();
    appendRuntimeLog("Runtime: Xbox kernel thunk bridge installed");
    g_runtime_stage.store(4);
#if defined(MOH_GENERATED_RECOMP_LINKED)
    if (!recomp_lookup(g_xbe_entry)) {
        g_runtime_stage.store(-3);
        g_runtime_status = "kernel ready, but generated dispatcher has no function for the XBE entry VA";
        appendRuntimeLog("Runtime: generated dispatcher lookup FAILED");
        return;
    }
    appendRuntimeLog("Runtime: generated dispatcher lookup succeeded");
    /* Match the desktop bootstrap: the generated entry is a void(void)
     * function and consumes the emulated x86 register file/stack. The lookup
     * above only proved that code exists; invoke it now on this same thread,
     * where the EGL context, guest memory and TLS register state are valid. */
    if (g_esp != XBOX_STACK_TOP) {
        char line[160];
        snprintf(line, sizeof(line),
            "Runtime: guest ESP validation FAILED actual=0x%08X expected=0x%08X",
            g_esp, XBOX_STACK_TOP);
        appendRuntimeLog(line);
        g_runtime_stage.store(-4);
        g_runtime_status = "dispatcher ready, but guest ESP was not initialized";
        return;
    }
    appendRuntimeLog("Runtime: guest ESP validation succeeded");
    void (*entry)(void) = recomp_lookup(g_xbe_entry);
    if (!entry) {
        g_runtime_stage.store(-3);
        g_runtime_status = "kernel ready, but generated dispatcher has no function for the XBE entry VA";
        return;
    }
    g_runtime_stage.store(5);
    g_runtime_status = "starting recompiled XBE entry point";
    appendRuntimeLog("Runtime: calling generated XBE entry point");
    appendGuestLayoutDiagnostics(entry);
    logGuestAddressDiagnostics(entry);
    fflush(stdout);
    entry();
    appendRuntimeLog("Runtime: generated XBE entry point returned");
    g_runtime_stage.store(6);
    g_runtime_status = "recompiled XBE entry point returned";
#else
    g_runtime_status = "Xbox guest memory and kernel thunk bridge ready; generated dispatcher not linked";
#endif
}


bool hasGameXbe() {
    if (g_game_dir.empty()) return false;
    const std::string path = g_game_dir + "/default.xbe";
    struct stat info{};
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

void renderLoop(ANativeWindow* window) {
    g_stage.store(1); g_error.store(0); g_frames.store(0);
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) {
        appendRuntimeLog("Renderer: eglInitialize failed");
        ANativeWindow_release(window);
        g_running.store(false);
        return;
    }
    const EGLint configAttrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE
    };
    EGLConfig config = nullptr;
    EGLint configCount = 0;
    if (!eglChooseConfig(display, configAttrs, &config, 1, &configCount) || configCount < 1) {
        appendRuntimeLog("Renderer: eglChooseConfig failed");
        eglTerminate(display); ANativeWindow_release(window); g_running.store(false); return;
    }
    const EGLint contextAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs);
    EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE ||
        !eglMakeCurrent(display, surface, surface, context)) {
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        appendRuntimeLog("Renderer: EGL context or window surface creation failed");
        eglTerminate(display); ANativeWindow_release(window); g_running.store(false); return;
    }
    GLint width = 0, height = 0;
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    d3d8_android_set_egl_surface(display, surface, width, height);

    /* Never auto-run imported guest code on process startup. The user must
     * explicitly request a start after import or after exporting crash logs. */
    appendRuntimeLog("Renderer: EGL context ready; waiting for explicit game start");
    g_stage.store(4);
    g_stage.store(5);
    IDirect3D8* d3d = xbox_Direct3DCreate8(0);
    IDirect3DDevice8* device = nullptr;
    D3DPRESENT_PARAMETERS pp{};
    pp.BackBufferWidth = static_cast<UINT>(width);
    pp.BackBufferHeight = static_cast<UINT>(height);
    pp.BackBufferCount = 1;
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    HRESULT createResult = d3d ? d3d->lpVtbl->CreateDevice(
        d3d, 0, 1u, nullptr, 0, &pp, &device) : static_cast<HRESULT>(-1);
    if (createResult == 0 && device) {
        g_stage.store(6);
        while (g_running.load()) {
            if (g_runtime_init_requested.exchange(false)) {
                appendRuntimeLog("UI: render thread accepted game start request");
                std::lock_guard<std::mutex> lock(g_game_dir_mutex);
                appendRuntimeLog("UI: game start request acquired game directory mutex");
                shutdownXboxRuntimeLocked();
                initializeXboxRuntimeLocked();
                {
                    char line[256];
                    snprintf(line, sizeof(line),
                        "UI: initialization attempt finished; runtime_stage=%d status=%s",
                        g_runtime_stage.load(), g_runtime_status.c_str());
                    appendRuntimeLog(line);
                }
            }
            HRESULT clearResult = device->lpVtbl->Clear(
                device, 0, nullptr, D3DCLEAR_TARGET, 0xFF17365Du, 1.0f, 0);
            if (clearResult != 0) { g_error.store(clearResult); g_stage.store(-7); break; }
            HRESULT presentResult = device->lpVtbl->Present(
                device, nullptr, nullptr, nullptr, nullptr);
            if (presentResult != 0) { g_error.store(presentResult); g_stage.store(-5); break; }
            g_frames.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    } else {
        char createMessage[128];
        snprintf(createMessage, sizeof(createMessage), "Renderer: D3D8 CreateDevice failed HRESULT=0x%08X", (unsigned int)createResult);
        appendRuntimeLog(createMessage);
        g_error.store(createResult); g_stage.store(-6);
    }
    /* Tear down guest kernel/memory on the same thread that initialized them,
     * before releasing the EGL context used by the Android D3D8 backend. */
    {
        std::lock_guard<std::mutex> lock(g_game_dir_mutex);
        shutdownXboxRuntimeLocked();
        if (g_runtime_stage.load() >= 0) {
            g_runtime_status = "renderer stopped; guest runtime is not active";
        }
        appendRuntimeLog("Renderer: EGL teardown complete");
    }
    d3d8_android_set_egl_surface(EGL_NO_DISPLAY, EGL_NO_SURFACE, 0, 0);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    ANativeWindow_release(window);
    g_running.store(false);
}
} // namespace

/* Called by the translated game's frame-limiter hook once the real game loop is active. */
extern "C" void moh_android_note_frame(void) {
    g_game_frames.fetch_add(1, std::memory_order_relaxed);
}

extern "C" int moh_android_is_foreground(void) {
    return g_running.load(std::memory_order_relaxed) ? 1 : 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeSetGameDirectory(
    JNIEnv* env, jobject, jstring path) {
    const char* chars = path ? env->GetStringUTFChars(path, nullptr) : nullptr;
    {
        std::lock_guard<std::mutex> lock(g_game_dir_mutex);
        g_game_dir = chars ? chars : "";
        inspectXbeLocked();
        char importMessage[256];
        snprintf(importMessage, sizeof(importMessage),
            "Import: XBE bytes=%zu entry=0x%08X thunk=0x%08X validation=%s",
            g_xbe_image.size(), g_xbe_entry, g_xbe_kernel_thunk, g_xbe_validation.c_str());
        appendRuntimeLog(importMessage);
        if (!g_xbe_image.empty() && g_xbe_entry && g_xbe_kernel_thunk) {
            g_runtime_stage.store(1);
            g_runtime_status = "waiting for EGL render thread";
        }
    }
    if (chars) env->ReleaseStringUTFChars(path, chars);
    g_runtime_init_requested.store(false);
    appendRuntimeLog("Import: game directory selected and XBE inspected");
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeConfigureLogging(
    JNIEnv* env, jobject, jstring directory) {
    const char* chars = directory ? env->GetStringUTFChars(directory, nullptr) : nullptr;
    if (chars) {
        snprintf(g_log_path, sizeof(g_log_path), "%s/moh-runtime.log", chars);
        snprintf(g_crash_marker_path, sizeof(g_crash_marker_path), "%s/moh-crash.marker", chars);
        env->ReleaseStringUTFChars(directory, chars);
        installCrashDiagnostics();
        appendRuntimeLog("Diagnostics: native crash handlers installed");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeRequestGameStart(JNIEnv*, jobject) {
    g_runtime_init_requested.store(true);
    char line[192];
    snprintf(line, sizeof(line),
        "UI: game start requested; renderer_running=%d xbe_bytes=%zu entry=0x%08X thunk=0x%08X",
        g_running.load() ? 1 : 0, g_xbe_image.size(), g_xbe_entry, g_xbe_kernel_thunk);
    appendRuntimeLog(line);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeStatus(JNIEnv* env, jobject) {
    char message[384];
    /* Status is polled from the UI thread. Never block it while the guest
     * startup path owns this mutex (including the generated XBE entry call). */
    std::unique_lock<std::mutex> lock(g_game_dir_mutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return env->NewStringUTF("runtime ocupado; diagnóstico local disponível");
    }
    const bool xbeReady = hasGameXbe();
    snprintf(message, sizeof(message),
        "stage=%d render_frames=%llu game_frames=%llu error=0x%08X. XBE %s (%zu bytes); %s; entry=0x%08X thunk=0x%08X; runtime_stage=%d (%s).",
        g_stage.load(), g_frames.load(), g_game_frames.load(), static_cast<unsigned int>(g_error.load()),
        xbeReady ? "imported" : "not imported", g_xbe_image.size(), g_xbe_validation.c_str(),
        g_xbe_entry, g_xbe_kernel_thunk, g_runtime_stage.load(), g_runtime_status.c_str());
    return env->NewStringUTF(message);
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeStart(
    JNIEnv* env, jobject, jobject javaSurface) {
    if (g_running.exchange(true)) return;
    ANativeWindow* window = ANativeWindow_fromSurface(env, javaSurface);
    if (!window) { g_running.store(false); return; }
    if (g_thread.joinable()) g_thread.join();
    g_thread = std::thread(renderLoop, window);
}


extern "C" JNIEXPORT void JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeShutdown(JNIEnv*, jobject) {
    g_running.store(false);
    std::lock_guard<std::mutex> lock(g_game_dir_mutex);
    shutdownXboxRuntimeLocked();
    g_runtime_status = "runtime shut down";
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeStop(JNIEnv*, jobject) {
    g_running.store(false);
    if (g_thread.joinable()) g_thread.join();
}
