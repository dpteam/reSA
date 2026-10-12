/*
 * librw bridge implementation (see RwBridge.h for why this exists).
 *
 * ⚠️ This is the ONLY translation unit that includes librw's headers. It deliberately does NOT include
 *    our `StdInc.h` (the PCH is disabled for this file in CMake) to avoid clashing with SA's RW headers.
 */

#ifdef NOTSA_STANDALONE

#include <windows.h>
#include <string.h>

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

const void* D3D9GetCaps() {
    // librw queries the device caps during device creation and caches them (returns `D3DCAPS9*`).
    return rw::d3d9::getD3D9Caps();
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

int RasterGetDepth(const void* raster) {
    return static_cast<const rw::Raster*>(raster)->depth;
}

unsigned char* RasterLock(void* raster, int level, int lockMode) {
    return static_cast<rw::Raster*>(raster)->lock(level, lockMode);
}

void RasterUnlock(void* raster) {
    static_cast<rw::Raster*>(raster)->unlock(0);
}

float Im2DGetNearScreenZ() {
    return rw::engine->device.zNear;
}

float Im2DGetFarScreenZ() {
    return rw::engine->device.zFar;
}

// Maps SA's `RwRenderState` to librw's `rw::RenderState`. Returns -1 if librw has no equivalent.
// (SA: rwplcore.h `enum RwRenderState`; librw: src/rwrender.h `enum RenderState`.)
static int TranslateRenderState(int sa) {
    switch (sa) {
    case  1: return  0; // TEXTURERASTER
    case  2: return  1; // TEXTUREADDRESS
    case  3: return  2; // TEXTUREADDRESSU
    case  4: return  3; // TEXTUREADDRESSV
    case  9: return  4; // TEXTUREFILTER
    case 12: return  5; // VERTEXALPHAENABLE
    case 10: return  6; // SRCBLEND
    case 11: return  7; // DESTBLEND
    case  6: return  8; // ZTESTENABLE
    case  8: return  9; // ZWRITEENABLE
    case 14: return 10; // FOGENABLE
    case 15: return 11; // FOGCOLOR
    case 20: return 12; // CULLMODE
    case 21: return 13; // STENCILENABLE
    case 22: return 14; // STENCILFAIL
    case 23: return 15; // STENCILZFAIL
    case 24: return 16; // STENCILPASS
    case 25: return 17; // STENCILFUNCTION
    case 26: return 18; // STENCILFUNCTIONREF
    case 27: return 19; // STENCILFUNCTIONMASK
    case 28: return 20; // STENCILFUNCTIONWRITEMASK
    case 29: return 21; // ALPHATESTFUNCTION
    case 30: return 22; // ALPHATESTFUNCTIONREF
    default: return -1; // NARENDERSTATE, TEXTUREPERSPECTIVE, SHADEMODE, BORDERCOLOR, FOGTYPE, FOGDENSITY
    }
}

bool RenderStateSet(int state, void* value) {
    const int s = TranslateRenderState(state);
    if (s < 0) {
        return true; // No librw equivalent (eg. shademode/perspective/fogtype) - ignore.
    }
    rw::SetRenderStatePtr(s, value);
    return true;
}

bool RenderStateGet(int state, void* value) {
    const int s = TranslateRenderState(state);
    if (s < 0) {
        return true;
    }
    *static_cast<void**>(value) = rw::GetRenderStatePtr(s);
    return true;
}

bool Im2DRenderLine(void* verts, int numVerts, int vert1, int vert2) {
    rw::im2d::RenderLine(verts, numVerts, vert1, vert2);
    return true;
}

bool Im2DRenderTriangle(void* verts, int numVerts, int vert1, int vert2, int vert3) {
    rw::im2d::RenderTriangle(verts, numVerts, vert1, vert2, vert3);
    return true;
}

bool Im2DRenderPrimitive(int primType, void* verts, int numVerts) {
    rw::im2d::RenderPrimitive(static_cast<rw::PrimitiveType>(primType), verts, numVerts);
    return true;
}

bool Im2DRenderIndexedPrimitive(int primType, void* verts, int numVerts, void* indices, int numIndices) {
    rw::im2d::RenderIndexedPrimitive(static_cast<rw::PrimitiveType>(primType), verts, numVerts, indices, numIndices);
    return true;
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

//-------------------------------------------------------------------------
// Streams
//-------------------------------------------------------------------------
void* StreamOpenFile(const char* filename) {
    auto* stream = new rw::StreamFile();
    if (!stream->open(filename, "rb")) {
        delete stream;
        return nullptr;
    }
    return stream;
}

void* StreamOpenMemory(const unsigned char* data, unsigned size) {
    auto* stream = new rw::StreamMemory();
    stream->open(const_cast<rw::uint8*>(data), size);
    return stream;
}

void StreamClose(void* stream) {
    if (auto* s = static_cast<rw::Stream*>(stream)) {
        s->close();
        delete s;
    }
}

unsigned StreamRead(void* stream, void* buffer, unsigned length) {
    return static_cast<rw::Stream*>(stream)->read8(buffer, length);
}

void StreamSkip(void* stream, unsigned offset) {
    static_cast<rw::Stream*>(stream)->seek(static_cast<rw::int32>(offset), 1 /* SEEK_CUR */);
}

bool StreamFindChunk(void* stream, unsigned type, unsigned* lengthOut, unsigned* versionOut) {
    rw::uint32 length{}, version{};
    if (!rw::findChunk(static_cast<rw::Stream*>(stream), type, &length, &version)) {
        return false;
    }
    if (lengthOut)  *lengthOut  = length;
    if (versionOut) *versionOut = version;
    return true;
}

void* TexDictionaryStreamRead(void* stream) {
    return rw::TexDictionary::streamRead(static_cast<rw::Stream*>(stream));
}

void* TexDictionaryGetCurrent() {
    return rw::TexDictionary::getCurrent();
}

void* TexDictionaryCreate() {
    return rw::TexDictionary::create();
}

void* TexDictionarySetCurrent(void* txd) {
    rw::TexDictionary::setCurrent(static_cast<rw::TexDictionary*>(txd));
    return txd;
}

void TexDictionaryAddTexture(void* txd, void* texture) {
    static_cast<rw::TexDictionary*>(txd)->add(static_cast<rw::Texture*>(texture));
}

void* TexDictionaryFindNamedTexture(void* txd, const char* name) {
    return static_cast<rw::TexDictionary*>(txd)->find(name);
}

void TextureDestroy(void* texture) {
    if (texture) {
        static_cast<rw::Texture*>(texture)->destroy();
    }
}

int TexDictionaryCount(void* txd) {
    return static_cast<rw::TexDictionary*>(txd)->count();
}

void* TextureRead(const char* name, const char* maskName) {
    return rw::Texture::read(name, maskName);
}

void* TextureCreate(void* raster) {
    return rw::Texture::create(static_cast<rw::Raster*>(raster));
}

void TextureSetRaster(void* texture, void* raster) {
    static_cast<rw::Texture*>(texture)->raster = static_cast<rw::Raster*>(raster);
}

void TextureSetName(void* texture, const char* name) {
    strncpy_s(static_cast<rw::Texture*>(texture)->name, name, _TRUNCATE);
}

void TextureSetMaskName(void* texture, const char* name) {
    strncpy_s(static_cast<rw::Texture*>(texture)->mask, name, _TRUNCATE);
}

void TexDictionaryRemoveTexture(void* texture) {
    if (auto* tex = static_cast<rw::Texture*>(texture)) {
        if (tex->dict) {
            tex->dict->remove(tex);
        }
    }
}

void TextureSetFilterAddressing(void* texture, unsigned filterAddressing) {
    static_cast<rw::Texture*>(texture)->filterAddressing = filterAddressing;
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

void RasterGetInfo(const void* raster, unsigned* format, int* depth, unsigned* flags) {
    if (!raster) { *format = 0; *depth = 0; *flags = 0; return; }
    const auto* r = static_cast<const rw::Raster*>(raster);
    *format = r->format;
    *depth  = r->depth;
    *flags  = r->flags;
}
} // namespace rwbridge

#endif // NOTSA_STANDALONE
