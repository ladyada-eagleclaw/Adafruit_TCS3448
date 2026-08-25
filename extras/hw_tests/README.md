# TCS3448 hardware tests

These sketches target the attached Metro Mini fixture:

- TCS3448 `INT` to D2, using the breakout's external 3.3 V pull-up.
- TCS3448 `GPIO` to D3, pulled to 3.3 V through 10 kOhm.
- Active-high UVA LED control to D4.
- 16-pixel NeoPixel ring data to D6.

Run the numbered sketches in order. Every automated test prints `ALL TESTS
PASSED` only after restoring its outputs to a safe state.

| Test | Coverage |
| --- | --- |
| `00_basic` | Identification, coherent read, reset, and power re-enable |
| `01_cold_start` | One complete power-up; repeat with ten physical power removals |
| `02_i2c_speed` | 100 kHz and 400 kHz identification and measurement |
| `03_gpio_output` | Sensor GPIO release, pull-low, and high-impedance cleanup |
| `04_gpio_input` | Metro pull-low/release without ever driving the 3.3 V node HIGH |
| `05_rgb_uva` | Red, green, blue, and active-high UVA wavelength-group response |
| `06_smux_modes` | 6-, 12-, and 18-result automatic SMUX modes |
| `07_gain` | Every gain code, concurrent gain report, and spectral response |
| `08_integration_time` | ATIME/ASTEP calculation, readback, and count scaling |
| `09_saturation` | Unsaturated and deliberately saturated coherent frames |
| `10_spectral_interrupt` | Threshold status and active-low D2 assertion/clear |
| `11_persistence` | One-cycle versus five-cycle interrupt persistence |
| `12_power_wait` | Power recovery, WTIME effect, and low-power wait operation |
| `13_autozero` | Auto-zero code readback, unsaturated frames, and timing report |
| `14_flicker` | Generated and detected 100 Hz and 120 Hz visible flicker |
| `15_long_duration` | Thirty seconds of continuous coherent reads at 400 kHz |

`01_cold_start` deliberately does not count resets in EEPROM. Opening Serial on
a Metro Mini resets the MCU without removing sensor power, so an automated boot
counter would falsely count Serial Monitor resets as TCS3448 cold starts.
