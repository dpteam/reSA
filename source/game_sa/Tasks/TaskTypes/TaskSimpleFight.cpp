#include "StdInc.h"

#include "TaskSimpleFight.h"

// 0x61C470
CTaskSimpleFight::CTaskSimpleFight(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) : CTaskSimple() {
    plugin::CallMethod<0x61C470, CTaskSimpleFight*, CEntity*, int32, uint32>(this, entity, nCommand, nIdlePeriod);
}

// 0x61C530
CTaskSimpleFight::~CTaskSimpleFight() {
    plugin::CallMethod<0x61C530, CTaskSimpleFight*>(this);
}

// 0x61C5E0
bool CTaskSimpleFight::ControlFight(CEntity* entity, uint8 command) {
    return plugin::CallMethodAndReturn<bool, 0x61C5E0, CTaskSimpleFight*, CEntity*, uint8>(this, entity, command);
}

// 0x4ABDC0
bool CTaskSimpleFight::IsComboSet() {
    return plugin::CallMethodAndReturn<bool, 0x4ABDC0, CTaskSimpleFight*>(this);
}

// 0x4ABDF0
bool CTaskSimpleFight::IsHitComboSet() {
    return plugin::CallMethodAndReturn<bool, 0x4ABDF0, CTaskSimpleFight*>(this);
}

// 0x4ABDA0
AssocGroupId CTaskSimpleFight::GetComboAnimGroupID() {
    return plugin::CallMethodAndReturn<AssocGroupId, 0x4ABDA0, CTaskSimpleFight*>(this);
}

// 0x5BEDC0
void CTaskSimpleFight::LoadMeleeData() {
    ZoneScoped;

#ifdef NOTSA_STANDALONE
    // Standalone: only the first half of the original is ported - the per-combo defaults, transcribed
    // from 0x5BEDC0..0x5BEE38. What is NOT ported yet (see TODO below) is the 7-entry float table at
    // 0xC177D0 and the `data\melee.dat` parse that overrides these defaults.
    for (auto& combo : m_aComboData) {
        combo.m_nAnimGroup  = AssocGroupId::ANIM_GROUP_MELEE_1; // 33 (0x21)
        combo.m_fRanges     = 1.5f;
        combo.m_fGroundLoop = 0.0f;
        combo.m_fBlockHit   = 100.0f;
        combo.m_fBlockChain = 100.0f;
        combo.m_wFlags      = 0;
        for (auto i = 0; i < 5; i++) {
            combo.m_fHit[i]      = 100.0f;
            combo.m_fChain[i]    = 100.0f;
            combo.m_fRadius[i]   = 1.0f;
            combo.m_aHitLevel[i] = 7;
            combo.m_aUnk55[i]    = 0;
            combo.m_Hit[i]       = 0;
            combo.m_AltHit[i]    = 0;
        }
    }

    // TODO(reversal): remaining parts of the original -
    //   a) a 7-entry table of 3 floats at 0xC177D0 (stride 0xC), each { 0.0f, 0.75f, 0.0f } - those
    //      globals are not declared anywhere in our headers yet;
    //   b) opening `data\melee.dat` (0x538900) and parsing it line-by-line (CFileLoader::LoadLine
    //      0x536F80, fields via 0x8214D0) to override the defaults set above.
    // Melee combat is not reachable during loading, so keeping the defaults is harmless for now.
#else
    plugin::Call<0x5BEDC0>();
#endif
}

// 0x6239F0
bool CTaskSimpleFight::MakeAbortable(CPed* ped, eAbortPriority priority, const CEvent* event) {
    return plugin::CallMethodAndReturn<bool, 0x6239F0, CTaskSimpleFight*, CPed*, eAbortPriority, const CEvent*>(this, ped, priority, event);
}

// 0x629920
bool CTaskSimpleFight::ProcessPed(CPed* ped) {
    return plugin::CallMethodAndReturn<bool, 0x629920, CTaskSimpleFight*, CPed*>(this, ped);
}

void CTaskSimpleFight::InjectHooks() {
    RH_ScopedVirtualClass(CTaskSimpleFight, 0x86D684, 9);
    RH_ScopedCategory("Tasks/TaskTypes");

}

CTaskSimpleFight* CTaskSimpleFight::Constructor(CEntity* entity, int32 nCommand, uint32 nIdlePeriod) {
    this->CTaskSimpleFight::CTaskSimpleFight(entity, nCommand, nIdlePeriod);
    return this;
}

CTaskSimpleFight* CTaskSimpleFight::Destructor() {
    this->CTaskSimpleFight::~CTaskSimpleFight();
    return this;
}
