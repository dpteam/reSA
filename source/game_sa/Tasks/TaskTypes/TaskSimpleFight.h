/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskSimple.h"
class CAnimBlendAssociation;
class CEntity;
class CPed;
class CPlayerPed;

enum eFightAttackType : int8 {
    FIGHT_ATTACK_HIT_1 = 0,
    FIGHT_ATTACK_HIT_2 = 1,
    FIGHT_ATTACK_HIT_3 = 2,
    FIGHT_ATTACK_FIGHT_BLOCK = 3,
    FIGHT_ATTACK_FIGHTIDLE = 4,
};

class NOTSA_EXPORT_VTABLE CMeleeInfo {
public:
    AssocGroupId           m_nAnimGroup;      // 0x00
    float                  m_fRanges;         // 0x04
    std::array<float, 5>   m_fHit;            // 0x08
    std::array<float, 5>   m_fChain;          // 0x1C
    std::array<float, 5>   m_fRadius;         // 0x30
    float                  m_fGroundLoop;     // 0x44
    float                  m_fBlockHit;       // 0x48
    float                  m_fBlockChain;     // 0x4C
    std::array<uint8, 5>   m_aHitLevel;       // 0x50
    std::array<uint8, 5>   m_aUnk55;          // 0x55
    uint8                  pad_5A[2];         // 0x5A
    std::array<int32, 5>   m_Hit;             // 0x5C
    std::array<int32, 5>   m_AltHit;          // 0x70
    uint16                 m_wFlags;          // 0x84
};
VALIDATE_SIZE(CMeleeInfo, 0x88);
// Layout verified against disasm of CTaskSimpleFight::LoadMeleeData (0x5BEDC0), which writes every one of
// these offsets. The previous `int32 ABlockHit/ABlockChain` (0x48/0x4C) were really floats - the same
// routine stores 100.0f into them - and `uint8 m_nHitLevel; int32 m_nDamage; int32 field_58;` was really
// two 5-byte arrays at 0x50 and 0x55. Nothing in the codebase read those old fields.

class NOTSA_EXPORT_VTABLE CTaskSimpleFight : public CTaskSimple {
public:
    bool                   m_bIsFinished;
    bool                   m_bIsInControl;
    bool                   m_bAnimsReferenced;
    AssocGroupId           m_nRequiredAnimGroup;
    uint16                 m_nIdlePeriod;
    uint16                 m_nIdleCounter;
    int8                   m_nContinueStrike;
    int8                   m_nChainCounter;
    CEntity*               m_pTargetEntity;
    CAnimBlendAssociation* m_pAnim;
    CAnimBlendAssociation* m_pIdleAnim;
    int8                   m_nComboSet;
    eFightAttackType       m_nCurrentMove;
    uint8                  m_nNextCommand;
    uint8                  m_nLastCommand;

    // 13 entries, not 12: verified against disasm of LoadMeleeData (0x5BEDC0), whose outer loop runs
    // (0xC177BC - 0xC170D4)/0x88 = 13 times over 0xC170D0-sized elements, and because
    // TaskSimpleUseGun.cpp reads m_aComboData[12].
    static inline auto& m_aComboData = StaticRef<std::array<CMeleeInfo, 13>>(0xC170D0);

public:
    static constexpr auto Type = eTaskType::TASK_SIMPLE_FIGHT;

    CTaskSimpleFight(CEntity* entity, int32 nCommand, uint32 nIdlePeriod = 10000);
    ~CTaskSimpleFight() override;

    eTaskType GetTaskType() const override { return Type; }
    CTask* Clone() const override { return new CTaskSimpleFight(m_pTargetEntity, m_nLastCommand, m_nIdlePeriod); }
    bool MakeAbortable(CPed* ped, eAbortPriority priority = ABORT_PRIORITY_URGENT, const CEvent* event = nullptr) override;
    bool ProcessPed(CPed* ped) override;

    static void LoadMeleeData();

    void BeHitWhileBlocking(CPed* ped1, CPed* ped2, int8, int8);
    void ChooseAttackAI(CPed* ped);
    void ChooseAttackPlayer(CPed* ped);
    bool ControlFight(CEntity* entity, uint8 command);

    void FightHitCar(CPed* ped, CVehicle* vehicle, CVector& posn1, CVector& posn2, int16, int8);
    void FightHitObj(CPed* ped, CObject* object, CVector& posn1, CVector& posn2, int16, int8);
    void FightHitPed(CPed* creator, CPed* victim, CVector& posn1, CVector& posn2, int16);
    void FightSetUpCol(float);
    void FightStrike(CPed* ped, CVector& posn);

    void FindTargetOnGround(CPed* ped);
    void FinishMeleeAnimCB(CAnimBlendAssociation*, void*);

    bool IsComboSet();
    bool IsHitComboSet();

    void GetAvailableComboSet(CPed* ped, int8);
    void GetComboType(char*);
    AssocGroupId GetComboAnimGroupID();
    void GetHitLevel(const char*);
    void GetHitSound(int32);
    void GetRange();
    void GetStrikeDamage(CPed* ped);

    void SetPlayerMoveAnim(CPlayerPed* player);
    void StartAnim(CPed* ped, int32);

private:
    friend void InjectHooksMain();
    static void InjectHooks();

    CTaskSimpleFight* Constructor(CEntity* entity, int32 nCommand, uint32 nIdlePeriod);
    CTaskSimpleFight* Destructor();

};
VALIDATE_SIZE(CTaskSimpleFight, 0x28);
