# Door work stage 3: StreamBehindLoadingScreen (2026-10-22, branch exp/stream-behind-loading)

Design: notes/design-stream-behind-loading.md. Code: `src/OblivionPerfFix/src/loading/stream_behind_loading.cpp` on
branch `exp/stream-behind-loading` (from exp/door-diag, from work @ 778da68, upstream `6deebc2`); fork-local, ini key
`[Fixes] StreamBehindLoadingScreen` (default 0; not in the shipped ini); self-test passes (sites located at the
expected addresses). Not merged.

Conditions: clean (SCSKiller app closed + proxy renamed, Ghidra off, monitor on); the experiment build installed for
these runs only (original DLL + ini restored after, SHA-256 identical). Every arm has DoorDiag on (timing) and the
HitchLedger. Cheydinhal `inn-door` + `chapel-door`, fresh launches, two variants in alternating order (AB BA ...).
Tools: `mytools/door_stream_metrics.py` (stream-all wait from DoorDiag; loading-screen length = transition freeze
start -> the fade-in's stream-all; on-screen arrival gap = largest PresentMon interval from 400 ms before to 100 ms
after that stream-all, by QPC), `mytools/Leg-Sheet.ps1`.

## What the fix does (final: mode 4)
When the transition arms its fade gate (detour of `fn_4882730`), and from then on once async loading is idle and no
travel is pending (the gate's own pure queries), it sets the texture pool size the game stored at loading start back
(the fade handler's own call and value) and runs the streaming manager's non-blocking update once per frame until
fade-to-game. The game's gate, fade-to-game and blocking stream-all are unchanged; the stream-all finds nothing left.

## How the trigger was found (smoke runs, native, one launch each)
| mode | start | result |
|---|---|---|
| 1 | when the gate's three checks pass (evaluated each frame) | no lead time: the checks only turn true at the gate's own tick (its first call, `fn_3873F60`, advances level streaming during the freeze) |
| 2 | when async loading is idle (transition known only from the gate tick) | the same |
| 3 | when the gate is armed (heavy loading over; first gate tick ~0.5 s later) | stream-all wait 300 -> ~1-10 ms; but the first exit's loading screen was 0.45 s longer (one gate period) in 2 of 5 native runs |
| **4** | **after arming, once async loading is idle** | **same wait reduction; gate misses no more often than without the fix** |

## Results (5 + 5 per resolution; medians, range)
**Native 5120x2160 (`native-nofg`)**

| transition | stream-all wait F -> S4 | on-screen arrival gap F -> S4 | freeze -> fade F -> S4 |
|---|---|---|---|
| first load into the world | 107 -> 0.2 ms | - | - |
| inn exit (to the exterior) | 310 (274-354) -> 8.4 (1.0-10.6) ms | **539 (531-594) -> 230 (225-241) ms** | 5.59 (5.57-6.08) -> 5.60 (5.56-6.07) s |
| inn entry (interior) | 150 (139-167) -> 10.9 (10.5-11.3) ms | **225 (216-232) -> 75 (75-78) ms** | 2.90 -> 2.88 s |
| second exit | 299 (274-594) -> 8.4 (6.9-8.7) ms | **459 (442-531) -> 174 (170-184) ms** | 3.41 -> 3.43 s |
| whole-GPU memory, max per run | 11.67 GB (11.46-13.16) | 11.47 GB (11.23-11.65) | |

**720p (`cpu`)**

| transition | stream-all wait F -> S4 | on-screen arrival gap F -> S4 | freeze -> fade F -> S4 |
|---|---|---|---|
| first load | 94 -> 8.1 ms | - | - |
| inn exit | 258 -> 8.9 ms | **501 (477-507) -> 226 (214-235) ms** | 5.58 (5.55-6.08) -> 5.56 (5.56-6.04) s |
| inn entry | 97 -> 10.8 ms | **163 (159-171) -> 73 (72-73) ms** | 2.90 -> 2.90 s |
| second exit | 267 -> 7.8 ms | **432 (427-451) -> 167 (165-174) ms** | 3.47 -> 3.49 s |
| whole-GPU memory, max per run | 9.22 GB | 9.30 GB | |

**Loading-screen length:** medians unchanged at both resolutions. Runs where the first exit took one extra gate period
(~6.05 s instead of ~5.58 s): fix only 2 of 15 (both blocks + the 720p block), mode 4 2 of 10, mode 3 2 of 5. Mode 4 is
not distinguishable from the fix; mode 3 looked worse (small numbers).

**Image:** visual pass (`small`, 7 views incl. inn room and chapel nave): fix-A vs mode-4-A "same" except west road,
which equally differs fix-A vs fix-B (a weather/wind event in fix-A, as in Phase 5); fix-B vs mode-4-B and mode-4-A vs
mode-4-B all "same". First seconds after the fade-in (screenshots every 100 ms during the inn exit, native, one run
each; captures/bench/_compare/legs-sbl/): the same sequence (loading screen, black, fade-in) and equally sharp
textures at every point; only NPC positions differ. **Identical within run-to-run noise.**

## City gate and fast travel (native-nofg, 3 + 3 fresh launches, order F S S F F S; p5sblt-*)
Scenario `transitions`, scenes `cheydinhal-gate-ft` (west gate in / out / in / out) and `fast-travel` (map travel to
Cheydinhal, Weye, Leyawiin, the Imperial City), DoorDiag + HitchLedger on both sides, mode-4 build (B8041E7B3C9A).
F = StreamBehindLoadingScreen 0, S = 4. Medians (range); 48 transitions per side.

| Transition | stream-all wait F -> S, ms | arrival gap F -> S, ms | freeze -> fade F -> S, s |
|---|---|---|---|
| gate in | 255 (250-301) -> 9.4 (8.2-10.3) | 437 (436-471) -> 180 (180-181) | 3.77 -> 3.73 |
| gate out | 207 (158-210) -> 3.3 (2.3-3.6) | 433 (359-488) -> 213 (203-221) | 4.13 -> 4.15 |
| gate in 2 | 248 (205-264) -> 8.7 (7.1-8.7) | 420 (374-440) -> 166 (165-174) | 3.82 -> 3.80 |
| gate out 2 | 208 (176-209) -> 2.3 (1.7-3.3) | 434 (380-441) -> 211 (198-219) | 4.12 -> 4.12 |
| FT Cheydinhal | 254 (205-268) -> 8.6 (5.9-9.6) | 421 (363-438) -> 159 (158-159) | 5.78 (5.78-5.86) -> 5.87 (5.80-5.88) |
| FT Weye | 234 (194-276) -> 5.0 (4.8-5.0) | 403 (352-447) -> 160 (160-163) | 6.83 -> 6.78 |
| FT Leyawiin | 170 (160-180) -> 9.4 (9.2-9.8) | 327 (319-462) -> 144 (144-151) | 5.80 -> 5.77 |
| FT Imperial City | 264 (212-265) -> 9.6 (9.4-9.7) | 373 (333-394) -> 113 (112-117) | 5.46 -> 5.48 |

No extra gate tick anywhere (loading-screen lengths overlap run for run). Streaming starts 0.4-0.7 s after the gate is
armed and ~2.0-2.2 s before the fade-in on gates and fast travel (~1 s on the inn), 0.9-1.2 ms per update at native.
Whole-GPU memory max per run (nvidia-smi, 2 s): F 11031 (10850-11179) MiB, S 11526 (10823-12076) MiB. The two high S
values are single samples at one-off spikes (S-r2 at +31 s: FSR targets created after the first load; S-r3: a
virtual-shadow page pool re-created), next-highest samples 10869 / 10929 MiB, as F. **Within spread, not shown equal**;
the inn A/B (5 + 5) showed no difference. Warnings (4 per run, both sides): "a menu is still open after the fast travel"
(the scenario's own check; same on both sides).

## Final build smoke run (on/off rewrite, 1.1.0+fork.1, DLL 002018CA90E1; 2026-10-22 10:22, native, one launch)
`cheydinhal-gate-ft` with DoorDiag: `[streamload] installed`, streamed early in every transition; waits 1.6-11.6 ms,
arrival gaps 170-225 ms, freeze -> fade 3.71-4.14 s, as the mode-4 build. Self-test of this build: OK (0 failures).
Note: the fix also runs on the loading screen back to the main menu (pool size back ~1.5 s early there); the game's own
fade-in ran after it as usual (its stream-all 11.6 ms), so it is the same restore, earlier.

## Labels
| Measure | Label |
|---|---|
| Arrival pause on screen, doors (exit / entry), native and 720p | **Confirmed improvement** (-300 / -150 ms native, -275 / -90 ms 720p; no overlap) |
| Arrival pause on screen, city gate and fast travel, native | **Confirmed improvement** (gate -210 to -255 ms, fast travel -180 to -260 ms; 3 + 3, no overlap) |
| Loading-screen length | Within noise (medians equal; occasional one-gate-period misses as often as without the fix) |
| Image | Identical within run-to-run noise |
| Video memory | Within noise at doors (native peak a little lower, 720p equal); gate/fast travel within spread (median +0.5 GB from two one-sample spikes) |
| Game-thread cost | ~0.1-1.2 ms per loading-screen frame for ~0.3-2.2 s (non-blocking update), only during transitions |

## Not covered yet
- Other cities/interiors, cards with less video memory (the pool is given back up to ~2.2 s earlier than the game does
  on gates and fast travel: a risk where the game's 1 MB pool exists to save memory during the load; not testable on
  this 32 GB card), longer play sessions (stability), frame generation on, DLSS on.
- Merged into `work` 2026-10-22 as a fix, default on (user's decision), with AGENTS.md's requirements (docs, CHANGELOG
  "Unreleased", OPF_VERSION 1.1.0+fork.1 / plugin 0x010101, shipped-ini entry `StreamBehindLoadingScreen=1`).
