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
/*!
 * @brief `_rwStreamInitialize(stream, rwOwned, rwSTREAMMEMORY, rwSTREAMREAD, {buf,size})`.
 *
 * The original initialises the *caller-supplied* buffer in place (disasm 0x7EC810: writes type/accessType/
 * rwOwned into `[stream]`,`[stream+4]`,`[stream+0x20]`, then the memory block at `[stream+0xC]`) and
 * returns it. Every call site passes the same global (`gRwStream`, 0x8E48AC) and re-initialises it, and
 * `rwOwned` is 0 (the data is not the stream's). librw's `Stream` is a polymorphic class with a different
 * layout, so we cannot reuse the SA buffer; instead we keep ONE cached `rw::StreamMemory` and re-open it,
 * which reproduces that exact behaviour (see also `StreamClose`, which must not delete this one).
 * @return The librw stream to use, or nullptr for anything but a readable memory stream.
 */
void* StreamInitializeMemory(const void* data, unsigned size);
void  StreamClose(void* stream);
unsigned StreamRead(void* stream, void* buffer, unsigned length);
/*! @brief `RwStreamReadChunkHeaderInfo` - fills type/length/version/build (SA's extra `isComplex` field
 *  is internal and unread by the game, so it is left alone). @return false on a short read. */
bool StreamReadChunkHeaderInfo(void* stream, unsigned* type, unsigned* length, unsigned* version, unsigned* build);
/*! @brief `RwStreamRead{Int16,Int32,Real}` - disasm 0x7ED4A0/0x7ED4F0/0x7ED540 all just forward
 *  (stream, buffer, numBytes) to the byte reader, so `numBytes` is a BYTE count, not an element count. */
bool StreamReadInt16(void* stream, void* buf, unsigned numBytes);
bool StreamReadInt32(void* stream, void* buf, unsigned numBytes);
bool StreamReadReal(void* stream, void* buf, unsigned numBytes);
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
void* ClumpClone(void* clump);                       //!< `RpClumpClone`
/*! @brief The clump membership mutators (disasm 0x74A490/0x74A4C0/0x74A4F0/0x74A520/0x74A550/0x74A580).
 *  All six just re-link the object's existing list node into/out of the clump's list and return the
 *  clump, so they map 1:1 onto librw's add/remove pairs. Safe to call during a ClumpForAllAtomics walk:
 *  that walk latches the next link before invoking the callback (as the original does), and
 *  `CFileLoader::SetRelatedModelInfoCB` really does remove atomics mid-iteration. */
void* ClumpAddAtomic(void* clump, void* atomic);
void* ClumpRemoveAtomic(void* clump, void* atomic);
void* ClumpAddLight(void* clump, void* light);
void* ClumpRemoveLight(void* clump, void* light);
void* ClumpAddCamera(void* clump, void* camera);
void* ClumpRemoveCamera(void* clump, void* camera);

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

//-------------------------------------------------------------------------
// RwImage / RT file readers (BMP, PNG, TGA).
// librw's `Image` differs from SA's `RwImage` (it inserts `bpp` after `depth`), so these are only safe
// because the game handles images purely through functions - it never touches RwImage fields. The
// raster handed to RasterSetFromImage is one this bridge created via RasterCreate, so both sides agree.
//-------------------------------------------------------------------------
void* ImageReadBMP(const char* path);        //!< `RtBMPImageRead`
void* ImageReadPNG(const char* path);        //!< `RtPNGImageRead`
void* ImageCreate(int width, int height, int depth);  //!< `RwImageCreate`
void  ImageDestroy(void* image);                      //!< `RwImageDestroy`
/*! @brief `RwImageFindRasterFormat`; fills the out-params, returns true on success. */
bool ImageFindRasterFormat(void* image, int rasterType, int* w, int* h, int* depth, unsigned* format);
/*! @brief `RwRasterSetFromImage` - upload an image into an existing (native) raster. */
void RasterSetFromImage(void* raster, void* image);

