# TCS3448 Arduino driver contract

Status: Gate 0 contract and Gate 1 public header are drafted for review. The
Gate 2/3 core implementation and Gate 6 hardware-test suite are present for
review. The attached Metro Mini fixture passed every automated test; ten
separate physical cold starts remain a manual repetition check.

## Architecture decision

`Adafruit_TCS3448` subclasses `Adafruit_AS7343`.

TCS3448 and AS7343 use the same part ID (`0x81`), bank selector, register
addresses, gain encodings, 18-result automatic SMUX order, and named spectral
channels. Their fixed I2C addresses differ: TCS3448 is `0x59`, while AS7343 is
`0x39`. TCS3448 calls the measurement engine "ALS" where AS7343 calls it
"spectral"; the corresponding register bit locations are unchanged.

AS7341 is a structural precedent but not a safe parent class. It uses a
different identification-register location and a substantially different
banked register map, and it exposes only six result registers at a time.

The subclass adds the correct default address, TCS3448-prefixed aliases, a
public reset, a bus-failure-aware data-ready query, and a coherent non-blocking
18-result read. Existing AS7343 configuration methods remain available.

## Bus, identification, and reset

- Bus: I2C. The fixed 7-bit address is `0x59`.
- The sensor-side bus is 1.2 V or 1.8 V. The breakout shifts SDA and SCL through
  a dual BSS138 stage and pulls the sensor side to 1.8 V.
- Registers `0x58` through `0x66` require `CFG0.REG_BANK` at `0xBF`, bit 4, to
  be set.
- Part ID is read from banked register `0x5A` and must equal `0x81`.
- `AUXID` is at `0x58`; `REVID` is at `0x59`.
- Software reset is `CONTROL` at `0xFA`, bit 3. It forces a power-on reset.
- The device NAKs I2C while initializing. The datasheet gives 200 us as the
  typical internal initialization interval and separately forbids I2C access
  during the first 600 us while it samples GPIO to select the bus voltage.

## Measurement semantics

- The initial driver default remains the AS7343 default: manual 256x gain,
  `ATIME=29`, `ASTEP=599`, and 18-result automatic SMUX mode.
- One integration is about 50.04 ms:
  `(ATIME + 1) * (ASTEP + 1) * 2.78 us`.
- Eighteen-result mode runs three automatic SMUX cycles in this order:
  `FZ, FY, FXL, NIR, VIS_TL, VIS_BR`, then
  `F2, F3, F4, F6, VIS_TL, VIS_BR`, then
  `F1, F7, F8, F5, VIS_TL, VIS_BR`.
- `STATUS2.AVALID` at `0x90`, bit 6, reports completion.
- Reading `ASTATUS` at `0x94` latches all 36 result bytes. A consecutive read
  from `0x94` through `0xB8` makes the gain, saturation flag, and counts
  concurrent.
- Each ADC result is unsigned 16-bit raw counts, low byte first. Raw counts are
  not lux, irradiance, or normalized values.
- The first API milestone does not enable sensor AGC and does not claim
  calibrated output. Gain-normalized and calibrated data will be added only
  after their semantics are reviewed.
- `getDataReady()` and `readData()` are non-blocking. Sketches decide whether
  and how long to poll.

## GPIO and interrupt electrical contract

- TCS3448 INT is active-low, open drain. The breakout pulls INT to its 3.3 V
  rail through 10 kOhm, so Metro Mini D2 is input-only and may use its external
  interrupt hardware without enabling the Metro's internal pull-up.
- Rev E pulls TCS3448 GPIO to the breakout 3.3 V rail through 10 kOhm. The
  attached earlier-revision fixture has the same 10 kOhm pull-up hand-soldered.
- The TCS3448 samples GPIO once, 600 us after power-up, to select 1.2 V or
  1.8 V I2C signaling. The current datasheet says GPIO must not float during
  that interval.
- After that scan, GPIO is an open-drain I/O. A 5 V Metro Mini must never drive
  it high or enable a 5 V internal pull-up. GPIO output testing must use an
  external pull-up no higher than 3.3 V and keep Metro D3 input-only.
- The verified 10 kOhm pull-up selects the sensor's 1.8 V I2C interface and
  gives D3 a safe readable high level after startup.

## Actual test fixture

- Controller: Metro Mini.
- TCS3448 INT: D2.
- TCS3448 GPIO: D3.
- External UVA LED control: D4, active HIGH. It defaults LOW and is returned LOW
  on every optical-test exit.
- NeoPixel 16 ring data: D6.
- I2C: the Metro Mini hardware SDA/SCL pins.
- Optical tests will discard the first sample after each emitter transition,
  average emitter-off and emitter-on readings, reject saturated results, and
  leave all emitters off on every success and failure path.
- Bench verification observed the expected strongest wavelength groups for red,
  green, blue, and UVA illumination. The UVA response was strongest on F1.
- Metro D3 remains input-only while reading sensor GPIO output. When testing
  sensor GPIO input, the Metro only pulls D3 LOW or releases it; it never drives
  the 3.3 V node HIGH.

## Primary sources

- ams OSRAM, `TCS3448 14-channel multi-spectral sensor`, DS001121 v2-00,
  2026-06-03:
  <https://look.ams-osram.com/m/1c24b057e65ee61e/original/TCS3448-14-Channel-multi-spectral-sensor.pdf>
  - PDF SHA-256:
    `305738d08f15d865c8519f295e3285ce1f5745c58821d74ea3c817ded74d08e3`
  - Hermes receipt:
    `2026-08-25T162846+0000-vendor_pdf-https-look.ams-osram.com-m-1c24b057e65ee61e-orig-8f16ad3d3f`
- ams OSRAM, `AS7343 14-Channel Multi-Spectral Sensor`, DS001046 v6-00,
  2023-06-07:
  <https://look.ams-osram.com/m/5f2d27fff9a874d2/original/AS7343-14-Channel-Multi-Spectral-Sensor.pdf>
  - PDF SHA-256:
    `4f322d30ca6435a4c58ec09aa8633838cc8af35ba46bc67fa8eaeb386d65b102`
- ams OSRAM, `AS7341`, DS000504 v3-00, 2020-06-25:
  <https://look.ams-osram.com/m/24266a3e584de4db/original/AS7341-DS000504.pdf>
  - PDF SHA-256:
    `6036d333ca526e1a28dc47401cf8350d47156c2a9614e5620c617a8bf39a23f0`
- Adafruit TCS3448 Rev D schematic and board in `MBAdafruitBoards`, commit
  `4585c0f8607c8c529975c619d84714586b83143a`.
