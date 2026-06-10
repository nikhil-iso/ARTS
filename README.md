# ARTS - Amateur Rocketry Telemetry System

ARTS is my flight computer project for logging rocket flight data. The first goal is not to build the whole final flight program at once. The goal is to test each soldered part of the perf board one at a time, make sure the wiring works, and then combine the working pieces later.

The main controller is a Teensy 4.1. Most of the small tests are kept in separate PlatformIO projects under:

```text
flight_computer/testing
```

This keeps the test code away from the main flight computer code.

## Current Hardware Pins

| Part | Teensy Pin / Bus | Notes |
| --- | --- | --- |
| I2C SDA | 18 | Shared by TMP102, KX134, BMP280, and MPU6050 |
| I2C SCL | 19 | Shared I2C clock |
| Buzzer | 5 | Uses `tone()` |
| GPS TX | 0 / RX1 | GPS transmits into Teensy receive |
| GPS RX | 1 / TX1 | Teensy transmits config commands to GPS |
| TMP102 | I2C address `0x48` | Found by scanner |
| KX134 | I2C address `0x1F` | Found by scanner |
| BMP280 | I2C address `0x77` | Confirmed from module |
| MPU6050 | I2C address `0x68` | Used for GPS antenna orientation testing |

For UART wiring, TX and RX are crossed:

```text
GPS TX -> Teensy pin 0 RX1
GPS RX -> Teensy pin 1 TX1
```

## How To Run A Test

Each test is its own PlatformIO project. Open the specific test folder in PlatformIO, upload it to the Teensy, then open the Serial Monitor at `115200` baud unless the test says otherwise.

Example:

```text
flight_computer/testing/i2c_scanner
```

Open that folder, not just the root repo, if PlatformIO is having trouble finding the right `platformio.ini`.

## Test Folder List

### Buzzer Test

Folder:

```text
flight_computer/testing/buzzer_test
```

This checks that the Teensy can drive the buzzer on pin 5.

Expected result:

- one beep at startup
- one beep every 2 seconds
- Serial Monitor prints matching beep messages

### I2C Scanner

Folder:

```text
flight_computer/testing/i2c_scanner
```

This scans the I2C bus and prints all detected devices.

Expected devices so far:

```text
0x1F  KX134/KX13X
0x48  TMP102
0x77  BMP280
0x68  MPU6050, when connected
```

If a device is missing, check 3.3 V, GND, SDA pin 18, SCL pin 19, and the solder joints.

### GPS Test

Folder:

```text
flight_computer/testing/gps_test
```

This checks the Adafruit GPS on `Serial1`.

Expected result:

- raw NMEA lines print in Serial Monitor
- summary lines show fix status, satellite count, UTC time, and position once it gets a fix
- buzzer beeps when a GPS fix is found

No-fix output is still useful. If NMEA lines are printing, the GPS is talking to the Teensy. A fix can take a few minutes, especially indoors.

### TMP102 Test

Folder:

```text
flight_computer/testing/tmp102_test
```

This reads the TMP102 temperature sensor at `0x48`.

Expected result:

- Serial Monitor prints temperature in Celsius and Fahrenheit once per second

### BMP280 Test

Folder:

```text
flight_computer/testing/bmp280_test
```

This reads the BMP280 at `0x77`.

Expected result:

- temperature in Celsius
- pressure in Pascals
- approximate altitude in meters

The altitude is only approximate because it depends on the sea level pressure value used in the code.

### KX134 Test

Folder:

```text
flight_computer/testing/kx134_test
```

This reads the KX134 accelerometer.

Expected result:

- X, Y, and Z acceleration in g
- total acceleration in g

When the board is sitting still, total acceleration should be around 1 g.

## GPS Antenna Orientation Test

Folder:

```text
flight_computer/testing/gps_orientation_test
```

This test is for checking which antenna angles give the best GPS signal. It uses:

- Adafruit GPS for satellite data
- MPU6050 for pitch and roll
- USB Serial to stream CSV data to the computer
- Python to save the CSV and make a hotspot plot

The test does not write to the SD card. The computer records the data instead.

### GPS Orientation Wiring

```text
GPS TX -> Teensy pin 0 RX1
GPS RX -> Teensy pin 1 TX1
MPU6050 SDA -> Teensy pin 18 SDA
MPU6050 SCL -> Teensy pin 19 SCL
All grounds connected
```

MPU6050 default address is `0x68`. If AD0 is tied high, it may be `0x69`.

### What GSV Means

The GPS normally sends basic NMEA sentences like RMC and GGA. For the orientation test, the code enables extra GSV sentences.

GSV gives satellite signal strength values, usually called SNR. This is useful because fix/no-fix alone is too basic for testing antenna orientation. SNR gives a better idea of which angle is actually stronger.

The downside is that the GPS sends more serial text. For a 1 Hz test this is fine.

### Running The Orientation Logger

Upload the firmware from:

```text
flight_computer/testing/gps_orientation_test
```

Install Python packages if needed:

```powershell
pip install pyserial matplotlib
```

Then run the collector from the test folder:

```powershell
python tools\collect_and_plot.py --port COM5 --seconds 600
```

Replace `COM5` with the Teensy serial port.

Hold each orientation for about 60 seconds. For example, test flat, tilted left, tilted right, tilted forward, and tilted backward. The script saves:

```text
gps_orientation_data.csv
gps_orientation_hotspot.png
```

The default plot uses `avg_snr`, which is the average GPS signal strength from visible satellites. Higher is better.

Other metrics can also be plotted:

```powershell
python tools\collect_and_plot.py --port COM5 --seconds 600 --metric best_snr
python tools\collect_and_plot.py --port COM5 --seconds 600 --metric sats_visible
python tools\collect_and_plot.py --port COM5 --seconds 600 --metric sats_used
python tools\collect_and_plot.py --port COM5 --seconds 600 --metric hdop
```

For HDOP, lower is better. For SNR and satellite counts, higher is better.

## Suggested Bring-Up Order

1. Run `buzzer_test`.
2. Run `i2c_scanner`.
3. Run `tmp102_test`.
4. Run `bmp280_test`.
5. Run `kx134_test`.
6. Run `gps_test`.
7. Add the MPU6050 and run `gps_orientation_test`.

This order keeps the debugging simple. If something fails, there are fewer parts involved.