//-------------------------------------------------------------------------
// RpSkin plugin (geometry skin + atomic hierarchy). `RpSkin*` is an *opaque* type in SA - the game
// only ever goes through these accessors - so librw's `rw::Skin` can stand in for it directly.
// Field correspondence (SA offsets from disasm, librw names from rwplugins.h):
//   numBones 0x00 -> numBones | skinToBoneMatrices 0x0C -> inverseMatrices
//   vertexBoneIndices 0x14 -> indices | vertexBoneWeights 0x18 -> weights
// Note librw stores the bone indices as uint8 where RW's docs say uint32; that is what the game
// actually assumes (see the RwUInt8 cast + TODO in ClothesBuilder.cpp:189).
//-------------------------------------------------------------------------
void* SkinGeometryGetSkin(void* geometry);                 //!< `RpSkinGeometryGetSkin`
void  SkinGeometrySetSkin(void* geometry, void* skin);     //!< `RpSkinGeometrySetSkin`
void* AtomicGetHAnimHierarchy(const void* atomic);         //!< `RpSkinAtomicGetHAnimHierarchy`
void  AtomicSetHAnimHierarchy(void* atomic, void* hier);   //!< `RpSkinAtomicSetHAnimHierarchy`
int   SkinGetNumBones(const void* skin);                   //!< `RpSkinGetNumBones`
void* SkinGetVertexBoneWeights(const void* skin);          //!< `RpSkinGetVertexBoneWeights`
void* SkinGetVertexBoneIndices(const void* skin);          //!< `RpSkinGetVertexBoneIndices`
void* SkinGetSkinToBoneMatrices(const void* skin);         //!< `RpSkinGetSkinToBoneMatrices`

//-------------------------------------------------------------------------
// RpHAnim plugin (bone hierarchies attached to frames/clumps/atomics). `RpHAnimHierarchy` is again an
// opaque type on the SA side, and librw implements the exact same lookup pair, so these are thin:
//   RpHAnimFrameGetHierarchy(f)  <-> HAnimHierarchy::get(f)
//   GetAnimHierarchyFromFrame(f)  = get(f), falling back to find(f)   (disasm 0x734AB0)
//   GetAnimHierarchyFromClump(c)  = the above on c's frame            (disasm 0x734B10)
//-------------------------------------------------------------------------
void* HAnimFrameGetHierarchy(void* frame);                 //!< `RpHAnimFrameGetHierarchy`
bool  HAnimFrameSetHierarchy(void* frame, void* hierarchy);//!< `RpHAnimFrameSetHierarchy`
/*! @brief `GetAnimHierarchyFromFrame` (0x734AB0): the frame's own hierarchy, else search its objects. */
void* HAnimGetHierarchyFromFrame(void* frame);
/*! @brief `RpHAnimHierarchyGetMatrixArray` - librw's `matrices` (16-byte aligned). */
void* HAnimHierarchyGetMatrices(void* hierarchy);
int   HAnimIDGetIndex(void* hierarchy, int id);            //!< `RpHAnimIDGetIndex`
void  HAnimHierarchyUpdateMatrices(void* hierarchy);       //!< `RpHAnimHierarchyUpdateMatrices`

/*! @brief The RW class tag of an `RwObject`/`RpAtomic`/`RwCamera`/`RpLight` (librw `Object::type`).
 *  Values (librw rwobjects.h): Frame 0, Atomic 1, Camera 2, Light 3, Material 4, World 7,
 *  TexDictionary 6, Geometry 8. Used to check a blind `RwObject* -> RpAtomic*` cast is valid. */
int ObjectGetType(const void* object);

/*! @brief `RwFrameForAllChildren` / `RwFrameForAllObjects` (disasm 0x7F0DC0 / 0x7F1200).
 *  Both walk the frame's children (resp. attached objects) and call `cb`; iteration **stops as soon as
 *  the callback returns NULL** (that is RW's convention, and librw's `forAllChildren` matches it).
 *  Both always return the input frame. `cb` is an `RwFrameCallBack`/`RwObjectCallBack`. */
void* FrameForAllChildren(void* frame, void* cb, void* data);
void* FrameForAllObjects(void* frame, void* cb, void* data);

