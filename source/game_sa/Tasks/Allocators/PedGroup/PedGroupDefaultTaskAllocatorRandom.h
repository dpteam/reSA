#pragma once

#include <Base.h>
#include <PluginBase.h>
#include <reversiblehooks/ReversibleHooks.h>

#include "./PedGroupDefaultTaskAllocator.h"

class NOTSA_EXPORT_VTABLE CPedGroupDefaultTaskAllocatorRandom final : public CPedGroupDefaultTaskAllocator {
public:
    /* no virtual destructor */

    void AllocateDefaultTasks(CPedGroup* pedGroup, CPed* ped) const override {
#ifdef NOTSA_STANDALONE
        // Standalone: not reversed (0x5F6E90, ~320 bytes, `.Reversed = false` upstream), and it is a
        // provable no-op at this point in the boot - which is why stubbing it is safe rather than lucky.
        //
        // What the disasm does:
        //   * loop esi = 0..6 over `pedGroup + 0x21C` with stride 0x14 - the group's 7 FOLLOWER task-pair
        //     slots (TOTAL_PED_GROUP_MEMBERS - 1), each { CPed* ped; ... task ... }:
        //         if (slot.ped == 0) continue;                 // 0x5F6EC3
        //         if (ped != nullptr && slot.ped != ped) continue;   // 0x5F6ECB
        //         ... allocate a task from pool 0x4C (0x61A5A0), build a random target point from
        //             gRandomPedWanderPoints at 0xC196E8/0xC196EC, call 0x65EAA0, store the task,
        //             then 0x65EE90 with the group's m_bMembersEnterLeadersVehicle flag ...
        //   * then, if `pedGroup + 0x2A8` is non-null (and matches `ped` when given), allocate from pool
        //     0x38 and call 0x65DED0.
        // Both loops are gated on an OCCUPIED slot / a non-null pointer. CPedGroups::AddGroup - which is
        // what created this group immediately before - flushes every member slot
        // (CPedGroupMembership::RemoveMember) and the intelligence (CPedGroupIntelligence::Flush), so a
        // freshly added group has neither, and the whole function iterates 7 times doing nothing.
        //
        // Degradation, stated plainly: once a group actually HAS members, gang followers will not get
        // their default random-wander tasks. That is gameplay (gangs wandering), not booting. Its four
        // callees (0x61A5A0 pool alloc, 0x65EAA0, 0x65EE90, 0x65DED0) are all still raw thunks, so this
        // belongs with the group-AI work rather than here. See IDEAS.md.
        (void)pedGroup;
        (void)ped;
        return;
#else
        plugin::CallMethod<0x5F6E90>(this, pedGroup, ped);
#endif
    }

    ePedGroupDefaultTaskAllocatorType GetType() const override { return ePedGroupDefaultTaskAllocatorType::RANDOM; }; // 0x5F6530

public:
    static inline void InjectHooks() {
        RH_ScopedVirtualClass(CPedGroupDefaultTaskAllocatorRandom, 0x86C77C, 2);
        RH_ScopedCategory("Tasks/Allocators/PedGroup");

        RH_ScopedVMTInstall(AllocateDefaultTasks, 0x5F6E90, { .Reversed = false });
        RH_ScopedVMTInstall(GetType, 0x5F6530);
    }
};
VALIDATE_SIZE(CPedGroupDefaultTaskAllocatorRandom, sizeof(void*)); /* vtable only */
