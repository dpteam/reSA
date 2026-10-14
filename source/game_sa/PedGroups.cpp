#include "StdInc.h"

#include "PedGroups.h"

#ifdef NOTSA_STANDALONE
#include "PedGroup.h"              // CPedGroup's members (m_groupMembership / m_bIsMissionGroup)
#include "PedGroupMembership.h"    // GetMember / RemoveMember, TOTAL_PED_GROUP_MEMBERS
#include "PedGroupIntelligence.h"  // Flush
#endif

void CPedGroups::InjectHooks() {
    RH_ScopedClass(CPedGroups);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Process, 0x5FC800);
    RH_ScopedInstall(RemoveGroup, 0x5FB870);
}

#ifdef ANDROID
void CPedGroups::Save() {
    
}

void CPedGroups::Load() {
    
}
#endif

// return the index of the added group , return -1 if failed.
// 0x5FB800
int32 CPedGroups::AddGroup() {
#ifdef NOTSA_STANDALONE
    // Reversed from disasm 0x5FB800 (98 bytes):
    //     eax = -1
    //     for (ebx = 0; ebx < 8; ebx++)
    //         if (ms_activeGroups[ebx] == 0) goto found;
    //     return -1;                       // no free slot
    // found:
    //     ms_activeGroups[ebx] = 1;
    //     esi = &ms_groups[ebx];           // 0xC09920 + ebx * 0x2D4
    //     for (edi = 0; edi < 8; edi++)
    //         if (esi->m_groupMembership.m_members[edi] != 0)
    //             esi->m_groupMembership.RemoveMember(edi);      // 0x5F80D0, ecx = group+8
    //     esi->m_groupIntelligence.Flush();                      // 0x5F7350, ecx = group+0x30
    //     esi->m_bIsMissionGroup = false;                        // [esi+0x2D0]
    //     return ebx;
    // The activation flushes whatever stale membership the slot still carried from its previous life, which
    // is why it is not simply "mark active".
    for (auto i = 0; i < static_cast<int32>(ms_groups.size()); i++) {
        if (ms_activeGroups[i]) {
            continue;
        }
        ms_activeGroups[i] = true;

        auto&       group      = ms_groups[i];
        auto&       membership = group.GetMembership();
        for (auto j = 0; j < TOTAL_PED_GROUP_MEMBERS; j++) {
            if (membership.GetMember(j)) {
                membership.RemoveMember(j);
            }
        }
        group.GetIntelligence().Flush();
        group.m_bIsMissionGroup = false;
        return i;
    }
    return -1;
#else
    return plugin::CallAndReturn<int32, 0x5FB800>();
#endif
}

// 0x5FB870
void CPedGroups::RemoveGroup(int32 groupId) {
    auto* const group = &GetGroup(groupId);
    if (!std::exchange(ms_activeGroups[groupId], false)) {
        return;
    }
    group->Flush();
}

// 0x5FB8A0
void CPedGroups::RemoveAllFollowersFromGroup(int32 groupId) {
    plugin::Call<0x5FB8A0, int32>(groupId);
}

// 0x5FB8C0
void CPedGroups::Init() {
#ifdef NOTSA_STANDALONE
    // Reversed from disasm 0x5FB8C0. Both branches are implemented; an earlier version of this file
    // contained only the else-branch, on the false premise that the active branch's two helpers were not
    // reversed and "not even declared in this tree". They were both already reversed and I had simply
    // searched in the wrong place - 0x5F80D0 is CPedGroupMembership::RemoveMember(int32) and 0x5F7350 is
    // CPedGroupIntelligence::Flush. Recorded here because the mistake is the transferable kind: an
    // assertion of absence is much easier to write than to verify, and it silently disabled a code path.
    //
    // The shape, per group i (stride 0x2D4 == VALIDATE_SIZE(CPedGroup)):
    //     if (ms_activeGroups[i]) {
    //         ms_activeGroups[i] = 0;
    //         for (j = 0..7) if (membership.m_members[j]) membership.RemoveMember(j);
    //         intelligence.Flush();                       // [edi+0x28]
    //         group.m_bIsMissionGroup = false;            // [edi+0x2C8]
    //     } else {
    //         ScriptReferenceIndex[i] = 1;
    //     }
    //
    // Reading note that resolves a discrepancy flagged in an earlier commit: the original's loop register
    // starts at 0xC09928, eight bytes above `ms_groups` at 0xC09920, which looked like a wrong base. It is
    // not - the register holds the *membership* pointer (group + 8), which is what the two calls need as
    // `this` (`mov ecx, edi` before the RemoveMember call). Hence +0x28 == group+0x30 == the intelligence,
    // and +0x2C8 == group+0x2D0 == m_bIsMissionGroup. Both functions now agree on the layout.
    for (auto i = 0; i < static_cast<int32>(ms_groups.size()); i++) {
        if (ms_activeGroups[i]) {
            ms_activeGroups[i] = false;

            auto& group      = ms_groups[i];
            auto& membership = group.GetMembership();
            for (auto j = 0; j < TOTAL_PED_GROUP_MEMBERS; j++) {
                if (membership.GetMember(j)) {
                    membership.RemoveMember(j);
                }
            }
            group.GetIntelligence().Flush();
            group.m_bIsMissionGroup = false;
        } else {
            ScriptReferenceIndex[i] = 1;
        }
    }
    return;
#else
    plugin::Call<0x5FB8C0>();
#endif
}

