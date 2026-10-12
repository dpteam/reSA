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
/*! @brief The D3D9 device capabilities (`D3DCAPS9*`) librw queried during device creation, or nullptr. */
const void* D3D9GetCaps();

void SetMipmapping(bool enable);
void SetAutoMipmapping(bool enable);
bool GetMipmapping();
bool GetAutoMipmapping();

/*! @brief Callbacks used when loading textures (librw's `Texture::findCB` / `readCB`). */
void SetTextureFindCallBack(void* cb);
void SetTextureReadCallBack(void* cb);

//-------------------------------------------------------------------------
// Object creation / access (opaque `void*` = librw objects).
// NOTE: The game treats these as the SA RW types; only the *prefix* of the
//       structs is shared (ObjectWithFrame), so all field access must go through here.
//-------------------------------------------------------------------------
void* CameraCreate();
void  CameraDestroy(void* camera);
void  CameraSetRaster(void* camera, void* raster);
void* CameraGetRaster(const void* camera);
void  CameraSetZRaster(void* camera, void* raster);
void* CameraGetZRaster(const void* camera);
void* CameraGetFrame(const void* camera);
void  CameraSetNearClipPlane(void* camera, float nearClip);
void  CameraSetFarClipPlane(void* camera, float farClip);
void  CameraSetViewWindow(void* camera, const float* xy);
void  CameraSetViewOffset(void* camera, const float* xy);
void  CameraSetProjection(void* camera, int projection);
void  CameraBeginUpdate(void* camera);
void  CameraEndUpdate(void* camera);
void  CameraClear(void* camera, const unsigned char* rgba, int clearMode);
void  CameraShowRaster(void* camera, unsigned flags);

void* FrameCreate();
void  FrameDestroy(void* frame);
void* FrameGetLTM(void* frame);
void  FrameRotate(void* frame, const float* axis, float angle, int combineOp);
void  FrameTranslate(void* frame, const float* translation, int combineOp);
void  FrameScale(void* frame, const float* scale, int combineOp);
void  FrameTransform(void* frame, const void* matrix, int combineOp);
void  FrameSetIdentity(void* frame);

void* RasterCreate(int width, int height, int depth, int flags);
void  RasterDestroy(void* raster);
int   RasterGetWidth(const void* raster);
int   RasterGetHeight(const void* raster);
int   RasterGetDepth(const void* raster);
unsigned char* RasterLock(void* raster, int level, int lockMode);
void  RasterUnlock(void* raster);

/*! @brief 2D immediate-mode near/far screen Z (from the render device). */
float Im2DGetNearScreenZ();
float Im2DGetFarScreenZ();

/*! @brief Render-state set/get (SA `RwRenderStateSet`/`RwRenderStateGet`). Set returns true. */
bool  RenderStateSet(int state, void* value);
bool  RenderStateGet(int state, void* value);

/*! @brief 2D immediate-mode drawing (SA `RwIm2DRender*`). `primType` is `rwPRIMTYPE*`. Return true. */
bool  Im2DRenderLine(void* verts, int numVerts, int vert1, int vert2);
bool  Im2DRenderTriangle(void* verts, int numVerts, int vert1, int vert2, int vert3);
bool  Im2DRenderPrimitive(int primType, void* verts, int numVerts);
bool  Im2DRenderIndexedPrimitive(int primType, void* verts, int numVerts, void* indices, int numIndices);

void* MatrixCreate();

/*! @brief `rwObjectHasFrameSetFrame` - attach a frame to an object-has-frame (camera/atomic/light). */
void  ObjectHasFrameSetFrame(void* object, void* frame);

//-------------------------------------------------------------------------
// World / Light
//-------------------------------------------------------------------------
void* WorldCreate(const void* bbox);   //!< bbox: SA `RwBBox` (sup,inf), layout-compatible with librw's BBox; may be null.
void  WorldAddCamera(void* world, void* camera);
void  WorldAddLight(void* world, void* light);
void* LightCreate(int type);
void  LightDestroy(void* light);
void  LightSetRadius(void* light, float radius);
void  LightSetColor(void* light, const float* rgba);  //!< rgba: 4 floats (red,green,blue,alpha).
void  LightSetFlags(void* light, unsigned flags);

//-------------------------------------------------------------------------
// Streams (librw `Stream` subclass instances, kept as opaque handles)
//-------------------------------------------------------------------------
void* StreamOpenFile(const char* filename);
void* StreamOpenMemory(const unsigned char* data, unsigned size);
void  StreamClose(void* stream);
unsigned StreamRead(void* stream, void* buffer, unsigned length);
void  StreamSkip(void* stream, unsigned offset);
bool  StreamFindChunk(void* stream, unsigned type, unsigned* lengthOut, unsigned* versionOut);
void* TexDictionaryStreamRead(void* stream); //!< librw `TexDictionary::streamRead`

