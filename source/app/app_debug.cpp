#include "StdInc.h"

#include "app_debug.h"
#include <windows.h>
#include <DbgHelp.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <extensions/debug.hpp>

#define FINAL 0

#ifndef FINAL
RtCharset* debugCharset;
bool g_bDebugRenderGroups;
#endif

#if !defined(FINAL)
static bool charsetOpen;
void OpenCharsetSafe() {
    if (!charsetOpen)
        RtCharsetOpen();
    charsetOpen = true;
}
#endif

// 0x734610
void CreateDebugFont() {
#ifndef FINAL
    RwRGBA color = { 255, 255, 128, 255 };
    RwRGBA colorbg = { 0, 0, 0, 0 };
    OpenCharsetSafe();
    debugCharset = RtCharsetCreate(&color, &colorbg);
#endif
}

// 0x734620
void DestroyDebugFont() {
#ifndef FINAL
    RtCharsetDestroy(debugCharset);
    RtCharsetClose();
    charsetOpen = false;
#endif
}

// unused
// 0x734630
void ObrsPrintfString(const char* str, int16 x, int16 y) {
#ifndef FINAL
    RtCharsetPrintBuffered(debugCharset, str, x * 8, y * 16, true);
#endif
}

// 0x734640
void FlushObrsPrintfs() {
#ifndef FINAL
    RtCharsetBufferFlush();
#endif
}

// This probably should be in winps :D
LONG WINAPI WindowsExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    // If this function itself crashes it's invoked again
    // So let's prevent the recusion with this simple hack
    static bool s_HasHandled = false;
    if (s_HasHandled) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    s_HasHandled = true;

    // In standalone this re-emission is not just useless but actively harmful: `enable_backtrace` keeps the
    // last N records, and `dump_backtrace` re-logs them *with their original timestamps* - so every message
    // that is already present in logs/log.log appears a second time. For a short log (fewer records than the
    // backtrace capacity) that doubles the entire file, which silently corrupts any line-counting metric
    // (this project's yardstick is the number of `Loading the Game -> Loading` markers, and it read ~2x high).
    // The backtrace only earns its keep when the log is not otherwise persisted; here it is, and it is
    // flushed every 100 ms, so skip it and keep the log exactly once-per-message.
#ifndef NOTSA_STANDALONE
    spdlog::apply_all([](auto&& logger) {
        logger->dump_backtrace();
    });
#endif
    
    const auto Section = [](const char* name) {
        SPDLOG_INFO("*********{}**********", name);
    };

    Section("UNHANDLED EXCEPTION");

    SPDLOG_INFO("Exception Code: {:#010x}", pExceptionInfo->ExceptionRecord->ExceptionCode);
    SPDLOG_INFO("Exception Flags: {:#010x}", pExceptionInfo->ExceptionRecord->ExceptionFlags);
    SPDLOG_INFO("Exception Address: {:#010x}", (uintptr_t)pExceptionInfo->ExceptionRecord->ExceptionAddress);

    // Dump exception parameters
    Section("PARAMETERS");
    {
        SPDLOG_INFO("Parameters[{}]:", pExceptionInfo->ExceptionRecord->NumberParameters);
        for (DWORD i = 0; i < pExceptionInfo->ExceptionRecord->NumberParameters; i++) {
            SPDLOG_INFO("{:>8}: {:#010x}", i, pExceptionInfo->ExceptionRecord->ExceptionInformation[i]);
        }
    }

    CONTEXT& context = *pExceptionInfo->ContextRecord;

    Section("REGISTERS");
    {
        const auto DumpRegister = [](auto name, auto value) {
            SPDLOG_INFO("\t{}: {:#010x}", name, value);
        };
        DumpRegister("EAX", context.Eax);
        DumpRegister("EBX", context.Ebx);
        DumpRegister("ECX", context.Ecx);
        DumpRegister("EDX", context.Edx);
        DumpRegister("ESI", context.Esi);
        DumpRegister("EDI", context.Edi);
        DumpRegister("EBP", context.Ebp);
        DumpRegister("ESP", context.Esp);
        DumpRegister("EIP", context.Eip);
        DumpRegister("EFLAGS", context.EFlags);
    }

#if 0
    Section("LOADED MODULES");
    {
        HANDLE hProcess = GetCurrentProcess();

        HMODULE hModules[1024];
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
            const DWORD numModules = cbNeeded / sizeof(HMODULE);
            for (DWORD i = 0; i < numModules; i++) {
                MODULEINFO moduleInfo;
                if (GetModuleInformation(hProcess, hModules[i], &moduleInfo, sizeof(moduleInfo))) {
                    char moduleName[MAX_PATH];
                    GetModuleBaseName (hProcess, hModules[i], moduleName, sizeof(moduleName));

                    SPDLOG_INFO("\t{:#010x}: {}", LOG_PTR(moduleInfo.lpBaseOfDll), moduleName);
                }
            }
        }
    }
