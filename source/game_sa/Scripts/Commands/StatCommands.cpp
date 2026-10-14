#include <StdInc.h>
#include <Stats.h>

#include "Commands.hpp"
#include <CommandParser/Parser.hpp>

namespace {
// INCREMENT_INT_STAT (1571)
// INCREMENT_FLOAT_STAT (1572)
template<typename T>
void IncrementStat(eStats stat, T by) {
    CStats::IncrementStat(stat, (float)(by));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_INCREASE, stat, (float)(by));
}

// DECREMENT_INT_STAT (1573)
// DECREMENT_FLOAT_STAT (1574)
template<typename T>
void DecrementStat(eStats stat, T by) {
    CStats::DecrementStat(stat, (float)(by));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_DECREASE, stat, (float)(by));
}

// REGISTER_INT_STAT (1575)
// REGISTER_FLOAT_STAT (1576)
template<typename T>
void RegisterStat(eStats stat, T value) {
    CStats::SetNewRecordStat(stat, (float)(value));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_INCREASE, stat, (float)(value));
}

// SET_INT_STAT (1577)
// SET_FLOAT_STAT (1578)
template<typename T>
void SetStat(eStats stat, T value) {
    CStats::SetStatValue(stat, (float)(value));
    CStats::DisplayScriptStatUpdateMessage(eStatUpdateState::STAT_UPDATE_INCREASE, stat, (float)(value));
}

// COMMAND_SET_TOTAL_NUMBER_OF_MISSIONS (0x42C)
// Transcribed from disasm: the 1000-1099 dispatcher (0x489500) switches on `command - 1000` through the
// byte table at 0x48A2B4; case 68 (0x42C - 1000) lands at 0x489D5B, which is
//     CollectParameters(&m_IP, 1);                                    // 0x464080
//     CStats::SetStatValue(STAT_TOTAL_NUMBER_OF_MISSIONS_IN_GAME,     // 0x55A070, stat 148
//                          (float)ScriptParams[0].iParam);            // ScriptParams == 0xA43C78
// Note it deliberately does NOT call DisplayScriptStatUpdateMessage, unlike the SET_INT/FLOAT_STAT pair
// above - the value is recorded silently, which is why this is written out separately rather than reusing
// SetStat<int32>.
void SetTotalNumberOfMissions(int32 count) {
    CStats::SetStatValue(STAT_TOTAL_NUMBER_OF_MISSIONS_IN_GAME, static_cast<float>(count));
}

// COMMAND_SET_PROGRESS_TOTAL (0x30D)
// Same shape, different stat. Resolved with tools/opcode_case.py: the 700-799 dispatcher (0x47FA30) uses
// `add eax,-703; cmp eax,95`, so index = 781-703 = 78, selector 43, case target 0x480D28:
//     CollectParameters(&m_IP, 1);                       // 0x464080
//     CStats::SetStatValue(STAT_TOTAL_PROGRESS,          // 0x55A070, stat 1
//                          (float)ScriptParams[0].iParam);
// Also silent (no DisplayScriptStatUpdateMessage), exactly like 0x42C.
void SetTotalProgress(int32 total) {
    CStats::SetStatValue(STAT_TOTAL_PROGRESS, static_cast<float>(total));
}

// COMMAND_SET_MISSION_RESPECT_TOTAL (0x997)
// COMMAND_AWARD_PLAYER_MISSION_RESPECT (0x998)
// Both found by `tools/opcode_case.py --stats 0x478000`, which enumerates the cases of the 2400-2499
// dispatcher that are nothing but `CollectParameters(1)` around an already-reversed stat call:
//     0x997 -> case 0x478EA4: CStats::SetStatValue(STAT_RESPECT_MISSION_TOTAL /*228*/, (float)param)
//     0x998 -> case 0x478ECB: CStats::IncrementStat(STAT_RESPECT_MISSION        /*224*/, (float)param)
// The command names match the stats they touch, which is the cross-check that the scan found the right
// cases rather than a lookalike.
void SetMissionRespectTotal(int32 total) {
    CStats::SetStatValue(STAT_RESPECT_MISSION_TOTAL, static_cast<float>(total));
}

void AwardMissionRespect(int32 respect) {
    CStats::IncrementStat(STAT_RESPECT_MISSION, static_cast<float>(respect));
}

// GET_INT_STAT (1618)
// GET_FLOAT_STAT (1619)
template<typename T>
T GetStat(eStats stat) {
    return (T)(CStats::GetStatValue(stat));
}
};

void notsa::script::commands::stat::RegisterHandlers() {
    REGISTER_COMMAND_HANDLER_BEGIN("Stat");

    REGISTER_COMMAND_HANDLER(COMMAND_INCREMENT_INT_STAT, IncrementStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_INCREMENT_FLOAT_STAT, IncrementStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_DECREMENT_INT_STAT, DecrementStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_DECREMENT_FLOAT_STAT, DecrementStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_INT_STAT, RegisterStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_REGISTER_FLOAT_STAT, RegisterStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_INT_STAT, SetStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_FLOAT_STAT, SetStat<float>);

    REGISTER_COMMAND_HANDLER(COMMAND_SET_TOTAL_NUMBER_OF_MISSIONS, SetTotalNumberOfMissions);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_PROGRESS_TOTAL, SetTotalProgress);
    REGISTER_COMMAND_HANDLER(COMMAND_SET_MISSION_RESPECT_TOTAL, SetMissionRespectTotal);
    REGISTER_COMMAND_HANDLER(COMMAND_AWARD_PLAYER_MISSION_RESPECT, AwardMissionRespect);

    REGISTER_COMMAND_HANDLER(COMMAND_GET_INT_STAT, GetStat<int32>);
    REGISTER_COMMAND_HANDLER(COMMAND_GET_FLOAT_STAT, GetStat<float>);

    REGISTER_COMMAND_UNIMPLEMENTED(COMMAND_GET_STAT_CHANGE_AMOUNT);
}
