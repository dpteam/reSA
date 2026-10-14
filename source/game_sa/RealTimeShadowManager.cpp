#include "StdInc.h"

#include "RealTimeShadowManager.h"
#include "Shadows.h"

auto& g_realTimeShadowMan = StaticRef<CRealTimeShadowManager>(0xC40350);

void CRealTimeShadowManager::InjectHooks() {
    RH_ScopedClass(CRealTimeShadowManager);
    RH_ScopedCategory("Shadows");

    RH_ScopedInstall(Init, 0x7067C0);
    RH_ScopedInstall(ReInit, 0x706870, {.Reversed = false});
    RH_ScopedInstall(ReturnRealTimeShadow, 0x705B30);
    RH_ScopedInstall(GetRealTimeShadow, 0x706970, { .Reversed = false });
    RH_ScopedInstall(Update, 0x706AB0);
    RH_ScopedInstall(DoShadowThisFrame, 0x706BA0);
    RH_ScopedInstall(Exit, 0x706A60);
}

// 0x7067C0
void CRealTimeShadowManager::Init() {
    if (m_bInitialised) {
        return;
    }

    for (auto& shdw : m_apShadows) {
        shdw = new CRealTimeShadow();
#ifdef NOTSA_STANDALONE
        // Standalone: the shadow *cameras* are not created, only the pool. Why this is the safe cut and
        // not a blanket stub:
        //   * Update() walks m_apShadows twice WITHOUT consulting m_bInitialised (the loop at the top
        //     dereferences shdw->m_pOwner, and the trailing debug loop writes shdw->m_bKeepAlive). The
        //     original's invariant is therefore "Init allocated all 16 slots", so the pool must exist.
        //     With m_pOwner left null, every iteration `continue`s and only the keepAlive write happens -
        //     i.e. the subsystem is genuinely inert rather than half-built.
        //   * What is skipped is exactly `CRealTimeShadow::Create` (a directional light + its frame, plus
        //     the shadow camera + its frame and rasters) and the two manager cameras below. Those exist
        //     solely to render shadows, and the shadow render path is not bridged yet, so nothing can
        //     observe their absence except by not seeing a shadow.
        //   * Leaving them uncreated is what makes the boot survive at all: creating them is what trips a
        //     real librw dirty-list/objectList corruption (see MEMORY.md "librw frame dirty-list
        //     corruption" - diagnosed 2026-10-14, fix outstanding). The crash surfaced as
        //     `Frame::syncDirty` calling 0x3F800000 (a float, 1.0f) as an ObjectWithFrame::syncCB from
        //     CShadowCamera::MakeGradientRaster -> RwCameraBeginUpdate.
        // Degradation, stated plainly: no real-time shadows under peds/vehicles until that bug is fixed.
        (void)shdw;
#else
        shdw->Create(true, 4, true);
#endif
    }

#ifdef NOTSA_STANDALONE
    // See the note above: the blur/gradient cameras and the gradient raster are the same cut. The original
    // order is Create(blur), Create(gradient), MakeGradientRaster() - all skipped.
    NOTSA_LOG_WARN("Standalone: real-time shadows disabled (librw frame dirty-list corruption in the "
                   "shadow camera path); the shadow pool is allocated so Update() stays safe");
    return;
#else
    m_BlurCamera.Create(6);

    m_GradientCamera.Create(6);
    m_GradientCamera.MakeGradientRaster();

    m_bInitialised = true;
#endif
}

// 0x706A60
void CRealTimeShadowManager::Exit() { // AKA `Shutdown`
    if (!m_bInitialised) {
        return;
    }

    for (auto& shdw : m_apShadows) {
        delete std::exchange(shdw, nullptr);
    }

    // Nice hack
    m_BlurCamera.Destroy();
    m_GradientCamera.Destroy();

    m_bInitialised = false;
}

// 0x705B30
void CRealTimeShadowManager::ReturnRealTimeShadow(CRealTimeShadow* shdw) {
    if (m_bInitialised) {
        shdw->m_pOwner->m_pShadowData = nullptr;
        shdw->m_pOwner = nullptr;
    }
}

// 0x706870
void CRealTimeShadowManager::ReInit() {
    plugin::CallMethod<0x706870, CRealTimeShadowManager*>(this);
}

// 0x706AB0
void CRealTimeShadowManager::Update() {
    ZoneScoped;

    if (m_bInitialised && m_bNeedsReinit) {
        ReInit();
        m_bNeedsReinit = false;
    }

    for (const auto shdw : m_apShadows) {
        if (!shdw->m_pOwner) {
            continue;
        }

        assert(shdw->m_pOwner->m_pShadowData == shdw);

        // 0x305eed - 0x305f0f: Update intensity
        constexpr auto INTENSITY_STEP = 3u;
        if (shdw->m_bKeepAlive) {
            shdw->m_nIntensity = std::min<uint8>(100u, shdw->m_nIntensity + INTENSITY_STEP);
        } else { // Fade out
            shdw->m_nIntensity = std::max<uint8>(shdw->m_nIntensity, INTENSITY_STEP) - INTENSITY_STEP; // Avoids underflow
        }

        if (shdw->m_nIntensity) {
            shdw->Update();
            CShadows::StoreRealTimeShadow(
                shdw->m_pOwner,

                CTimeCycle::m_fShadowDisplacementX[CTimeCycle::m_CurrentStoredValue],
                CTimeCycle::m_fShadowDisplacementY[CTimeCycle::m_CurrentStoredValue],

                CTimeCycle::m_fShadowFrontX[CTimeCycle::m_CurrentStoredValue],
                CTimeCycle::m_fShadowFrontY[CTimeCycle::m_CurrentStoredValue],

                CTimeCycle::m_fShadowSideX[CTimeCycle::m_CurrentStoredValue],
                CTimeCycle::m_fShadowSideY[CTimeCycle::m_CurrentStoredValue]
            );
        } else if (m_bInitialised) {
            shdw->m_pOwner->m_pShadowData = nullptr;
            shdw->m_pOwner = nullptr;
        }
    }

    // TODO: ??? - Perhaps debug code left accidentally in?
    for (const auto shdw : m_apShadows) {
        shdw->m_bKeepAlive = false;
    }
}

CRealTimeShadow& CRealTimeShadowManager::GetRealTimeShadow(CPhysical* physical) {
    return plugin::CallMethodAndReturn<CRealTimeShadow&, 0x706970, CRealTimeShadowManager*, CPhysical*>(this, physical);
    /*
    * Unfinished
    if (m_bInitialised) {
        return;
    }

    bool isFirstPlayer{};

    if (!physical->GetIsTypePed() || physical->AsPed()->IsPlayer()) {
        if (FindPlayerPed()->IsInVehicle()) { // Maybe wrong?
            if (FindPlayerPed()->m_pVehicle->GetMoveSpeed().SquaredMagnitude() < sq(0.3f)) {
                return;
            }
        }
    }
    */
}

// 0x706BA0
void CRealTimeShadowManager::DoShadowThisFrame(CPhysical* physical) {
    switch (g_fx.GetFxQuality()) {
    case FX_QUALITY_VERY_HIGH: // Always render
        break;
    case FX_QUALITY_HIGH: { // Only draw for main player
        if (physical->GetIsTypePed()) {
            if (physical->AsPed()->m_nPedType == PED_TYPE_PLAYER1) {
                break;
            }
        }
        return;
    }
    default: // For any other quality: skip
        return;
    }

    if (const auto shdw = physical->m_pShadowData) {
        shdw->m_bKeepAlive = true;
    } else {
        (void)GetRealTimeShadow(physical); // ???
    }
}
