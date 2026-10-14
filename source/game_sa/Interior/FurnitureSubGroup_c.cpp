#include "StdInc.h"
#include "FurnitureSubGroup_c.h"

#ifdef NOTSA_STANDALONE
#include "FurnitureManager_c.h"
#include "Furniture_c.h"
#include "ModelInfo.h"
#include "Collision/ColModel.h"

// The allocation cursor for the global furniture pool. Declared at file scope here for the same reason
// FurnitureManager_c.cpp already declares its own copy: the class member is private, and both are just
// references into the same fixed address, so there is no duplicated state. NOTE the original touches only
// the LOW WORD of 0xBAB378 (`mov cx, word ptr [0xBAB378]`), because 0xBAB37A is a separate 16-bit slot -
// hence the word view rather than the uint32, to avoid clobbering the neighbour.
inline auto& g_currFurnitureIdRaw = StaticRef<uint32>(0xBAB378);
auto& g_currFurnitureIdWord       = *reinterpret_cast<uint16*>(&g_currFurnitureIdRaw);

namespace {
/*!
 * @brief The original's width/height quantisation: `i = floorf(v); if (v - i >= 0.02f) ++i`.
 *
 * @details Transcribed from disasm 0x5C0190..0x5C01B2. The helper at 0x821B40 is a floorf built out of
 *          `fistp qword` (round-to-nearest) plus a sign-aware adjustment (`add ecx,0x7fffffff; sbb eax,0`
 *          for v >= 0), so the value it returns is floor(v), and the comparison constant at 0x858B38 is
 *          0.02f (measured, not assumed - the other constant used in this function, 0x858B8C, is 0.5f).
 *          Net effect is a ceiling with a 0.02 tolerance: a footprint that is within 0.02 below the next
 *          whole cell is NOT rounded up, which is what stops floating error from costing an extra tile.
 */
int8 CeilWithEpsilon(float v) {
    const int32 i = static_cast<int32>(std::floor(v));
    return static_cast<int8>(v - static_cast<float>(i) >= 0.02f ? i + 1 : i);
}
} // namespace
#endif

void FurnitureSubGroup_c::InjectHooks() {
    RH_ScopedClass(FurnitureSubGroup_c);
    RH_ScopedCategory("Interior");

    RH_ScopedInstall(GetFurniture, 0x590EE0, { .Reversed = false });
    RH_ScopedInstall(GetRandomId, 0x590FD0, { .Reversed = false });
    RH_ScopedInstall(AddFurniture, 0x5C00C0, { .Reversed = false });
}

// 0x5C00C0
bool FurnitureSubGroup_c::AddFurniture(uint16 modelId, int16 id, uint8 wealthMin, uint8 wealthMax, uint8 maxAng) {
#ifdef NOTSA_STANDALONE
    // Reversed from disasm 0x5C00C0..0x5C021A (~350 bytes). Layout cross-checked against the headers:
    // Furniture_c derives from ListItem_c (8 bytes) so m_nModelId is 0x8 / m_nId 0xA / widths 0xC,0xD /
    // wealth 0xE,0xF / the three inherited flags 0x10,0x11,0x12 / m_nMaxAng 0x13, size 0x14 - which is
    // exactly the stride the original computes (`lea esi,[eax+eax*4]` then `lea esi,[esi*4+0xBAE1F8]`).
    if (g_currFurnitureIdWord >= std::size(g_furniturePool)) { // 0x5C00CD: cmp cx, 0x100
        return false;                                          // 0x5C021A: xor al,al; ret 0x14
    }
    const auto furniture  = &g_furniturePool[g_currFurnitureIdWord];
    g_currFurnitureIdWord++;

    furniture->m_nModelId = static_cast<int16>(modelId); // [esi+0x8]
    furniture->m_nId      = id;                          // [esi+0xA]

    // Extents come from the COLLISION model's bounding box (CColModel::m_boundBox: m_vecMin at +0x00,
    // m_vecMax at +0x0C), reached via CBaseModelInfo::m_pColModel at +0x14. The original does not
    // null-check the model info or the col model, so neither does this - a bad modelId would have faulted
    // there too, and silently returning false here would diverge from it.
    const auto& box = CModelInfo::GetModelInfo(modelId)->m_pColModel->m_boundBox;
    float extX, extY;
    if (m_bCanSteal) { // [this+0x1A]
        extX = box.m_vecMax.x - box.m_vecMin.x;
        extY = box.m_vecMax.y - box.m_vecMin.y;
    } else {
        extX = box.m_vecMax.x + 0.5f; // 0x858B8C
        extY = box.m_vecMax.y + 0.5f;
    }
    furniture->m_nWidthX = CeilWithEpsilon(extX); // [esi+0xC]
    furniture->m_nWidthY = CeilWithEpsilon(extY); // [esi+0xD]

    furniture->m_nWealthMin                 = static_cast<int8>(wealthMin); // [esi+0xE]
    furniture->m_nWealthMax                 = static_cast<int8>(wealthMax); // [esi+0xF]
    furniture->m_bCanPlaceInFrontOfWindow   = m_bCanPlaceInFrontOfWindow;   // [esi+0x10] <- [this+0x18]
    furniture->m_bIsTall                    = m_bIsTall;                    // [esi+0x11] <- [this+0x19]
    furniture->m_bCanSteal                  = m_bCanSteal;                  // [esi+0x12] <- [this+0x1A]
    furniture->m_nMaxAng                    = static_cast<int8>(maxAng);    // [esi+0x13]

    m_Furnitures.AddItem(furniture); // 0x4A8DF0 (TList_c<Furniture_c>::AddItem), ecx = this+0xC
    return true;
#else
    return plugin::CallMethodAndReturn<bool, 0x5C00C0, FurnitureSubGroup_c*, uint16, int16, uint8, uint8, uint8>(this, modelId, id, wealthMin, wealthMax, maxAng);
#endif
}

// 0x590EE0
Furniture_c* FurnitureSubGroup_c::GetFurniture(int16 id, uint8 wealth) {
    return plugin::CallMethodAndReturn<Furniture_c*, 0x590EE0, FurnitureSubGroup_c*, int16, uint8>(this, id, wealth);
}

// 0x590FD0
int32 FurnitureSubGroup_c::GetRandomId(uint8 a2) {
    return plugin::CallMethodAndReturn<int32, 0x590FD0, FurnitureSubGroup_c*, uint8>(this, a2);
}

void FurnitureSubGroup_c::Exit() {
    m_Furnitures.RemoveAll();
}
