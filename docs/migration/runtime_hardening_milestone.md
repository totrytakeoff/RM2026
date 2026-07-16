# Runtime hardening and component-boundary milestone

Date: 2026-07-16

This milestone closes the software-side cleanup items left after control
ownership migration: bounded BMI088 startup, scheduler-independent reset
containment, repository-owned newlib boundaries, non-RWX ELF segments, and a
formal component set that no longer links every compatibility driver.

## Bounded BMI088 and INS initialization

The formal gimbal now calls `INS_InitWithTimeout()` with one 15,000 ms budget.
That budget covers BMI088 register setup and online calibration. The legacy
`INS_Init()` and `BMI088Init()` entry points remain source-compatible, but now
use the same finite default instead of spinning forever.

The BMI088 path now has all of the following bounds and failure results:

- each SPI byte has a 2 ms HAL timeout;
- each configuration register is written and verified at most three times;
- chip-ID, register, argument, total-timeout, and calibration errors are
  returned to the caller;
- calibration checks its overall deadline inside the sample loop and discards
  interrupted partial averages;
- a failed calibration loads the robot-specific offline values for diagnostic
  continuity but still returns failure to the formal application;
- the former gyro-register retry index underflow and wrong error-table lookup
  are removed.

`INS_InitWithTimeout()` also checks heater PWM startup. On any sensor failure it
sets heater output to zero, stops PWM, records the initialization state and
BMI088 error, and returns `NULL`. `Gimbal_Init()` propagates that failure to
`InfantryApp_Init()`, which keeps the global actuator gate closed and refuses
to start the runtime task set.

Initial quaternion construction now handles zero-length and parallel vectors
and clamps the `acosf` input, preventing invalid floating-point state from a
borderline startup sample.

## Independent hardware watchdog

The formal firmware starts STM32F407 IWDG only after the application has
initialized successfully and its five control/runtime tasks have been created.
The remaining bounded USB-init task creation then runs before the scheduler is
started. This avoids resetting during the deliberate two-second motor
stabilization and IMU calibration phases. BMI088 startup is independently
bounded as described above; other pre-scheduler initialization still relies on
explicit return paths rather than IWDG.

The watchdog uses prescaler 256 and computes reload from a conservative 48 kHz
LSI upper bound. For the configured 1,000 ms minimum, the reload value is 187.
The STM32F407 datasheet specifies a 17–47 kHz LSI range, so the expected
hardware reset interval is approximately 1.02–2.83 seconds. The conservative
calculation prevents a fast LSI from resetting earlier than the configured
minimum. See the official
[STM32F407 datasheet](https://www.st.com/resource/en/datasheet/stm32f407ie.pdf).

Only the 5 ms health task refreshes IWDG, and only when every critical task
passes heartbeat, consecutive-deadline-miss, and stack-margin checks. This
contains:

- a stalled INS, motor, control, or health task;
- a scheduler-wide stall;
- sustained critical-task deadline or stack-health failure.

The existing safety manager still closes actuator output immediately when the
health task can run and observe a fault. IWDG is the independent last resort
when software scheduling can no longer enforce that stop. Debug builds freeze
IWDG while the Cortex-M core is halted, so a breakpoint does not masquerade as
a runtime failure.

## Newlib and ELF contracts

Every embedded executable directly includes the repository-owned bare-metal
syscall boundary in `platform/stm32f4/runtime`.

- stdout and stderr are non-blocking RTT diagnostic streams;
- unsupported file/process operations fail explicitly with `errno`;
- `_exit` disables interrupts and enters a terminal wait;
- `_sbrk` is intentionally absent, and the formal linked-symbol audit still
  rejects heap allocation.

This removes the libnosys `_close`, `_fstat`, `_getpid`, `_isatty`, `_kill`,
`_lseek`, `_read`, and `_write` warnings without suppressing linker diagnostics.

The linker script now declares RAM/CCMRAM non-executable, marks constructor and
destructor arrays read-only, and marks BSS plus reserved heap/stack as
`NOLOAD`. `readelf -lW` reports three non-RWX load segments:

1. read/execute Flash;
2. read/write initialized data;
3. read/write BSS and reserved stack.

The linker script is also an explicit link dependency for the formal and all
embedded test targets, so changing it reliably triggers relinking.

## Formal versus compatibility components

The former monolithic `rm_components` archive is split into two ownership
sets:

- `RM::components_runtime` contains only the algorithms, BMI088/INS, DJI motor,
  input, referee, vision, and device-health implementation currently approved
  for the formal infantry firmware;
- `rm_components_compat` retains historical drivers and registration APIs used
  by standalone board demos.

`RM::components` remains an interface umbrella for the 30 existing regression
firmwares. `app.elf` and the bare-metal infantry comparison target link the
runtime set directly and cannot pull the compatibility archive accidentally.
This is an intermediate boundary: later work can split the runtime set into
narrow algorithm, device, and service targets without disturbing the demo
compatibility gate.

## Verification baseline

The verification set for this milestone is:

- formal FreeRTOS `app.elf` plus its heap/stdio symbol audit;
- bare-metal `test_infantry_minimal` comparison firmware;
- all 30 embedded demo/regression firmware targets;
- all five strict-warning native unit-test programs;
- ELF program-header inspection for absence of RWX segments.

In Debug, the formal image uses 51,216 bytes RAM (39.07% of 128 KiB) and 97,452
bytes Flash (9.29% of 1 MiB). The formal image retains the zero-byte C-heap and
8 KiB main-stack reservations.

## Hardware acceptance still required

Software builds cannot validate the physical oscillator, sensor, or actuator.
Before competition use, record all of the following on the target board:

- stable cold-start calibration completes and publishes the first INS sample;
- a disconnected or moving BMI088 exits within the 15-second budget, keeps
  every actuator disabled, and exposes the expected error code;
- suspending each critical task stops output and then produces an IWDG reset;
- a scheduler-wide halt produces an IWDG reset in the measured board-specific
  interval;
- normal maximum-load operation never causes a false watchdog reset;
- debugger halt/resume freezes and resumes IWDG as intended;
- the existing yaw/pitch direction, command-lease, CAN-load, and smoothness
  gates remain satisfied.

Remaining software ownership debt includes runtime IMU data-ready/plausibility
health, the mutable diagnostic `g_robot` view, public compatibility fields in
`DJIMotorInstance`, finer component targets, and removal of the inactive
`Src/application` reference tree.
