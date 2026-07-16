# RM2026 repository reorganization baseline

Date: 2026-07-16

Status: approved implementation baseline

## Objective

The repository will stop treating vendor code, platform adapters, reusable
robot components, application behavior, and firmware composition as one
framework library. The final source tree must have explicit ownership and an
acyclic dependency direction while preserving the proven infantry behavior
during migration.

The migration has three product goals:

1. Move the stable infantry behavior from the comparison demo into a reliable
   FreeRTOS firmware without retuning control behavior during structural work.
2. Replace the former upstream-shaped library wrapper with repository-owned
   targets, APIs, naming, tests, and documentation.
3. Establish measurable safety, timing, memory, and hardware acceptance gates
   before control optimization begins.

## Current baseline

Before this reorganization:

- `lib/HNUYueLuRM` mixes CMSIS, STM32 HAL, FreeRTOS, USB Device, CMSIS-DSP,
  SEGGER RTT, BSP code, algorithms, device drivers, services, and unused
  experimental modules in one 63 MB tree.
- Recursive source discovery compiles every BSP and module into broad archives,
  while recursive public include paths make internal headers globally visible.
- `app.elf` requires linker groups because CMake dependencies are cyclic.
- The formal firmware actually pulls only 15 robot module objects and four BSP
  objects from those broad archives.
- FreeRTOS dynamic allocation is disabled, but CAN, UART, device-health, motor,
  math, and Kalman initialization still use the C library heap.
- Device timeouts are expressed as health-task invocation counts. Changing the
  health task period therefore changes timeout behavior.
- INS directly publishes vision data, which pulls the vision protocol, CRC, and
  UART stack into the attitude path.
- Application headers expose HAL handles through `main.h`, while algorithm
  headers expose HAL, RTOS, DWT, and CMSIS-DSP implementation details.
- `Src/application` is obsolete and excluded from the formal build but remains
  in the working tree.
- The embedded test tree contains 30 board-level/demo targets and roughly
  20,000 lines of test entry-point code, most of which link broad framework
  archives instead of exact dependencies.

The safety/health migration milestone is commit `935e25d`. At that point:

- host safety tests pass;
- formal `app.elf`, `test_infantry_minimal`, and all embedded targets build;
- formal firmware uses 68,624 bytes RAM and 88,312 bytes flash in Debug;
- hardware validation is still pending;
- existing newlib syscall and RWX linker warnings remain known debt.

## Target dependency direction

```text
applications ------> components/devices ------> platform ------> third_party
      |                       |
      +------> components/services + algorithms

firmware ------> applications + components + platform + FreeRTOS
```

Rules:

- `third_party` contains unmodified external dependencies and never depends on
  repository-owned code.
- `platform` owns MCU and board adaptation and depends only on third-party code.
- `components` contains reusable algorithms, services, and device drivers and
  never depends on a robot application.
- `applications` contains robot behavior and must not include HAL, FreeRTOS, or
  CMSIS-OS headers.
- `firmware` owns task creation, board binding, interrupt/runtime composition,
  and the executable entry point.
- Control-path objects use caller-owned storage or fixed static capacity. No
  runtime heap allocation is permitted in competition firmware.
- Timeouts use absolute time units and monotonic timestamps, never scheduler
  invocation counts.
- Interrupt handlers copy bounded data and publish events; they do not block,
  format logs, or execute high-level robot behavior.

## Target repository layout

