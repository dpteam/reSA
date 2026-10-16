#include "StdInc.h"

#include "rwplcore.h"

#ifdef NOTSA_STANDALONE
#include "RwBridge.h" // librw-backed RenderWare (standalone build)
#endif

RwMemoryFunctions* RwOsGetMemoryInterface() {
    return ((RwMemoryFunctions*(__cdecl *)(void))0x802230)();
}

RwFreeList* _rwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment, RwUInt32 hint, const RwChar* fileCreate, RwUInt32 lineCreate) {
    return ((RwFreeList*(__cdecl *)(RwInt32, RwInt32, RwInt32, RwUInt32, const RwChar*, RwUInt32))0x801980)(entrySize, entriesPerBlock, alignment, hint, fileCreate, lineCreate);
}

RwFreeList* RwFreeListCreateAndPreallocateSpace(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment, RwInt32 numBlocksToPreallocate, RwFreeList* inPlaceSpaceForFreeListStruct, RwUInt32 hint) {
    return ((RwFreeList*(__cdecl *)(RwInt32, RwInt32, RwInt32, RwInt32, RwFreeList*, RwUInt32))0x801B70)(entrySize, entriesPerBlock, alignment, numBlocksToPreallocate, inPlaceSpaceForFreeListStruct, hint);
}

RwBool RwFreeListDestroy(RwFreeList* freelist) {
    return ((RwBool(__cdecl *)(RwFreeList*))0x801B80)(freelist);
}

void RwFreeListSetFlags(RwFreeList* freeList, RwUInt32 flags) {
    ((void(__cdecl *)(RwFreeList*, RwUInt32))0x801C10)(freeList, flags);
}

RwUInt32 RwFreeListGetFlags(RwFreeList* freeList) {
    return ((RwUInt32(__cdecl *)(RwFreeList*))0x801C20)(freeList);
}

RwInt32 RwFreeListPurge(RwFreeList* freelist) {
    return ((RwInt32(__cdecl *)(RwFreeList*))0x801E00)(freelist);
}

RwFreeList* RwFreeListForAllUsed(RwFreeList* freelist, RwFreeListCallBack fpCallBack, void* data) {
    return ((RwFreeList*(__cdecl *)(RwFreeList*, RwFreeListCallBack, void*))0x801E90)(freelist, fpCallBack, data);
}

RwInt32 RwFreeListPurgeAllFreeLists() {
    return ((RwInt32(__cdecl *)(void))0x801F90)();
}

void RwStreamSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    ((void(__cdecl *)(RwInt32, RwInt32))0x7EC760)(blockSize, numBlocksToPrealloc);
}

RwStream* _rwStreamInitialize(RwStream* stream, RwBool rwOwned, RwStreamType type, RwStreamAccessType accessType, const void* data) {
#ifdef NOTSA_STANDALONE
    // Standalone: librw's Stream is a polymorphic class with a different layout, so the caller's SA-sized
    // buffer (`stream`, always the `gRwStream` global) cannot be initialised in place. Re-open librw's
    // cached memory stream instead and hand that back - every call site uses the returned pointer.
    (void)stream;
    (void)rwOwned;
    (void)accessType;
    if (type != rwSTREAMMEMORY || !data) {
        return nullptr; // only readable memory streams are used this way by the game
    }
    // Mirrors `tRwStreamInitializeData` (Streaming.h:141, VALIDATE_SIZE 0x8) - that header is not
    // available in this TU, and the layout is confirmed by disasm of 0x7EC810.
    struct RwMemStreamInitData { uint8* buffer; uint32 size; };
    const auto* mem = static_cast<const RwMemStreamInitData*>(data);
    return (RwStream*)rwbridge::StreamInitializeMemory(mem->buffer, mem->size);
#else
    return ((RwStream*(__cdecl *)(RwStream*, RwBool, RwStreamType, RwStreamAccessType, const void*))0x7EC810)(stream, rwOwned, type, accessType, data);
#endif
}

RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void* data) {
#ifdef NOTSA_STANDALONE
    switch (type) {
    case rwSTREAMFILENAME:
        return (RwStream*)rwbridge::StreamOpenFile((const char*)data);
    case rwSTREAMMEMORY: {
        const auto* mem = (const RwStreamMemory*)data;
        return (RwStream*)rwbridge::StreamOpenMemory(mem->memBlock, mem->nSize);
    }
    default:
        return nullptr; // TODO: custom streams
    }
#else
    return ((RwStream*(__cdecl *)(RwStreamType, RwStreamAccessType, const void*))0x7ECEF0)(type, accessType, data);
#endif
}

RwBool RwStreamClose(RwStream* stream, void* data) {
#ifdef NOTSA_STANDALONE
    rwbridge::StreamClose(stream);
    return true;
#else
    return ((RwBool(__cdecl *)(RwStream*, void*))0x7ECE20)(stream, data);
#endif
}

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length) {
#ifdef NOTSA_STANDALONE
    return rwbridge::StreamRead(stream, buffer, length);
#else
    return ((RwUInt32(__cdecl *)(RwStream*, void*, RwUInt32))0x7EC9D0)(stream, buffer, length);
#endif
}

RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length) {
    return ((RwStream*(__cdecl *)(RwStream*, const void*, RwUInt32))0x7ECB30)(stream, buffer, length);
}

RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset) {
#ifdef NOTSA_STANDALONE
    rwbridge::StreamSkip(stream, offset);
    return stream;
#else
    return ((RwStream*(__cdecl *)(RwStream*, RwUInt32))0x7ECD00)(stream, offset);
#endif
}

RwBool _rwStringOpen() {
    return ((RwBool(__cdecl *)(void))0x80A240)();
}

void _rwStringClose() {
    ((void(__cdecl *)(void))0x80A440)();
}

RwBool _rwStringDestroy(RwChar* string) {
    return ((RwBool(__cdecl *)(RwChar*))0x80A450)(string);
}

RwUInt32 _rwStringStreamGetSize(const RwChar* string) {
    return ((RwUInt32(__cdecl *)(const RwChar*))0x80A470)(string);
}

const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream) {
    return ((const RwChar*(__cdecl *)(const RwChar*, RwStream*))0x80A4A0)(string, stream);
}

RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream) {
    return ((RwChar*(__cdecl *)(RwChar*, RwStream*))0x80A510)(string, stream);
}

void RwPluginRegistrySetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    ((void(__cdecl *)(RwInt32, RwInt32))0x8087B0)(blockSize, numBlocksToPrealloc);
}

RwBool _rwPluginRegistrySetStaticPluginsSize(RwPluginRegistry* reg, RwInt32 size) {
    return ((RwBool(__cdecl *)(RwPluginRegistry*, RwInt32))0x808430)(reg, size);
}

RwInt32 _rwPluginRegistryAddPlugin(RwPluginRegistry* reg, RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor constructCB, RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB) {
    return ((RwInt32(__cdecl *)(RwPluginRegistry*, RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor, RwPluginObjectCopy))0x8084A0)(reg, size, pluginID, constructCB, destructCB, copyCB);
}

RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* reg, RwUInt32 pluginID) {
    return ((RwInt32(__cdecl *)(const RwPluginRegistry*, RwUInt32))0x808470)(reg, pluginID);
}

const RwPluginRegistry* _rwPluginRegistryInitObject(const RwPluginRegistry* reg, void* object) {
    return ((const RwPluginRegistry*(__cdecl *)(const RwPluginRegistry*, void*))0x8086E0)(reg, object);
}

const RwPluginRegistry* _rwPluginRegistryDeInitObject(const RwPluginRegistry* reg, void* object) {
    return ((const RwPluginRegistry*(__cdecl *)(const RwPluginRegistry*, void*))0x808740)(reg, object);
}

const RwPluginRegistry* _rwPluginRegistryCopyObject(const RwPluginRegistry* reg, void* dstObject, const void* srcObject) {
    return ((const RwPluginRegistry*(__cdecl *)(const RwPluginRegistry*, void*, const void*))0x808770)(reg, dstObject, srcObject);
}

RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB, RwPluginDataChunkWriteCallBack writeCB, RwPluginDataChunkGetSizeCallBack getSizeCB) {
    return ((RwInt32(__cdecl *)(RwPluginRegistry*, RwUInt32, RwPluginDataChunkReadCallBack, RwPluginDataChunkWriteCallBack, RwPluginDataChunkGetSizeCallBack))0x8088E0)(reg, pluginID, readCB, writeCB, getSizeCB);
}

RwInt32 _rwPluginRegistryAddPlgnStrmlwysCB(RwPluginRegistry* reg, RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB) {
    return ((RwInt32(__cdecl *)(RwPluginRegistry*, RwUInt32, RwPluginDataChunkAlwaysCallBack))0x808920)(reg, pluginID, alwaysCB);
}

RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(RwPluginRegistry* reg, RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB) {
    return ((RwInt32(__cdecl *)(RwPluginRegistry*, RwUInt32, RwPluginDataChunkRightsCallBack))0x808950)(reg, pluginID, rightsCB);
}

const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg, RwStream* stream, void* object) {
    return ((const RwPluginRegistry*(__cdecl *)(const RwPluginRegistry*, RwStream*, void*))0x808980)(reg, stream, object);
}

const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(const RwPluginRegistry* reg, RwStream* stream, const void* object) {
    return ((const RwPluginRegistry*(__cdecl *)(const RwPluginRegistry*, RwStream*, const void*))0x808B40)(reg, stream, object);
}

const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(const RwPluginRegistry* reg, RwStream* stream) {
    return ((const RwPluginRegistry*(__cdecl *)(const RwPluginRegistry*, RwStream*))0x808C10)(reg, stream);
}

RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry* reg, const void* object) {
    return ((RwInt32(__cdecl *)(const RwPluginRegistry*, const void*))0x808B00)(reg, object);
}

const RwPluginRegistry* _rwPluginRegistryInvokeRights(const RwPluginRegistry* reg, RwUInt32 id, void* obj, RwUInt32 extraData) {
    return ((const RwPluginRegistry*(__cdecl *)(const RwPluginRegistry*, RwUInt32, void*, RwUInt32))0x808AB0)(reg, id, obj, extraData);
}

RwBool RwEngineGetMatrixTolerances(RwMatrixTolerance * const tolerance) {
    return ((RwBool(__cdecl *)(RwMatrixTolerance * const))0x7F1780)(tolerance);
}

RwBool RwEngineSetMatrixTolerances(const RwMatrixTolerance * const tolerance) {
    return ((RwBool(__cdecl *)(const RwMatrixTolerance*))0x7F17B0)(tolerance);
}

void RwMatrixSetFreeListCreateParams(RwInt32 blockSize, RwInt32 numBlocksToPrealloc) {
    ((void(__cdecl *)(RwInt32, RwInt32))0x7F16A0)(blockSize, numBlocksToPrealloc);
}

RwBool RwMatrixDestroy(RwMatrix* mpMat) {

#ifdef NOTSA_STANDALONE
    rwbridge::MatrixDestroy(mpMat);
    return true;
#else
    return ((RwBool(__cdecl *)(RwMatrix*))0x7F2A20)(mpMat);
#endif
}

RwMatrix* RwMatrixCreate() {
#ifdef NOTSA_STANDALONE
    return (RwMatrix*)rwbridge::MatrixCreate();
#else
    return ((RwMatrix*(__cdecl *)(void))0x7F2A50)();
#endif
}

RwMatrix* RwMatrixMultiply(RwMatrix* matrixOut, const RwMatrix* MatrixIn1, const RwMatrix* matrixIn2) {

#ifdef NOTSA_STANDALONE
    return (RwMatrix*)rwbridge::MatrixMultiply(matrixOut, MatrixIn1, matrixIn2);
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwMatrix*, const RwMatrix*))0x7F18B0)(matrixOut, MatrixIn1, matrixIn2);
#endif
}

RwMatrix* RwMatrixTransform(RwMatrix* matrix, const RwMatrix* transform, RwOpCombineType combineOp) {

#ifdef NOTSA_STANDALONE
    return (RwMatrix*)rwbridge::MatrixTransform(matrix, transform, combineOp);
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwMatrix*, RwOpCombineType))0x7F25A0)(matrix, transform, combineOp);
#endif
}

