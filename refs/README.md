# refs/ — external reference sources

This folder is for **third-party source code we only *read***: other reversals / open-source reimplementations
used as a reference when turning this project into a standalone game.

> ⚠️ **Everything in here is ignored by git** (`.gitignore`: `/refs/*`, except this file).
> Nothing in `refs/` may ever be committed, and **no code may be copy-pasted from it into `source/`** —
> these projects have their own licenses (often non-commercial / copyleft) which are **not** compatible with
> this repository. Use them to *understand architecture and behavior*, then write our own code.
>
> Third-party folders here are their own git repos; **don't** add them as submodules of this repo.

## What's here (populated 2026-10-11)

| Path | Repo / branch | Why |
|---|---|---|
| `refs/re3` | [`rocketguedes/re3`](https://github.com/rocketguedes/re3) @ `master` | GTA III — standalone game, same engine family |
| `refs/reVC` | [`rocketguedes/re3`](https://github.com/rocketguedes/re3) @ `miami` | GTA Vice City — same, newer |
| `refs/librw` | [`aap/librw`](https://github.com/aap/librw) @ `master` | RenderWare reimplementation |

> Note: the original `GTAmodding/re3` was DMCA'd and `master`/`miami` no longer exist there — hence the `rocketguedes`
> mirror, which has exactly `master` (III), `miami` (VC) and `lcs`.

To re-create (or update) them:

```powershell
git clone --depth 1 --single-branch --branch master https://github.com/rocketguedes/re3 refs/re3
git clone --depth 1 --single-branch --branch miami  https://github.com/rocketguedes/re3 refs/reVC
git clone --depth 1 --single-branch --branch master https://github.com/aap/librw      refs/librw
```

(The user may also just drop folders in here manually — it's ignored either way.)

## Findings so far (details in `MEMORY.md` §3, §5b)

1. **re3/reVC are the closest prior art.** Same boot flow, same subsystem names (`MemoryMgr`, `MemoryHeap`,
   `TxdStore`, `RwHelper`, `VisibilityPlugins`). Their `src/skel/win/win.cpp: WinMain` mirrors our `NOTSA_WinMain`.
2. **RenderWare = librw + a compat shim.** re3 does *not* rewrite its `Rw*` call sites — it keeps the classic C API and
   maps it onto librw's `rw::` C++ API with `src/fakerw/*.{h,cpp}` (~2100 lines: `typedef rw::Raster RwRaster;`,
   `RwCameraClear` → `rw::Camera::clear`, ...). **We can do the same**, which avoids touching every RW call site.
3. **librw is a C++ (`rw::`) API only.** `aap/librw` has just `master`; the C API ended at commit `14c773d`
   (2019-08-04). re3/reVC pin librw at **`5501c4fd…` (2021-08-19)**. So expect to write the shim, not to get a
   drop-in `RwCameraClear`.
4. **Memory manager is opt-in in re3.** `USE_CUSTOM_ALLOCATOR` OFF ⇒ `MemoryMgrMalloc` just calls `malloc`;
   ON ⇒ `WinMain` calls `InitMemoryMgr()` first. Directly relevant to our global-`operator new` blocker.
5. **librw is initialized with** `RwEngineInit(memFuncs, 0, rsRESOURCESDEFAULTARENASIZE)` where
   `memFuncs = { Malloc, Free, Realloc, Calloc }` — that's how it gets our allocator.

## Rules for using `refs/`
* Read-only. Never `#include` anything from `refs/`, never link against it.
* Never copy code verbatim. Reimplement in our own style per `docs/CodingGuidelines.md`.
* If a reference answers an open question, write the finding (and where you saw it) into `MEMORY.md` — the folder
  may be absent on someone else's machine, the knowledge must not be.
