#include "StdInc.h"

#include "Decision.h"
#include "Enums/eTaskType.h"

void CDecision::InjectHooks() {    
    RH_ScopedClass(CDecision);
    RH_ScopedCategory("DecisionMakers");

    RH_ScopedInstall(SetDefault, 0x600530, { .Reversed = false });
    RH_ScopedInstall(Set, 0x600570);
    //RH_ScopedInstall(Add, 0x600600, { .Reversed = false });
    RH_ScopedInstall(From, 0x6006B0);
    //RH_ScopedInstall(HasResponse, 0x600710, { .Reversed = false });
    //RH_ScopedInstall(MakeDecision, 0x6040D0, { .Reversed = false });
}

// 0x6040C0
CDecision::CDecision() {
    SetDefault();
}

// 0x600530
void CDecision::SetDefault() {
#ifdef NOTSA_STANDALONE
    // Standalone: original body lives in gta_sa.exe.
    // A "default" decision has no responses (all probabilities 0), so the tasks are irrelevant.
    // TODO: verify exact semantics against the original. See IDEAS.md.
    for (auto i = 0u; i < MAX_NUM_CHOICES; i++) {
        m_Tasks[i] = TASK_NONE;
        for (auto r = 0u; r < m_Probs[i].size(); r++) {
            m_Probs[i][r] = 0;
        }
        for (auto b = 0u; b < 2; b++) {
            m_Bools[b][i] = false;
        }
    }
#else
    plugin::CallMethod<0x600530, CDecision*>(this);
#endif
}

// 0x6006B0
void CDecision::From(const CDecision& rhs) {
    *this = rhs;
}

// 0x600570
void CDecision::Set(
    notsa::mdarray<int32, MAX_NUM_CHOICES>&    tasks,
    notsa::mdarray<float, MAX_NUM_CHOICES, 4>& probs,
    notsa::mdarray<int32, MAX_NUM_CHOICES, 2>& bools,
    notsa::mdarray<float, MAX_NUM_CHOICES, 6>& facialProbs
) {
#ifdef NOTSA_STANDALONE
    // Standalone: original body lives in gta_sa.exe.
    // `facialProbs` aren't stored in CDecision (it has no room for them) - they're dropped.
    // TODO: verify exact semantics against the original. See IDEAS.md.
    for (auto i = 0u; i < MAX_NUM_CHOICES; i++) {
        m_Tasks[i] = static_cast<eTaskType>(tasks[i]);
        for (auto r = 0u; r < m_Probs[i].size(); r++) {
            // Data holds percentages (0..100), stored as a byte
            const auto p = probs[i][r];
            m_Probs[i][r] = static_cast<uint8>(p < 0.0f ? 0.0f : (p > 100.0f ? 100.0f : p));
        }
        for (auto b = 0u; b < 2; b++) {
            m_Bools[b][i] = bools[i][b] != 0;
        }
    }
#else
    plugin::CallMethod<0x600570>(this, &tasks, &probs, &bools, &facialProbs);
#endif
}

/*
// 0x6040D0
void CDecision::MakeDecision(int32, bool, int32, int32, int32, int32, int16&, int16&) {

}

// 0x600710
bool CDecision::HasResponse() {

}

// 0x600600
void CDecision::Add(int32, float*, int32*) {

}
*/
