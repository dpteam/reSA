#include "StdInc.h"

/*
 * Backing storage for globals in the standalone (non-mod) build.
 *
 * See the declaration of `notsa::detail::GetStandaloneStaticStorage` in `Base.h` for the "why".
 *
 * NOTE: This file is compiled in every build, but only does something in a real standalone build
 *       (Ie. NOT `NOTSA_STANDALONE_DUMP_HOOKS_ONLY`, which doesn't care about the actual storage).
 */

#if defined(NOTSA_STANDALONE) && !defined(NOTSA_STANDALONE_DUMP_HOOKS_ONLY)

#include <mutex>

namespace {
/*!
 * @brief A single reverse-referenced global.
 */
struct Entry {
    //! Address of the variable in the original game. Zero marks an unused (empty) slot.
    uintptr Address;

    //! Our storage backing it. Never null for an occupied slot.
    void* Storage;
};

/*!
 * @brief Extremely simple open-addressing hash table mapping an original-game address to our storage.
 *
 * @details We deliberately avoid `std::unordered_map` (And any other allocating container / the C++ heap) here.
 *          `StaticRef` is used to initialize namespace-scope variables, ie. during static-initialization, so we
 *          want the storage to be independent of every allocator (both the CRT heap *and* the game's `CMemoryMgr`,
 *          which in the non-heap configuration forwards to `plugin::Call`, see `game_sa/MemoryMgr.cpp`).
 *          Everything is therefore allocated straight from the OS via `VirtualAlloc`.
 */
struct Registry {
    Entry*  Entries{};  //!< Hash table (Power-of-two sized). Empty slots have `Address == 0`.
    size_t  Capacity{}; //!< Size of `Entries` (Always a power of two)
    size_t  Count{};    //!< Number of occupied slots in `Entries`

    static constexpr size_t MIN_CAPACITY = 1024;

    //! Rehash when this many slots are occupied (75% load factor)
    bool NeedsGrow() const { return Count * 4 >= Capacity * 3; }
};

/*!
 * @note Both `Registry` and `std::mutex` are function-local statics on purpose:
 *       `StaticRef` is used to initialize namespace-scope variables, ie. during static-initialization,
 *       so we can't rely on any dynamically-initialized global being usable yet.
 */
Registry& GetRegistry() {
    static Registry registry{};
    return registry;
}

std::mutex& GetRegistryMutex() {
    static std::mutex mutex;
    return mutex;
}

/*!
 * @brief Allocate zero-initialized memory directly from the OS.
 * @note  `VirtualAlloc` returns 64KiB-aligned memory, which satisfies any alignment we could ever need.
 */
void* OSAlloc(size_t size) {
    return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void OSFree(void* memory) {
    if (memory) {
        VERIFY(VirtualFree(memory, 0, MEM_RELEASE));
    }
}

size_t HashAddress(uintptr address) {
    // Addresses share a lot of high bits, so mix them well before reducing to a table index.
    auto x = static_cast<uint64>(address);
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdull;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ull;
    x ^= x >> 33;
    return static_cast<size_t>(x);
}

/*!
 * @brief Find the slot for `address`.
 * @return An occupied slot for `address` if there is one, an empty slot otherwise.
 */
Entry* Probe(Registry& registry, uintptr address) {
    const auto mask = registry.Capacity - 1;
    auto idx = HashAddress(address) & mask;
    while (registry.Entries[idx].Address) {
        if (registry.Entries[idx].Address == address) {
            break;
        }
        idx = (idx + 1) & mask;
    }
    return &registry.Entries[idx];
}

void Grow(Registry& registry) {
    const auto oldEntries  = registry.Entries;
    const auto oldCapacity = registry.Capacity;
    const auto newCapacity = oldCapacity ? oldCapacity * 2 : Registry::MIN_CAPACITY;

    registry.Entries  = static_cast<Entry*>(OSAlloc(newCapacity * sizeof(Entry))); // Zeroed
    registry.Capacity = newCapacity;

    // Re-insert all previously occupied slots (Probe() relies on `Address`, not `Storage`, so just copy the pairs)
    for (size_t i = 0; i < oldCapacity; i++) {
        if (oldEntries[i].Address) {
            *Probe(registry, oldEntries[i].Address) = oldEntries[i];
        }
    }

    OSFree(oldEntries);
}
} // anonymous namespace

void* notsa::detail::GetStandaloneStaticStorage(uintptr addr, size_t size, size_t alignment) {
    std::scoped_lock lock{ GetRegistryMutex() };

    auto& registry = GetRegistry();

    if (!registry.Capacity || registry.NeedsGrow()) {
        Grow(registry);
    }

    if (Entry* const entry = Probe(registry, addr); entry->Address) {
        assert(entry->Address == addr);
        return entry->Storage;
    } else {
        static_cast<void>(alignment); // VirtualAlloc's 64KiB alignment always suffices
        void* const storage = OSAlloc(size);
        VERIFY(storage);

        *entry = Entry{ addr, storage };
        registry.Count++;

        return storage;
    }
}

#endif // defined(NOTSA_STANDALONE) && !defined(NOTSA_STANDALONE_DUMP_HOOKS_ONLY)