// 0x5F7E30
void CPedGroups::RegisterKillByPlayer() {
    if (ms_bIsPlayerOnAMission)
        ms_iNoOfPlayerKills++;
}

// 0x5FB930
void CPedGroups::CleanUpForShutDown() {
    plugin::Call<0x5FB930>();
}

// 0x5F7E40
bool CPedGroups::IsGroupLeader(CPed* ped) {
    return plugin::CallAndReturn<bool, 0x5F7E40, CPed*>(ped);
}

// 0x5F7E80
CPedGroup* CPedGroups::GetPedsGroup(const CPed* ped) {
    return plugin::CallAndReturn<CPedGroup*, 0x5F7E80>(ped);
}

// 0x5F7EE0
int32 CPedGroups::GetGroupId(const CPedGroup* pedGroup) {
    return plugin::CallAndReturn<int32, 0x5F7EE0, const CPedGroup*>(pedGroup);
}

// 0x5FC800
void CPedGroups::Process() {
    for (auto&& [i, g] : GetActiveGroupsWithIDs()) {
        g.Process();
        if (!g.GetMembership().CountMembers()) {
            RemoveGroup(i);
        }
    }

    const auto SetGroupsDecisionMakerType = [](eDecisionMakerType dm) {
        for (auto&& [_, g] : GetActiveGroupsWithIDs()) {
            g.GetIntelligence().SetGroupDecisionMakerType(dm);
        }
    };
    if (CTheScripts::IsPlayerOnAMission() && !ms_bIsPlayerOnAMission) { // 0x5FC897 - Player started a mission
        ms_iNoOfPlayerKills = 0;
        SetGroupsDecisionMakerType(eDecisionMakerType::GROUP_RANDOM_PASSIVE);
    } else if (!CTheScripts::IsPlayerOnAMission() && ms_bIsPlayerOnAMission) { // 0x5FC8B2 - Player ended a mission (inverted)
        SetGroupsDecisionMakerType(eDecisionMakerType::UNKNOWN);
    } else if (CTheScripts::IsPlayerOnAMission() && CPedGroups::ms_iNoOfPlayerKills == 8) { // 0x5FC8BF (inverted)
        SetGroupsDecisionMakerType(eDecisionMakerType::UNKNOWN);
    } else if (CTheScripts::IsPlayerOnAMission()) {
        SetGroupsDecisionMakerType(eDecisionMakerType::GROUP_RANDOM_PASSIVE);
    }

    if (!std::exchange(ms_bIsPlayerOnAMission, CTheScripts::IsPlayerOnAMission())) { // 0x5FC98D
        ms_iNoOfPlayerKills = 0;
    }
}

// 0x5F7F10
bool CPedGroups::IsInPlayersGroup(CPed* ped) {
    return plugin::CallAndReturn<bool, 0x5F7F10, CPed*>(ped);
}

CPedGroup& CPedGroups::GetGroup(int32 groupId) {
    assert(ms_activeGroups[groupId]);
    return ms_groups[groupId];
}

// 0x5F7F40
bool CPedGroups::AreInSameGroup(const CPed* ped1, const CPed* ped2) {
    return plugin::CallAndReturn<bool, 0x5F7F40, const CPed*, const CPed*>(ped1, ped2);
}
