#include "StdInc.h"

#include "ProjectileInfo.h"

#include "Entity/Object/Projectile.h"
#include "Radar.h"
#include "World.h"
#include "Pools/Pools.h"
#include "Collision/Box.h"

void CProjectileInfo::InjectHooks() {
    RH_ScopedClass(CProjectileInfo);
    RH_ScopedCategoryGlobal();

    // Install("CProjectileInfo", "", , &CProjectileInfo::);
    RH_ScopedInstall(Initialise, 0x737B40, { .Reversed = false });
    RH_ScopedInstall(Shutdown, 0x737BC0, { .Reversed = false });
    RH_ScopedInstall(GetProjectileInfo, 0x737BF0, { .Reversed = false });
    RH_ScopedInstall(RemoveNotAdd, 0x737C00, { .Reversed = false });
    RH_ScopedInstall(AddProjectile, 0x737C80, { .Reversed = false });
    RH_ScopedInstall(RemoveDetonatorProjectiles, 0x738860, { .Reversed = false });
    RH_ScopedInstall(RemoveProjectile, 0x7388F0, { .Reversed = false });
    RH_ScopedInstall(Update, 0x738B20, { .Reversed = false });
    RH_ScopedInstall(IsProjectileInRange, 0x739860);
    RH_ScopedInstall(RemoveAllProjectiles, 0x7399B0, { .Reversed = false });
    RH_ScopedInstall(RemoveIfThisIsAProjectile, 0x739A40, { .Reversed = false });
    RH_ScopedInstall(RemoveFXSystem, 0x737B80, { .Reversed = false });
}

// 0x737B40
void CProjectileInfo::Initialise() {
#ifdef NOTSA_STANDALONE
    // Transcribed from disasm 0x737B40 (58 bytes, no calls at all - pure static init):
    //   rep stos 0x20 dwords at 0xC89110          -> all 32 ms_apProjectile entries = nullptr
    //   then 32 iterations (stride 0x24 = sizeof(CProjectileInfo), 0xC891A8..0xC89628):
    //       m_nWeaponType   = 16      (ecx is loaded once with 0x10 and never decremented, so EVERY
    //                                  entry gets 16 == WEAPON_GRENADE - that is what the original
    //                                  stores; it is inert because every reader checks m_bActive
    //                                  first, eg. IsProjectileInRange above the m_nWeaponType use)
    //       m_pCreator      = nullptr
    //       m_nDestroyTime  = 0
    //       m_bActive       = false
    //       m_pFxSystem     = nullptr
    //   NOTE the original does NOT clear m_pVictim (+8) or m_vecLastPosn (+0x14..+0x1B) - reproduced
    //   exactly rather than "tidied up", so the memory state matches the mod build's.
    ms_apProjectile.fill(nullptr);
    for (auto& info : gaProjectileInfo) {
        info.m_nWeaponType   = 16; // == WEAPON_GRENADE, as stored by the original
        info.m_pCreator      = nullptr;
        info.m_nDestroyTime  = 0;
        info.m_bActive       = false;
        info.m_pFxSystem     = nullptr;
    }
    return;
#else
    plugin::Call<0x737B40>();
#endif
}

// 0x737BC0
void CProjectileInfo::Shutdown() {
    plugin::Call<0x737BC0>();
}

// 0x737BF0
CProjectileInfo* CProjectileInfo::GetProjectileInfo(int32 infoId) {
    return plugin::CallAndReturn<CProjectileInfo*, 0x737BF0, int32>(infoId);
}

// 0x737C00
void CProjectileInfo::RemoveNotAdd(CEntity* creator, eWeaponType weaponType, CVector pos) {
    plugin::Call<0x737C00, CEntity*, eWeaponType, CVector>(creator, weaponType, pos);
}

// 0x737C80
bool CProjectileInfo::AddProjectile(CEntity* creator, eWeaponType projectileType, CVector origin, float force, const CVector* dir, CEntity* target) {
    return plugin::CallAndReturn<bool, 0x737C80>(creator, projectileType, origin, force, dir, target);
}

// 0x738860
void CProjectileInfo::RemoveDetonatorProjectiles() {
    plugin::Call<0x738860>();
}

// 0x7388F0
void CProjectileInfo::RemoveProjectile(CProjectileInfo* info, CProjectile* object) {
    plugin::Call<0x7388F0, CProjectileInfo*, CProjectile*>(info, object);
}

// 0x738B20
void CProjectileInfo::Update() {
#if defined(NOTSA_STANDALONE)
    // Standalone: not ported (measured 1056 instructions and 35 calls - a real per-frame job, not a
    // one-step unblock), and a PROVABLE no-op here. The disasm shape, in full:
    //     738b20: if (CReplay::Mode == 1) return;                 // 0xA43088 == CReplay::Mode; the ==1
    //                                                             // path jumps straight to the epilogue
    //     738b37: for (eax = 0; eax < 32; eax++) {                // 0xC89110 = ms_apProjectile (32 ptrs)
    //                 esi = ms_apProjectile[eax];
    //                 ebx = &gaProjectileInfo[eax];               // 0xC891A8 + eax*0x24
    //                 if (!gaProjectileInfo[eax].m_bActive)       // 0xC891B8 == base + 0x10
    //                     goto next;                              // 0x73983A = inc/cmp/jl, i.e. the
    //                                                             // loop increment, so the body is skipped
    //                 ... the entire 1000-instruction body, which dereferences esi ...
    //             }
    // The guard is the ONLY entry into the body, and `CProjectileInfo::Initialise` (reversed earlier in
    // this file) sets m_bActive = false for all 32 - so at boot, and until a weapon actually fires, the
    // loop runs 32 times doing nothing and returns. That also means ms_apProjectile's null entries are
    // never dereferenced, which is what makes the stub safe rather than merely untested.
    //
    // Degradation, stated plainly: once projectiles exist (grenades, rockets, molotovs in flight), their
    // per-frame update does not run, so they will not move, explode or expire. That is gameplay, and the
    // function is self-contained enough to port on its own when weapons work. See IDEAS.md.
    return;
#else
    plugin::Call<0x738B20>();
#endif
}

// 0x739860
bool CProjectileInfo::IsProjectileInRange(float x1, float x2, float y1, float y2, float z1, float z2, bool bDestroy) {
    const CBox bb{
        CVector{ x1, y1, z1 },
        CVector{ x2, y2, z2 }
    };
    bool found = false;
    for (auto&& [info, proj] : rngv::zip(gaProjectileInfo, ms_apProjectile)) {
        if (!info.m_bActive) {
            continue;
        }

        if (!IsWeaponTypeProjectile(static_cast<eWeaponType>(info.m_nWeaponType))) {
            continue;
        }

        if (!bb.IsPointInside(proj->GetPosition())) {
            continue;
        }

        found = true;
        if (bDestroy) {
            info.m_bActive = false;
            info.RemoveFXSystem(false);
            CRadar::ClearBlipForEntity(BLIP_OBJECT, GetObjectPool()->GetRef(proj));
            CWorld::Remove(proj);
            delete proj;
        }
    }
    return found;
}

// 0x7399B0
void CProjectileInfo::RemoveAllProjectiles() {
    plugin::Call<0x7399B0>();
}

// 0x739A40
bool CProjectileInfo::RemoveIfThisIsAProjectile(CObject* object) {
    return plugin::CallAndReturn<bool, 0x739A40, CObject*>(object);
}

// 0x737B80
void CProjectileInfo::RemoveFXSystem(bool bInstantly) {
    plugin::CallMethod<0x737B80, CProjectileInfo*, bool>(this, bInstantly);
}
