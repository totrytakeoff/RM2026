# Wheelleg IMU Baseline (DM-IMU-L1)

Status
- CAN2 request mode OK, USART6 output OK
- Mounting: normal orientation, right-hand coordinate system

Coordinate definition (from vendor)
- X axis: roll
- Y axis: pitch
- Z axis: yaw

Baseline sample (static)
- Accel (m/s^2): A[-0.571 -0.413 9.988]
- Gyro (rad/s):  G[-0.003 -0.002 -0.006]
- Euler (deg):  E[-2.38 3.20 -54.95]
- Quaternion:   Q[0.887 -0.006 0.034 -0.460]
- Valid mask:   0x0F

Notes
- These values are for the current mounting orientation and will be used for balance initialization.
