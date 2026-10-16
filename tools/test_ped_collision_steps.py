"""Compile the standalone collision-step body with small test doubles (no game/assets needed).

The engine build separately checks the real member offsets via static_assert. This harness extracts the
production function rather than copying its arithmetic, then checks the 1.0 US branch/rounding contract.
Run: python tools/test_ped_collision_steps.py [--cxx path/to/g++]
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default=shutil.which("g++"))
    args = parser.parse_args()
    if not args.cxx:
        parser.error("g++ not found; pass --cxx explicitly")

    root = Path(__file__).resolve().parents[1]
    source = (root / "source/game_sa/Entity/Ped/Ped.cpp").read_text(encoding="utf-8")
    start = source.index("uint8 CPed::SpecialEntityCalcCollisionSteps(")
    end = source.index("\n}\n", start) + 3
    body = source[start:end]
    # The test double intentionally has no SA layout. Only these two assertions are omitted; the engine
    # compile validates them against real CPed/CPhysical. All production control flow stays unchanged.
    body, assertions = re.subn(r"^\s*static_assert\(offsetof\([^\n]+\n", "\n", body, flags=re.MULTILINE)
    if assertions != 2:
        raise RuntimeError("Production offset checks changed; review the test harness")

    driver = r'''
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "extensions/CommandLine.h"
using uint8 = uint8_t;
using int32 = int32_t;
struct CTimer {
    static inline float step = 1.0f;
    static float GetTimeStep() { return step; }
};
struct Vector {
    float x{}, y{}, z{};
    float SquaredMagnitude() const { return x*x + y*y + z*z; }
};
struct CPed {
    void* m_pAttachedTo{};
    void* m_pPlayerData{};
    void* m_standingOnEntity{};
    Vector m_vecMoveSpeed{};
    float m_fElasticity = 0.5f;
    uint8 SpecialEntityCalcCollisionSteps(bool&, bool&);
};
'''
    driver += body
    driver += r'''
int main() {
    struct Case {
        float speed, step;
        bool player, standing, attached;
        unsigned result;
        float elasticity;
    };
    const Case cases[] = {
        {0,    1, false, false, false, 1, .5f}, // slow NPC early exit
        {.29f, 1, false, false, false, 1, .5f},
        {.31f, 1, false, false, false, 2, 1.f}, // ceil(1.55), not floor
        {.61f, 1, false, false, false, 4, 1.f},
        {2.1f, 1, false, false, false, 11, 1.f},
        {0,    1, true, false, false, 2, .5f}, // player minimum without support
        {0,    1, true, true,  false, 4, .5f}, // support minimum is FOUR, not two
        {.91f, 1, true, false, false, 4, .5f}, // ceil(.91 * 10/3)
        {.91f, 1, true, true,  false, 7, .5f}, // ceil(.91 * 20/3)
        {.91f, 2, true, true,  false, 13, .5f},
        {2.1f, 1, true, false, true, 1, .5f}, // attachment bypasses all work
        {2.1f, 1, false, true, true, 1, .5f},
    };
    for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        const auto& c = cases[i];
        CPed p;
        p.m_vecMoveSpeed = {c.speed, 0, 0};
        p.m_pPlayerData = c.player ? &p : nullptr;
        p.m_standingOnEntity = c.standing ? &p : nullptr;
        p.m_pAttachedTo = c.attached ? &p : nullptr;
        CTimer::step = c.step;
        bool before = true, half = false;
        const auto got = p.SpecialEntityCalcCollisionSteps(before, half);
        if (got != c.result || p.m_fElasticity != c.elasticity || !before || half) {
            std::fprintf(stderr, "case %u: steps=%u expected=%u elasticity=%f expected=%f\n",
                         i, unsigned(got), c.result, p.m_fElasticity, c.elasticity);
            return 1;
        }
    }
    CommandLine::Probe probe;
    if (probe.HasReachedLimit()) return 2;
    probe.QuitAfterFrames = 400;
    probe.Frames = 400;
    if (!probe.HasReachedLimit()) return 3;
    probe.QuitAfterGameFrames = 3;
    if (probe.HasReachedLimit()) return 4; // Spinning 400 times must not satisfy the game-frame limit.
    probe.GameIdleCalls = 2;
    if (probe.HasReachedLimit()) return 5;
    probe.GameIdleCalls = 3;
    if (!probe.HasReachedLimit()) return 6;
    std::puts("PASS: 12 collision-step cases and 5 probe-limit checks");
}
'''
    # Use the harness-approved scratch directory when running in the project environment.
    scratch = Path.home() / "AppData/Local/Temp/opencode"
    with tempfile.TemporaryDirectory(prefix="resa-steps-", dir=scratch if scratch.is_dir() else None) as temp:
        cpp = Path(temp) / "steps.cpp"
        exe = Path(temp) / "steps.exe"
        cpp.write_text(driver, encoding="utf-8")
        subprocess.run([args.cxx, "-std=c++20", "-DNOTSA_STANDALONE", "-I", str(root / "source"),
                        str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
