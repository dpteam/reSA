#pragma once

/*
 * Shared startup/"bootstrap" logic used by both entry points of the project:
 * - The regular mod (ASI/DLL) build's `DllMain` (See `dllmain.cpp`)
 * - The standalone (game) build's `WinMain` (See `app/platform/win/WinMain.cpp`)
 */

namespace notsa {
namespace bootstrap {
/*!
 * @brief Loads the INI configuration files (Eg. `gta-reversed.ini`) into memory.
 * @note May be called only once, after the current working directory is the game's directory.
 */
void LoadConfigurations();
} // namespace bootstrap
} // namespace notsa
