#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace CommandLine {
    // Debug features
    extern bool s_WaitForDebugger;

#ifdef NOTSA_STANDALONE
    /*!
     * @brief Unattended probe harness: lets the engine be booted and driven without a human at the
     *        keyboard, so the load path can be iterated autonomously.
     *
     * @details Motivation: every fix to the loading path previously needed a manual "START GAME" click,
     *          which made the loop interactive and slow. The harness supplies the three things a
     *          non-interactive test needs, and nothing else:
     *            * drive New Game            (--autostart)
     *            * terminate deterministically (--quit-after=N)
     *            * report progress numerically (--dump-progress)
     *
     *          Design follows the field guide's §4 ("instrument before you experiment"): the yardstick is
     *          a counter, not a screenshot. Kept deliberately small - no window-less mode yet, because
     *          the render device is created during boot and detaching it is a separate job; the window is
     *          simply never touched. All state lives here rather than in a new TU because `source/`
     *          is GLOB_RECURSE'd, so a new file needs a CMake re-configure to be picked up.
     *
     *          Thread note: the counters are written from the main thread only (LoadLevel / Init2 /
     *          the frame loop), so they are plain ints rather than atomics.
     */
    struct Probe {
        // ---- settings (command line) --------------------------------------------------------
        bool AutostartNewGame = false;   //!< --autostart    : press "START GAME" ourselves, no click needed
        int  QuitAfterFrames  = 0;       //!< --quit-after=N : leave the loop after N frames; 0 = run forever
        int  QuitAfterGameFrames = 0;    //!< --game-frames=N: completed in-game rsIDLE calls; overrides --quit-after
        int  IdleFramesBeforeStart = 30; //!< --idle-frames=N: how long to let the frontend settle before autostarting
        bool WantProgress     = false;   //!< --dump-progress: print the machine-readable summary on the way out

        // ---- observed state ------------------------------------------------------------------
        int  Frames           = 0;       //!< outer loop iterations (including frame-limiter skips)
        int  GameIdleCalls    = 0;       //!< completed rsIDLE dispatches in GAME_STATE_IDLE
        int  IdleFrames       = 0;       //!< consecutive frames spent in FRONTEND_IDLE
        int  NumFileMarkers   = 0;       //!< `Loading the Game -> Loading <file>` calls seen
        int  NumPhaseMarkers  = 0;       //!< `LoadingScreen(<title>, <phase>)` calls seen
        char LastFileMarker[128]{};      //!< most recent file marker, for the summary
        char LastPhaseMarker[128]{};     //!< most recent phase marker
        bool ReachedGameLoop  = false;   //!< the frontend handed over to the world (i.e. loading finished)

        // ---- recording ----------------------------------------------------------------------
        void RecordFile(const char* name);
        void RecordPhase(const char* title, const char* phase);
        bool HasReachedLimit() const {
            return QuitAfterGameFrames > 0 ? GameIdleCalls >= QuitAfterGameFrames
                                          : QuitAfterFrames > 0 && Frames >= QuitAfterFrames;
        }
        //! Prints the summary (only when --dump-progress was given). Safe to call more than once.
        void DumpProgress(const char* why) const;
    };

    extern Probe s_Probe;
#endif

    void Load(int argc, char** argv);

    fs::path GetExePath();
    void     PostHooksInjected();
}; // namespace CommandLine;