RwMatrix* RwMatrixOrthoNormalize(RwMatrix* matrixOut, const RwMatrix* matrixIn) {
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwMatrix*))0x7F1920)(matrixOut, matrixIn);
}

RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn) {

#ifdef NOTSA_STANDALONE
    return (RwMatrix*)rwbridge::MatrixInvert(matrixOut, matrixIn);
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwMatrix*))0x7F2070)(matrixOut, matrixIn);
#endif
}

RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale, RwOpCombineType combineOp) {

#ifdef NOTSA_STANDALONE
    return (RwMatrix*)rwbridge::MatrixScale(matrix, scale, combineOp);
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwV3d*, RwOpCombineType))0x7F22C0)(matrix, scale, combineOp);
#endif
}

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, RwOpCombineType combineOp) {

#ifdef NOTSA_STANDALONE
    return (RwMatrix*)rwbridge::MatrixTranslate(matrix, translation, combineOp);
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwV3d*, RwOpCombineType))0x7F2450)(matrix, translation, combineOp);
#endif
}

RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle, RwOpCombineType combineOp) {

#ifdef NOTSA_STANDALONE
    return (RwMatrix*)rwbridge::MatrixRotate(matrix, axis, angle, combineOp);
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwV3d*, RwReal, RwOpCombineType))0x7F1FD0)(matrix, axis, angle, combineOp);
#endif
}

RwMatrix* RwMatrixRotateOneMinusCosineSine(RwMatrix* matrix, const RwV3d* unitAxis, RwReal oneMinusCosine, RwReal sine, RwOpCombineType combineOp) {
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwV3d*, RwReal, RwReal, RwOpCombineType))0x7F1D00)(matrix, unitAxis, oneMinusCosine, sine, combineOp);
}

const RwMatrix* RwMatrixQueryRotate(const RwMatrix* matrix, RwV3d* unitAxis, RwReal* angle, RwV3d* center) {
    return ((const RwMatrix*(__cdecl *)(const RwMatrix*, RwV3d*, RwReal*, RwV3d*))0x7F2720)(matrix, unitAxis, angle, center);
}

RwMatrix* RwMatrixUpdate(RwMatrix* matrix) {
#ifdef NOTSA_STANDALONE
    // Disasm 0x7F18A0 is two instructions, and RwMatrix's layout is identical in librw (MEMORY.md §10),
    // so no bridge is needed - this is pure bit-twiddling on a struct we share:
    //     mov eax,[esp+4] ; and dword [eax+0xC],0xFFFDFFFC ; ret      (eax doubles as the return value)
    // i.e. clear the matrix *type* bits (librw Matrix::TYPEMASK = 0x3) and the IDENTITY flag
    // (rwMATRIXINTERNALIDENTITY / librw Matrix::IDENTITY = 0x20000), marking it as "not derived yet".
    matrix->flags &= ~(RwUInt32)(0x3 | 0x20000);
    return matrix;
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*))0x7F18A0)(matrix);
#endif
}

RwMatrix* RwMatrixOptimize(RwMatrix* matrix, const RwMatrixTolerance* tolerance) {

#ifdef NOTSA_STANDALONE
    rwbridge::MatrixOptimize(matrix); // librw applies its own tolerances
    return matrix;
#else
    return ((RwMatrix*(__cdecl *)(RwMatrix*, const RwMatrixTolerance*))0x7F17E0)(matrix, tolerance);
#endif
}

RwReal _rwMatrixDeterminant(const RwMatrix* matrix) {
    return ((RwReal(__cdecl *)(const RwMatrix*))0x7F1450)(matrix);
}

RwReal _rwMatrixNormalError(const RwMatrix* matrix) {
    return ((RwReal(__cdecl *)(const RwMatrix*))0x7F1500)(matrix);
}

RwReal _rwMatrixOrthogonalError(const RwMatrix* matrix) {
    return ((RwReal(__cdecl *)(const RwMatrix*))0x7F14A0)(matrix);
}

