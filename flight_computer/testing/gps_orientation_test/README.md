# GPS Orientation Test

This test streams GPS quality and MPU6050 tilt data from the Teensy to the computer.
The computer saves the data as CSV and creates a hotspot plot.

## What GSV Enables

The GPS normally sends basic sentences such as RMC and GGA. This test enables extra
NMEA sentences, including GSV. GSV reports satellites in view and each satellite's
signal strength/SNR. That extra serial data is useful for antenna testing because it
shows more than just fix/no-fix.

## Wiring

- GPS TX -> Teensy pin 0 RX1
- GPS RX -> Teensy pin 1 TX1
- MPU6050 SDA -> Teensy pin 18 SDA
- MPU6050 SCL -> Teensy pin 19 SCL
- GPS and MPU6050 power -> correct 3.3 V/VIN pins for the modules
- All grounds connected

## Run

1. Open this folder in PlatformIO.
2. Upload the firmware to the Teensy.
3. Install Python packages if needed:

```powershell
pip install pyserial matplotlib
```

4. Run the collector, replacing `COM5` with your Teensy's serial port:

```powershell
python tools\collect_and_plot.py --port COM5 --seconds 600
```

Hold each antenna orientation for about 60 seconds. The script saves:

- `gps_orientation_data.csv`
- `gps_orientation_hotspot.png`

The default hotspot color is `avg_snr`, average satellite signal strength.
