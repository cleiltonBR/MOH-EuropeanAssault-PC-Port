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
extern __thread uint32_t g_esp;
#else
extern uint32_t g_esp;
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
            }
#endif
            (void)write(fd, "\n", 1);

            close(fd);
        }
    }
    _exit(128 + signalNumber);
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
    if (g_kernel_started) {
        xbox_kernel_shutdown();
        g_kernel_started = false;
    }
    if (g_memory_started) {
        xbox_MemoryLayoutShutdown();
        g_memory_started = false;
    }
    g_runtime_stage.store(0);
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
        g_runtime_stage.store(-1);
        g_runtime_status = "could not enable the Android 4 GiB guest address window";
        return;
    }
    g_runtime_status = "mapping Xbox guest memory and loading XBE sections";
    if (!xbox_MemoryLayoutInit(g_xbe_image.data(), g_xbe_image.size())) {
        xbox_MemoryLayoutShutdown();
        g_runtime_stage.store(-1);
        const char* memoryError = xbox_MemoryLayoutLastError();
        g_runtime_status = (memoryError && memoryError[0])
            ? memoryError : "Xbox guest memory initialization failed (no host diagnostic)";
        return;
    }
    g_memory_started = true;
    appendRuntimeLog("Runtime: Xbox guest memory initialized");
    g_runtime_stage.store(2);

    const std::string save_dir = g_game_dir + "/../saves";
    xbox_path_init(g_game_dir.c_str(), save_dir.c_str());
    appendRuntimeLog("Runtime: initializing Xbox kernel");
    xbox_kernel_init();
    g_kernel_started = true;
    appendRuntimeLog("Runtime: Xbox kernel initialized");
    g_runtime_stage.store(3);

    xbox_kernel_get_thunk_address(&thunk_va, &thunk_count);
    if (!xbox_GetMemoryBase() || !thunk_va || !thunk_count) {
        g_runtime_status = "guest memory loaded, but XBE kernel thunk table is unavailable";
        g_runtime_stage.store(-2);
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
        return;
    }
    /* Match the desktop bootstrap: the generated entry is a void(void)
     * function and consumes the emulated x86 register file/stack. The lookup
     * above only proved that code exists; invoke it now on this same thread,
     * where the EGL context, guest memory and TLS register state are valid. */
    if (g_esp != XBOX_STACK_TOP) {
        g_runtime_stage.store(-4);
        g_runtime_status = "dispatcher ready, but guest ESP was not initialized";
        return;
    }
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
                appendRuntimeLog("UI: explicit game start requested");
                std::lock_guard<std::mutex> lock(g_game_dir_mutex);
                shutdownXboxRuntimeLocked();
                initializeXboxRuntimeLocked();
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
        g_runtime_status = "runtime shut down with EGL context";
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
    appendRuntimeLog("UI: game start requested");
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
