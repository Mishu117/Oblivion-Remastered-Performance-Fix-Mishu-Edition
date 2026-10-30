// Fork-local diagnostic PixGpuCapturer (off by default; [Fixes] PixGpuCapturer=1, read at start-up). Not a fix: no game
// code is patched and nothing the game does changes. With it on, Microsoft PIX's GPU capture library
// (WinPixGpuCapturer.dll from the newest "Program Files\Microsoft PIX\<version>" folder) is loaded into the game, so
// PIX can attach to a game started the normal way (Steam) and take GPU captures (pixtool attach <pid>, or the PIX
// window's Attach). PIX cannot launch the game itself: the Shipping exe started directly hands the launch back to Steam.
//
// The library must be in the process before the game creates its D3D12 device (PIX's rule for attaching), so it is
// loaded at start-up only (the version.dll route), from a thread started in Init: DllMain holds the loader lock, the
// thread runs as soon as it is released, seconds before the engine sets up its renderer. On the OBSE64 route the device
// may already exist: not loaded. Written for the DLAA magenta specks (2026-10-29). Log tag [pix].
#include "common.h"

#include <string>
#include <vector>

namespace opf {
namespace {

// "2603.25" -> {2603, 25}; folders that are not version numbers sort first
std::vector<unsigned> VersionKey(const std::wstring& name)
{
    std::vector<unsigned> key;
    unsigned v = 0;
    bool digits = false;
    for (wchar_t c : name) {
        if (c >= L'0' && c <= L'9') { v = v * 10 + static_cast<unsigned>(c - L'0'); digits = true; }
        else if (c == L'.' && digits) { key.push_back(v); v = 0; digits = false; }
        else return {};
    }
    if (digits) key.push_back(v);
    return key;
}

DWORD WINAPI LoadThread(void* param)
{
    std::wstring* path = static_cast<std::wstring*>(param);
    HMODULE m = LoadLibraryW(path->c_str());
    if (m) Log("[pix] PIX's GPU capturer loaded (%ls): attach PIX to pid %lu to take GPU captures", path->c_str(), GetCurrentProcessId());
    else Log("[pix] %ls could not be loaded (error %lu): no PIX captures", path->c_str(), GetLastError());
    delete path;
    return 0;
}

}  // namespace

// The newest PIX install's WinPixGpuCapturer.dll under Program Files, or "" when there is none.
std::wstring FindPixGpuCapturer()
{
    // the 64-bit Program Files folder from the environment (the DLL links kernel32 only)
    wchar_t pf[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"ProgramW6432", pf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) n = GetEnvironmentVariableW(L"ProgramFiles", pf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return L"";
    const std::wstring root = std::wstring(pf, n) + L"\\Microsoft PIX\\";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((root + L"*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return L"";
    std::wstring best;
    std::vector<unsigned> bestKey;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        const std::vector<unsigned> key = VersionKey(fd.cFileName);
        if (key.empty() || key < bestKey) continue;
        const std::wstring dll = root + fd.cFileName + L"\\WinPixGpuCapturer.dll";
        if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        best = dll;
        bestKey = key;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return best;
}

void InstallPixGpuCapturer(bool beforeGameCode)
{
    if (!beforeGameCode) {
        Log("[pix] PixGpuCapturer: can only be loaded at start-up (version.dll), the renderer may already exist: not loaded");
        return;
    }
    const std::wstring path = FindPixGpuCapturer();
    if (path.empty()) {
        Log("[pix] PixGpuCapturer: no Microsoft PIX install with WinPixGpuCapturer.dll under Program Files: not loaded");
        return;
    }
    auto* arg = new std::wstring(path);
    HANDLE t = CreateThread(nullptr, 0, LoadThread, arg, 0, nullptr);
    if (!t) {
        Log("[pix] PixGpuCapturer: no thread for the load (error %lu): not loaded", GetLastError());
        delete arg;
        return;
    }
    CloseHandle(t);
}

}  // namespace opf