//-------------------------------------------------------------------------
// RW object-plugin registration - the game's *own* plugins (Visibility, TxdStore, Collision, 2dEffect,
// Breakable, NodeName, Pipeline, RpAnimBlend, ...), as opposed to the RW-SDK plugins that
// `RegisterCorePlugins()` already installs.
//
// librw's `PluginList` is RW-compatible: `registerPlugin()` returns the byte offset of the plugin's data
// inside the object allocation, and `RWPLUGINOFFSET(type, obj, off)` means the same thing on both sides.
// So the values the game caches in its `ms_*PluginOffset`/`g*PluginOffset` globals (which live at fixed
// addresses!) work unchanged - they just have to be *registered* before any object is created.
//
// Callbacks are `void*` here because the bridge cannot see SA's `RwPlugin*` typedefs; their real types
// (rwplcore.h) match librw's `Constructor`/`Destructor`/`CopyConstructor`/`StreamRead`/`StreamWrite`/
// `StreamGetSize`/`RightsCallback`/`AlwaysCallback` modulo a `const` on one parameter, so they are passed
// straight through. `RpWorldSector` has no librw counterpart, so it is deliberately absent.
//-------------------------------------------------------------------------
#define NOTSA_PLG_FUNCS(P)                                                             \
    int P##RegisterPlugin(int size, unsigned id, void* ctor, void* dtor, void* copy);   \
    int P##RegisterPluginStream(unsigned id, void* read, void* write, void* getSize);   \
    int P##GetPluginOffset(unsigned id);                                               \
    int P##SetStreamAlwaysCallback(unsigned id, void* cb);                              \
    int P##SetStreamRightsCallback(unsigned id, void* cb)

NOTSA_PLG_FUNCS(Frame);
NOTSA_PLG_FUNCS(Raster);
NOTSA_PLG_FUNCS(Texture);
NOTSA_PLG_FUNCS(TexDictionary);
NOTSA_PLG_FUNCS(Material);
NOTSA_PLG_FUNCS(Geometry);
NOTSA_PLG_FUNCS(Atomic);
NOTSA_PLG_FUNCS(Clump);
NOTSA_PLG_FUNCS(Light);
NOTSA_PLG_FUNCS(Camera);
NOTSA_PLG_FUNCS(World);
#undef NOTSA_PLG_FUNCS

//-------------------------------------------------------------------------
// Matrix / Vector math (rwplcore).
// `RwMatrix`/`RwV3d`/`RwV2d` are layout-identical to librw's (`MEMORY.md` §10), so these are plain
// pointer casts around librw's own routines - no per-field translation, and the mod build is untouched.
// Bridging the whole family at once because Fx, the world/streaming and every renderer path use it.
//-------------------------------------------------------------------------
void  MatrixDestroy(void* matrix);          //!< `RwMatrixDestroy`
void* MatrixMultiply(void* out, const void* src1, const void* src2);  //!< `RwMatrixMultiply` (out = src1*src2)
void* MatrixInvert(void* out, const void* src);                        //!< `RwMatrixInvert`
void* MatrixTranspose(void* out, const void* src);                     //!< `RwMatrixTranspose`
void* MatrixTransform(void* matrix, const void* transform, int combineOp);
void* MatrixRotate(void* matrix, const void* axis, float angle, int combineOp);
void* MatrixTranslate(void* matrix, const void* translation, int combineOp);
void* MatrixScale(void* matrix, const void* scale, int combineOp);
void  MatrixSetIdentity(void* matrix);      //!< `RwMatrixSetIdentity`
void  MatrixOptimize(void* matrix);         //!< `RwMatrixOptimize` (default tolerances)
/*! @brief `RwV3dTransformPoint(s)` / `RwV3dTransformVector(s)` (points include translation). */
void  V3dTransformPoints(void* out, const void* in, int n, const void* matrix);
void  V3dTransformVectors(void* out, const void* in, int n, const void* matrix);

/*! @brief librw raster format/depth/flags of a raster, for debugging (0s if null). */
void RasterGetInfo(const void* raster, unsigned* format, int* depth, unsigned* flags);

/*! @brief The underlying D3D texture/surface handle of a raster (nullptr if none was created). */
void* RasterGetNativeTexture(const void* raster);
} // namespace rwbridge
