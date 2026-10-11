#pragma once

#include <Windows.h>

namespace notsa {
/*!
 * @brief Handle of the module our code is loaded from.
 * @note In the regular (ASI/DLL) build this is the DLL's `HMODULE` (The one passed to `DllMain`).
 *       In the standalone (game) build we're the executable, so this returns the exe's module.
 */
HMODULE GetDLLHandle();
};
