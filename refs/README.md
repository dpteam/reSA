# refs/ — external reference sources

This folder is for **third-party source code we only *read***: other reversals / open-source reimplementations
used as a reference when turning this project into a standalone game.

> ⚠️ **Everything in here is ignored by git** (see `.gitignore`: `/refs/*`, except this file).
> Nothing in `refs/` may ever be committed, and **no code may be copy-pasted from it into `source/`** —
> these projects have their own licenses (often non-commercial / copyleft) which are **not** compatible with
> this repository. Use them to *understand architecture and behavior*, then write our own code.

## How to populate it

```powershell
# re3 (GTA III) - standalone game, `master` branch
git clone --depth 1 https://github.com/GTAmodding/re3 refs/re3

# reVC (GTA Vice City) - standalone game, `miami` branch of the same repo
git clone --depth 1 --branch miami https://github.com/GTAmodding/re3 refs/reVC

# librw - RenderWare reimplementation used by re3/reVC (MIT-ish licensed)
git clone --depth 1 https://github.com/aap/librw refs/librw
```

(The user may also just drop a folder in here manually — that works too; it's ignored either way.)

## Why these, and what to look at

| Ref | What it's good for | Watch out for |
|---|---|---|
| `refs/re3` (III), `refs/reVC` (miami) | The **closest existing thing to what we want**: complete standalone games (their own `WinMain`, no original exe), with a working platform/RW/memory/file-system layer. | Different game (III/VC ≠ SA). Class layouts, addresses and logic differ — borrow *architecture*, not code. |
| `refs/librw` | An actual **RenderWare implementation**. We need one: our RW calls are absolute-address calls into `gta_sa.exe` (eg. `((void(*)(...))0x809F90)(...)` in `source/game_sa/RenderWare/rw/rwcore.cpp`), which don't exist in a standalone build. | Different RW version than SA's; needs verifying per call. |

### Specific threads to pull (see `MEMORY.md` §5–6)
1. **Entry point / startup order** — how re3 boots: windowing, RW init, video mode, input, then game init.
   Compare with our `NOTSA_WinMain` (`source/app/platform/win/WinMain.cpp`).
2. **Memory manager** — how re3 does `CMemoryMgr`-style allocation and global `new/delete`, since our global
   `operator new` routes into `CMemoryMgr` even in standalone (our `MEMORY.md` §5.2).
3. **RenderWare** — `librw` vs. our `source/game_sa/RenderWare/**` headers; what's needed to actually *provide* RW.
4. **Platform layer** — `oswrapper/` here vs. re3's `src/platform/**` (windows) for window/message loop/input.
5. **Asset/file system** — where and how the two expect `models/`, `data/`, etc.

## Rules for using `refs/`
* Read-only. Never `#include` anything from `refs/`, never link against it.
* Never copy code verbatim. Reimplement in our own style per `docs/CodingGuidelines.md`.
* If a reference answers an open question, write the finding (and where you saw it) into `MEMORY.md` —
  the reference folder may be absent on someone else's machine, the knowledge must not be.
