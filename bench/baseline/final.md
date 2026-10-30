# Final benchmark: stock vs upstream 1.2.2 vs fork.22 (2026-10-29)

**Setup.** 30 fresh launches, 09:26-12:16, all `ok`, no restarts. Three sides, each with its own shipped ini:
- stock (no DLL)
- upstream 1.2.2: built from main 4a2c82d, DLL 696F3CC13958, self-test 1159 PASS
- fork.22: DLL D1C68AC597BD; trade-offs off, `ShaderScreenAfterDriverChange=1`

Two scenarios, `showcase` (Imperial City flight, 111 s) and `silver-road` (249 s). Five rounds, with the side order
rotated each round: SUF, UFS, FSU, FUS, SFU. Every run used `-Profile native-nofg -Session fresh -MenuPsoWait`.

The rig: 9800X3D, RTX 5090 (driver 32.0.16.1770), window 5120x2160, FSR Performance, hardware RT on, no frame
generation, warm shader cache.

Runs are in `captures/bench/20261029-*-final-*`, the chain log in `captures/final-bench/chain.txt`, and the script in
the session scratchpad (`final_bench.ps1`, `final_analyse.py`).

**Measures.** The bench's own numbers per scene, plus CLAUDE.md's measures computed from `perf-frames.csv` over the
scene window:
- hitch = a frame over 2x the rolling median of the previous 120 frames;
- 99th percentile frame time;
- frames over 15 ms;
- worst 5 frames.

Cells are median (min-max) of 5 runs. Thresholds come from `noise.md`: fps and GameThread ~3.5 %, 1 % low ~6.5 %,
Render/RHI ~12 %, hitches/min ~16 %. Each scene's own stock spread is also used.

**The scenes are GPU-limited.** The GPU is the limit in 65-85 % of frames, at 86-105 fps against a 165 Hz target. So
average fps moves little, and the stutter measures are the ones that matter. Scaled targets (CLAUDE.md):
- showcase, ~91 fps (11 ms): 1 % low within ~30 % of the average, worst frame ~27 ms;
- Silver Road, ~102 fps (9.8 ms): worst frame ~25 ms.

## showcase (rumare-loop-flight)

| measure | stock | upstream 1.2.2 | fork.22 |
|---|---|---|---|
| fps | 86.1 (83.9-94.8) | 92.0 (86.2-99.3) | 91.1 (88.9-92.1) |
| 1% low fps | 34.3 (32.5-38.0) | 51.2 (49.7-54.8) | 52.7 (49.5-53.8) |
| 99th pct ms | 29.15 (26.31-30.79) | 19.52 (18.24-20.13) | 18.97 (18.59-20.19) |
| worst ms | 232.2 (81.1-280.3) | 44.4 (33.3-45.0) | 47.5 (40.1-52.2) |
| hitches/min (2x median) | 131.6 (126.6-138.5) | 26.0 (21.6-41.1) | 23.8 (18.9-30.3) |
| frames >15 ms /min | 569 (384-666) | 255 (180-332) | 253 (215-327) |
| frames >=50 ms | 10 (7-12) | 0 (0-0) | 0 (0-1) |
| GameThread ms | 7.55 (7.06-7.79) | 7.36 (6.92-7.49) | 7.23 (7.07-7.53) |
| RenderThread ms | 4.99 (4.57-5.23) | 4.00 (3.93-4.17) | 4.02 (3.94-4.20) |
| RHIThread ms | 8.79 (8.45-9.31) | 3.63 (3.51-3.73) | 3.49 (3.47-3.78) |
| OblivionThread ms | 1.75 (1.60-1.94) | 2.67 (2.49-2.75) | 2.62 (2.57-2.82) |
| GPU busy ms | 10.48 (9.73-10.49) | 10.81 (9.74-11.29) | 10.66 (10.58-10.94) |
| launch to world, s | 91 (88-93) | 32 (32-34) | 32 (32-32) |
| main-menu PSO wait, s | 76 (74-78) | 18 (18-20) | 18 (18-18) |

Worst 5 frames over all runs:
- stock: 280, 266, 232, 173, 94 ms
- upstream: 45, 45, 44, 41, 38 ms
- fork: 52, 50, 48, 45, 43 ms

## silver-road (silver-road-north)

