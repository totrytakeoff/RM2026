# Infantry FreeRTOS migration baseline

Date: 2026-07-15

## Scope

The first migration target is the single-board infantry firmware. Hero,
wheel-leg, dual-board communication, control retuning, and new algorithms are
outside this baseline.

The source of truth for infantry behavior is now `application/infantry`.

- `app.elf` executes it through statically allocated FreeRTOS tasks.
- `test_infantry_minimal` executes the same code from the original bare-metal
  scheduling shell for behavior comparison.
- `Src/application` is no longer linked into `app.elf` and remains only as a
  temporary migration reference.

## Preserved behavior

- VT is the primary input and ET08 can take over according to the existing
  switch policy.
- High-level control order remains gimbal, chassis, then shooter.
- The high-level control period remains 20 ms.
- Motor-control scheduling initially remains 5 ms to preserve the CAN send-rate
  constraint from the minimal firmware.
- PID values, limits, calibration values, and control-mode transitions are not
  retuned during the migration.
- Offline, emergency-stop, or critical-task faults stop chassis, gimbal, and
  shooter together. Once all faults clear, the safety state automatically
  returns to active as in the minimal baseline.

## FreeRTOS task baseline

All application tasks use `xTaskCreateStatic` and `vTaskDelayUntil`.

| Task | Period | Priority | Stack |
| --- | ---: | ---: | ---: |
| `ins` | 1 ms | idle + 4 | 1024 words |
| `motor` | 5 ms | idle + 3 | 384 words |
| `health` | 5 ms | idle + 3 | 256 words |
| `control` | 20 ms | idle + 2 | 768 words |
| `diagnostics` | 10 ms | idle + 1 | 384 words |
| `usb_init` | one shot | idle + 1 | 128 words |

FreeRTOS dynamic allocation is disabled. Stack-overflow checking and the
allocation-failure hook are enabled as defensive configuration.
Task stack sizes are deliberately conservative until hardware high-water marks
have been collected.

The critical tasks now publish heartbeat, release interval, execution time,
deadline-miss, and stack high-water data. Three consecutive deadline misses, a
four-period heartbeat loss after the startup grace period, or fewer than 64
unused stack words drives the application safety state to `STOPPED`.
`diagnostics` remains observable but non-critical because its transport may
block without making actuator control unsafe. See `safety_health_service.md`.

## Build baseline

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target app.elf test_infantry_minimal --parallel
cmake --build build --parallel

cmake -S test/unit -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

Expected status at this baseline:

- formal firmware builds;
- bare-metal comparison firmware builds;
- all embedded firmware targets build;
- formal firmware RAM usage is approximately 52%;
- existing newlib syscall and RWX linker warnings remain tracked cleanup work.

## Hardware acceptance gates

These items must be recorded on hardware before the FreeRTOS target replaces
the comparison firmware for tuning:

- [ ] cold start remains output-safe during the two-second stabilization delay;
- [ ] VT online/offline transitions stop and recover without output jumps;
- [ ] ET08 takeover and release match the comparison firmware;
- [ ] emergency stop disables chassis, gimbal, friction wheels, and loader;
- [ ] chassis FOLLOW, SEPARATE, and spin behavior match the comparison firmware;
- [ ] yaw and pitch feedback directions are correct after task separation;
- [ ] single, double, and continuous fire state transitions are unchanged;
- [ ] no CAN mailbox saturation occurs with the 5 ms motor task;
- [ ] task periods and execution times remain bounded for at least 10 minutes;
- [ ] stack high-water marks leave an agreed safety margin;
- [ ] no stack-overflow or allocation-failure hook is reached;
- [ ] USB initialization completes and its one-shot task terminates;
- [ ] referee read-only interlocks behave correctly when enabled.

## Known transitional debt

- Application modules still call motor, input, INS, referee, daemon, BSP, and
  algorithm implementations from `lib/HNUYueLuRM`.
- Module APIs and configuration macros retain some `Minimal*` naming.
- The mutable `g_robot` context is still shared with diagnostic code.
- A hardware watchdog is not implemented yet; the software health task cannot
  diagnose its own total starvation.
- CubeMX `.ioc` is not yet tracked.
- linker-reserved C heap/stack still require memory-budget cleanup.
- The former `Src/application` implementation has not yet been removed.

The next ownership slice is time and log infrastructure, followed by CAN/UART
BSP and motor/input components.
