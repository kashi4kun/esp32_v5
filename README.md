# ESP32 SpO2 Monitor

This Qt application connects to an ESP32 device that streams MAX30102 sensor readings. It visualises IR, RED, BPM, temperature and calculates SpO₂ by two methods:

1. **AC/DC** – continuous calculation using the last few seconds of data.
2. **Peak Cycle** – calculation for each pulse interval.

## Building

Requirements: Qt 6 with Charts module, C++17 compiler.

```bash
qmake esp32_v5.pro
make
```

Run the resulting executable and configure the ESP32 IP address if needed.

Exported data is written to `Result/` and `Result_Binar/`.
