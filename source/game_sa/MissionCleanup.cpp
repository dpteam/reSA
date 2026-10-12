/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#include "StdInc.h"

#include "MissionCleanup.h"

CMissionCleanup::CMissionCleanup() {
    Init();
}

/* Initializes data
 * @addr 0x4637A0
 */
void CMissionCleanup::Init() {
    m_Count = 0;
    m_Objects.fill(tMissionCleanupEntity());
}

/* Performs a clean-up
 * @addr 0x468560
 */
void CMissionCleanup::Process() {
    ((void(__thiscall*)(CMissionCleanup*))0x468560)(this);
}

/* Finds a free entity, returns NULL if no free entity can be found.
 * @addr 0x4637C0
 */
tMissionCleanupEntity* CMissionCleanup::FindFree() {
    return ((tMissionCleanupEntity * (__thiscall*)(CMissionCleanup*))0x4637C0)(this);
}

/* Adds entity to list
 * @addr 0x4637E0
 */
void CMissionCleanup::AddEntityToList(int32 handle, MissionCleanUpEntityType type) {
    for (auto& entity : m_Objects) {
        if (entity.type == MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_EMPTY) {
            entity.handle = handle;
            entity.type   = type;
            ++m_Count;
            return;
        }
    }
}

/* Remotes entity from list
 * @addr 0x4654B0
 */
void CMissionCleanup::RemoveEntityFromList(int32 handle, MissionCleanUpEntityType type) {
    for (auto& entity : m_Objects) {
        if (entity.type != type || entity.handle != handle) {
            continue;
        }

        switch (entity.type) {
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_VEHICLE: {
            auto* veh = GetVehiclePool()->GetAtRef(entity.handle);

            if (veh && veh->m_bIsStaticWaitingForCollision) {
                veh->m_bIsStaticWaitingForCollision = false;
                if (!veh->GetIsStatic()) {
                    veh->AddToMovingList();
                }
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_PED: {
            auto* ped = GetPedPool()->GetAtRef(entity.handle);

            if (ped && ped->m_bIsStaticWaitingForCollision) {
                ped->m_bIsStaticWaitingForCollision = false;
                if (!ped->GetIsStatic()) {
                    ped->AddToMovingList();
                }
            }
            break;
        }
        case MissionCleanUpEntityType::MISSION_CLEANUP_ENTITY_TYPE_OBJECT: {
            auto* obj = GetObjectPool()->GetAtRef(entity.handle);

            if (obj && obj->m_bIsStaticWaitingForCollision) {
                obj->m_bIsStaticWaitingForCollision = false;
                if (!obj->GetIsStatic()) {
                    obj->AddToMovingList();
                }
            }
            break;
        }
        default:
            break;
        }

        entity = tMissionCleanupEntity();
        --m_Count;
    }
}

/* Checks if collision has loaded for mission objects
 * @addr 0x4652D0
 */
void CMissionCleanup::CheckIfCollisionHasLoadedForMissionObjects() {
    ZoneScoped;

#ifdef NOTSA_STANDALONE
    // Standalone: not ported. Reached from CTheScripts::Process (the first per-frame logic), i.e. this is
    // the *runtime* boundary rather than loading.
    //
    // Verified inert for the current stage, not merely assumed: the real body (reached through a jump
    // thunk - see MEMORY.md "jump thunks") walks the 75-entry m_Objects array while `type != 0` and, for
    // each live entry whose model info lacks flag 0x40000 (collision-loaded), clears the flag and removes
    // the entity. CMissionCleanup::Init() sets m_Count = 0 and fills every entry with the default
    // (EMPTY == 0) type, and the only writer is AddEntityToList (script/gameplay driven). So with an
    // all-EMPTY list the loop body never executes once - the function is a no-op right now.
    //
    // TODO(reversal): implement it when mission objects actually get cleaned up. Its dependencies are
    // 0x4633E0 and 0x542800 (both still exe thunks) plus the model-info lookup through 0xB7449C.
    return;
#else
    ((void(__thiscall*)(CMissionCleanup*))0x4652D0)(this);
#endif
}

// NOTSA
void CMissionCleanup::AddEntityToList(CObject& obj) {
    return AddEntityToList(GetObjectPool()->GetRef(&obj), MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
}

// NOTSA
void CMissionCleanup::AddEntityToList(CPed& ped) {
    return AddEntityToList(GetPedPool()->GetRef(&ped), MISSION_CLEANUP_ENTITY_TYPE_PED);
}

// NOTSA
void CMissionCleanup::AddEntityToList(CVehicle& veh) {
    return AddEntityToList(GetVehiclePool()->GetRef(&veh), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
}

// NOTSA
void CMissionCleanup::RemoveEntityFromList(CObject& obj) {
    return RemoveEntityFromList(GetObjectPool()->GetRef(&obj), MISSION_CLEANUP_ENTITY_TYPE_OBJECT);
}

void CMissionCleanup::RemoveEntityFromList(CPed& ped) {
    return RemoveEntityFromList(GetPedPool()->GetRef(&ped), MISSION_CLEANUP_ENTITY_TYPE_PED);
}

void CMissionCleanup::RemoveEntityFromList(CVehicle& veh) {
    return RemoveEntityFromList(GetVehiclePool()->GetRef(&veh), MISSION_CLEANUP_ENTITY_TYPE_VEHICLE);
}
