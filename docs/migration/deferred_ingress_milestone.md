# Deferred communication ingress milestone

Date: 2026-07-16

This milestone removes protocol parsing and device-state publication from the
formal infantry firmware's CAN/UART interrupt path. It does not change PID
parameters, actuator limits, input arbitration, or control-mode transitions.

## Data flow

```text
CAN/UART interrupt
    -> copy bounded receive data
    -> endpoint inbox
    -> 5 ms motor task dispatch
    -> protocol validation and parsing
    -> coherent device snapshot
    -> control/diagnostic consumer
```

The existing motor task owns dispatch before `DJIMotorControl()`. This keeps
motor feedback parsing and PID consumption in one task and makes ingress work
part of the existing 5 ms execution/deadline monitor instead of introducing an
unmonitored communication task.

## Inbox contracts

`platform/common/transport/rm_rx_queue` is a platform-independent,
caller-storage queue. It never allocates memory. Its single-core ISR producer
and task consumer are serialized by a short STM32 critical section while the
consumer copies one retained item.

| Transport | Retention | Overflow policy | Formal dispatch bound |
| --- | --- | --- | ---: |
| CAN | one slot per endpoint | replace unconsumed feedback with latest | 16 callbacks / 5 ms |
| UART | four events per endpoint | discard oldest, retain newest event | 12 callbacks / 5 ms |

CAN feedback represents current device state, so replaying every 1 kHz motor
frame would add latency without adding control information. Coalescing leaves
at most one latest frame per endpoint for each dispatch step. UART retains
event boundaries because receive-to-idle DMA may deliver partial stream data.

In deferred UART mode, DMA storage is separate from the callback-visible
`recv_buff`. The ISR can restart DMA immediately without overwriting bytes that
the task is parsing. ET08 still requires an exact 25-byte SBUS frame; VT keeps
its stream reassembly and CRC validation.

Queue counters distinguish expected CAN coalescing from rejected input and
UART event overwrite. `CANGetDispatchStats()` and
`USARTGetDispatchStats()` expose aggregate counters without formatting or
logging in interrupt context.

## Compatibility and startup

Interrupt dispatch remains the default for existing board demos. Firmware that
wants task-context parsing must call `CANConfigureDispatch()` and
`USARTConfigureDispatch()` before the first endpoint registration; changing
mode afterward is rejected.

The formal firmware selects deferred mode before `InfantryApp_Init()`. During
the remaining startup sequence, CAN retains the latest feedback and UART
retains the newest four receive events. This prevents stale, unbounded startup
backlogs while still allowing DMA and CAN reception to begin before the
scheduler.

## Coherent device reads

Deferred parsing alone does not make task-to-task state access safe. The formal
application therefore no longer reads live ET08, VT, or DJI structures:

- `ET08_Read()` copies a protected control snapshot and confirms health before
  and after the copy;
- `VT_Read()` applies the same contract to CRC-validated VT state;
- `DJIMotorGetMeasure()` copies encoder, speed, current, and temperature as one
  protected feedback snapshot;
- DJI decoding builds the next state locally and publishes it in one short
  critical section;
- the first valid DJI feedback frame establishes the encoder baseline instead
  of being interpreted as a wrap relative to the zero-initialized structure.

Legacy live-view getters remain for comparison demos, but new concurrent
firmware must use snapshot APIs.

## Verification baseline

The native suite now contains four programs. The receive-queue test covers FIFO
order, index wrap, newest-data overflow, single-slot coalescing, rejected input,
and destination-capacity failure. The existing safety, device-health, and
formatter tests remain unchanged.

In Debug, the formal image uses 50,696 bytes RAM (38.68% of 128 KiB) and 95,028
bytes Flash (9.06% of 1 MiB). The additional RAM is fixed UART DMA/event inbox
storage; no heap reservation was reintroduced. The linked-symbol guard still
reports no C/FreeRTOS allocation, `_sbrk`, `snprintf`, or `vsnprintf` symbol.

Compilation and host tests cannot validate interrupt timing or bus behavior.
Hardware acceptance still requires CAN-load measurement, UART burst/partial
frame injection, link-loss recovery, task execution-time observation, and a
long-running soak test.

## Remaining ownership debt

Motor command fields and PID runtime are still shared between the 20 ms control
task and 5 ms motor task through transitional mutable motor objects. INS
attitude is likewise published as a live structure. These are the next
ownership boundaries to replace with command/state snapshots; this milestone
only claims coherent communication feedback ingress.

Device initialization results and individual motor-health deadlines are not yet
aggregated into the top-level safety manager. A missing actuator therefore
stays locally stopped, but does not yet force a robot-wide initialization or
runtime fault. That aggregation is required before the framework can claim
whole-robot device-fault containment.
