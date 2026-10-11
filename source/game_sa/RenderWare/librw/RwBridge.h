#pragma once

/*
 * Bridge between our (SA) RenderWare-call-sites and librw.
 *
 * Why a bridge instead of including librw directly at the call sites:
 *  - Our `source/game_sa/RenderWare/rw/*.h` headers (SA RW 3.7) and librw's headers define overlapping
 *    macros/types (eg. `RWALIGN`, `FALSE`/`TRUE`, RW enums), so including both in the same translation unit
 *    is fragile.
 *  - This header therefore exposes librw through plain C-style functions with POD (int/float/pointer) signatures.
 *    `RwBridge.cpp` (the only TU that includes librw) implements them; our RW thunks call them.
 *
 * Only compiled/linked in the standalone build (`NOTSA_STANDALONE`) - see `RwBridge.cpp`.
 */

namespace rwbridge {
/*! @brief `RwEngineInit` - initialize the engine (with librw's default memory functions). */
bool EngineInit();

/*! @brief `RwEngineOpen` - open the engine. @param hwnd The window handle (Win32), or nullptr. */
bool EngineOpen(void* hwnd);

bool EngineStart();
void EngineStop();
void EngineClose();
void EngineTerm();

/*! @brief `RwEngineRegisterPlugin`. `ctor`/`dtor` may be null. @return The plugin's object-data offset, or -1. */
int  RegisterPlugin(int size, unsigned pluginID, void* ctor, void* dtor);
/*! @brief `RwEngineGetPluginOffset`. @return The plugin's object-data offset, or -1 if not registered. */
int  GetPluginOffset(unsigned pluginID);

/*!
 * @brief Registers the RenderWare plugins the game expects attached after engine init
 *        (world/mesh/nativeData/rights, MatFX, skin, HAnim, UVAnim, anisotropy, userdata).
 * @note  Replaces the game's `PluginAttach()` chain of `RpXxxPluginAttach()` calls - those ultimately
 *        just call librw's `registerXxxPlugin()`, which is what this does (once).
 */
void RegisterCorePlugins();

/*!
 * @brief D3D9 engine settings that the game sets during device selection.
 * @note  These are the boot-critical subset of the game's `RwD3D9*` API; the rest is still `plugin::Call`.
 */
unsigned GetMaxMultiSamplingLevels();
void     SetMultiSamplingLevels(unsigned levels);
void     SetRefreshRate(unsigned hz);        //!< No-op in librw (refresh rate is part of the video mode).
bool     DeviceSupportsDXTTexture();         //!< librw's D3D9 backend supports DXT textures.

void SetMipmapping(bool enable);
void SetAutoMipmapping(bool enable);
bool GetMipmapping();
bool GetAutoMipmapping();

/*! @brief Callbacks used when loading textures (librw's `Texture::findCB` / `readCB`). */
void SetTextureFindCallBack(void* cb);
void SetTextureReadCallBack(void* cb);

int  GetNumSubSystems();bool GetSubSystemInfo(char* nameOut, int nameSize, int index);
int  GetCurrentSubSystem();
bool SetSubSystem(int index);

int  GetNumVideoModes();
/*! @brief Fills the given out-params. @return false if `index` is out of range. */
bool GetVideoModeInfo(int* width, int* height, int* depth, unsigned* flags, int index);
int  GetCurrentVideoMode();
bool SetVideoMode(int index);

/*! @brief The RenderWare library version (as reported by the engine). */
unsigned GetVersion();
} // namespace rwbridge
