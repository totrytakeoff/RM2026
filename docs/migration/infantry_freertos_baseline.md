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

CAN, UART, device-health, DJI motor, EKF, and diagnostic-formatting storage on
the formal path is now static. Device timeouts use monotonic millisecond
deadlines and are independent of the health-task period. See
`platform_runtime_milestone.md` for capacities, timeout values, and remaining
transport constraints.

The formal firmware selects deferred CAN/UART dispatch before registering any
device. Interrupts only retain bounded receive data; the 5 ms motor task parses
it before running motor control. ET08, VT, and DJI feedback are copied through
coherent snapshot APIs before application use. See
`deferred_ingress_milestone.md` for overflow and compatibility semantics.

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
- all 30 embedded demo/regression firmware targets build;
- all four native unit-test programs pass;
- formal firmware RAM usage is 50,696 bytes (38.68%) and Flash usage is 95,028
  bytes (9.06%) in Debug, including an 8 KiB main-stack reservation and no C
  heap reservation;
- the formal image contains no linked C/FreeRTOS heap-allocation or libc
  formatting symbol;
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

- Application modules still call transitional motor, input, INS, referee,
  device-health, platform, and algorithm APIs from `components/` and
  `platform/`.
- Several components not linked into `app.elf` still provide heap-backed
  compatibility APIs and cannot be promoted into the formal firmware yet.
- Module APIs and configuration macros retain some `Minimal*` naming.
- Motor commands/PID runtime and INS attitude data still cross task boundaries
  through transitional mutable structures; their ownership must be narrowed.
- Device initialization failures and individual motor-health deadlines are not
  yet aggregated into the top-level safety state.
- The mutable `g_robot` context is still shared with diagnostic code.
- A hardware watchdog is not implemented yet; the software health task cannot
  diagnose its own total starvation.
- CubeMX `.ioc` is not yet tracked.
- the formal main-stack reservation is conservatively 8 KiB until interrupt
  nesting and startup behavior are measured on hardware;
- The former `Src/application` implementation has not yet been removed.

The next ownership slice is portable algorithm cleanup and device-component
separation: explicit PID timing, narrow public headers, protocol/transport
boundaries, and removal of the remaining inactive heap-backed registrations.
