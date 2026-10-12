#include "StdInc.h"

#ifdef NOTSA_USE_SDL3
#include <SDL3/SDL.h>
#include "SDLWrapper.hpp"
#include <bindings/imgui_impl_sdl3.h>
#include <WindowedMode.hpp>
#include "PostEffects.h"
#include "UIRenderer.h"
#include "app.h"   // ForegroundApp
#include "Pad.h"   // CPad::Pads / CPad::Clear

static bool IsInFullscreen()
{
    // TODO: Currently with our hacky windowed mode, looks like there's no
    // good way to gather this information.
    return false;
}

namespace notsa {
namespace SDLWrapper {
bool Initialize() {
    return true;
}

void Terminate() {

}

void ProcessEvents() {
    ZoneScoped;

    // Now process events
    const auto* const imCtx = ImGui::GetCurrentContext();
    const auto* const imIO  = imCtx ? &imCtx->IO : nullptr;
    for (SDL_Event e; SDL_PollEvent(&e);) {
        if (imIO) {
            ImGui_ImplSDL3_ProcessEvent(&e);
        }

        switch (e.type) {
        case SDL_EVENT_QUIT: {
            RsGlobal.quit = true;
            continue;
        }
        case SDL_EVENT_WINDOW_RESIZED: {
            const auto w = e.window.data1,
                       h = e.window.data2;

            NOTSA_LOG_DEBUG(
                "SDL: Window resized: {} x {}",
                w, h
            );

            continue;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            if (notsa::ui::UIRenderer::GetInstance().IsActive()) {
                break;
            }
            static CVector2D s_MousePos{};

            // Use desktop cursor speed if it's windowed, accelerate ingame otherwise.
            if (IsInFullscreen() && FrontEndMenuManager.m_bMenuActive) {
                s_MousePos.x += e.motion.xrel * CCamera::m_fMouseAccelHorzntl * 100.f;
                s_MousePos.y += e.motion.yrel * CCamera::m_fMouseAccelVertical * 100.f;
            } else {
                s_MousePos.x = e.motion.x;
                s_MousePos.y = e.motion.y;
            }
            FrontEndMenuManager.m_nMousePosWinX = (int32)(s_MousePos.x);
            FrontEndMenuManager.m_nMousePosWinY = (int32)(s_MousePos.y);
            break;
        }
        // Standalone: `ForegroundApp` used to be maintained by the Win32 WndProc (WM_ACTIVATEAPP /
        // WM_SETFOCUS / WM_KILLFOCUS, see WndProc.cpp), which isn't compiled under SDL. Left at its
        // zero-initialized `false` it silently disabled mouse input, because `CPad::UpdateMouse()`
        // bails out when `!IsForegroundApp()` and so the button state never propagates from
        // `TempMouseControllerState` to `NewMouseControllerState` (keyboard survived only because
        // `OldKeyState = exchange(NewKeyState, TempKeyState)` happens outside that guard).
        // Mirror the original's semantics using SDL's focus events.
        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            ForegroundApp = true;
            if (!FrontEndMenuManager.m_bMainMenuSwitch && !FrontEndMenuManager.m_bMenuActive) {
                FrontEndMenuManager.m_bActivateMenuNextFrame = true; // WM_SETFOCUS
            }
            RsEventHandler(rsACTIVATE, (void*)true); // WM_ACTIVATEAPP
            for (auto& pad : CPad::Pads) {
                pad.Clear(false, true);
            }
            NOTSA_LOG_DEBUG("SDL: focus gained");
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_LOST: {
            ForegroundApp = false;
            RsEventHandler(rsACTIVATE, (void*)false);
            for (auto& pad : CPad::Pads) {
                pad.Clear(false, true);
            }
            // NOTE: the original also PauseAllSounds()/SetTimer() here; audio is stubbed in the
            // standalone build for now, so only the foreground flag + pad clearing are replicated.
            NOTSA_LOG_DEBUG("SDL: focus lost");
            break;
        }
        }

        if (CPad::ProcessEvent(e, imIO && imIO->WantCaptureMouse, imIO && imIO->WantCaptureKeyboard)) {
            continue;
        }

        //NOTSA_LOG_DEBUG("SDL: Unprocessed event: {}", e.type);
    }
}
}; // namespace SDLWrapper
}; // namespace notsa
#endif
