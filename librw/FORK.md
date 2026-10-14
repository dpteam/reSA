# librw — our fork (standalone RenderWare)

This is a **fork of [aap/librw](https://github.com/aap/librw)** (MIT — see `LICENSE`) that we treat as
**our own code**: edit it freely.

> **Why:** the standalone build has no real RenderWare (it lives inside `gta_sa.exe`), so we use librw for
> the `Rw*` API. librw has some bugs / missing pieces we need to fix ourselves (e.g. 16-bit font textures
> not sampling → no text). Keeping it here (instead of treating it as an untouchable third-party module)
> lets us patch it directly.

## Provenance

* **Upstream:** <https://github.com/aap/librw>
* **Base commit:** `18532c2e13efbc1aa43f0b00b4459831f9414de0` (`master`) — everything was copied except
  `skeleton/`, `.github/`, CI and the `premake*` scripts. `tools/CMakeLists.txt` is a no-op stub the build
  needs.
* **License:** MIT (kept in `LICENSE`).

## Our modifications (keep this list updated!)

1. `src/d3d/rwd3d9.h` + `src/d3d/d3d9.cpp`: added `rw::d3d9::getD3D9Caps()` (exposes the cached `D3DCAPS9`
   as `void*`; the game's `RwD3D9GetCaps()` needs it).
2. `src/frame.cpp`: **`Frame::create()` now self-links `inDirtyList`.** It used to initialise `objectList`
   but leave `inDirtyList` as raw allocator bytes, while `Frame::destroy()` / `destroyHierarchy()`
   conditionally do `inDirtyList.remove()` when `HIERARCHYSYNC` is set — so a frame carrying that bit
   without having been queued wrote through uninitialised pointers. Self-linking makes such a remove an
   exact no-op.
3. `src/frame.cpp`: **`cloneRecurse()` now clears the dirty-state bits on every clone.** It copies
   `old->object` via `Object::copy()`, which copies `privateFlags` — and for a Frame those are
   `HIERARCHYSYNCLTM/OBJ` + `SUBTREESYNCLTM/OBJ`, i.e. bookkeeping about membership of the *dirty list*,
   which a fresh clone provably is not in. `cloneAndLink()` already cleared them for the hierarchy root
   only; the children were missed. Together with (2) this removes a silent-wild-write class. Neither one
   turned out to be the shadow-camera corruption they were found alongside — see `IDEAS.md`, entry
   "ОТКРЫТЫЙ БАГ: порча frameDirtyList/objectList".
4. _…add here as we fix things (font textures, viewport, …)._

## Notes

* **Don't** change librw's public API wildly — our bridge (`source/game_sa/RenderWare/RwBridge.{h,cpp}`) is
  the only TU that includes `<rw.h>`, and the game's `Rw*` thunks call into it.
* Built as a CMake subproject **only in the standalone build** (`LIBRW_PLATFORM=D3D9`), via
  `add_subdirectory(librw)` in `source/CMakeLists.txt`.
* Reference (read-only, not linked): `refs/librw` (same base revision) — useful for diffs.
