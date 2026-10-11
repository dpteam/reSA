#include "StdInc.h"
#include <cstdlib>
#include <app_debug.h>
#include "Bootstrap.h"
#include "extensions/CommandLine.h"
#include "extensions/debug.hpp"
#include "InjectHooksMain.h"
#include "dllmain.h"

/*
 * This file contains the entry point of the regular (ASI/DLL) build of the project.
 *
 * In the standalone (game) build there's no `DllMain` - the executable's entry point is `WinMain`
 * (See `app/platform/win/WinMain.cpp`), and the bootstrap logic it shares with `DllMain` lives in
 * `Bootstrap.cpp`.
 */

#ifndef NOTSA_STANDALONE

HMODULE s_HandleOfDLL{};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        s_HandleOfDLL = hModule;

        // Fail if RenderWare has already been started
        if (*(RwCamera**)0xC1703C) {
            MessageBox(NULL, "gta_reversed failed to load (RenderWare has already been started)", "Error", MB_ICONERROR | MB_OK);
            return FALSE;
        }

        std::setlocale(LC_ALL, "en_US.UTF-8");

        notsa::debug::DisplayConsole();
        notsa::debug::LoadSymbols(); // Used by logging
        notsa::Logging::CreateInstance();

        CommandLine::Load(__argc, __argv);
        if (CommandLine::s_WaitForDebugger) {
            notsa::debug::WaitForDebugger();
        }
        notsa::bootstrap::LoadConfigurations();

        ReversibleHooks::RHManager::CreateInstance();
        InjectHooksMain();

        break;
    }
    case DLL_PROCESS_DETACH: {
        if (lpReserved == nullptr) {
            NOTSA_LOG_INFO("DLL_PROCESS_DETACH: Shutting down normally...");

            ReversibleHooks::RHManager::DestroyInstance();
            notsa::Logging::DestroyInstance();
            notsa::debug::UnloadSymbols();
        } else { // This is pretty much the only thing that's ever reached
            NOTSA_LOG_INFO("DLL_PROCESS_DETACH: Process is terminating, shutting down only what's necessary");

            notsa::Logging::DestroyInstance();
        }
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

#endif // NOTSA_STANDALONE

HMODULE notsa::GetDLLHandle() {
#ifdef NOTSA_STANDALONE
    // Standalone: We're the executable (Not a DLL), so our own module is the one exporting the vtables.
    return GetModuleHandleA(nullptr);
#else
    return s_HandleOfDLL;
#endif
}
