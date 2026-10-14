/*
 * librw bridge implementation (see RwBridge.h for why this exists).
 *
 * ⚠️ This is the ONLY translation unit that includes librw's headers. It deliberately does NOT include
 *    our `StdInc.h` (the PCH is disabled for this file in CMake) to avoid clashing with SA's RW headers.
 */

#ifdef NOTSA_STANDALONE

#include <windows.h>
#include <string.h>
#include <vector>

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

#define NOTSA_DEF_PLG_FUNCS(P, T)                                                                  \
    int P##RegisterPlugin(int size, unsigned id, void* ctor, void* dtor, void* copy) {              \
        return T::registerPlugin(size, id, reinterpret_cast<rw::Constructor>(ctor),                 \
                                 reinterpret_cast<rw::Destructor>(dtor),                            \
                                 reinterpret_cast<rw::CopyConstructor>(copy));                      \
    }                                                                                              \
    int P##RegisterPluginStream(unsigned id, void* read, void* write, void* getSize) {              \
        return T::registerPluginStream(id, reinterpret_cast<rw::StreamRead>(read),                  \
                                       reinterpret_cast<rw::StreamWrite>(write),                    \
                                       reinterpret_cast<rw::StreamGetSize>(getSize));               \
    }                                                                                              \
    int P##GetPluginOffset(unsigned id) { return T::getPluginOffset(id); }                          \
    int P##SetStreamAlwaysCallback(unsigned id, void* cb) {                                         \
        return T::setStreamAlwaysCallback(id, reinterpret_cast<rw::AlwaysCallback>(cb));            \
    }                                                                                              \
    int P##SetStreamRightsCallback(unsigned id, void* cb) {                                         \
        return T::setStreamRightsCallback(id, reinterpret_cast<rw::RightsCallback>(cb));            \
    }

NOTSA_DEF_PLG_FUNCS(Frame, rw::Frame)
NOTSA_DEF_PLG_FUNCS(Raster, rw::Raster)
NOTSA_DEF_PLG_FUNCS(Texture, rw::Texture)
NOTSA_DEF_PLG_FUNCS(TexDictionary, rw::TexDictionary)
NOTSA_DEF_PLG_FUNCS(Material, rw::Material)
NOTSA_DEF_PLG_FUNCS(Geometry, rw::Geometry)
NOTSA_DEF_PLG_FUNCS(Atomic, rw::Atomic)
NOTSA_DEF_PLG_FUNCS(Clump, rw::Clump)
NOTSA_DEF_PLG_FUNCS(Light, rw::Light)
NOTSA_DEF_PLG_FUNCS(Camera, rw::Camera)
NOTSA_DEF_PLG_FUNCS(World, rw::World)
#undef NOTSA_DEF_PLG_FUNCS

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

// RW's D3D9 driver keeps the "stencil value to clear to" as a driver global (0xC97C44 in the original).
// librw has no such slot, so the bridge stores it here; see SetStencilClear.
static unsigned s_stencilClear{};

void SetStencilClear(unsigned value) {
    s_stencilClear = value;
}