| measure | stock | upstream 1.2.2 | fork.22 |
|---|---|---|---|
| fps | 105.4 (103.8-113.9) | 102.7 (98.2-104.9) | 102.5 (100.6-104.7) |
| 1% low fps | 64.9 (62.5-70.4) | 71.4 (70.1-74.6) | 73.5 (70.8-75.6) |
| 99th pct ms | 15.41 (14.20-16.01) | 14.02 (13.40-14.26) | 13.60 (13.22-14.13) |
| worst ms | 57.4 (56.0-143.3) | 42.2 (28.1-44.1) | 41.9 (38.1-232.1) |
| hitches/min (2x median) | 16.8 (14.9-17.6) | 4.3 (1.4-5.3) | 2.6 (1.9-4.8) |
| frames >15 ms /min | 85 (45-103) | 31 (21-38) | 20 (20-33) |
| frames >=50 ms | 1 (1-3) | 0 (0-0) | 0 (0-4) |
| GameThread ms | 5.79 (5.45-5.88) | 5.72 (5.58-5.77) | 5.66 (5.45-5.75) |
| RenderThread ms | 4.13 (3.91-4.20) | 3.40 (3.24-3.45) | 3.35 (3.29-3.41) |
| RHIThread ms | 7.54 (7.13-7.76) | 3.14 (3.04-3.18) | 3.07 (2.95-3.13) |
| OblivionThread ms | 1.40 (1.26-1.46) | 2.26 (2.21-2.40) | 2.30 (2.19-2.32) |
| GPU busy ms | 9.25 (8.57-9.38) | 9.66 (9.61-10.09) | 9.74 (9.63-9.91) |
| launch to world, s | 89 (86-93) | 32 (32-32) | 32 (32-32) |

Worst 5 frames over all runs:
- stock: 143, 59, 57, 57, 57 ms
- upstream: 44, 43, 42, 41, 41 ms
- fork: 232, 223, 151, 118, 44 ms (the first four all come from round 2)

## Verdicts

**Either fix vs stock**

*Confirmed improvement:*
- 1 % low: +49-54 % on showcase, +10-13 % on Silver Road (the fork's range lies wholly above stock's).
- Hitches per minute: -80 % on showcase, -74-85 % on Silver Road.
- Frames of 50 ms or more: 10 to 0 on showcase.
- Worst frame on showcase: 232 to 44-48 ms.
- RHIThread: -59 % on both scenes.
- RenderThread: -19 % on both scenes.
- Launch to world: 90 to 32 s. The main-menu PSO wait: 76 to 18 s.

These reproduce Phase 1 and upstream's mechanism: RHI work moved to the task workers, and the start-up precompile
finishing in the menu instead of in play. Stock's 170-280 ms frames on showcase are that precompile still running in
the world.

*Within noise:*
- Average fps: showcase +6-7 % (stock spread 13 %); Silver Road -2.7 % (stock spread 10 %).
- GameThread time.

*Regression (small, GPU):*
- GPU busy time is higher with either DLL: Silver Road +0.41 / +0.49 ms (+4-5 %; the ranges don't overlap),
  showcase +0.2-0.3 ms.
- Phase 1 saw the same: native Silver Road, stock 102.2 vs fix 100.8 fps.
- It comes from upstream's fixes, not the fork's additions. No cause has been found; the first suspect is the parallel
  render-graph translation splitting command lists (unverified).
- At 5120x2160 this costs about 2-3 % average fps on Silver Road, inside that scene's noise.

*Known:* OblivionThread runs +50-64 % ms per frame with either DLL, as found in Phase 1. It does not limit the frame
rate.

**fork.22 vs upstream 1.2.2: within noise on every measure, on both scenes.**
- The fork's medians are a little better on Silver Road: hitches/min 2.6 vs 4.3, frames over 15 ms 20 vs 31, 1 % low
  73.5 vs 71.4. The ranges overlap, so these are not results.
- This is expected. The fork's own additions target doors, fast travel, loading screens, NPC fade-in, menus, HDR, the
  shader screen after a driver change, and crash fixes. These two warm-cache flights don't exercise any of them.

**Unexplained: fork Silver Road round 2.**
- What happened: four frames of 118-232 ms, 242-245 s into the flight.
  - The CPU was busy for the whole frame (frame times of 118-232 ms).
  - The background workers peaked at about 470 % and disk reads at 256 MB/s.
  - System page-ins reached 15 k/s.
  - This was about 10 s after a cell-row crossing (the cell buffer drained 9 cells at 10:20:25).
- The same 236-249 s stretch in the other 14 Silver Road runs never goes above 20 ms, except a single 41 ms frame in
  fork round 3.
- The fix log has nothing at that moment, and the ledger was off.
- A cluster like this (110-199 ms) also hit one of five upstream-only runs in the 19 October noise test.
- So one run can't tell a fork cause from a rare event in the game. The test that would settle it: about 10 Silver Road
  runs each of fork and upstream with `HitchLedger=1`.

**Against the targets.** No side reaches them.
- The fork's worst frames, 40-52 ms, are about twice the scaled ~25-27 ms target.
- The fork's 1 % low is 52.7 against 91 fps on showcase (52 % below the average) and 73.5 against 102.5 on Silver Road
  (28 % below).
- The scenes are GPU-limited at this resolution, so a CPU-side fix can't move the average.

**Notes.**
- Every run warns "profile native-nofg not in effect: window 5120x2160, profile 5504x4128". The profile's "monitor
  resolution" reads as 5504x4128, but the game stayed at 5120x2160 in all 30 runs, so the comparison holds. The
  profile's resolution detection needs a look (a bench tool issue).
- 5 runs per side per scene, per CLAUDE.md. The spreads are wider at native than at the CPU profile.