RwReal _rwMatrixIdentityError(const RwMatrix* matrix) {
    return ((RwReal(__cdecl *)(const RwMatrix*))0x7F1590)(matrix);
}

RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in) {
    return ((RwReal(__cdecl *)(RwV3d*, const RwV3d*))0x7ED9B0)(out, in);
}

RwReal RwV3dLength(const RwV3d* in) {
    return ((RwReal(__cdecl *)(const RwV3d*))0x7EDAC0)(in);
}

RwReal RwV2dLength(const RwV2d* in) {
    return ((RwReal(__cdecl *)(const RwV2d*))0x7EDBF0)(in);
}

RwReal RwV2dNormalize(RwV2d* out, const RwV2d* in) {
    return ((RwReal(__cdecl *)(RwV2d*, const RwV2d*))0x7EDC60)(out, in);
}

RwV3d* RwV3dTransformPoint(RwV3d* pointOut, const RwV3d* pointIn, const RwMatrix* matrix) {

#ifdef NOTSA_STANDALONE
    rwbridge::V3dTransformPoints(pointOut, pointIn, 1, matrix);
    return pointOut;
#else
    return ((RwV3d*(__cdecl *)(RwV3d*, const RwV3d*, const RwMatrix*))0x7EDD60)(pointOut, pointIn, matrix);
#endif
}

RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints, const RwMatrix* matrix) {

#ifdef NOTSA_STANDALONE
    rwbridge::V3dTransformPoints(pointsOut, pointsIn, numPoints, matrix);
    return pointsOut;
#else
    return ((RwV3d*(__cdecl *)(RwV3d*, const RwV3d*, RwInt32, const RwMatrix*))0x7EDD90)(pointsOut, pointsIn, numPoints, matrix);
#endif
}

RwV3d* RwV3dTransformVector(RwV3d* vectorOut, const RwV3d* vectorIn, const RwMatrix* matrix) {

#ifdef NOTSA_STANDALONE
    rwbridge::V3dTransformVectors(vectorOut, vectorIn, 1, matrix);
    return vectorOut;
#else
    return ((RwV3d*(__cdecl *)(RwV3d*, const RwV3d*, const RwMatrix*))0x7EDDC0)(vectorOut, vectorIn, matrix);
#endif
}

RwV3d* RwV3dTransformVectors(RwV3d* vectorsOut, const RwV3d* vectorsIn, RwInt32 numPoints, const RwMatrix* matrix) {

#ifdef NOTSA_STANDALONE
    rwbridge::V3dTransformVectors(vectorsOut, vectorsIn, numPoints, matrix);
    return vectorsOut;
#else
    return ((RwV3d*(__cdecl *)(RwV3d*, const RwV3d*, RwInt32, const RwMatrix*))0x7EDDF0)(vectorsOut, vectorsIn, numPoints, matrix);
#endif
}

RwReal _rwSqrt(const RwReal num) {
    return ((RwReal(__cdecl *)(const RwReal))0x7EDB30)(num);
}

RwReal _rwInvSqrt(const RwReal num) {
    return ((RwReal(__cdecl *)(const RwReal))0x7EDB90)(num);
}

RwReal _rwV3dNormalize(RwV3d* out, const RwV3d* in) {
    return ((RwReal(__cdecl *)(RwV3d*, const RwV3d*))0x7ED910)(out, in);
}

RwSList* _rwSListCreate(RwInt32 size, RwUInt32 hint) {
    return ((RwSList*(__cdecl *)(RwInt32, RwUInt32))0x809160)(size, hint);
}

RwBool _rwSListDestroy(RwSList* sList) {
    return ((RwBool(__cdecl *)(RwSList*))0x809440)(sList);
}

void _rwSListDestroyEndEntries(RwSList* sList, RwInt32 amount) {
    ((void(__cdecl *)(RwSList*, RwInt32))0x809400)(sList, amount);
}

void* _rwSListGetEntry(RwSList* sList, RwInt32 entry) {
    return ((void*(__cdecl *)(RwSList*, RwInt32))0x809510)(sList, entry);
}

