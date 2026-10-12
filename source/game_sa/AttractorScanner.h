/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "TaskTimer.h"

class C2dEffect;

class CAttractorScanner {
public:
    char       field_0;
    char       _pad[3];
    CTaskTimer field_4;
    C2dEffect* m_pEffectInUse;
    int32      field_14;
    // The three arrays below are written by Clear() (0x5FFF90): field_18/field_40 get 0 and field_68 gets
    // squared distances (225.0f / 25.0f), so they are float, not int32 as the old plugin-sdk header had
    // them. Nothing else in the codebase reads them, so fixing the type is safe for both builds.
    int32      field_18[10];
    int32      field_40[10];
    float      field_68[10];

    void Clear();
};

VALIDATE_SIZE(CAttractorScanner, 0x90);
