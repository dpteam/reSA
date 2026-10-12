#include "StdInc.h"

#include "CarFXRenderer.h"
#include "CustomCarEnvMapPipeline.h"

void CCarFXRenderer::InjectHooks() {
    RH_ScopedClass(CCarFXRenderer);
    RH_ScopedCategory("Fx");

    RH_ScopedInstall(RegisterPlugins, 0x5D5B00);
    RH_ScopedInstall(Initialise, 0x5D5AC0);
    RH_ScopedInstall(InitialiseDirtTexture, 0x5D5BC0, { .Reversed = false });
    RH_ScopedInstall(Shutdown, 0x5D5AD0);
    RH_ScopedInstall(PreRenderUpdate, 0x5D5B10);
    RH_ScopedInstall(IsCCPCPipelineAttached, 0x5D5B80);
    RH_ScopedInstall(CustomCarPipeAtomicSetup, 0x5D5B20);
    RH_ScopedInstall(CustomCarPipeClumpSetup, 0x5D5B40);
    RH_ScopedInstall(SetCustomFXAtomicRenderPipelinesVMICB, 0x5D5B60);
    RH_ScopedInstall(SetFxEnvMapLightMult, 0x5D5BA0);
}

// 0x5D5B00
bool CCarFXRenderer::RegisterPlugins() {
    return CCustomCarEnvMapPipeline::RegisterPlugin() != 0;
}

// 0x5D5AC0
bool CCarFXRenderer::Initialise() {
#ifdef NOTSA_STANDALONE
    // Standalone: the custom D3D9 pipelines use RenderWare's RxPipeline API, which librw doesn't provide
    // (it has its own, much simpler, pipeline system). These are cosmetic effects (car env maps), so skip.
    // TODO: reimplement on top of librw's pipelines/plugins - see IDEAS.md.
    return true;
#else
    return CCustomCarEnvMapPipeline::CreatePipe() != 0;
#endif
}

// 0x5D5BC0
void CCarFXRenderer::InitialiseDirtTexture() {
#ifdef NOTSA_STANDALONE
    // Standalone: this loads the 16 car "dirt/grunge" textures (TXD "vehicle", e.g. "vehiclegrunge256"
    // + its mask - verified in disasm 0x5D5BC0) into ms_aDirtTextures (0xC02BD0). They are consumed only
    // by CVehicleModelInfo::SetDirtTextures, which guards every access with `if (info->m_apDirtMaterials[i])`,
    // and those dirt materials are created exclusively by the custom RxPipeline car-FX renderer - which
    // librw cannot provide (see Initialise() just above) and is therefore not attached here. So leaving
    // the array zero-filled is inert: no dirt, no crash. TODO: real car env-map/dirt rendering on top of
    // librw's pipelines - see IDEAS.md.
    return;
#else
    plugin::Call<0x5D5BC0>();
#endif
}

// 0x5D5AD0
void CCarFXRenderer::Shutdown() {
    for (auto& texture : ms_aDirtTextures) {
        RwTextureDestroy(texture);
    }
    CCustomCarEnvMapPipeline::DestroyPipe();
}

// 0x5D5B10
void CCarFXRenderer::PreRenderUpdate() {
    CCustomCarEnvMapPipeline::PreRenderUpdate();
}

// 0x5D5B80
bool CCarFXRenderer::IsCCPCPipelineAttached(RpAtomic* atomic) {
    return GetPipelineID(atomic) == 0x53F2009A;
}

// 0x5D5B20
void CCarFXRenderer::CustomCarPipeAtomicSetup(RpAtomic* atomic) {
    CCustomCarEnvMapPipeline::CustomPipeAtomicSetup(atomic);
}

// 0x5D5B40
void CCarFXRenderer::CustomCarPipeClumpSetup(RpClump* clump) {
    RpClumpForAllAtomics(clump, SetCustomFXAtomicRenderPipelinesVMICB, nullptr);
}

// 0x5D5B60
RpAtomic* CCarFXRenderer::SetCustomFXAtomicRenderPipelinesVMICB(RpAtomic* atomic, void* data) {
    CCustomCarEnvMapPipeline::CustomPipeAtomicSetup(atomic);
    return atomic;
}

// 0x5D5BA0
void CCarFXRenderer::SetFxEnvMapLightMult(float multiplier) {
    gSpecIntensity = multiplier;
}
