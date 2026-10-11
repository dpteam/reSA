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

void SetTextureFindCallBack(void* cb) {
    rw::Texture::findCB = reinterpret_cast<rw::Texture* (*)(const char*)>(cb);
}

void SetTextureReadCallBack(void* cb) {
    rw::Texture::readCB = reinterpret_cast<rw::Texture* (*)(const char*, const char*)>(cb);
}

//-------------------------------------------------------------------------
// Object creation / access
//-------------------------------------------------------------------------
void* CameraCreate() {
    // NOTE: the frame + rasters are created by the game afterwards (RwFrameCreate / RwRasterCreate).
    return rw::Camera::create();
}

void CameraDestroy(void* camera) {
    if (!camera) {
        return;
    }
    static_cast<rw::Camera*>(camera)->destroy();
}

void CameraSetRaster(void* camera, void* raster) {
    static_cast<rw::Camera*>(camera)->frameBuffer = static_cast<rw::Raster*>(raster);
}

void* CameraGetRaster(const void* camera) {
    return const_cast<rw::Camera*>(static_cast<const rw::Camera*>(camera))->frameBuffer;
}

void CameraSetZRaster(void* camera, void* raster) {
    static_cast<rw::Camera*>(camera)->zBuffer = static_cast<rw::Raster*>(raster);
}

void* CameraGetZRaster(const void* camera) {
    return const_cast<rw::Camera*>(static_cast<const rw::Camera*>(camera))->zBuffer;
}

void* CameraGetFrame(const void* camera) {
    return const_cast<rw::Camera*>(static_cast<const rw::Camera*>(camera))->getFrame();
}

void CameraSetNearClipPlane(void* camera, float nearClip) {
    static_cast<rw::Camera*>(camera)->setNearPlane(nearClip);
}

void CameraSetFarClipPlane(void* camera, float farClip) {
    static_cast<rw::Camera*>(camera)->setFarPlane(farClip);
}

void CameraSetViewWindow(void* camera, const float* xy) {
    const rw::V2d v{xy[0], xy[1]};
    static_cast<rw::Camera*>(camera)->setViewWindow(&v);
}

void CameraSetViewOffset(void* camera, const float* xy) {
    const rw::V2d v{xy[0], xy[1]};
    static_cast<rw::Camera*>(camera)->setViewOffset(&v);
}

void CameraSetProjection(void* camera, int projection) {
    static_cast<rw::Camera*>(camera)->setProjection(projection);
}

void CameraBeginUpdate(void* camera) {
    static_cast<rw::Camera*>(camera)->beginUpdate();
}

void CameraEndUpdate(void* camera) {
    static_cast<rw::Camera*>(camera)->endUpdate();
}

void CameraClear(void* camera, const unsigned char* rgba, int clearMode) {
    rw::RGBA col{rgba[0], rgba[1], rgba[2], rgba[3]};
    static_cast<rw::Camera*>(camera)->clear(&col, static_cast<rw::uint32>(clearMode));
}

void CameraShowRaster(void* camera, unsigned flags) {
    static_cast<rw::Camera*>(camera)->showRaster(flags);
}

void* FrameCreate() {
    return rw::Frame::create();
}

void FrameDestroy(void* frame) {
    if (frame) {
        static_cast<rw::Frame*>(frame)->destroy();
    }
}

void* FrameGetLTM(void* frame) {
    return static_cast<rw::Frame*>(frame)->getLTM();
}

void FrameRotate(void* frame, const float* axis, float angle, int combineOp) {
    const rw::V3d v{axis[0], axis[1], axis[2]};
    static_cast<rw::Frame*>(frame)->rotate(&v, angle, static_cast<rw::CombineOp>(combineOp));
}

void FrameTranslate(void* frame, const float* translation, int combineOp) {
    const rw::V3d v{translation[0], translation[1], translation[2]};
    static_cast<rw::Frame*>(frame)->translate(&v, static_cast<rw::CombineOp>(combineOp));
}

void FrameScale(void* frame, const float* scale, int combineOp) {
    const rw::V3d v{scale[0], scale[1], scale[2]};
    static_cast<rw::Frame*>(frame)->scale(&v, static_cast<rw::CombineOp>(combineOp));
}

void FrameTransform(void* frame, const void* matrix, int combineOp) {
    static_cast<rw::Frame*>(frame)->transform(static_cast<const rw::Matrix*>(matrix), static_cast<rw::CombineOp>(combineOp));
}

void FrameSetIdentity(void* frame) {
    static_cast<rw::Frame*>(frame)->matrix.setIdentity();
}

void* RasterCreate(int width, int height, int depth, int flags) {
    return rw::Raster::create(width, height, depth, flags);
}

void RasterDestroy(void* raster) {
    if (raster) {
        static_cast<rw::Raster*>(raster)->destroy();
    }
}

int RasterGetWidth(const void* raster) {
    return static_cast<const rw::Raster*>(raster)->width;
}

int RasterGetHeight(const void* raster) {
    return static_cast<const rw::Raster*>(raster)->height;
}

void* MatrixCreate() {
    return rw::Matrix::create();
}

void ObjectHasFrameSetFrame(void* object, void* frame) {
    static_cast<rw::ObjectWithFrame*>(object)->setFrame(static_cast<rw::Frame*>(frame));
}

//-------------------------------------------------------------------------
// World / Light
//-------------------------------------------------------------------------
void* WorldCreate(const void* bbox) {
    return rw::World::create(const_cast<rw::BBox*>(reinterpret_cast<const rw::BBox*>(bbox)));
}

void WorldAddCamera(void* world, void* camera) {
    static_cast<rw::World*>(world)->addCamera(static_cast<rw::Camera*>(camera));
}

void WorldAddLight(void* world, void* light) {
    static_cast<rw::World*>(world)->addLight(static_cast<rw::Light*>(light));
}

void* LightCreate(int type) {
    return rw::Light::create(type);
}

void LightDestroy(void* light) {
    if (light) {
        static_cast<rw::Light*>(light)->destroy();
    }
}

void LightSetRadius(void* light, float radius) {
    static_cast<rw::Light*>(light)->radius = radius;
}

void LightSetColor(void* light, const float* rgba) {
    auto* l = static_cast<rw::Light*>(light);
    l->color.red   = rgba[0];
    l->color.green = rgba[1];
    l->color.blue  = rgba[2];
    l->color.alpha = rgba[3];
}

void LightSetFlags(void* light, unsigned flags) {
    static_cast<rw::Light*>(light)->setFlags(flags);
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
