#include "StdInc.h"

#include "DecisionMakerTypes.h"
#include "Enums/eEventType.h"
#include "Enums/eDecisionMakerEvents.h"

void CDecisionMakerTypes::InjectHooks() {
    RH_ScopedClass(CDecisionMakerTypes);
    RH_ScopedCategory("DecisionMakers");
}

// 0x607050
int32 CDecisionMakerTypes::AddDecisionMaker(CDecisionMaker* decisionMaker, eDecisionTypes decisionMakerType, bool bDecisionMakerForMission) {
#ifdef NOTSA_STANDALONE
    // Standalone: original body lives in gta_sa.exe.
    // `decisionMakerType` is the slot index (0..COUNT_TOTAL-1).
    // TODO: verify return value (script handle) & semantics against the original. See IDEAS.md.
    const auto idx = (int32)decisionMakerType < 0
        ? 0
        : ((int32)decisionMakerType >= (int32)eDecisionMakerType::COUNT_TOTAL ? (int32)eDecisionMakerType::COUNT_TOTAL - 1 : (int32)decisionMakerType);
    m_DecisionMakers[idx] = *decisionMaker;
    m_IsActive[idx]         = true;
    if (idx + 1 > m_NoOfDecisionMakers) {
        m_NoOfDecisionMakers = idx + 1;
    }
    return idx;
#else
    return plugin::CallMethodAndReturn<int32, 0x607050, CDecisionMakerTypes*, CDecisionMaker*, eDecisionTypes, bool>(this, decisionMaker, decisionMakerType, bDecisionMakerForMission);
#endif
}

// 0x4684F0
CDecisionMakerTypes* CDecisionMakerTypes::GetInstance() {
#ifdef NOTSA_STANDALONE
    // Standalone: the instance normally lives in the original exe's data segment.
    static CDecisionMakerTypes instance{};
    return &instance;
#else
    return plugin::CallAndReturn<CDecisionMakerTypes*, 0x4684F0>();
#endif
}

// 0x606E70
void CDecisionMakerTypes::MakeDecision(CPed* ped, eEventType eventType, int32 eventSourceType, bool bIsPedInVehicle, eTaskType taskTypeToAvoid1, eTaskType taskTypeToAvoid2, eTaskType taskTypeToAvoid3, eTaskType taskTypeToSeek, bool bUseInGroupDecisionMaker, int16& taskType, int16& facialTaskType) {
    plugin::CallMethod<0x606E70>(this, ped, eventType, eventSourceType, bIsPedInVehicle, taskTypeToAvoid1, taskTypeToAvoid2, taskTypeToAvoid3, taskTypeToSeek, bUseInGroupDecisionMaker, &taskType, &facialTaskType);
}

// 0x6043A0
void CDecisionMakerTypes::RemoveDecisionMaker(eDecisionTypes dm) {
#ifdef NOTSA_STANDALONE
    // Standalone: original body lives in gta_sa.exe.
    const auto idx = (int32)dm;
    if (idx >= 0 && idx < (int32)eDecisionMakerType::COUNT_TOTAL) {
        m_IsActive[idx]         = false;
        ScriptReferenceIndex[idx] = 0;
    }
#else
    plugin::CallMethod<0x6043A0>(this, dm);
#endif
}

// 0x606F80
eTaskType CDecisionMakerTypes::MakeDecision(CPedGroup* pedGroup, eEventType eventType, int32 eventSourceType, bool bIsPedInVehicle, eTaskType taskId1, eTaskType taskId2, eTaskType taskId3, eTaskType taskId4) {
    return plugin::CallMethodAndReturn<eTaskType, 0x606F80, CDecisionMakerTypes*, CPedGroup*, int32, int32, bool, int32, int32, int32, int32>(
        this, pedGroup, eventType, eventSourceType, bIsPedInVehicle, taskId1, taskId2, taskId3, taskId4);
}

// 0x6044C0
void CDecisionMakerTypes::AddEventResponse(int32 decisionMakerIndex, eEventType eventType, eTaskType taskId, float* responseChances, int32* flags) {
    plugin::CallMethod<0x6044C0, CDecisionMakerTypes*, int32, int32, int32, float*, int32*>(this, decisionMakerIndex, eventType, taskId, responseChances, flags);
}

// 0x604490
void CDecisionMakerTypes::FlushDecisionMakerEventResponse(int32 decisionMakerIndex, eEventType eventId) {
    plugin::CallMethod<0x604490>(this, decisionMakerIndex, eventId);
}