//-------------------------------------------------------------------------
// TexDictionary / Texture
//-------------------------------------------------------------------------
void* TexDictionaryCreate();
void* TexDictionaryGetCurrent();
void* TexDictionarySetCurrent(void* txd);
void  TexDictionaryAddTexture(void* txd, void* texture);
void* TexDictionaryFindNamedTexture(void* txd, const char* name);
void  TexDictionaryDestroy(void* txd);
void  TextureDestroy(void* texture);
int   TexDictionaryCount(void* txd);
void* TextureRead(const char* name, const char* maskName);
void* TextureCreate(void* raster);
void  TextureSetRaster(void* texture, void* raster);
void  TextureSetName(void* texture, const char* name);
void  TextureSetMaskName(void* texture, const char* name);
void  TexDictionaryRemoveTexture(void* texture);
void  TextureSetFilterAddressing(void* texture, unsigned filterAddressing);

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

//-------------------------------------------------------------------------
// RpWorld layer: clumps / atomics / geometry / materials.
// `RpClump*`, `RpAtomic*`, `RpGeometry*`, `RpMaterial*` in the standalone build ARE librw's
// `rw::Clump`/`Atomic`/`Geometry`/`Material` (opaque void* here). Their layouts differ from SA's RW
// 3.7 (MEMORY.md §10), so every field access must come through these - see the `#undef` blocks in
// `rw/rpworld.h`. Pointers handed to game callbacks are the librw ones, so the game can pass them
// straight back in.
//-------------------------------------------------------------------------

/*! @brief `RpClumpStreamRead` - read a whole clump (frames, geometries, atomics, lights, cameras). */
void* ClumpStreamRead(void* stream);
void* ClumpCreate();
bool  ClumpDestroy(void* clump);
/*! @brief `RpClumpForAllAtomics`. `cb` is an `RpAtomicCallBack` (`void*(*)(void*,void*)`). */
void* ClumpForAllAtomics(void* clump, void* cb, void* data);
/*! @brief The first atomic of a clump, or nullptr (SA: `GetFirstAtomic`). */
void* ClumpGetFirstAtomic(void* clump);
int   ClumpCountAtomics(const void* clump);
void* ClumpGetFrame(const void* clump);
void  ClumpSetFrame(void* clump, void* frame);

void* AtomicClone(void* atomic);
void  AtomicDestroy(void* atomic);
void* AtomicCreate();
void* AtomicGetGeometry(const void* atomic);
void* AtomicSetGeometry(void* atomic, void* geometry, unsigned flags);
void* AtomicSetFrame(void* atomic, void* frame);
void* AtomicGetFrame(const void* atomic);
void* AtomicGetClump(const void* atomic);
/*! @brief `RpAtomicGetBoundingSphere` - librw's `Sphere` has the same layout as SA's `RwSphere`. */
void* AtomicGetBoundingSphere(const void* atomic);
unsigned AtomicGetFlags(const void* atomic);
void   AtomicSetFlags(void* atomic, unsigned flags);

/*! @brief `RpGeometryGetFlags` as an **lvalue** - SA code does `RpGeometryGetFlags(g) &= ~mask`. */
unsigned& GeometryFlagsRef(void* geometry);
void* GeometryLock(void* geometry, int lockMode);
void* GeometryUnlock(void* geometry);
int   GeometryGetNumVertices(const void* geometry);
int   GeometryGetNumTriangles(const void* geometry);
int   GeometryGetNumMaterials(const void* geometry);
/*! @brief `RpGeometryGetPreLightColors` - librw calls these `colors`. */
void* GeometryGetPreLitColors(void* geometry);
void* GeometryGetVertices(void* geometry);
void* GeometryGetNormals(void* geometry);
void* GeometryGetTexCoords(void* geometry, int uvIndex);
void* GeometryGetTriangles(void* geometry);
void* GeometryGetMorphTarget(void* geometry, int index);
void* GeometryGetMaterial(void* geometry, int num);
/*! @brief `RpGeometryGetNumMorphTargets` (SA reads the field directly). */
int   GeometryGetNumMorphTargets(const void* geometry);
/*! @brief `RpGeometryGetNumTexCoordSets` (SA reads the field directly). */
int   GeometryGetNumTexCoordSets(const void* geometry);
/*! @brief `RpMorphTargetGetVertices` / `RpMorphTargetGetVertexNormals`. */
void* MorphTargetGetVertices(void* morphTarget);
void* MorphTargetGetNormals(void* morphTarget);
/*! @brief `RpGeometryForAllMaterials`. `cb` is an `RpMaterialCallBack` (`void*(*)(void*,void*)`). */
void* GeometryForAllMaterials(void* geometry, void* cb, void* data);

void* MaterialSetTexture(void* material, void* texture);
void* MaterialGetTexture(const void* material);
/*! @brief `RpMaterialSetColor`; `rgba` points to 4 bytes (r,g,b,a). */
void* MaterialSetColor(void* material, const unsigned char* rgba);
void* MaterialGetColor(const void* material);

/*! @brief librw raster format/depth/flags of a raster, for debugging (0s if null). */
void RasterGetInfo(const void* raster, unsigned* format, int* depth, unsigned* flags);

/*! @brief The underlying D3D texture/surface handle of a raster (nullptr if none was created). */
void* RasterGetNativeTexture(const void* raster);
} // namespace rwbridge
