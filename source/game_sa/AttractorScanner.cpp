#include "StdInc.h"

#include "AttractorScanner.h"

// 0x5FFF90
void CAttractorScanner::Clear()
{
    // Disasm (62 bytes, verified instruction by instruction):
    //     esi = 0; edx = 0; eax = this + 0x68
    //     do {
    //         [eax-0x50] = 0                      -> field_18[edx]
    //         [eax-0x28] = 0                      -> field_40[edx]
    //         [eax]      = g_AttractorRange * g_AttractorRange   (0x86C52C = 15.0f -> 225.0f)
    //         if (edx != 4 && edx != 7) [eax] = 25.0f           // i.e. 5.0f squared
    //         ++edx; eax += 4;
    //     } while (edx < 10)
    // Note it deliberately does NOT touch field_0/field_4/m_pEffectInUse/field_14 - only the three arrays.
    // The two magic indices keep 15^2; every other slot is overwritten with 5^2.
    constexpr float ATTRACTOR_RANGE_SQ   = 15.0f * 15.0f; // the original folds [0x86C52C]^2
    constexpr float ATTRACTOR_NEAR_RANGE = 5.0f * 5.0f;
    for (auto i = 0; i < 10; ++i) {
        field_18[i] = 0;
        field_40[i] = 0;
        field_68[i] = (i == 4 || i == 7) ? ATTRACTOR_RANGE_SQ : ATTRACTOR_NEAR_RANGE;
    }
}
