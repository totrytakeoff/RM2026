# RM2026 Robot Control

STM32F407IGT6 robot-control firmware for the RoboMaster 2026 season.

The current primary target is the single-board infantry firmware. Its control
logic lives in `application/infantry`, while deterministic scheduling lives in
`system/freertos`. The original bare-metal firmware remains as a comparison
target during migration; both targets compile the same application sources.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target app.elf --parallel
cmake --build build --parallel

# Native logic tests (uses the host compiler, not the ARM toolchain)
cmake -S test/unit -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

Primary outputs are collected in `build/output`.

## Repository layers

- `application/`: robot behavior and configuration.
- `services/`: platform-independent control and safety services.
- `firmware/`: concrete firmware composition and startup.
- `system/`: FreeRTOS tasks and reliability hooks.
- `hal/`: CubeMX-generated peripheral initialization.
- `lib/`: transitional upstream and vendor dependencies being separated.
- `test/`: standalone board-level and comparison firmware.
- `docs/migration/`: migration status, gates, and hardware acceptance records.

## Migration status

The stable infantry logic formerly maintained under
`test/infantry/infantry_minimal` is now the implementation used by the formal
FreeRTOS `app.elf`. Hardware validation is still required before treating it as
competition-ready firmware. See
`docs/migration/infantry_freertos_baseline.md` for current tasks and open gates.
The safety-state and task-health contract is documented in
`docs/migration/safety_health_service.md`.

## Provenance

The early framework architecture and portions of the current transitional
dependencies were developed from the YueLu team open-source `basic_framework`.
The project is being reorganized into independently owned application,
platform, component, and system layers. Required third-party notices are kept
in `THIRD_PARTY_NOTICES.md`.
