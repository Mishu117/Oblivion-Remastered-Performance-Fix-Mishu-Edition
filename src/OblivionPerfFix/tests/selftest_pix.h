// Fork-local diagnostic PixGpuCapturer self-test, included by selftest.cpp: the PIX install search only (the library is
// never loaded in the test). Passes on a PC without PIX: the search must then find nothing.
#pragma once

namespace pix {
void Run()
{
    const std::wstring path = FindPixGpuCapturer();
    const std::wstring tail = L"\\WinPixGpuCapturer.dll";
    const bool shaped = path.size() > tail.size() && path.compare(path.size() - tail.size(), tail.size(), tail) == 0;
    Check(path.empty() || (shaped && GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES),
          "PixGpuCapturer: the PIX search finds nothing or an existing WinPixGpuCapturer.dll");
    printf("  PIX capturer: %ls\n", path.empty() ? L"(no PIX install)" : path.c_str());
}
}  // namespace pix
