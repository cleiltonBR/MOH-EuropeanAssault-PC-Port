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
#include <string>
#include <mutex>
#include <sys/stat.h>
#include <cstdint>
#include <fstream>
#include <vector>

extern "C" void d3d8_android_set_egl_surface(
    EGLDisplay display, EGLSurface surface, int width, int height);

extern "C" {
extern __thread uint32_t g_esp;
int recomp_dispatch_init(void);
}

extern "C" void moh_android_note_frame(void);

namespace {
std::atomic<bool> g_running{false};
std::thread g_thread;
std::atomic<int> g_stage{0};
std::atomic<int> g_error{0};
std::atomic<unsigned long long> g_frames{0};
std::mutex g_game_dir_mutex;
std::string g_game_dir;
uint32_t g_xbe_entry = 0;
uint32_t g_xbe_kernel_thunk = 0;
std::string g_xbe_validation = "XBE not inspected";
std::string g_runtime_status = "waiting for game import";
std::atomic<bool> g_runtime_requested{false};
std::atomic<bool> g_runtime_ready{false};
std::atomic<bool> g_runtime_attempted{false};
std::vector<uint8_t> g_xbe_image;

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

bool hasGameXbe() {
    if (g_game_dir.empty()) return false;
    const std::string path = g_game_dir + "/default.xbe";
    struct stat info{};
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

void setRuntimeStatus(const std::string& value) {
    std::lock_guard<std::mutex> lock(g_game_dir_mutex);
    g_runtime_status = value;
}

void renderLoop(ANativeWindow* window) {
    g_stage.store(1); g_error.store(0); g_frames.store(0);
    bool runtimeMemoryReady = false;
    bool runtimeKernelReady = false;
    {
        std::lock_guard<std::mutex> lock(g_game_dir_mutex);
        if (!g_xbe_image.empty()) g_runtime_requested.store(true);
    }
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) {
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
        eglTerminate(display); ANativeWindow_release(window); g_running.store(false); return;
    }
    const EGLint contextAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs);
    EGLSurface surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE ||
        !eglMakeCurrent(display, surface, surface, context)) {
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        eglTerminate(display); ANativeWindow_release(window); g_running.store(false); return;
    }
    GLint width = 0, height = 0;
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    d3d8_android_set_egl_surface(display, surface, width, height);
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
            if (!g_runtime_ready.load() && g_runtime_requested.exchange(false) &&
                !g_runtime_attempted.exchange(true)) {
                std::vector<uint8_t> xbe;
                std::string gameDir;
                {
                    std::lock_guard<std::mutex> lock(g_game_dir_mutex);
                    xbe = g_xbe_image;
                    gameDir = g_game_dir;
                }
                if (!xbe.empty() && !gameDir.empty()) {
                    g_stage.store(7);
                    if (xbox_MemoryLayoutInit(xbe.data(), xbe.size())) {
                        runtimeMemoryReady = true;
                        g_stage.store(8);
                        xbox_kernel_init();
                        runtimeKernelReady = true;
                        const std::string saveDir = gameDir + "/saves";
                        xbox_path_init(gameDir.c_str(), saveDir.c_str());
                        xbox_kernel_bridge_init();
                        g_esp = XBOX_STACK_TOP;
                        const int dispatchReady = recomp_dispatch_init();
                        g_runtime_ready.store(true);
                        g_stage.store(9);
                        setRuntimeStatus(dispatchReady
                            ? "XBE sections mapped; Xbox kernel and thunk bridge ready; dispatcher prepared. Game entry point not started yet."
                            : "XBE sections mapped; Xbox kernel and thunk bridge ready; dispatcher will use fallback lookup. Game entry point not started yet.");
                    } else {
                        xbox_MemoryLayoutShutdown();
                        g_error.store(-8);
                        g_stage.store(-8);
                        setRuntimeStatus("Xbox guest-memory/XBE section initialization failed.");
                    }
                } else {
                    setRuntimeStatus("XBE is not ready; waiting for a valid default.xbe.");
                }
            }
            HRESULT clearResult = device->lpVtbl->Clear(
                device, 0, nullptr, D3DCLEAR_TARGET, 0xFF17365Du, 1.0f, 0);
            if (clearResult != 0) { g_error.store(clearResult); g_stage.store(-7); break; }
            HRESULT presentResult = device->lpVtbl->Present(
                device, nullptr, nullptr, nullptr, nullptr);
            if (presentResult != 0) { g_error.store(presentResult); g_stage.store(-5); break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    } else { g_error.store(createResult); g_stage.store(-6); }
    if (runtimeKernelReady) xbox_kernel_shutdown();
    if (runtimeMemoryReady) xbox_MemoryLayoutShutdown();
    g_runtime_ready.store(false);
    g_runtime_attempted.store(false);
    if (runtimeMemoryReady)
        setRuntimeStatus("Xbox runtime shut down with the Android graphics surface.");
    d3d8_android_set_egl_surface(EGL_NO_DISPLAY, EGL_NO_SURFACE, 0, 0);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    ANativeWindow_release(window);
    g_running.store(false);
}
} // namespace

extern "C" void moh_android_note_frame(void) {
    g_frames.fetch_add(1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeSetGameDirectory(
    JNIEnv* env, jobject, jstring path) {
    const char* chars = path ? env->GetStringUTFChars(path, nullptr) : nullptr;
    {
        std::lock_guard<std::mutex> lock(g_game_dir_mutex);
        g_game_dir = chars ? chars : "";
        inspectXbeLocked();
        g_runtime_attempted.store(false);
        g_runtime_requested.store(!g_xbe_image.empty() && g_xbe_entry != 0 && g_xbe_kernel_thunk != 0);
        g_runtime_status = g_xbe_image.empty()
            ? "waiting for game import"
            : "XBE validated; waiting for Android graphics thread to initialize runtime";
    }
    if (chars) env->ReleaseStringUTFChars(path, chars);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeStatus(JNIEnv* env, jobject) {
    char message[640];
    std::lock_guard<std::mutex> lock(g_game_dir_mutex);
    const bool xbeReady = hasGameXbe();
    snprintf(message, sizeof(message),
        "stage=%d frames=%llu error=0x%08X. XBE %s (%zu bytes loaded); %s; entry=0x%08X thunk=0x%08X. Runtime: %s",
        g_stage.load(), g_frames.load(), static_cast<unsigned int>(g_error.load()),
        xbeReady ? "imported" : "not imported", g_xbe_image.size(), g_xbe_validation.c_str(),
        g_xbe_entry, g_xbe_kernel_thunk, g_runtime_status.c_str());
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
Java_com_cleiltonbr_moheuropeanassault_MainActivity_nativeStop(JNIEnv*, jobject) {
    g_running.store(false);
    if (g_thread.joinable()) g_thread.join();
}
