#pragma once

#include <Base.h>
#include <windows.h>
#include <thread>
#include <chrono>
#include <debugapi.h>
#include <cstdio>


namespace notsa {
namespace debug {
inline bool IsDebuggerPresent() {
    return ::IsDebuggerPresent();
}

inline void WaitForDebugger() {
    while (!IsDebuggerPresent()) {
        NOTSA_LOG_INFO("[debug] Waiting for debugger\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

inline void DisplayConsole() {
    // Support UTF-8 IO for Windows Terminal. (or CMD if a supported font is used)
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

#ifdef NOTSA_STANDALONE
    // In standalone mode try using the parent process's console if it has one
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        VERIFY(AllocConsole());
    }
#else
    // In asi mode always allocate a new console
    VERIFY(AllocConsole());
#endif

    FILESTREAM fs{};
    VERIFY(freopen_s(&fs, "CONIN$", "r", stdin) == NOERROR);
    VERIFY(freopen_s(&fs, "CONOUT$", "w", stdout) == NOERROR);
    VERIFY(freopen_s(&fs, "CONOUT$", "w", stderr) == NOERROR);
}

/*!
 * @return function information (module, function name, line number) at the given address.
 */
std::string GetFunctionInfoAtAddress(uintptr_t address, bool compact = false, HANDLE hProcess = GetCurrentProcess());

/*!
 * @brief Initialize debugging symbols
 * @note Call after the CommandLine has initialized, as it uses the exe path to initialize symbols.
 */
void LoadSymbols();

/*!
 * @brief Unload symbols
 */
void UnloadSymbols();

/*!
 * @brief Route the CRT's debug reports (assert(), _ASSERTE, ...) to `logs\runtime_reports.txt`
 *        instead of the modal "Assertion Failed" Abort/Retry/Ignore dialog.
 *
 * @details The standalone build runs with the debug CRT, so every failed `assert()` pops a modal
 *          dialog that blocks the game and - worse - whose text only goes to the console window,
 *          which is lost when the process is started detached. That made the assert storm on the
 *          new-game path undiagnosable (the user had to click "Ignore" by hand, and the log held
 *          nothing but a clean cut-off).
 *
 *          With this hook each report is written to a file (tagged ASSERT/ERROR/WARN, with a running
 *          count per kind) and the dialog is suppressed, so debugging is never blocked and the
 *          failures are on disk afterwards.
 *
 * @note    Must NOT use spdlog/fmt or any C++ allocation here: this runs inside the CRT's own report
 *          path, so re-entering it (or the allocator) risks recursion/deadlock. Plain C stdio only.
 * @note    No-op outside `_DEBUG` (release builds compile `assert` away).
 */
void InstallRuntimeReportLogging();

/*!
 * @brief Append one line to the runtime-report file opened by `InstallRuntimeReportLogging`.
 * @note  Thread-safe, allocation-free-ish (plain C stdio) and bounded, so it is safe to call from
 *        failure paths (including our own non-fatal `VERIFY`).
 */
void ReportRuntimeMessage(const char* msg);

};
};
