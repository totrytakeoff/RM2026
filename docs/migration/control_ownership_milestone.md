# Control ownership and device-fault containment milestone

Date: 2026-07-16

This milestone closes the command/PID/INS ownership debt left after deferred
communication ingress. It preserves the infantry control parameters and state
transitions, but changes how data crosses the 1 ms, 5 ms, and 20 ms task
boundaries.

## Runtime ownership

```text
1 ms INS task
    -> solve attitude
    -> publish protected attitude snapshot

20 ms control task
    -> read input, referee, motor-measurement, and attitude snapshots
    -> calculate chassis/gimbal/shooter behavior
    -> atomically publish one complete command per motor

5 ms motor task
    -> dispatch retained CAN/UART ingress
    -> refresh gimbal's high-rate IMU feedback snapshot
    -> consume motor commands
    -> reject commands older than the configured lease
    -> own PID runtime and motor settings
    -> apply local/global safety gates
    -> transmit grouped CAN current commands
```

The formal application's 20 ms path no longer writes `motor_settings`,
`motor_controller`, `stop_flag`, or any `PIDInstance` runtime field. Those
objects are now updated only inside `DJIMotorControl()`.

## Motor command contract

Every fixed-storage `DJIMotorInstance` contains a `DJIMotorCommand` mailbox. A
command carries the complete persistent state needed for one motor:

- reference value;
- outer-loop and feedback-source settings;
- enabled/stopped state;
- optional value-based external feedback or feedforward inputs;
- an edge-triggered PID reset mask.

`DJIMotorPublishCommand()` copies the persistent command in one short critical
section. PID reset bits are OR-latched separately so that a second publication
cannot erase an unconsumed reset request. `DJIMotorControl()` copies the command,
clears the latched reset bits, and then applies it to the private runtime
objects.

`PIDReset()` is now an algorithm API that clears measurement, error, integral,
derivative, output, timing, and error-handler history while preserving gains,
limits, filters, and improvement flags. Mode changes request resets through the
mailbox instead of modifying a live PID from the control task.

The legacy setters remain source-compatible for board demos. They now update
the same mailbox under a critical section. They are compatibility operations,
not a multi-field transaction; new concurrent application code must use the
complete-command API. The command-age check is disabled by default for those
legacy demos and enabled explicitly by the formal infantry composition.

Every successful full-command publication records the same coherent monotonic
timestamp as its mailbox contents. The formal application publishes at 20 ms
and configures a 100 ms lease. If the control task stops running, the 5 ms motor
task rejects all stale commands after five missed application periods even
though the last command was enabled and the robot-wide gate remains open. An
expired generation stays latched off until a newer command is published, so a
32-bit millisecond-clock wrap cannot revive an old command.

## Output containment

A DJI motor produces nonzero CAN output only when all four conditions are true:

1. its consumed command is enabled;
2. the robot-wide atomic output gate is enabled;
3. its own feedback daemon is online and at least one valid frame was decoded;
4. its command is inside the configured age lease.

The 20 ms safety path closes the robot-wide gate before publishing module stop
commands. On recovery it publishes the complete gimbal, chassis, and shooter
command set before reopening the gate. Independently, the 5 ms motor task zeros
an individual motor as soon as that motor's 20 ms feedback deadline expires,
without waiting for the next high-level safety evaluation. It likewise zeros
stale commands without depending on the stalled control task to observe its
own heartbeat fault.

PID runtime is reset on output disable/enable transitions. A stopped or offline
motor does not continue integrating an invisible command and therefore cannot
resume with accumulated PID history.

## INS snapshot contract

`INS_Task()` now builds and publishes a complete `attitude_t` snapshot after
each EKF update. `INS_Read()` copies that snapshot under the same short
single-core critical-section contract used by other formal device snapshots.
`INS_IsReady()` becomes true only after the first complete solution.

The formal gimbal no longer retains or reads the mutable INS implementation
object. The 20 ms behavior path uses `INS_Read()`. The 5 ms motor stage also
copies the newest INS result into three gimbal feedback scalars that are written
and consumed only inside the motor task. This preserves the previous 200 Hz
motor-loop access to yaw angle, yaw rate, and pitch rate instead of reducing IMU
feedback to the 50 Hz application rate.

`INS_Init()` still returns a pointer to the published snapshot for source
compatibility with older single-loop demos. Concurrent firmware must use
`INS_Read()`.

## Initialization and device health

The infantry input, chassis, gimbal, shooter, and top-level application
initializers now report success. The formal firmware refuses to create tasks
when a required input endpoint, motor endpoint, motor health deadline, or INS
instance cannot be initialized.

At runtime, the safety manager receives a separate `device_health_ok` input and
reports `RM_SAFETY_REASON_DEVICE_UNHEALTHY`. The formal aggregate covers:

- all four chassis motors;
- yaw and pitch motors plus the first valid INS solution;
- both friction motors and the loader motor.

Input link health remains a distinct `INPUT_OFFLINE` reason. Scheduler timing
and stack health remain a distinct `TASK_UNHEALTHY` reason, so simultaneous
faults remain observable rather than being collapsed into one boolean.

## Verification baseline

The native suite contains five strict-warning programs. In addition to safety,
device-health, formatting, and retained-ingress tests, the PID test verifies
that runtime reset clears every state field while preserving controller
configuration. The safety test covers an isolated device fault and automatic
recovery.

The following build set passes:

- formal FreeRTOS `app.elf`;
- bare-metal `test_infantry_minimal` using the same application and motor-stage
  APIs;
- all 30 embedded demo/regression firmware targets;
- all five native unit-test programs.

In Debug, the formal image uses 51,240 bytes RAM (39.09% of 128 KiB) and 96,868
bytes Flash (9.24% of 1 MiB). Compared with the deferred-ingress baseline, the
544-byte RAM increase is entirely fixed static state, dominated by per-motor
command and lease storage. No runtime heap or libc formatting symbol was
reintroduced; the linked-symbol audit still passes.

Compilation does not prove control equivalence or fault timing on hardware.
Before tuning, hardware acceptance must confirm:

- each motor feedback loss zeros that CAN slot within one 5 ms motor period and
  drives the whole robot to `DEVICE_UNHEALTHY` on the next control step;
- recovery starts with reset PID history and no output impulse;
- yaw/pitch feedback signs and bandwidth match the comparison firmware;
- pitch speed/brake/angle transitions remain smooth;
- global emergency stop closes every actuator output before command recovery;
- suspending the control task leaves every DJI command slot at zero no later
  than the 100 ms command lease plus one 5 ms motor period;
- task execution time and motor stack high-water margin remain acceptable with
  the added snapshot copies.

## Remaining ownership debt

- `DJIMotorInstance` remains publicly mutable for old demos; the formal
  application no longer relies on that access, but a later compatibility
  cleanup should make the runtime portion opaque.
- INS readiness confirms publication and task health, not independent sensor
  plausibility or a hardware data-ready deadline.
- `g_robot` remains a mutable application/diagnostics context.
- Hardware comparison and soak-test gates remain open; this milestone is not a
  control-tuning result.

Bounded BMI088 initialization and the independent IWDG feed contract are closed
by [`runtime_hardening_milestone.md`](runtime_hardening_milestone.md).