unsigned GetStencilClear() {
    return s_stencilClear;
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
    // NOTE: this used to be just `matrix.setIdentity()`, which left the hierarchy marked clean - so any
    // cached LTM / attached objects kept their old transform until something else dirtied the frame.
    // RW's RwFrameSetIdentity sets the sync flags too (disasm 0x7F10B0 ORs 0x3 == HIERARCHYSYNC into the
    // frame flags after writing the identity), which is what Frame::updateObjects() does in librw.
    auto* f = static_cast<rw::Frame*>(frame);
    f->matrix.setIdentity();
    f->updateObjects();
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

/*! @brief Translate the VALUE of rwRENDERSTATEALPHATESTFUNCTION from SA's enum to librw's.

    This is the one render-state enum whose *values* differ between the two (blend, stencil, cull,
    filter, addressing and primitive type all agree, so they are passed through untouched):
      * SA  (rwplcore.h `RwAlphaTestFunction`, D3D-style): NEVER=1 LESS=2 EQUAL=3 LESSEQUAL=4
        GREATER=5 NOTEQUAL=6 GREATEREQUAL=7 ALWAYS=8
      * librw (rwrender.h `rw::AlphaTestFunc`): ALWAYS=0 GREATEREQUAL=1 LESS=2
    Passing 5..8 straight through indexed `alphafuncMap[]` out of bounds, which yielded a garbage
    compare func (measured: D3DCMP_LESS) - so the alpha test librw enables for any texture with
    alpha discarded the *opaque* texels and kept the transparent ones. That made every hasAlpha
    texture invisible (font glyphs!), while opaque textures (DXT1 menus, C888) drew fine. */
static unsigned TranslateAlphaTestFunc(uintptr_t sa) {
    switch (sa) {
    case 8:  return 0; // ALWAYS       -> ALPHAALWAYS
    case 7:  return 1; // GREATEREQUAL -> ALPHAGREATEREQUAL
    case 6:  return 0; // NOTEQUAL     -> ALPHAALWAYS        (librw has no EQUAL/NOTEQUAL)
    case 5:  return 1; // GREATER      -> ALPHAGREATEREQUAL  (closest supported)
    case 4:  return 2; // LESSEQUAL    -> ALPHALESS          (closest supported)
    case 3:  return 1; // EQUAL        -> ALPHAGREATEREQUAL  (closest supported)
    case 2:  return 2; // LESS         -> ALPHALESS
    case 1:  return 2; // NEVER        -> ALPHALESS (passes only if ref > 0; librw has no NEVER)
    default: return 0; // NA/invalid   -> ALPHAALWAYS
    }
}

/*! @brief Inverse of TranslateAlphaTestFunc(), for RwRenderStateGet(). */
static unsigned UntranslateAlphaTestFunc(uintptr_t lib) {
    switch (lib) {
    case 0:  return 8; // ALPHAALWAYS       -> ALWAYS
    case 1:  return 7; // ALPHAGREATEREQUAL -> GREATEREQUAL
    case 2:  return 2; // ALPHALESS         -> LESS
    default: return 8;
    }
}

bool RenderStateSet(int state, void* value) {
    const int s = TranslateRenderState(state);
    if (s < 0) {
        return true; // No librw equivalent (eg. shademode/perspective/fogtype) - ignore.
    }
    if (s == 21) // ALPHATESTFUNC - the enum values need translating too, not just the state id.
        value = reinterpret_cast<void*>(static_cast<uintptr_t>(
            TranslateAlphaTestFunc(reinterpret_cast<uintptr_t>(value))));
    rw::SetRenderStatePtr(s, value);
    return true;
}

bool RenderStateGet(int state, void* value) {
    const int s = TranslateRenderState(state);
    if (s < 0) {
        return true;
    }
    void* v = rw::GetRenderStatePtr(s);
    if (s == 21)
        v = reinterpret_cast<void*>(static_cast<uintptr_t>(
            UntranslateAlphaTestFunc(reinterpret_cast<uintptr_t>(v))));
    *static_cast<void**>(value) = v;
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

// The one-and-only stream backing `_rwStreamInitialize` (see StreamInitializeMemory below). Process-lifetime,
// never deleted - `StreamClose` must not free it, because the game closes it more than once.
static rw::StreamMemory& CachedMemoryStream() {
    static rw::StreamMemory s{};
    return s;
}

void* StreamInitializeMemory(const void* data, unsigned size) {
    // One cached stream, re-opened per call - see the comment in RwBridge.h. `StreamMemory::open` resets
    // position to 0 and `close` is a no-op, so re-opening is exactly what the original's in-place
    // re-initialisation of its `gRwStream` buffer did.
    if (!data) {
        return nullptr;
    }
    CachedMemoryStream().open(const_cast<rw::uint8*>(static_cast<const rw::uint8*>(data)), size);
    return &CachedMemoryStream();
}

void StreamClose(void* stream) {
    if (auto* s = static_cast<rw::Stream*>(stream)) {
        s->close();
        if (static_cast<rw::Stream*>(&CachedMemoryStream()) != s) {
            delete s; // owned by us (file / ad-hoc memory stream)
        }
    }
}

unsigned StreamRead(void* stream, void* buffer, unsigned length) {
    return static_cast<rw::Stream*>(stream)->read8(buffer, length);
}

bool StreamReadChunkHeaderInfo(void* stream, unsigned* type, unsigned* length, unsigned* version, unsigned* build) {
    rw::ChunkHeaderInfo hdr{};
    if (!rw::readChunkHeaderInfo(static_cast<rw::Stream*>(stream), &hdr)) {
        return false;
    }
    if (type)    *type    = hdr.type;
    if (length)  *length  = hdr.length;
    if (version) *version = hdr.version;
    if (build)   *build   = hdr.build;
    return true;
}

bool StreamReadInt16(void* stream, void* buf, unsigned numBytes) {
    return static_cast<rw::Stream*>(stream)->read16(buf, numBytes) == numBytes;
}

bool StreamReadInt32(void* stream, void* buf, unsigned numBytes) {
    return static_cast<rw::Stream*>(stream)->read32(buf, numBytes) == numBytes;
}

bool StreamReadReal(void* stream, void* buf, unsigned numBytes) {
    // RwReal is float on D3D9; the original forwards to the plain byte reader (disasm 0x7ED4F0),
    // so no conversion is wanted here.
    return static_cast<rw::Stream*>(stream)->read8(buf, numBytes) == numBytes;
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

void TexDictionaryDestroy(void* txd) {
    if (txd) {
        static_cast<rw::TexDictionary*>(txd)->destroy();
    }
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

//-------------------------------------------------------------------------
// RpWorld layer: clumps / atomics / geometry / materials.
// The `Rp*` pointers the game holds ARE librw objects, so these are thin wrappers that keep the
// layout difference (MEMORY.md §10) inside this TU. Pointers passed to/from game callbacks are the
// librw ones, so the game can hand them straight back to these functions.
//-------------------------------------------------------------------------
namespace {
template <typename T> T* rp(const void* p) { return static_cast<T*>(const_cast<void*>(p)); }
// SA's RpAtomicCallBack / RpMaterialCallBack, in the bridge's POD spelling.
using RwCb = void* (*)(void*, void*);
} // namespace

void* ClumpStreamRead(void* stream) {
    return rw::Clump::streamRead(static_cast<rw::Stream*>(stream));
}

void* ClumpCreate() {
    return rw::Clump::create();
}

bool ClumpDestroy(void* clump) {
    if (!clump) return false;
    rp<rw::Clump>(clump)->destroy();
    return true;
}

void* ClumpForAllAtomics(void* clump, void* cb, void* data) {
    auto* c = rp<rw::Clump>(clump);
    if (!c || !cb) return clump;
    // The callback is allowed to remove the atomic it is given, so latch the next link first (as RW does).
    for (rw::LLLink* link = c->atomics.link.next; link != c->atomics.end(); ) {
        rw::LLLink* next = link->next;
        static_cast<RwCb>(cb)(rw::Atomic::fromClump(link), data);
        link = next;
    }
    return clump;
}

void* ClumpGetFirstAtomic(void* clump) {
    auto* c = rp<rw::Clump>(clump);
    if (!c || c->atomics.isEmpty()) return nullptr;
    return rw::Atomic::fromClump(c->atomics.link.next);
}

int ClumpCountAtomics(const void* clump) {
    return rp<rw::Clump>(clump)->countAtomics();
}

void* ClumpGetFrame(const void* clump) {
    return rp<rw::Clump>(clump)->getFrame();
}

void ClumpSetFrame(void* clump, void* frame) {
    rp<rw::Clump>(clump)->setFrame(static_cast<rw::Frame*>(frame));
}

void* ClumpClone(void* clump) {
    return rp<rw::Clump>(clump)->clone();
}

void* ClumpAddAtomic(void* clump, void* atomic) {
    auto* c = rp<rw::Clump>(clump);
    c->addAtomic(rp<rw::Atomic>(atomic));
    return c;
}

void* ClumpRemoveAtomic(void* clump, void* atomic) {
    auto* c = rp<rw::Clump>(clump);
    c->removeAtomic(rp<rw::Atomic>(atomic));
    return c;
}

void* ClumpAddLight(void* clump, void* light) {
    auto* c = rp<rw::Clump>(clump);
    c->addLight(rp<rw::Light>(light));
    return c;
}

void* ClumpRemoveLight(void* clump, void* light) {
    auto* c = rp<rw::Clump>(clump);
    c->removeLight(rp<rw::Light>(light));
    return c;
}

void* ClumpAddCamera(void* clump, void* camera) {
    auto* c = rp<rw::Clump>(clump);
    c->addCamera(rp<rw::Camera>(camera));
    return c;
}

void* ClumpRemoveCamera(void* clump, void* camera) {
    auto* c = rp<rw::Clump>(clump);
    c->removeCamera(rp<rw::Camera>(camera));
    return c;
}

void* AtomicCreate() {
    return rw::Atomic::create();
}

void* AtomicClone(void* atomic) {
    return rp<rw::Atomic>(atomic)->clone();
}

void AtomicDestroy(void* atomic) {
    if (atomic) rp<rw::Atomic>(atomic)->destroy();
}

void* AtomicGetGeometry(const void* atomic) {
    return rp<rw::Atomic>(atomic)->geometry;
}

void* AtomicSetFrame(void* atomic, void* frame) {
    rp<rw::Atomic>(atomic)->setFrame(static_cast<rw::Frame*>(frame));
    return atomic;
}

void* AtomicGetFrame(const void* atomic) {
    return rp<rw::Atomic>(atomic)->getFrame();
}

void* AtomicSetGeometry(void* atomic, void* geometry, unsigned flags) {
    rp<rw::Atomic>(atomic)->setGeometry(rp<rw::Geometry>(geometry), flags);
    return atomic;
}

void* AtomicGetClump(const void* atomic) {
    return rp<rw::Atomic>(atomic)->clump;
}

void* AtomicGetBoundingSphere(const void* atomic) {
    return &rp<rw::Atomic>(atomic)->boundingSphere;
}

unsigned AtomicGetFlags(const void* atomic) {
    return rp<rw::Atomic>(atomic)->getFlags();
}

void AtomicSetFlags(void* atomic, unsigned flags) {
    rp<rw::Atomic>(atomic)->setFlags(flags);
}

unsigned& GeometryFlagsRef(void* geometry) {
    // Returned by reference on purpose: SA writes through it (`RpGeometryGetFlags(g) &= ...`).
    return rp<rw::Geometry>(geometry)->flags;
}

void* GeometryLock(void* geometry, int lockMode) {
    rp<rw::Geometry>(geometry)->lock(lockMode);
    return geometry;
}

void* GeometryUnlock(void* geometry) {
    rp<rw::Geometry>(geometry)->unlock();
    return geometry;
}

int GeometryGetNumVertices(const void* geometry) {
    return rp<rw::Geometry>(geometry)->numVertices;
}

int GeometryGetNumTriangles(const void* geometry) {
    return rp<rw::Geometry>(geometry)->numTriangles;
}

int GeometryGetNumMaterials(const void* geometry) {
    return rp<rw::Geometry>(geometry)->matList.numMaterials;
}

void* GeometryGetPreLitColors(void* geometry) {
    return rp<rw::Geometry>(geometry)->colors; // librw's name for SA's `preLitLum`
}

void* GeometryGetVertices(void* geometry) {
    auto* g = rp<rw::Geometry>(geometry);
    return g->numMorphTargets ? (void*)g->morphTargets[0].vertices : nullptr;
}

void* GeometryGetNormals(void* geometry) {
    auto* g = rp<rw::Geometry>(geometry);
    return g->numMorphTargets ? (void*)g->morphTargets[0].normals : nullptr;
}

void* GeometryGetTexCoords(void* geometry, int uvIndex) {
    auto* g = rp<rw::Geometry>(geometry);
    // SA's RpGeometryGetVertexTexCoords(g, n) is 1-based: it reads texCoords[n - 1].
    if (uvIndex < 1 || uvIndex > 8) return nullptr;
    return g->texCoords[uvIndex - 1];
}

void* GeometryGetTriangles(void* geometry) {
    return rp<rw::Geometry>(geometry)->triangles;
}

void* GeometryGetMorphTarget(void* geometry, int index) {
    auto* g = rp<rw::Geometry>(geometry);
    if (index < 0 || index >= g->numMorphTargets) return nullptr;
    return &g->morphTargets[index];
}

int GeometryGetNumMorphTargets(const void* geometry) {
    return rp<rw::Geometry>(geometry)->numMorphTargets;
}

int GeometryGetNumTexCoordSets(const void* geometry) {
    return rp<rw::Geometry>(geometry)->numTexCoordSets;
}

void* MorphTargetGetVertices(void* morphTarget) {
    return rp<rw::MorphTarget>(morphTarget)->vertices;
}

void* MorphTargetGetNormals(void* morphTarget) {
    return rp<rw::MorphTarget>(morphTarget)->normals;
}

void* GeometryGetMaterial(void* geometry, int num) {
    auto* g = rp<rw::Geometry>(geometry);
    if (num < 0 || num >= g->matList.numMaterials) return nullptr;
    return g->matList.materials[num];
}

void* GeometryForAllMaterials(void* geometry, void* cb, void* data) {
    auto* g = rp<rw::Geometry>(geometry);
    if (!g || !cb) return geometry;
    for (int i = 0; i < g->matList.numMaterials; i++) {
        if (auto* m = g->matList.materials[i]) {
            static_cast<RwCb>(cb)(m, data);
        }
    }
    return geometry;
}

void* MaterialSetTexture(void* material, void* texture) {
    rp<rw::Material>(material)->setTexture(static_cast<rw::Texture*>(texture));
    return material;
}

void* MaterialGetTexture(const void* material) {
    return rp<rw::Material>(material)->texture;
}

void* MaterialSetColor(void* material, const unsigned char* rgba) {
    auto& c = rp<rw::Material>(material)->color;
    c.red = rgba[0]; c.green = rgba[1]; c.blue = rgba[2]; c.alpha = rgba[3];
    return material;
}

void* MaterialGetColor(const void* material) {
    return &rp<rw::Material>(material)->color;
}

//-------------------------------------------------------------------------
// RwImage / RT file readers - see the declarations in RwBridge.h for the layout caveat.
//-------------------------------------------------------------------------
void* ImageReadBMP(const char* path) {
    return rw::readBMP(path);
}

void* ImageReadPNG(const char* path) {
    return rw::readPNG(path);
}

void* ImageCreate(int width, int height, int depth) {
    return rw::Image::create(width, height, depth);
}

void ImageDestroy(void* image) {
    if (image) {
        rp<rw::Image>(image)->destroy();
    }
}

bool ImageFindRasterFormat(void* image, int rasterType, int* w, int* h, int* depth, unsigned* format) {
    int32_t fmt = 0;
    const bool ok = rw::Raster::imageFindRasterFormat(rp<rw::Image>(image), rasterType, w, h, depth, &fmt);
    if (format) {
        *format = static_cast<unsigned>(fmt);
    }
    return ok != 0;
}

void RasterSetFromImage(void* raster, void* image) {
    rp<rw::Raster>(raster)->setFromImage(rp<rw::Image>(image));
}

//-------------------------------------------------------------------------
// RpSkin plugin - see the declarations in RwBridge.h for the field correspondence.
//-------------------------------------------------------------------------
void* SkinGeometryGetSkin(void* geometry) {
    return rw::Skin::get(rp<const rw::Geometry>(geometry));
}

void SkinGeometrySetSkin(void* geometry, void* skin) {
    rw::Skin::set(rp<rw::Geometry>(geometry), rp<rw::Skin>(skin));
}

void* AtomicGetHAnimHierarchy(const void* atomic) {
    return rw::Skin::getHierarchy(rp<const rw::Atomic>(atomic));
}

void AtomicSetHAnimHierarchy(void* atomic, void* hier) {
    rw::Skin::setHierarchy(rp<rw::Atomic>(atomic), rp<rw::HAnimHierarchy>(hier));
}

int SkinGetNumBones(const void* skin) {
    return rp<const rw::Skin>(skin)->numBones;
}

void* SkinGetVertexBoneWeights(const void* skin) {
    return rp<const rw::Skin>(skin)->weights;
}

void* SkinGetVertexBoneIndices(const void* skin) {
    return rp<const rw::Skin>(skin)->indices;
}

void* SkinGetSkinToBoneMatrices(const void* skin) {
    return rp<const rw::Skin>(skin)->inverseMatrices;
}

//-------------------------------------------------------------------------
// RpHAnim plugin - see the declarations in RwBridge.h.
//-------------------------------------------------------------------------
void* HAnimFrameGetHierarchy(void* frame) {
    return rw::HAnimHierarchy::get(rp<rw::Frame>(frame));
}

bool HAnimFrameSetHierarchy(void* frame, void* hierarchy) {
    // RpHAnimFrameSetHierarchy returns FALSE when the frame already has one attached (the original
    // refuses to overwrite); mirror that so callers that check it behave identically.
    auto* f  = rp<rw::Frame>(frame);
    auto* hd = rw::HAnimData::get(f);
    if (hd->hierarchy) {
        return false;
    }
    hd->hierarchy = rp<rw::HAnimHierarchy>(hierarchy);
    return true;
}

void* HAnimGetHierarchyFromFrame(void* frame) {
    auto* f = rp<rw::Frame>(frame);
    // Disasm 0x734AB0: HAnimHierarchy::get(f), and if that is null, RwFrameForAllObjects with a callback
    // that does the same lookup per child object - i.e. librw's recursive HAnimHierarchy::find(f).
    if (auto* hier = rw::HAnimHierarchy::get(f)) {
        return hier;
    }
    return rw::HAnimHierarchy::find(f);
}

void* HAnimHierarchyGetMatrices(void* hierarchy) {
    return rp<rw::HAnimHierarchy>(hierarchy)->matrices;
}

int HAnimIDGetIndex(void* hierarchy, int id) {
    return rp<rw::HAnimHierarchy>(hierarchy)->getIndex(id);
}

void HAnimHierarchyUpdateMatrices(void* hierarchy) {
    if (!hierarchy) {
        return;
    }
    auto* h = rp<rw::HAnimHierarchy>(hierarchy);
    // Standalone: librw's `HAnimHierarchy::create` does allocate an AnimInterpolator (anim.cpp:186), but
    // `AnimInterpolator::create` initialises only up to `numNodes` and leaves the *callbacks*
    // (applyCB/blendCB/interpCB/addCB) unset - its own header says "TODO some callbacks, parent/sub".
    // They are filled in when an animation is assigned via setCurrentAnim, and `updateMatrices` calls
    // `anim->applyCB(&animMat, anim->getInterpFrame(i))` unconditionally (hanim.cpp:175). So updating a
    // hierarchy that has no current animation is a call through uninitialised heap - measured as EIP
    // 0xCDCDCDCD from CEntity::UpdateRpHAnim during CPed::SetModelIndex.
    //
    // This is not a workaround for a librw bug so much as the original's precondition stated explicitly:
    // disasm 0x7C52B1 loads the hierarchy's currentAnim and immediately dereferences it (+0x24, +0x3C,
    // +0x4C) with NO null check, i.e. RW 3.7 also requires an animation to be attached first - in the
    // original that is guaranteed by RpHAnimHierarchyAttach (0x7C4F40), which our RtAnim bridge does not
    // implement yet (see IDEAS.md: the interpolator/animation storage model differs - SA keeps a pFrames
    // pointer, librw keeps frames inline after the struct).
    //
    // Skipping is the honest interim: the bone matrices simply keep whatever the hierarchy was built with,
    // i.e. skinned peds hold their bind pose. Checking applyCB rather than only currentAnim means the
    // *actual* thing being called is verified, so a future path that sets currentAnim without caching the
    // callbacks cannot reintroduce the same silent garbage call.
    const auto* interp = h->interpolator;
    if (!interp || !interp->currentAnim || !interp->applyCB) {
        return;
    }
    h->updateMatrices();
}

void* FrameForAllChildren(void* frame, void* cb, void* data) {
    auto* f = rp<rw::Frame>(frame);
    if (!cb) return f;
    f->forAllChildren(reinterpret_cast<rw::Frame::Callback>(cb), data);
    return f;
}

int ObjectGetType(const void* object) {
    return rp<const rw::ObjectWithFrame>(object)->object.type;
}

// NOTE: FrameGetLTM, FrameDestroy and FrameSetIdentity already exist above (near FrameCreate) - not
// duplicated here.

void* FrameGetRoot(const void* frame) {
    return rp<const rw::Frame>(frame)->root;
}

void FrameUpdateObjects(void* frame) {
    rp<rw::Frame>(frame)->updateObjects();
}

void FrameSyncDirty() {
    rw::Frame::syncDirty();
}

int FrameCount(const void* frame) {
    return const_cast<rw::Frame*>(rp<const rw::Frame>(frame))->count();
}

void* FrameAddChild(void* parent, void* child, int append) {
    auto* p = rp<rw::Frame>(parent);
    p->addChild(rp<rw::Frame>(child), append != 0);
    return p;
}

void* FrameRemoveChild(void* child) {
    return rp<rw::Frame>(child)->removeChild();
}

void* FrameCloneHierarchy(void* root) {
    return rp<rw::Frame>(root)->cloneHierarchy();
}

void FrameDestroyHierarchy(void* root) {
    rp<rw::Frame>(root)->destroyHierarchy();
}

void* FrameForAllObjects(void* frame, void* cb, void* data) {
    auto* f = rp<rw::Frame>(frame);
    if (!cb) return f;
    // Disasm 0x7F1200: walk frame->objectList, converting each LLLink to its ObjectWithFrame (the link
    // sits at +8 in the object), and latch `next` *before* invoking the callback - so a callback that
    // detaches its object (the common case: it stops by returning NULL) cannot corrupt the walk.
    using ObjCb = void* (*)(void*, void*);
    for (rw::LLLink* lnk = f->objectList.link.next; lnk != f->objectList.end(); ) {
        rw::LLLink* next = lnk->next;
        auto* obj = rw::ObjectWithFrame::fromFrame(lnk);
        if (static_cast<ObjCb>(cb)(obj, data) == nullptr) {
            break;
        }
        lnk = next;
    }
    return f;
}

//-------------------------------------------------------------------------
// Matrix / Vector math - see the declarations in RwBridge.h for why this is safe as plain casts.
// (`MatrixCreate()` already exists above, next to the other object helpers.)
//-------------------------------------------------------------------------
void MatrixDestroy(void* matrix) {
    if (matrix) {
        rp<rw::Matrix>(matrix)->destroy();
    }
}

void* MatrixMultiply(void* out, const void* src1, const void* src2) {
    rw::Matrix::mult(rp<rw::Matrix>(out), rp<const rw::Matrix>(src1), rp<const rw::Matrix>(src2));
    return out;
}

void* MatrixInvert(void* out, const void* src) {
    rw::Matrix::invert(rp<rw::Matrix>(out), rp<const rw::Matrix>(src));
    return out;
}

void* MatrixTranspose(void* out, const void* src) {
    rw::Matrix::transpose(rp<rw::Matrix>(out), rp<const rw::Matrix>(src));
    return out;
}

void* MatrixTransform(void* matrix, const void* transform, int combineOp) {
    return rp<rw::Matrix>(matrix)->transform(rp<const rw::Matrix>(transform),
                                             static_cast<rw::CombineOp>(combineOp));
}

void* MatrixRotate(void* matrix, const void* axis, float angle, int combineOp) {
    return rp<rw::Matrix>(matrix)->rotate(rp<const rw::V3d>(axis), angle,
                                          static_cast<rw::CombineOp>(combineOp));
}

void* MatrixTranslate(void* matrix, const void* translation, int combineOp) {
    return rp<rw::Matrix>(matrix)->translate(rp<const rw::V3d>(translation),
                                             static_cast<rw::CombineOp>(combineOp));
}

void* MatrixScale(void* matrix, const void* scale, int combineOp) {
    return rp<rw::Matrix>(matrix)->scale(rp<const rw::V3d>(scale),
                                         static_cast<rw::CombineOp>(combineOp));
}

void MatrixSetIdentity(void* matrix) {
    rp<rw::Matrix>(matrix)->setIdentity();
}

void MatrixOptimize(void* matrix) {
    rp<rw::Matrix>(matrix)->optimize();
}

void V3dTransformPoints(void* out, const void* in, int n, const void* matrix) {
    rw::V3d::transformPoints(rp<rw::V3d>(out), rp<const rw::V3d>(in), n, rp<const rw::Matrix>(matrix));
}

void V3dTransformVectors(void* out, const void* in, int n, const void* matrix) {
    rw::V3d::transformVectors(rp<rw::V3d>(out), rp<const rw::V3d>(in), n, rp<const rw::Matrix>(matrix));
}

void* RasterGetNativeTexture(const void* raster) {
    if (!raster) return nullptr;
    const auto* r = static_cast<const rw::Raster*>(raster);
    return GETD3DRASTEREXT(const_cast<rw::Raster*>(r))->texture;
}
} // namespace rwbridge

#endif // NOTSA_STANDALONE
