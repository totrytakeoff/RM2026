# Third-party notices

This repository contains third-party code under its respective licenses.
Copyright notices embedded in vendor source files remain authoritative.

## YueLu basic_framework

Portions of the transitional BSP and robot modules originate from the YueLu
team open-source `basic_framework`. The referenced upstream distribution
contains the following MIT license:

Historical headers in the PID, CAN communication, DJI motor, motor-definition,
and remote-control sources also carried the notice
`Copyright (c) 2022 HNU YueLu EC all rights reserved`. Those source-level team
labels are consolidated here so ordinary repository-owned APIs and narration
remain origin-neutral without dropping the upstream attribution.

```text
MIT License

Copyright (c) 2022 NeoZng

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

STM32 HAL, CMSIS, FreeRTOS, STM32 USB Device, CMSIS-DSP, and SEGGER sources are
kept under `third_party/` with their own authoritative license notices.
