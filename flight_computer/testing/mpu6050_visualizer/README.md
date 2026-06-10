# MPU6050 Rocket Orientation Visualizer

This test streams pitch, roll, and yaw from the Teensy to the computer and draws a simple 3D rocket orientation view.

## Important Note

The MPU6050 has an accelerometer and gyroscope, but no magnetometer. That means:

- pitch and roll are corrected by gravity from the accelerometer
- yaw comes only from the gyro
- yaw will slowly drift over time

This is still useful for bench testing and seeing the rocket rotate, but it is not a perfect long-term attitude solution.

## Wiring

```text
MPU6050 SDA -> Teensy pin 18 SDA
MPU6050 SCL -> Teensy pin 19 SCL
MPU6050 VCC -> correct power pin for your breakout
MPU6050 GND -> Teensy GND
```

Default MPU6050 address is `0x68`. If AD0 is tied high, it may be `0x69`.

## Run

1. Open this folder in PlatformIO.
2. Upload the firmware to the Teensy.
3. Keep the rocket still during gyro calibration at startup.
4. Install Python packages if needed:

```powershell
pip install pyserial matplotlib numpy
```

5. Run the visualizer:

```powershell
python tools\visualize_orientation.py --port COM5
```

Replace `COM5` with the Teensy serial port.

The red line is the rocket body/nose direction. The code assumes the rocket nose is along the sensor's local `+Y` direction. If the display moves wrong, change the `nose` and `tail` vectors in `tools/visualize_orientation.py`.
