# Vendored third-party libraries

These are **third-party** libraries copied into the repo (rather than fetched), so the build is reproducible and
network-independent. They are **not** part of our reversed code.

> ⚠️ Do **not** treat anything here as project code. Do not edit these files unless absolutely unavoidable — if you do,
> document why (so a re-vendor doesn't silently drop the change).
>
> NOTE: this lives **outside `source/`** on purpose — `source/CMakeLists.txt` globs every source file under `source/`,
> which would otherwise pull these (and their `.hlsl` files, which MSBuild would try to compile with FXC) into the main
> target.

## librw

* **What:** an open-source (partial) re-implementation of RenderWare Graphics — the engine we need for the standalone
  build (the original RW lives inside `gta_sa.exe`, which the standalone build doesn't have).
* **Upstream:** <https://github.com/aap/librw>
* **Vendored commit:** `18532c2e13efbc1aa43f0b00b4459831f9414de0` (branch `master`).
* **License:** MIT — see `librw/LICENSE` (compatible with this repo; attribution kept in that file).
* **What was copied:** everything except `skeleton/`, `tools/*` (only `tools/CMakeLists.txt`, a no-op stub that the
  build requires), `.github/`, CI files and the `premake*` scripts. Platform sources for all backends are kept because
  librw's build compiles them unconditionally; we only *use* the **D3D9** backend.
* **How it's used:** configured as a CMake subproject **only in the standalone build**
  (`if (${RE_PROJECT}_STANDALONE)` in `source/CMakeLists.txt`, via `add_subdirectory("${CMAKE_SOURCE_DIR}/extern/librw"
  "${CMAKE_BINARY_DIR}/extern/librw")`), with `LIBRW_PLATFORM=D3D9` (forced). The mod/ASI build does not build or link
  it.
* **To update:** `git clone --depth 1 https://github.com/aap/librw`, replace `librw/` with the new tree (re-applying the
  same exclusions), re-check that librw's `struct`s still line up with our shim, and update the commit hash here.
