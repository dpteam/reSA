# AGENT.md — Working rules for AI agents & contributors

This file is the rulebook for **anyone** (human or AI) making changes to this repository.
Read it before touching code. It is intentionally short — the details live in the documents it links to.

> Goal of this fork: **turn the reverse-engineered code from a *mod* (an ASI/DLL injected into the original
> `gta_sa.exe`) into a *standalone game* (its own `.exe` with `WinMain`, no original executable required).**
> Everything below serves that goal, but the mod build must keep working at all times.

---

## 1. Documents you must keep up to date

| File | Purpose | When to update |
|---|---|---|
| `AGENT.md` | This rulebook. | When process/rules change. |
| `MEMORY.md` | Living knowledge: architecture, gotchas, verified facts, blockers, roadmap. | Whenever you learn something non-obvious. |
| `HISTORY.md` | Append-only changelog of work done by agents/sessions. | At the end of every work session. |
| `RESUME.md` | Snapshot for restoring context after compaction / a new session. | At the end of every work session. |
| `PROGRESS.md` | Step-by-step progress towards the fork's main goal ("get the game to run"). | When the boot chain advances. |
| `IDEAS.md` | Backlog / raw ideas (features, porting upstream PRs, task to-dos). Nothing here is committed work. | When new ideas appear or something moves into work. |
| `README.md` | User-facing build/usage docs. | When build/usage changes. |
| `PROMPT.md` | The user's own high-level prompt for the agent. | When the direction/priorities change. |
| `docs/CodingGuidelines.md` | The upstream coding standard. **Authoritative for code style.** | As upstream. |
| `docs/HookRegistration.md` | Rules for registering reversed functions. | As upstream. |
| `docs/ai/TipsAndTricks.md` | Aggregated reversing knowledge base. | Append when you discover something. |
| `refs/` | Read-only third-party reference sources (re3/reVC/librw). **Gitignored.** See `refs/README.md`. | When you learn something from them, copy the *knowledge* into `MEMORY.md`. |

Never delete a rule/document without replacing it. Prefer **appending** to `MEMORY.md`/`HISTORY.md`.

---

## 2. Hard rules

1. **Never break the mod build.** The ASI/DLL build must remain identical in behavior.
   * New standalone-only code goes behind `#ifdef NOTSA_STANDALONE` (or the CMake option `GTASA_STANDALONE`).
   * Don't change `StaticRef`/hook semantics for the mod build.
2. **Respect the upstream guidelines** (`docs/CodingGuidelines.md`, `docs/HookRegistration.md`). Match the
   surrounding style, naming, and file layout.
3. **One subject per change/commit.** Don't mix refactors with features.
4. **Never commit build artifacts** (`bin/`, `*.dll`, `*.pdb`, `build/`). They are gitignored.
5. **Never invent memory addresses.** Addresses come from the reversing effort; copy them from `docs/hooks.json`
   or existing code. If unsure, mark a `TODO` and write it in `MEMORY.md`.
6. **Document verification honestly.** If you could not build/test (see §4), say so explicitly in the commit body
   and in `RESUME.md`. Never claim it "works" without a build.
7. **Keep the docs in §1 in sync** — an undocumented session is a lost session.
8. **`refs/` is read-only and gitignored.** Use it to understand architecture/behavior, never copy code from it
   (different, incompatible licenses) and never `#include`/link against it. Findings go into `MEMORY.md`.

---

## 3. Architecture in 60 seconds

```
source/
  dllmain.cpp                 Entry point of the MOD build (DllMain). No-op in standalone.
  Bootstrap.{h,cpp}           Startup shared by both entry points (INI/config loading).
  StandaloneStorage.cpp       Backing storage for the game's globals in the standalone build.
  Base.h                      StaticRef() — how the game's globals are accessed. Core of the conversion.
  InjectHooksMain.cpp         Registers *all* reversed functions (via RH_Scoped* macros).
  reversiblehooks/            The hooking framework (RHManager, TwoWayHook, NullHook, ...).
  extensions/                 Cross-cutting helpers (CommandLine, Configuration, Configs, debug, ...).
  app/                        The (mostly reversed) game application layer.
    platform/win/WinMain.cpp  WinMain + NOTSA_WinMain (the reversed game main); standalone entry point.
  game_sa/                    The reversed game classes.
toolsmenu/ , oswrapper/ , ...
```

Two build modes (CMake option `GTASA_STANDALONE`):

* **Mod (default, `OFF`)** → `gta_reversed.asi`. Injected into the original game; `DllMain` runs, real hooks
  redirect the game's functions to our reversed ones.
* **Standalone (`ON`)** → `reSA.exe`. Our own `WinMain` runs; the hooks become documentation-only
  (`NullHook`) and the reversed code must be called directly. ⚠️ **Work in progress** — see `MEMORY.md`.

Related: `GTASA_STANDALONE_DUMP_HOOKS_ONLY` (`ON`) builds an exe that only dumps hook metadata (used by CI).

---

## 4. Building & verification

Full instructions: `README.md`. Short version (Windows):

```powershell
conan profile detect        # once
python setup.py             # configure (windows-msvc x86 Debug by default)
python setup.py --build     # build
```

Requirements: Visual Studio 2026, Python 3, Conan 2.x, CMake ≥ 4.2.

> **⚠️ Important for agents working in restricted environments:**
> If `cmake`/`conan`/`cl` are **not** available (check with `cmake --version`), you **cannot compile**.
> In that case:
> * Make changes extra carefully and conservatively, favor small diffs and existing patterns.
> * Do **not** claim the code compiles. State clearly that the build was not run.
> * Add a note to `RESUME.md` under "Unverified changes".

---

## 5. Workflow with git

* Branch off `master`: `feature/standalone-...`, `fix/...`, `docs/...`.
* Commit messages: [Conventional Commits](https://www.conventionalcommits.org/) —
  `feat(standalone): ...`, `fix(MemoryMgr): ...`, `docs: ...`, `refactor: ...`, `chore: ...`.
* Commit body: **what** changed, **why**, and **how it was verified** (or that it wasn't).
* `origin` = `dpteam/reSA` (this fork), `upstream` = `gta-reversed/gta-reversed`.
* Do not push to `upstream`. PRs to upstream must be clean of standalone-specific work unless agreed upon.

---

## 6. Definition of "done" for a change

- [ ] Change is scoped to a single subject.
- [ ] Mod build is unaffected (or the impact is explained and isolated).
- [ ] Style matches `docs/CodingGuidelines.md`.
- [ ] Build/run verified **if a toolchain is available**; otherwise flagged as unverified.
- [ ] `HISTORY.md` and `RESUME.md` updated; `MEMORY.md` updated if knowledge changed.
