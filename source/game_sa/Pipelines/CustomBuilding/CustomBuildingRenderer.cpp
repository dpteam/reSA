#include "StdInc.h"

#include "CustomBuildingRenderer.h"

#include "CustomBuildingDNPipeline.h"
#include "CustomBuildingPipeline.h"

void CCustomBuildingRenderer::InjectHooks() {
    RH_ScopedClass(CCustomBuildingRenderer);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(Initialise, 0x5D7EC0);
    RH_ScopedInstall(Shutdown, 0x5D7EE0);
    RH_ScopedInstall(PluginAttach, 0x5D7EF0);
    RH_ScopedInstall(AtomicSetup, 0x5D7F00, { .Reversed = false });
    RH_ScopedInstall(IsCBPCPipelineAttached, 0x5D7F40, { .Reversed = false });
    RH_ScopedInstall(UpdateDayNightBalanceParam, 0x5D7F80, { .Reversed = false });
    RH_ScopedInstall(Update, 0x5D8050, { .Reversed = false });
}

// 0x5D7EC0
bool CCustomBuildingRenderer::Initialise() {
#ifdef NOTSA_STANDALONE
    // See CCarFXRenderer::Initialise - RxPipeline-based custom pipelines aren't available on librw.
    return true;
#else
    auto pipe = CCustomBuildingPipeline::CreatePipe();
    if (pipe)
        return CCustomBuildingDNPipeline::CreatePipe();

    return pipe;
#endif
}

// 0x5D7EE0
void CCustomBuildingRenderer::Shutdown() {
#ifndef NOTSA_STANDALONE
    CCustomBuildingPipeline::DestroyPipe();
    CCustomBuildingDNPipeline::DestroyPipe();
#endif
}

// 0x5D7EF0
bool CCustomBuildingRenderer::PluginAttach() {
    return CCustomBuildingDNPipeline::ExtraVertColourPluginAttach();
}

// 0x5D7F00
void CCustomBuildingRenderer::AtomicSetup(RpAtomic* atomic) {
    plugin::Call<0x5D7F00, RpAtomic*>(atomic);
}

// 0x5D7F40
bool CCustomBuildingRenderer::IsCBPCPipelineAttached(RpAtomic* atomic) {
#ifdef NOTSA_STANDALONE
    // Standalone: this asks whether the atomic carries one of the custom building RxPipelines
    // (disasm: RpAtomicGetPipeline(atomic) (0x72FC40) compared against the CBPC pipeline IDs / their
    // plugin data at 0x5D6E90). RenderWare's RxPipeline system does not exist in librw, and
    // CCustomBuildingRenderer::PluginAttach/CreatePipe are not attached in standalone (see PluginAttach
    // in app.cpp), so no atomic can ever have one attached here => false is the correct answer, not a
    // shortcut. Every caller takes the plain-RW branch on false (AtomicSetupLightingCB,
    // CAtomicModelInfo, CDamageAtomicModelInfo, CObject, CEntity), which is what we want.
    // TODO: real custom-building rendering on top of librw's pipelines - see IDEAS.md.
    (void)atomic;
    return false;
#else
    return plugin::CallAndReturn<bool, 0x5D7F40, RpAtomic*>(atomic);
#endif
}

// 0x5D7F80
void CCustomBuildingRenderer::UpdateDayNightBalanceParam() {
    plugin::Call<0x5D7F80>();
    /*
    const auto minutes = (float)CClock::GetMinutesToday();

    const auto six_hours = 360.0f;
    if (minutes < six_hours) {
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 1.0f;
        return;
    }

    const auto seven_hours = 420.0f;
    if (minutes < seven_hours) {
        CCustomBuildingDNPipeline::m_fDNBalanceParam = float(seven_hours - minutes) / 60.0f;
        return;
    }

    const auto twenty_hours = 1200.0f;
    if (minutes < twenty_hours) {
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 0.0f;
        return;
    }

    const auto twenty_one_hour = 1260.0f;
    if (minutes >= twenty_one_hour)
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 1.0f;
    else
        CCustomBuildingDNPipeline::m_fDNBalanceParam = 1.0f - float(twenty_one_hour - minutes) / 60.0f;
    */
}

// 0x5D8050
void CCustomBuildingRenderer::Update() {
    ZoneScoped;

    plugin::Call<0x5D8050>();

    /*
    void sub_5D6830(int a1) {
        static uint32 dword_C02C14, dword_C02C18 = 0;

        dword_C02C14 = (dword_C02C14 + 1) & 15;
        dword_C02C18 = a1;
    }

    CCustomBuildingRenderer::UpdateDayNightBalanceParam();
    sub_5D6830(0);
    */
}