#endif

    Section("CALL STACK");
    {
        HANDLE hProcess = GetCurrentProcess();
        HANDLE hThread = GetCurrentThread();

        STACKFRAME stackFrame = {};
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrStack.Offset = context.Esp;
        stackFrame.AddrFrame.Offset = context.Ebp;

        DWORD prevFrameOffset = 0;
        while (StackWalk(
            IMAGE_FILE_MACHINE_I386, hProcess, hThread, &stackFrame, &context, NULL,
            SymFunctionTableAccess, SymGetModuleBase, NULL))
        {
            NOTSA_LOG_INFO("\t{}", notsa::debug::GetFunctionInfoAtAddress(stackFrame.AddrPC.Offset));
        }
    }

    Section("END OF UNHANDLED EXCEPTION");

    spdlog::apply_all([](auto&& logger) {
        logger->flush();
    });

    return EXCEPTION_EXECUTE_HANDLER;
}

notsa::Logging::Logging() {
    using namespace std::chrono_literals;
#if 0
    while (!IsDebuggerPresent()) {
        Sleep(1);
    }
#endif
    spdlog::init_thread_pool(1 << 16, 4);

    // See https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags
    spdlog::set_pattern("%^[%l][%H:%M:%S.%e][%s:%#]: %v%$");
    spdlog::enable_backtrace(128);
    spdlog::set_level(spdlog::level::debug);

    m_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/log.log"));
    m_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    spdlog::set_default_logger(Create("default"));
    spdlog::flush_every(100ms);

    SetUnhandledExceptionFilter(WindowsExceptionHandler);
}

notsa::Logging::~Logging() {
    spdlog::shutdown();
}

auto notsa::Logging::Create(std::string name, std::optional<spdlog::level::level_enum> level) -> notsa::log_ptr {
    auto logger = std::make_shared<spdlog::logger>(name, m_sinks.begin(), m_sinks.end());
    spdlog::initialize_logger(logger);
    if (level.has_value()) {
        logger->set_level(*level);
    }
    return logger;
}

#ifdef NOTSA_STANDALONE
// Non-fatal VERIFY for the standalone build - see the declaration/comment in app_debug.h.
// A failing VERIFY is recorded (per call-site, with counts) and execution continues, which is what
// manually clicking "Ignore" in the CRT dialog used to do.
void notsa::detail::VerifyMacroImpl(bool result, const char* expr, const char* file, int line) {
    if (result) {
        return;
    }

    struct Site { const char* file; int line; int hits; };
    static Site s_sites[128]{};
    static int  s_numSites{};
    static int  s_totalHits{};

    constexpr int MAX_SITES      = 128; // distinct call-sites we track
    constexpr int MAX_PER_SITE   = 25;  // reported lines per distinct call-site
    constexpr int MAX_TOTAL      = 4000; // overall cap (an assert storm must not fill the disk)

    // Dedupe by (file, line). `__FILE__`/`__LINE__` are constant per expansion, so the pointer
    // comparison identifies the same call-site reliably.
    Site* site = nullptr;
    for (int i = 0; i < s_numSites; i++) {
        if (s_sites[i].file == file && s_sites[i].line == line) {
            site = &s_sites[i];
            break;
        }
    }
    if (!site && s_numSites < MAX_SITES) {
        site      = &s_sites[s_numSites++];
        site->file = file;
        site->line = line;
        site->hits = 0;
    }

    const int n = site ? ++site->hits : 0;
    const int t = ++s_totalHits;

    char buf[512];
    if (t > MAX_TOTAL) {
        if (t == MAX_TOTAL + 1) {
            sprintf_s(buf, "[VERIFY] ... further VERIFY failures suppressed (cap %d reached) ...", MAX_TOTAL);
            notsa::debug::ReportRuntimeMessage(buf);
            NOTSA_LOG_ERR("{}", buf);
        }
        return;
    }

    if (site && n <= MAX_PER_SITE) {
        sprintf_s(buf, "[VERIFY #%d @site %d] `%s` failed  (%s:%d)%s", n,
                  static_cast<int>(site - s_sites), expr ? expr : "?", file ? file : "?", line,
                  n == MAX_PER_SITE ? "  [further failures at this site suppressed]" : "");
        notsa::debug::ReportRuntimeMessage(buf);
        NOTSA_LOG_ERR("{}", buf);
    } else if (site && n == MAX_PER_SITE + 1) {
        sprintf_s(buf, "[VERIFY] site %d (`%s` @ %s:%d) exceeded %d failures - counting continues, reporting stopped",
                  static_cast<int>(site - s_sites), expr ? expr : "?", file ? file : "?", line, MAX_PER_SITE);
        notsa::debug::ReportRuntimeMessage(buf);
    }
}
#endif
