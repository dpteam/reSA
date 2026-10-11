/*
 * librw bridge implementation (see RwBridge.h for why this exists).
 *
 * ⚠️ This is the ONLY translation unit that includes librw's headers. It deliberately does NOT include
 *    our `StdInc.h` (the PCH is disabled for this file in CMake) to avoid clashing with SA's RW headers.
 */

#ifdef NOTSA_STANDALONE

#include <windows.h>

#include <rw.h>

#include "RwBridge.h"

namespace rwbridge {
bool EngineInit() {
    return rw::Engine::init() != 0;
}

bool EngineOpen(void* hwnd) {
    rw::EngineOpenParams params{};
    params.window = static_cast<HWND>(hwnd);
    return rw::Engine::open(&params) != 0;
}

bool EngineStart() {
    return rw::Engine::start() != 0;
}

void EngineStop()  { rw::Engine::stop(); }
void EngineClose() { rw::Engine::close(); }
void EngineTerm()  { rw::Engine::term(); }

int RegisterPlugin(int size, unsigned pluginID, void* ctor, void* dtor) {
    return rw::Engine::registerPlugin(
        size, pluginID,
        reinterpret_cast<rw::Constructor>(ctor),
        reinterpret_cast<rw::Destructor>(dtor)
    );
}

int GetPluginOffset(unsigned pluginID) {
    return rw::Engine::s_plglist.getPluginOffset(pluginID);
}

void RegisterCorePlugins() {
    rw::registerMeshPlugin();
    rw::d3d9::registerNativeDataPlugin();
    rw::registerAtomicRightsPlugin();
    rw::registerMaterialRightsPlugin();
    rw::registerMatFXPlugin();
    rw::registerSkinPlugin();
    rw::registerHAnimPlugin();
    rw::registerUVAnimPlugin();
    rw::registerAnisotropyPlugin();
    rw::registerUserDataPlugin();
}

unsigned GetMaxMultiSamplingLevels() {
    return rw::Engine::getMaxMultiSamplingLevels();
}

void SetMultiSamplingLevels(unsigned levels) {
    rw::Engine::setMultiSamplingLevels(levels);
}

void SetRefreshRate(unsigned) {
    // librw has no separate refresh-rate setting (it's part of the video mode).
}

bool DeviceSupportsDXTTexture() {
    // librw's D3D9 backend always supports DXT (it creates DXT textures directly).
    return true;
}

void SetMipmapping(bool enable) {
    rw::Texture::setMipmapping(enable ? 1 : 0);
}

void SetAutoMipmapping(bool enable) {
    rw::Texture::setAutoMipmapping(enable ? 1 : 0);
}

bool GetMipmapping() {
    return rw::Texture::getMipmapping() != 0;
}

bool GetAutoMipmapping() {
    return rw::Texture::getAutoMipmapping() != 0;
}

int GetNumSubSystems() {
    return rw::Engine::getNumSubSystems();
}

bool GetSubSystemInfo(char* nameOut, int nameSize, int index) {
    rw::SubSystemInfo info{};
    if (!rw::Engine::getSubSystemInfo(&info, index)) {
        return false;
    }
    if (nameOut && nameSize > 0) {
        strncpy_s(nameOut, nameSize, info.name, _TRUNCATE);
    }
    return true;
}

int  GetCurrentSubSystem() { return rw::Engine::getCurrentSubSystem(); }
bool SetSubSystem(int index) { return rw::Engine::setSubSystem(index) != 0; }

int GetNumVideoModes() {
    return rw::Engine::getNumVideoModes();
}

bool GetVideoModeInfo(int* width, int* height, int* depth, unsigned* flags, int index) {
    rw::VideoMode mode{};
    if (!rw::Engine::getVideoModeInfo(&mode, index)) {
        return false;
    }
    if (width)  *width  = mode.width;
    if (height) *height = mode.height;
    if (depth)  *depth  = mode.depth;
    if (flags)  *flags  = mode.flags;
    return true;
}

int  GetCurrentVideoMode() { return rw::Engine::getCurrentVideoMode(); }
bool SetVideoMode(int index) { return rw::Engine::setVideoMode(index) != 0; }

unsigned GetVersion() {
    return static_cast<unsigned>(rw::version);
}
} // namespace rwbridge

#endif // NOTSA_STANDALONE