void* _rwSListGetNewEntry(RwSList* sList, RwUInt32 hint) {
    return ((void*(__cdecl *)(RwSList*, RwUInt32))0x809240)(sList, hint);
}

RwInt32 _rwSListGetNumEntries(const RwSList* sList) {
    return ((RwInt32(__cdecl *)(const RwSList*))0x8094B0)(sList);
}

void* _rwSListGetBegin(RwSList* sList) {
    return ((void*(__cdecl *)(RwSList*))0x809530)(sList);
}

void* _rwSListGetEnd(RwSList* sList) {
    return ((void*(__cdecl *)(RwSList*))0x809540)(sList);
}

#ifndef NOTSA_STANDALONE

RwUInt32 RwEngineGetVersion() {
    return ((RwUInt32(__cdecl *)(void))0x7F2BA0)();
}

RwBool RwEngineInit(const RwMemoryFunctions* memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize) {
    return ((RwBool(__cdecl *)(const RwMemoryFunctions*, RwUInt32, RwUInt32))0x7F3170)(memFuncs, initFlags, resArenaSize);
}

RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor initCB, RwPluginObjectDestructor termCB) {
    return ((RwInt32(__cdecl *)(RwInt32, RwUInt32, RwPluginObjectConstructor, RwPluginObjectDestructor))0x7F2BB0)(size, pluginID, initCB, termCB);
}

RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID) {
    return ((RwInt32(__cdecl *)(RwUInt32))0x7F2BE0)(pluginID);
}

RwBool RwEngineOpen(RwEngineOpenParams* initParams) {
    return ((RwBool(__cdecl *)(RwEngineOpenParams*))0x7F2F70)(initParams);
}

RwBool RwEngineStart() {
    return ((RwBool(__cdecl *)(void))0x7F2E70)();
}

RwBool RwEngineStop() {
    return ((RwBool(__cdecl *)(void))0x7F2E20)();
}

RwBool RwEngineClose() {
    return ((RwBool(__cdecl *)(void))0x7F2F00)();
}

RwBool RwEngineTerm() {
    return ((RwBool(__cdecl *)(void))0x7F3130)();
}

RwInt32 RwEngineGetNumSubSystems() {
    return ((RwInt32(__cdecl *)(void))0x7F2C00)();
}

RwSubSystemInfo* RwEngineGetSubSystemInfo(RwSubSystemInfo* subSystemInfo, RwInt32 subSystemIndex) {
    return ((RwSubSystemInfo*(__cdecl *)(RwSubSystemInfo*, RwInt32))0x7F2C30)(subSystemInfo, subSystemIndex);
}

RwInt32 RwEngineGetCurrentSubSystem() {
    return ((RwInt32(__cdecl *)(void))0x7F2C60)();
}

RwBool RwEngineSetSubSystem(RwInt32 subSystemIndex) {
    return ((RwBool(__cdecl *)(RwInt32))0x7F2C90)(subSystemIndex);
}

RwInt32 RwEngineGetNumVideoModes() {
    return ((RwInt32(__cdecl *)(void))0x7F2CC0)();
}

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* vmi, RwInt32 modeIndex) {
    return plugin::CallAndReturn<RwVideoMode*, 0x7F2CF0>(vmi, modeIndex);
}

RwInt32 RwEngineGetCurrentVideoMode() {
    return ((RwInt32(__cdecl *)(void))0x7F2D20)();
}

RwBool RwEngineSetVideoMode(RwInt32 modeIndex) {
    return ((RwBool(__cdecl *)(RwInt32))0x7F2D50)(modeIndex);
}

RwInt32 RwEngineGetTextureMemorySize() {
    return ((RwInt32(__cdecl *)(void))0x7F2D80)();
}

RwInt32 RwEngineGetMaxTextureSize() {
    return ((RwInt32(__cdecl *)(void))0x7F2DB0)();
}

RwBool RwEngineSetFocus(RwBool enable) {
    return ((RwBool(__cdecl *)(RwBool))0x7F2DE0)(enable);
}

