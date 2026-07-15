# Infantry bare-metal comparison firmware

This target preserves the original single-loop scheduling shell for hardware
comparison during the FreeRTOS migration. It is no longer the owner of infantry
control logic.

Both this target and the formal `app.elf` compile the modules under
`application/infantry`:

```text
application/infantry
  command -> input arbitration
  chassis -> mecanum and follow control
  gimbal  -> yaw/pitch control
  shoot   -> friction wheels and loader
  referee -> read-only interlocks
```

The comparison target schedules INS, daemon, and motor control from a polling
loop and executes the high-level application every 20 ms. The formal target
uses static FreeRTOS tasks. Control parameters and application call order are
shared, so observed differences should primarily come from scheduling.

Build both targets before hardware comparison:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target app.elf test_infantry_minimal --parallel
```

The acceptance checklist is maintained in
`docs/migration/infantry_freertos_baseline.md`.
