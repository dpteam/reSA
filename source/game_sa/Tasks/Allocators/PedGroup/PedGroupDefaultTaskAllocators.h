#pragma once

#include <reversiblehooks/ReversibleHooks.h>

#include "./PedGroupDefaultTaskAllocator.h"
#include "./PedGroupDefaultTaskAllocatorType.h"
#include "./PedGroupDefaultTaskAllocatorFollowAnyMeans.h"
#include "./PedGroupDefaultTaskAllocatorFollowLimited.h"
#include "./PedGroupDefaultTaskAllocatorStandStill.h"
#include "./PedGroupDefaultTaskAllocatorChat.h"
#include "./PedGroupDefaultTaskAllocatorSitInLeaderCar.h"
#include "./PedGroupDefaultTaskAllocatorRandom.h"

class CPedGroupDefaultTaskAllocators {
private:
    static inline const auto& ms_FollowAnyMeansAllocator = StaticRef<CPedGroupDefaultTaskAllocatorFollowAnyMeans>(0xC09908);
    static inline const auto& ms_FollowLimitedAllocator  = StaticRef<CPedGroupDefaultTaskAllocatorFollowLimited>(0xC0990C);
    static inline const auto& ms_StandStillAllocator     = StaticRef<CPedGroupDefaultTaskAllocatorStandStill>(0xC09910);
    static inline const auto& ms_ChatAllocator           = StaticRef<CPedGroupDefaultTaskAllocatorChat>(0xC09914);
    static inline const auto& ms_SitInLeaderCarAllocator = StaticRef<CPedGroupDefaultTaskAllocatorSitInLeaderCar>(0xC0991C);
    static inline const auto& ms_RandomAllocator         = StaticRef<CPedGroupDefaultTaskAllocatorRandom>(0xC09918);

public:
    static const CPedGroupDefaultTaskAllocator& Get(ePedGroupDefaultTaskAllocatorType type) {
        using enum ePedGroupDefaultTaskAllocatorType;
        switch (type) {
        case FOLLOW_ANY_MEANS:  return ms_FollowAnyMeansAllocator;
        case FOLLOW_LIMITED:    return ms_FollowLimitedAllocator;
        case STAND_STILL:       return ms_StandStillAllocator;
        case CHAT:              return ms_ChatAllocator;
        case SIT_IN_LEADER_CAR: return ms_SitInLeaderCarAllocator;
        case RANDOM:            return ms_RandomAllocator;
        default:                NOTSA_UNREACHABLE();
        }
    }

#ifdef NOTSA_STANDALONE
    /*!
     * @brief Construct the six static allocator objects, which the original's CRT used to build for us.
     *
     * @details These are the "polymorphic statics" failure mode: each is a stateless, vtable-only object
     *          (VALIDATE_SIZE == sizeof(void*), the headers even say "vtable only") living at a fixed
     *          address. In the ASI build the original exe has already run its static initialisers, so the
     *          vtable pointer is in place; the standalone build zero-fills the statics region, so the
     *          pointer is null and the first virtual call goes to garbage. Measured:
     *          CreatePlayer -> CPlayerPed ctor -> SetDefaultTaskAllocatorType ->
     *          SetDefaultTaskAllocator -> `m_DefaultTaskAllocator->AllocateDefaultTasks(...)` faulted.
     *
     *          Constructing them is enough - they hold no data, so there is nothing else to initialise.
     *          Called from InitGameGlobalStatics() before the game loop.
     */
    static void SeedStaticAllocators() {
        notsa::ConstructStatic(const_cast<CPedGroupDefaultTaskAllocatorFollowAnyMeans&>(ms_FollowAnyMeansAllocator));
        notsa::ConstructStatic(const_cast<CPedGroupDefaultTaskAllocatorFollowLimited&>(ms_FollowLimitedAllocator));
        notsa::ConstructStatic(const_cast<CPedGroupDefaultTaskAllocatorStandStill&>(ms_StandStillAllocator));
        notsa::ConstructStatic(const_cast<CPedGroupDefaultTaskAllocatorChat&>(ms_ChatAllocator));
        notsa::ConstructStatic(const_cast<CPedGroupDefaultTaskAllocatorRandom&>(ms_RandomAllocator));
        notsa::ConstructStatic(const_cast<CPedGroupDefaultTaskAllocatorSitInLeaderCar&>(ms_SitInLeaderCarAllocator));
    }
#endif

public:
    static void InjectHooks() {
        RH_ScopedClass(CPedGroupDefaultTaskAllocators);
        RH_ScopedCategory("Tasks/Allocators/PedGroup");

        //RH_ScopedInstall(Get, 0x5F7200, { .State = HS::RedirectToGTA, .Locked = true }); // Spoils <ecx>, can't hook
    }
};