RwMetrics* RwEngineGetMetrics() {
    return ((RwMetrics*(__cdecl *)(void))0x7F2E10)();
}

#else // NOTSA_STANDALONE

// Standalone: RenderWare is provided by librw (see RwBridge). The original game's RW lives inside
// gta_sa.exe, which the standalone build doesn't have.

RwUInt32 RwEngineGetVersion() {
    return rwbridge::GetVersion();
}

RwBool RwEngineInit(const RwMemoryFunctions*, RwUInt32, RwUInt32) {
    return rwbridge::EngineInit();
}

RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor initCB, RwPluginObjectDestructor termCB) {
    return rwbridge::RegisterPlugin(size, pluginID, (void*)initCB, (void*)termCB);
}

RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID) {
    return rwbridge::GetPluginOffset(pluginID);
}

RwBool RwEngineOpen(RwEngineOpenParams* initParams) {
    return rwbridge::EngineOpen(initParams ? initParams->displayID : nullptr);
}

RwBool RwEngineStart() { return rwbridge::EngineStart(); }
RwBool RwEngineStop()  { rwbridge::EngineStop();  return true; }
RwBool RwEngineClose() { rwbridge::EngineClose(); return true; }
RwBool RwEngineTerm()  { rwbridge::EngineTerm();  return true; }

RwInt32 RwEngineGetNumSubSystems() { return rwbridge::GetNumSubSystems(); }

RwSubSystemInfo* RwEngineGetSubSystemInfo(RwSubSystemInfo* subSystemInfo, RwInt32 subSystemIndex) {
    if (!subSystemInfo) {
        return nullptr;
    }
    return rwbridge::GetSubSystemInfo(subSystemInfo->name, sizeof(subSystemInfo->name), subSystemIndex)
        ? subSystemInfo
        : nullptr;
}

RwInt32 RwEngineGetCurrentSubSystem() { return rwbridge::GetCurrentSubSystem(); }
RwBool RwEngineSetSubSystem(RwInt32 subSystemIndex) { return rwbridge::SetSubSystem(subSystemIndex); }

RwInt32 RwEngineGetNumVideoModes() { return rwbridge::GetNumVideoModes(); }

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* vmi, RwInt32 modeIndex) {
    if (!vmi) {
        return nullptr;
    }
    unsigned flags{};
    if (!rwbridge::GetVideoModeInfo(&vmi->width, &vmi->height, &vmi->depth, &flags, modeIndex)) {
        return nullptr;
    }
    vmi->flags   = (RwVideoModeFlag)flags;
    vmi->refRate = 0;
    vmi->format  = 0;
    return vmi;
}

RwInt32 RwEngineGetCurrentVideoMode() { return rwbridge::GetCurrentVideoMode(); }
RwBool RwEngineSetVideoMode(RwInt32 modeIndex) { return rwbridge::SetVideoMode(modeIndex); }

RwInt32 RwEngineGetTextureMemorySize() { return 0; }
RwInt32 RwEngineGetMaxTextureSize() { return 0; }
RwBool RwEngineSetFocus(RwBool) { return true; }
RwMetrics* RwEngineGetMetrics() { return nullptr; }

#endif // NOTSA_STANDALONE

//RwFileFunctions* RwOsGetFileInterface() {
//    return ((RwFileFunctions*(__cdecl *)(void))0x804130)();
//}

RwError* RwErrorGet(RwError* code) {
    return ((RwError*(__cdecl *)(RwError*))0x808880)(code);
}

RwError* RwErrorSet(RwError* code) {
    return ((RwError*(__cdecl *)(RwError*))0x808820)(code);
}

RwInt32 _rwerror(RwInt32 code, ...) {
    return code;
}

RwBool RwResourcesSetArenaSize(RwUInt32 size) {
    return ((RwBool(__cdecl *)(RwUInt32))0x8080C0)(size);
}

RwInt32 RwResourcesGetArenaSize() {
    return ((RwInt32(__cdecl *)(void))0x8081B0)();
}

RwInt32 RwResourcesGetArenaUsage() {
    return ((RwInt32(__cdecl *)(void))0x8081C0)();
}

