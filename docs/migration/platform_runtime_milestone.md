# Platform runtime milestone

Date: 2026-07-16

This milestone establishes the first repository-owned runtime path used by the
formal infantry firmware. It changes storage, timing, recovery, and diagnostic
infrastructure without retuning PID parameters, actuator limits, or application
mode transitions.

## Runtime contracts

### Monotonic time

`platform/stm32f4/time/rm_time.h` is the application-facing platform clock.

- `RmTime_NowMs()` returns the unsigned HAL millisecond tick.
- `RmTime_NowUs()` returns the DWT-backed 64-bit microsecond timeline.
- `RmTime_ElapsedMs()` uses unsigned subtraction and is valid across the
  32-bit millisecond wrap.
- `RmTime_DeadlineReached()` uses serial-number arithmetic; configured
  deadlines must be less than `2^31` ms into the future.

The DWT 32-to-64-bit extension is serialized across task and interrupt callers
and uses `2^32` cycles per wrap. It must still be sampled at least once per DWT
wrap, approximately 25.6 seconds at 168 MHz. The formal control path samples it
far more frequently through INS, motor, PID, and CAN activity.

### Device health

`components/services/device_health` now owns a fixed registry of 64 opaque
instances. Registration selects an explicit timeout and startup grace in
milliseconds. A valid feed writes a new absolute deadline; health-task polling
frequency no longer changes the timeout.

Feeds may originate in CAN/UART interrupts while the health task evaluates
deadlines. Atomic deadline and transition state prevents an interrupt feed from
suppressing the next genuine offline notification. A newly registered instance
is not online until its first valid feed; startup grace only postpones the first
offline callback. The callback then runs once per offline episode and is
re-armed by the next valid feed. Once declared offline, that transition remains
latched until a new feed, preventing serial-number ambiguity after very long
uptime from reviving stale device health.

Current device deadlines are:

| Consumer | Deadline |
| --- | ---: |
| DJI and HT motor feedback | 20 ms |
| LK motor feedback | 50 ms |
| DT7 and virtual DBUS | 100 ms |
| Vision UART / USB | 100 ms / 50 ms |
| VT input | 200 ms |
| Referee link | 300 ms |
| ET08 compatibility default | 4000 ms |
| ET08 in formal infantry firmware | 1000 ms |

Zero-valued configuration fields select the 1000 ms service defaults. Values
at or above `2^31` ms are rejected because their wrapped ordering is ambiguous.

### CAN and UART registration

CAN and UART endpoint objects are no longer allocated from the C heap.

| Resource | Fixed capacity | Registration rule |
| --- | ---: | --- |
| CAN endpoint | 16 total | unique standard RX ID per CAN handle |
| CAN filters | 14 per bus | one 16-bit ID-list bank per endpoint |
| UART endpoint | 3 total | one endpoint per UART handle |
| UART receive buffer | 256 bytes per endpoint | requested length must fit |

CAN transmit timeouts are integer microseconds. Receive callbacks drain the
selected FIFO and copy at most eight bytes per classic-CAN frame. UART receive
uses receive-to-idle DMA; recovery handles a busy HAL state by aborting the
stale receive operation and starting it once more.

At this milestone the transport callbacks still invoked protocol/device
callbacks in interrupt context. The follow-up
[`deferred_ingress_milestone.md`](deferred_ingress_milestone.md) closes that
gap for the formal infantry firmware with bounded receive inboxes and coherent
device snapshots.

### Heap-free formal control path

The formal firmware now uses static storage for:

- 12 DJI motor instances;
- CAN, UART, and device-health registries;
- the quaternion EKF Kalman workspace (about 1.6 KiB);
- all FreeRTOS tasks and stacks;
- bounded diagnostic formatting.

Its linker contract reserves no C heap and keeps an 8 KiB main stack for
startup and interrupt context. Demo targets retain the historical linker
defaults because several of them still exercise heap-backed compatibility
drivers. A post-link audit fails the formal build if a forbidden heap or libc
formatting symbol is introduced.

`RmFormat_Snprintf()` and `RmFormat_Vsnprintf()` provide the integer, string,
pointer, and fixed-point float formatting used by firmware logging without
stdio allocation. They are a bounded firmware formatter, not a promise of full
libc `printf` compatibility.

Heap-backed compatibility APIs remain in components that are not pulled into
`app.elf`, including GPIO/IIC/PWM/SPI registration, message bus, generic math,
CAN communication, and several inactive device drivers. These must be migrated
or removed before their targets can become competition-firmware dependencies.

## Verification baseline

The native suite contains three independently linked test programs for safety,
device-health deadlines, and formatting. It covers exact deadline boundaries,
poll-rate independence, clock wrap, long-offline latching, one-shot callbacks,
invalid deadline rejection, registry capacity, formatting flags and widths,
float rounding, and buffer truncation.

On 2026-07-16 the following passed in Debug:

- all three native test programs;
- formal `app.elf`;
- bare-metal `test_infantry_minimal` comparison target;
- all 30 embedded demo/regression targets.

The formal image uses 45,872 bytes RAM (35.00% of 128 KiB) and 92,476 bytes
Flash (8.82% of 1 MiB). Static registries increased `.bss`, while removing the
unused 20 KiB C-heap reservation and reducing the main-stack reservation from
20 KiB to 8 KiB made the reported RAM budget reflect the formal runtime.

The automatic linked-symbol audit produces no C/FreeRTOS allocation, `_sbrk`,
`snprintf`, or `vsnprintf` symbol in `app.elf`.

Existing newlib `_close`, `_lseek`, `_read`, and `_write` stub warnings and the
RWX load-segment warning are unchanged. This is a compile and host-test gate;
hardware timing, link-loss recovery, CAN saturation, and soak validation remain
required before competition use.

The subsequent
[`runtime_hardening_milestone.md`](runtime_hardening_milestone.md) replaces the
libnosys process-I/O stubs, removes the RWX segment, and adds explicit linker
program-header verification.
