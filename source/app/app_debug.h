#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <extensions/Singleton.hpp>

#define PUSH_RENDERGROUP(str) 0
#define POP_RENDERGROUP() 0

namespace notsa {
namespace detail {
static void VerifyMacroImpl(bool result) {
    assert(result); // In release mode this won't do anything
}

#ifdef NOTSA_STANDALONE
/*!
 * @brief Standalone flavour of `VERIFY`: reports a failure and **keeps running**.
 *
 * Why this exists: UCRT's `assert` ends in `abort()` (the modal Abort/Retry/Ignore dialog is the only
 * escape hatch - "Ignore" returns from `_wassert` and execution continues). The standalone build
 * suppresses that dialog so debugging is not blocked, which would otherwise turn every failing `VERIFY`
 * into a hard, silent process death. This overload reproduces the "Ignore" choice automatically: the
 * failure is written to `logs\runtime_reports.txt` (and the normal log) with a per-site counter, and the
 * game carries on - so an assert storm during loading no longer hides the *first* real problem.
 */
void VerifyMacroImpl(bool result, const char* expr, const char* file, int line);
#endif
};
};
#ifdef NOTSA_STANDALONE
#define VERIFY(expr) ::notsa::detail::VerifyMacroImpl((expr), #expr, __FILE__, __LINE__)
#else
#define VERIFY ::notsa::detail::VerifyMacroImpl
#endif
#define VERIFY_TODO_FIX(_expr) (_expr) // Macro used to mark shit that uses `VERIFY and sometimes fails

namespace notsa {
using log_ptr = std::shared_ptr<spdlog::logger>;
struct Logging : public Singleton<Logging> {
    Logging();
    ~Logging();

    auto Create(std::string name, std::optional<spdlog::level::level_enum> level = std::nullopt) -> log_ptr;

private:
    std::vector<spdlog::sink_ptr> m_sinks;
};
};
//! Make sure to *NOT* call this function to initialize static variables, it will fuck up spdlog.
//! Instead, make the static variable with type `notsa::log_ptr` and initialize it in `InjectHooks`!
#define NOTSA_MAKE_LOGGER notsa::Logging::GetInstance().Create

#define NOTSA_LOG_CRIT SPDLOG_CRITICAL
#define NOTSA_LOG_INFO SPDLOG_INFO
#define NOTSA_LOG_ERR SPDLOG_ERROR
#define NOTSA_LOG_WARN SPDLOG_WARN
#define NOTSA_LOG_DEBUG SPDLOG_DEBUG
#define NOTSA_LOG_TRACE SPDLOG_TRACE

//! Use this to pass pointers to logging functions [both std::format, logs, and printf-style stuff]
// We're casting to uintptr_t, because formatting just works better that way
#define LOG_PTR(x) ((uintptr_t)(x))
static_assert(sizeof(void*) == sizeof(uintptr_t));

void CreateDebugFont();
void DestroyDebugFont();
//void ObrsPrintfString(const char* str, int16 x, int16 y);
void FlushObrsPrintfs();