RwBool RwResourcesEmptyArena() {
    return ((RwBool(__cdecl *)(void))0x8081F0)();
}

RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry* ownerRef, RwInt32 size, RwResEntryDestroyNotify destroyNotify) {
    return ((RwResEntry*(__cdecl *)(void*, RwResEntry*, RwInt32, RwResEntryDestroyNotify))0x807ED0)(owner, ownerRef, size, destroyNotify);
}

RwBool RwResourcesFreeResEntry(RwResEntry* entry) {
    return ((RwBool(__cdecl *)(RwResEntry*))0x807DE0)(entry);
}

void _rwResourcesPurge() {
    ((void(__cdecl *)(void))0x807E50)();
}

RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut) {
#ifdef NOTSA_STANDALONE
    return rwbridge::StreamFindChunk(stream, type, lengthOut, versionOut);
#else
    return ((RwBool(__cdecl *)(RwStream*, RwUInt32, RwUInt32*, RwUInt32*))0x7ED2D0)(stream, type, lengthOut, versionOut);
#endif
}

RwStream* _rwStreamWriteVersionedChunkHeader(RwStream* stream, RwInt32 type, RwInt32 size, RwUInt32 version, RwUInt32 buildNum) {
    return ((RwStream*(__cdecl *)(RwStream*, RwInt32, RwInt32, RwUInt32, RwUInt32))0x7ED270)(stream, type, size, version, buildNum);
}

RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* reals, RwUInt32 numBytes) {
    return ((RwStream*(__cdecl *)(RwStream*, const RwReal*, RwUInt32))0x7ED3D0)(stream, reals, numBytes);
}

RwStream* RwStreamWriteInt32(RwStream* stream, const RwInt32* ints, RwUInt32 numBytes) {
    return ((RwStream*(__cdecl *)(RwStream*, const RwInt32*, RwUInt32))0x7ED460)(stream, ints, numBytes);
}

RwStream* RwStreamWriteInt16(RwStream* stream, const RwInt16* ints, RwUInt32 numBytes) {
    return ((RwStream*(__cdecl *)(RwStream*, const RwInt16*, RwUInt32))0x7ED480)(stream, ints, numBytes);
}

RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals, RwUInt32 numBytes) {
#ifdef NOTSA_STANDALONE
    return rwbridge::StreamReadReal(stream, reals, numBytes) ? stream : nullptr;
#else
    return ((RwStream*(__cdecl *)(RwStream*, RwReal*, RwUInt32))0x7ED4F0)(stream, reals, numBytes);
#endif
}

RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* ints, RwUInt32 numBytes) {
#ifdef NOTSA_STANDALONE
    return rwbridge::StreamReadInt32(stream, ints, numBytes) ? stream : nullptr;
#else
    return ((RwStream*(__cdecl *)(RwStream*, RwInt32*, RwUInt32))0x7ED540)(stream, ints, numBytes);
#endif
}

RwStream* RwStreamReadInt16(RwStream* stream, RwInt16* ints, RwUInt32 numBytes) {
#ifdef NOTSA_STANDALONE
    return rwbridge::StreamReadInt16(stream, ints, numBytes) ? stream : nullptr;
#else
    return ((RwStream*(__cdecl *)(RwStream*, RwInt16*, RwUInt32))0x7ED4A0)(stream, ints, numBytes);
#endif
}

RwStream* RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo) {
#ifdef NOTSA_STANDALONE
    // Fills the 4 header dwords; SA's struct has a 5th field (`isComplex`) that the original sets from an
    // extra call, but it is documented "Internal Use" and nothing in the game reads it, so it is left
    // untouched rather than invented here.
    return rwbridge::StreamReadChunkHeaderInfo(stream, &chunkHeaderInfo->type,
                                               &chunkHeaderInfo->length,
                                               &chunkHeaderInfo->version,
                                               &chunkHeaderInfo->buildNum) ? stream : nullptr;
#else
    return ((RwStream*(__cdecl *)(RwStream*, RwChunkHeaderInfo*))0x7ED590)(stream, chunkHeaderInfo);
#endif
}
