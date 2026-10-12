#pragma once

#include <cstdlib>
#include <cstdio>
#include <windows.h>
#include <DbgHelp.h>
#include <format>
#include <extensions/CommandLine.h>
#include "debug.hpp"

#ifdef _DEBUG
#include <crtdbg.h> // CRT debug-report API (hook / report modes)
#endif


namespace notsa {
namespace debug {
std::string GetFunctionInfoAtAddress(uintptr_t address, bool compact, HANDLE hProcess) {
    const auto moduleBase = SymGetModuleBase(hProcess, address);
    if (moduleBase == NULL) {
        return "<unknown>";
    }

    DWORD         lineDisplacement = 0; // Offset from the start address of the function
    IMAGEHLP_LINE lineInfo     = { sizeof(IMAGEHLP_LINE) };
    const auto    hasLineInfo  = SymGetLineFromAddr(hProcess, address, &lineDisplacement, &lineInfo);

    IMAGEHLP_MODULE moduleInfo{ sizeof(IMAGEHLP_MODULE) };
    const auto      hasModuleInfo = SymGetModuleInfo(hProcess, moduleBase, &moduleInfo);

    std::byte   symbuf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME + 1]{};
    auto* const sym = new (symbuf) SYMBOL_INFO{
        .SizeOfStruct = sizeof(SYMBOL_INFO),
        .MaxNameLen   = MAX_SYM_NAME,
    };
    DWORD64    symDisplacement = 0;
    const auto hasSym = SymFromAddr(hProcess, address, &symDisplacement, sym);

    char undecoratedName[MAX_SYM_NAME + 1]{};
    const auto hasUndecoratedName = hasSym && UnDecorateSymbolName(sym->Name, undecoratedName, sizeof(undecoratedName), UNDNAME_COMPLETE) != 0;

    const auto name = hasUndecoratedName
            ? undecoratedName
            : hasSym
                ? sym->Name
                : "<unknown>";

    const auto module = hasModuleInfo
        ? moduleInfo.ModuleName
        : "<unknown>";

    const auto line = hasLineInfo
        ? lineInfo.LineNumber
        : 0;

    if (compact) {
        if (hasSym) {
            return std::format(
                "{}!{}:{}",
                module,
                name,
                line
            );
        }
        return std::format(
            "{}!{}",
            module,
            address
        );
    }

    return std::format(
        "0x{:010x}+0x{:04x}: {}!{}:{}",
        static_cast<uintptr_t>(sym->Address),
        symDisplacement,
        module,
        name,
        line
    );
}

void LoadSymbols() {
    SymInitialize(
        GetCurrentProcess(),
        CommandLine::GetExePath().parent_path().string().c_str(),
        TRUE
    );
}

void UnloadSymbols() {
    SymCleanup(GetCurrentProcess());
}

namespace {
    FILE* g_repFile{};
    CRITICAL_SECTION g_repCs;
    bool  g_repCsReady{};
#ifdef _DEBUG
    int   g_repCount[3]{};      //!< per-kind counters (_CRT_WARN, _CRT_ERROR, _CRT_ASSERT)
    constexpr int REP_MAX_PER_KIND = 400; //!< keep the file bounded during an assert storm

    /*!
     * @brief CRT debug-report hook: log to a file and swallow the modal dialog.
     * @return 1 = "handled". NOTE: this does NOT stop the CRT from aborting after an assert - see
     *         `InstallRuntimeReportLogging` and the non-fatal `VERIFY` in app_debug.h.
     */
    int __cdecl CrtReportHook(int reportType, char* message, int* /*retVal*/) {
        const int kind = (reportType >= 0 && reportType < 3) ? reportType : 0;

        EnterCriticalSection(&g_repCs);
        const int n = ++g_repCount[kind];
        if (message && g_repFile && n <= REP_MAX_PER_KIND + 1) {
            static char const* const tags[3] = { "WARN  ", "ERROR ", "ASSERT" };
            if (n <= REP_MAX_PER_KIND) {
                fprintf(g_repFile, "[%s #%d] %s\n", tags[kind], n, message);
            } else {
                fprintf(g_repFile, "[.... further reports of this kind suppressed; counting continues ....]\n");
            }
            fflush(g_repFile);
        }
        LeaveCriticalSection(&g_repCs);
        return 1;
    }
#endif
} // namespace

void ReportRuntimeMessage(const char* msg) {
    if (!msg) {
        return;
    }
    if (!g_repCsReady) {
        return; // logging not installed yet - nothing we can do, and we must not allocate here
    }
    EnterCriticalSection(&g_repCs);
    if (g_repFile) {
        fputs(msg, g_repFile);
        fputc('\n', g_repFile);
        fflush(g_repFile);
    }
    LeaveCriticalSection(&g_repCs);
}

void InstallRuntimeReportLogging() {
    InitializeCriticalSection(&g_repCs);
    g_repCsReady = true;
    // Open eagerly (relative to the working dir, i.e. `bin/<cfg>`), so the first report does not have
    // to wrestle with a missing file from inside a failure path. Share it for reading (_SH_DENYNO) so
    // the file can be tailed while the game is still running.
    g_repFile = _fsopen("logs\\runtime_reports.txt", "a", _SH_DENYNO);
    fprintf(g_repFile, "--- runtime report logging installed (pid %lu) ---\n", GetCurrentProcessId());
    fflush(g_repFile);

#ifdef _DEBUG
    // Take _CRTDBG_MODE_WNDW out of every report kind so a failing assert cannot put up the modal
    // Abort/Retry/Ignore dialog; the text goes to stderr (the console) and the hook below mirrors it
    // into our file.
    //
    // ⚠️ This alone is NOT enough: UCRT's `assert(expr)` expands to
    //        (!!(expr)) || (_wassert(#expr, __FILE__, __LINE__), 0)
    //    and `_wassert` *aborts* once the report comes back - the dialog's "Ignore" was the only thing
    //    that let execution continue. Suppressing the dialog therefore turned every failing assert into
    //    a hard, silent process death. Hence the standalone build routes its own `VERIFY` through the
    //    non-fatal overload in app_debug.{h,cpp} (log + continue), which is what clicking "Ignore" by
    //    hand used to achieve.
    for (int t = _CRT_WARN; t <= _CRT_ASSERT; t++) {
        _CrtSetReportMode(t, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(t, _CRTDBG_FILE_STDERR);
    }
    _set_error_mode(_OUT_TO_STDERR);
    _CrtSetReportHook(&CrtReportHook);
    // Write the abort message to stderr (so it lands in the console / our log) but do not raise Watson,
    // which would put up yet another dialog.
    _set_abort_behavior(_WRITE_ABORT_MSG, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

};
};
