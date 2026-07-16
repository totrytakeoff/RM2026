# Safety and task-health service

Date: 2026-07-16

This slice replaces the infantry application's local safe-stop boolean with a
repository-owned, platform-independent safety state machine. It also makes the
FreeRTOS task assumptions measurable before any control retuning begins.

## Safety contract

`components/services/safety` has no HAL or FreeRTOS dependency and is the first
reusable service in the new framework.

| State | Meaning | Actuator updates |
| --- | --- | --- |
| `BOOT` | Manager initialized but not evaluated | blocked |
| `DISARMED` | Application initialization is incomplete | blocked |
| `ACTIVE` | Every safety input is healthy | permitted |
| `STOPPED` | At least one runtime fault is active | blocked |

Fault reasons are a bit mask, so simultaneous input-offline, emergency-stop,
task-health, required-device-health, and invalid-input faults are retained
instead of hiding one another. A null input snapshot fails closed. The control
task is the only owner of safety-state transitions. The health task publishes a
single atomic boolean input through `InfantryApp_SetTaskHealth`, avoiding
actuator changes from multiple tasks. Required DJI motor deadlines and INS
readiness are aggregated separately as `DEVICE_UNHEALTHY`; the 5 ms motor layer
also gates each offline actuator locally. A separate 100 ms motor-command lease
contains complete control-task starvation without requiring that task to run
and process its own unhealthy heartbeat.

Recovery remains automatic when all inputs are healthy. This is intentional
behavior compatibility with `infantry_minimal`, not the final competition
arming policy. A deliberate operator re-arm state can be added after hardware
comparison without changing chassis, gimbal, or shooter code.

## Task-health contract

Each static task records:

- expected period and run count;
- last and maximum start-to-start interval;
- last and maximum workload execution time using the DWT cycle counter;
- total and consecutive deadline misses;
- last heartbeat tick;
- historical minimum unused stack words from FreeRTOS.

For `ins`, `motor`, `control`, and `health`, any of these conditions sets that
task's bit in `unhealthy_mask`:

- no heartbeat for four configured periods after a 100 ms startup grace;
- three consecutive releases later than 125% of the configured period, or
  three consecutive workload executions at least as long as the period;
- historical stack margin below 64 words.

`diagnostics` collects the same metrics but is not safety-critical because UART
or RTT output may block. A consistent copy is available through
`InfantryTasks_GetHealthSnapshot` for telemetry and debugger inspection.

## Automated verification

The safety service is compiled with the native compiler under strict warnings
and tested independently from the ARM firmware:

```bash
cmake -S test/unit -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

CI runs these tests before configuring the embedded build. Covered transitions
include boot, activation, input loss, emergency stop, task-health failure,
isolated device-health failure, combined reasons, automatic recovery, and
null-argument fail-safe behavior.

The same native suite now also verifies the independent
`components/services/device_health` deadline service. Its millisecond timeout,
wrap, one-shot transition, fixed-capacity, and null-safety contract is recorded
in `platform_runtime_milestone.md`.

## Hardware work still required

- Record all five stack margins and maximum execution/release intervals during
  a continuous ten-minute run.
- Force each software fault independently and confirm all actuator modules stop.
- Decide stack and deadline thresholds from measurements instead of estimates.
- Add an independent hardware watchdog; the command lease still depends on the
  system tick and motor task, so it cannot contain a scheduler-wide lockup.
- Decide whether competition firmware requires an explicit operator re-arm
  after emergency stop or task-health failure.
