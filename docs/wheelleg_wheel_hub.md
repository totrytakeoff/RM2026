# Wheelleg Wheel Hub Test Notes

Overview
- Test target: wheel hub motors (M3508) basic speed control
- Test firmware: `test/wheelleg_basic_test`
- Remote: ET08 (USART3 SBUS)

Wiring / Bus
- CAN1: wheel hub motors
- Motor IDs: 1 (left wheel), 2 (right wheel)
- Gear ratio: 268:17 (about 15.7647:1)

Control mapping (ET08)
- Left stick Y -> wheel id 1 (inverted for mirror)
- Right stick Y -> wheel id 2 (normal)

Key parameters (current code)
- Speed max: 20000.0 deg/s (motor-side)
- Output speed max at wheel: about 1269.1 deg/s (20000 / 15.7647)
- Control mode: SPEED_LOOP
- Speed PID: Kp=5.0, Ki=0.0, Kd=0.0, MaxOut=12000
- Current PID: Kp=0.4, Ki=0.0, Kd=0.0, MaxOut=15000

Tuning notes
- If jitter appears at certain angles, reduce Speed Kp first (20% step).
- If still jittery, reduce Current Kp slightly (10-20% step).
- After stable, raise Speed Kp in small steps for response.