void CDecisionMakerTypes::LoadEventIndices() {
#ifdef NOTSA_STANDALONE
    // Standalone: original body lives in gta_sa.exe.
    // Maps game events (`eEventType`) to decision-maker events (`eDecisionMakerEvents`).
    // Events with no DM counterpart map to -1.
    // TODO: verify against the original. See IDEAS.md.
    static constexpr std::pair<eEventType, eDecisionMakerEvents> mapping[]{
        { EVENT_DRAGGED_OUT_CAR,                 DM_EVENT_DRAGGED_OUT_CAR                 },
        { EVENT_KNOCK_OFF_BIKE,                  DM_EVENT_KNOCK_OFF_BIKE                  },
        { EVENT_DAMAGE,                          DM_EVENT_DAMAGE                          },
        { EVENT_DEAD_PED,                        DM_EVENT_DEAD_PED                        },
        { EVENT_POTENTIAL_GET_RUN_OVER,          DM_EVENT_POTENTIAL_GET_RUN_OVER          },
        { EVENT_POTENTIAL_WALK_INTO_PED,         DM_EVENT_POTENTIAL_WALK_INTO_PED         },
        { EVENT_SHOT_FIRED,                      DM_EVENT_SHOT_FIRED                      },
        { EVENT_PED_ENTERED_MY_VEHICLE,          DM_EVENT_PED_ENTERED_MY_VEHICLE          },
        { EVENT_SEXY_PED,                        DM_EVENT_SEXY_PED                        },
        { EVENT_ATTRACTOR,                       DM_EVENT_ATTRACTOR                       },
        { EVENT_VEHICLE_THREAT,                  DM_EVENT_VEHICLE_THREAT                  },
        { EVENT_GUN_AIMED_AT,                    DM_EVENT_GUN_AIMED_AT                    },
        { EVENT_ACQUAINTANCE_PED_HATE,           DM_EVENT_ACQUAINTANCE_PED_HATE           },
        { EVENT_ACQUAINTANCE_PED_DISLIKE,        DM_EVENT_ACQUAINTANCE_PED_DISLIKE        },
        { EVENT_ACQUAINTANCE_PED_LIKE,           DM_EVENT_ACQUAINTANCE_PED_LIKE           },
        { EVENT_ACQUAINTANCE_PED_RESPECT,        DM_EVENT_ACQUAINTANCE_PED_RESPECT        },
        { EVENT_VEHICLE_DAMAGE_WEAPON,           DM_EVENT_VEHICLE_DAMAGE_WEAPON           },
        { EVENT_SPECIAL,                         DM_EVENT_SPECIAL                         },
        { EVENT_GOT_KNOCKED_OVER_BY_CAR,         DM_EVENT_GOT_KNOCKED_OVER_BY_CAR         },
        { EVENT_POTENTIAL_WALK_INTO_OBJECT,      DM_EVENT_POTENTIAL_WALK_INTO_OBJECT      },
        { EVENT_POTENTIAL_WALK_INTO_FIRE,        DM_EVENT_POTENTIAL_WALK_INTO_FIRE        },
        { EVENT_SCRIPTED_ATTRACTOR,              DM_EVENT_SCRIPTED_ATTRACTOR              },
        { EVENT_SHOT_FIRED_WHIZZED_BY,           DM_EVENT_SHOT_FIRED_WHIZZED_BY           },
        { EVENT_LOW_ANGER_AT_PLAYER,             DM_EVENT_LOW_ANGER_AT_PLAYER             },
        { EVENT_HIGH_ANGER_AT_PLAYER,            DM_EVENT_HIGH_ANGER_AT_PLAYER            },
        { EVENT_HEALTH_REALLY_LOW,               DM_EVENT_HEALTH_REALLY_LOW               },
        { EVENT_HEALTH_LOW,                      DM_EVENT_HEALTH_LOW                      },
        { EVENT_LEADER_ENTERED_CAR_AS_DRIVER,    DM_EVENT_LEADER_ENTERED_CAR_AS_DRIVER    },
        { EVENT_LEADER_EXITED_CAR_AS_DRIVER,     DM_EVENT_LEADER_EXITED_CAR_AS_DRIVER     },
        { EVENT_POTENTIAL_WALK_INTO_VEHICLE,     DM_EVENT_POTENTIAL_WALK_INTO_VEHICLE     },
        { EVENT_FIRE_NEARBY,                     DM_EVENT_FIRE_NEARBY                     },
        { EVENT_SOUND_LOUD,                      DM_EVENT_SOUND_LOUD                      },
        { EVENT_SOUND_QUIET,                     DM_EVENT_SOUND_QUIET                     },
        { EVENT_ACQUAINTANCE_PED_HATE_BADLY_LIT, DM_EVENT_ACQUAINTANCE_PED_HATE_BADLY_LIT },
        { EVENT_SEEN_PANICKED_PED,               DM_EVENT_SEEN_PANICKED_PED               },
        { EVENT_PLAYER_COMMAND_TO_GROUP,         DM_EVENT_PLAYER_COMMAND_TO_GROUP         },
        { EVENT_SEEN_COP,                        DM_EVENT_SEEN_COP                        },
        { EVENT_VEHICLE_DAMAGE_COLLISION,        DM_EVENT_VEHICLE_DAMAGE_COLLISION        },
        { EVENT_DANGER,                          DM_EVENT_DANGER                          },
        { EVENT_PLAYER_COMMAND_TO_GROUP_GATHER,  DM_EVENT_PLAYER_COMMAND_TO_GROUP_GATHER  },
        { EVENT_VEHICLE_ON_FIRE,                 DM_EVENT_VEHICLE_ON_FIRE                 },
    };

    // Unmapped events fall back to 0 (the original relies on zero-initialized storage;
    // some events used in the data files - e.g. 16, 45, 74 - have no DM counterpart).
    m_EventIndices.fill(0);
    for (const auto& [event, dmEvent] : mapping) {
        m_EventIndices[(size_t)event] = (int32)dmEvent;
    }
#else
    plugin::CallMethod<0x600840>(this);
#endif
}
