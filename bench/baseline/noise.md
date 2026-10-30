# Noise floor (this PC)

**Setup:** 5 fresh launches (`-Session fresh -Runs 5`), unchanged fix build (upstream 6deebc2, default ini),
`-Scenario showcase` (rumare-loop-flight, 111 s), `-Profile cpu` (1280x720 window, FSR Performance, HWRT on, FG off),
2026-10-19 18:39-18:55. Conditions: bench/conditions.md (2026-10-19). Runs: `captures/bench/*-noise-cpu-r1..r5`.
Metrics: `mytools/hitch_metrics.py` (hitch = frame > 2x rolling median of 61 frames and >= 4 ms; 1% low = 1000/p99;
it reproduces the report's fps / 1% low / worst ms exactly on the Phase 0 smoke run).

| run | fps | avg ms | 1% low | hitches/min | >= 50 ms | worst 5 (ms) | Game | Render | RHI | Obl | GPU |
|---|---|---|---|---|---|---|---|---|---|---|---|
| r1 | 93.1 | 10.74 | 43.7 | 51.4 | 4 | 199.0 150.8 109.8 52.1 42.9 | 8.98 | 4.50 | 3.55 | 2.84 | 7.27 |
| r2 | 94.7 | 10.56 | 45.0 | 51.4 | 2 | 53.4 51.1 47.0 44.5 44.3 | 8.80 | 4.29 | 3.60 | 2.93 | 7.29 |
| r3 | 96.0 | 10.42 | 46.0 | 48.7 | 1 | 53.9 38.9 34.8 34.0 32.3 | 8.73 | 4.26 | 3.63 | 2.82 | 7.27 |
| r4 | 93.6 | 10.68 | 46.1 | 44.4 | 1 | 54.0 38.6 35.3 33.4 32.2 | 9.03 | 4.70 | 3.80 | 2.96 | 7.50 |
| r5 | 95.4 | 10.48 | 46.7 | 52.5 | 1 | 52.2 40.4 39.4 39.0 35.9 | 8.79 | 4.29 | 3.38 | 2.98 | 7.09 |

| metric | median | min-max | spread (max-min)/median |
|---|---|---|---|
| fps | 94.7 | 93.1-96.0 | 3.0 % |
| avg frametime ms | 10.56 | 10.42-10.74 | 3.0 % |
| 1% low fps | 46.0 | 43.7-46.7 | 6.4 % |
| hitches/min (2x median) | 51.4 | 44.4-52.5 | 15.8 % |
| frames >= 50 ms | 1 | 1-4 | (count; one run had a 110-199 ms cluster) |
| GameThread ms | 8.80 | 8.73-9.03 | 3.4 % |
| RenderThread ms | 4.29 | 4.26-4.70 | 10.3 % |
| RHIThread ms | 3.60 | 3.38-3.80 | 11.7 % |
| OblivionThread ms | 2.93 | 2.82-2.98 | 5.5 % |

**Thresholds used from now on (median-to-median, 5 runs per side, this scene/profile):** fps, avg frametime and
GameThread changes under ~3.5 % = within noise; 1% low under ~6.5 %; Render/RHI ms under ~12 %; hitches/min under
~16 %; >= 50 ms counts and worst frames are too noisy for single-scene conclusions (one outlier run) — compare their
medians and say so. Much tighter than upstream's quoted +/-10 %, likely because every run here is a fresh launch at the
same spot with a warm pipeline cache. Other scenes/profiles may be noisier (silver-road, native): their stock and fix
spreads in Phase 1 serve as their own noise estimate.

Observation (not yet a result): each run has one frame of ~52-54 ms, apparently at a fixed point of the flight.
