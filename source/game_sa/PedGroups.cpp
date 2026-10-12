#include "StdInc.h"

#include "PedGroups.h"

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
    return plugin::CallAndReturn<int32, 0x5FB800>();
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
    // Reversed from disasm 0x5FB8C0: for each of the 8 groups (stride 0x2D4 == VALIDATE_SIZE(CPedGroup),
    // walking 0xC09928..0xC0AFC8):
    //     if (ms_activeGroups[i]) {
    //         ms_activeGroups[i] = 0;
    //         for (8 membership slots) if (slot != 0) <CPedGroup::0x5F80D0>(slot);   // per-member flush
    //         <CPedGroup::0x5F7350>() at +0x28;
    //         byte at +0x2C8 = 0;
    //     } else {
    //         ScriptReferenceIndex[i] = 1;
    //     }
    // The active branch cannot be expressed here: its two helpers (0x5F80D0, 0x5F7350) are not reversed
    // and are not even declared in this tree. That is fine *for this call site*, because
    // ms_activeGroups is provably all-zero when CTheScripts::Init runs - the only writer is
    // CPedGroups::AddGroup (script/runtime driven), which has not run yet. So only the else-branch is
    // reachable, and it is what is implemented. Guarded so a future caller that reaches Init() with live
    // groups fails loudly instead of silently skipping the flush.
    // NOTE (pre-existing, unrelated to standalone): the original walks groups starting at 0xC09928 while
    // `ms_groups` is declared at 0xC09920 - an 8-byte difference. Worth checking against the PDB if group
    // data ever looks shifted.
    VERIFY(std::all_of(ms_activeGroups.begin(), ms_activeGroups.end(), [](char a) { return a == 0; }));
    ScriptReferenceIndex.fill(1);
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