```text
RM2026/
|-- applications/
|   `-- infantry/
|       |-- include/rm/app/infantry/
|       |-- src/
|       |-- command/
|       |-- chassis/
|       |-- gimbal/
|       |-- shooter/
|       |-- referee/
|       `-- config/
|-- components/
|   |-- algorithms/
|   |   |-- math/
|   |   |-- pid/
|   |   |-- filters/
|   |   |-- quaternion_ekf/
|   |   `-- checksum/
|   |-- devices/
|   |   |-- motor/{dji,dm}/
|   |   |-- imu/{bmi088,dm,ins}/
|   |   |-- remote/{et08,vt}/
|   |   |-- referee/
|   |   `-- vision/
|   `-- services/
|       |-- safety/
|       |-- device_health/
|       `-- log/
|-- platform/
|   |-- stm32f4/{can,uart,spi,time,usb,log_backends}/
|   `-- boards/infantry_f407/{include,generated,startup,linker}/
|-- firmware/
|   `-- infantry_f407/{config,freertos}/
|-- third_party/
|   |-- cmsis/
|   |-- cmsis_dsp/
|   |-- stm32f4xx_hal/
|   |-- freertos_kernel/
|   |-- stm32_usb_device/
|   `-- segger_rtt/
|-- tests/
|   |-- host/{algorithms,protocols,services}/
|   `-- firmware/{bringup,regression,experiments}/
|-- cmake/{toolchains,modules}/
|-- docs/
|-- scripts/
`-- tools/
```

The final tree does not contain top-level `lib`, `hal`, `Inc`, `Src`, `system`,
`services`, `test`, or `config/linker` directories. Their owned content moves
to the layers above; obsolete content is removed and remains recoverable from
Git history.

## Source ownership mapping

| Current source | Final ownership |
| --- | --- |
| CMSIS and STM32 HAL | `third_party` |
| FreeRTOS, USB Device, CMSIS-DSP, SEGGER RTT | `third_party` |
| CAN, UART, SPI, DWT, USB platform adapters | `platform/stm32f4` |
| CubeMX-generated board files, startup, linker script | `platform/boards/infantry_f407` |
| Scalar/vector math, PID, Kalman, quaternion EKF, checksums | `components/algorithms` |
| Safety, device health, logging facade | `components/services` |
| DJI/DM motors, BMI088/DM IMU, INS, ET08, VT, referee, vision | `components/devices` |
| Stable infantry control behavior | `applications/infantry` |
| FreeRTOS tasks and executable composition | `firmware/infantry_f407` |
| Board bring-up and behavior comparison images | `tests/firmware` |
| Pure algorithms, protocols, and state machines | `tests/host` |

Modules with no production or test consumer are not promoted into the active
framework. They are audited against planned hardware, then removed unless an
owner, target, and verification path are assigned.

## Naming and public API policy

- Files use `snake_case.c` and `snake_case.h`.
- Public C types use `RmXxx`.
- Public functions use `RmXxx_Action`.
- Public macros use `RM_XXX`.
- CMake implementation targets use `rm_xxx` and aliases use `RM::xxx`.
- Public headers use namespaced paths such as `<rm/device/dji_motor.h>`.
- Ambiguous names such as `user_lib`, `daemon`, `general_def`, and broad
  `framework` archives are removed.
- Team-origin naming is removed from paths, targets, public APIs, and ordinary
  project narration. Required copyright/license notices remain authoritative;
  provenance is documented in `THIRD_PARTY_NOTICES.md`.

## CMake policy

- Every maintained component has its own target and explicit source list.
- Recursive source and include discovery is removed.
- Dependencies are `PRIVATE` unless a public header exposes the dependency.
- `app.elf` eventually links only `RM::firmware_infantry`.
- Linker groups are removed after dependency cycles are eliminated.
- Host and ARM builds use separate presets/toolchains.
- Third-party targets do not inherit repository-owned strict warning policy.
- Each test links only the components it verifies.

## Migration sequence

### 1. Structural ownership

- Move vendor sources into `third_party` and create vendor targets.
- Move BSP, modules, services, and DM code into their target ownership roots.
- Replace all former framework target names and include paths.
- Preserve behavior while eliminating the old wrapper directory.

### 2. Platform runtime

- Replace heap-backed CAN/UART registration with caller-owned or fixed storage.
- Introduce a monotonic integer time API.
- Split log facade from RTT/UART backends.
- Replace invocation-count health checks with timestamp deadlines.

### 3. Portable algorithms

- Split `user_lib` into focused math/vector utilities.
- Pass `dt_s` explicitly to PID updates.
- Give Kalman/EKF caller-owned workspaces.
- Remove HAL/RTOS includes from algorithm public headers.
- Add host tests before switching application consumers.

### 4. Device components

- Migrate device health, DJI motor, ET08/VT, BMI088/INS, referee, DM devices,
  and vision in dependency order.
- Separate protocol parsing from transport adapters.
- Remove vision publication from INS.

### 5. Application and firmware closure

- Split control parameters, task policy, and board mapping.
- Remove HAL and RTOS dependencies from the infantry application.
- Move all task/runtime composition into the concrete firmware target.
- Remove obsolete CubeMX-era and comparison implementations.

### 6. Test and final cleanup

- Organize host, bring-up, regression, and experimental tests explicitly.
- Consolidate duplicate demos and remove recursive CMake discovery.
- Remove compatibility targets, paths, headers, and unused modules.

## Per-milestone gates

Every structural milestone must pass:

```bash
cmake -S test/unit -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target app.elf test_infantry_minimal --parallel
cmake --build build --parallel
```

It must also record formal RAM/flash usage, preserve control parameters and
execution order unless explicitly scoped otherwise, and leave a clean Git
worktree after a focused commit.

## Final acceptance

- Former framework directories and CMake target names are absent.
- `app.elf` has no `malloc`, `calloc`, `free`, or `pvPortMalloc` reference.
- Application sources do not include HAL, FreeRTOS, or CMSIS-OS headers.
- `app.elf` does not require linker groups.
- Host safety, algorithms, and protocol tests pass.
- Formal, comparison, and supported bring-up firmware targets build.
- Input loss, emergency stop, and task-health faults stop every actuator.
- Hardware runs record bounded task timing, adequate stack margin, and no CAN
  mailbox saturation for the agreed soak duration.
