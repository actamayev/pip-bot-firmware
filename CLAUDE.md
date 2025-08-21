This is an embedded systems project, using PlatformIO, Arduino Framework

The MCU is an ESP32 S3. I use both cores. I use both of my ESP's I2C lines

The first I2C line: is connected to pins: 18 (SDA), and 8 (SCL), and has the following devices:
1. Left Side Tof: 0x51
2. Right Side Tof: 0x60
3. Multizone: 0x29
4. IMU: 0x4A
5. Display: 0x3C
6. Color Sensor: 0x10

The second I2C line is connected to pins: 9 (SDA), and 10 (SCL), and has the following devices:
1. Battery monitor 0x55

Other sensors not connected to I2C include:
- 2 Buttons: Pins 1 & 12
- Encoders for each motor (47 & 48 for Left motor) and (2 and 1 for Right motor)
- Speaker on pins 13, 14, and 21 (DOUT, BCLK, LRC, respectively)
- A string of 5 IR sensors using a multiplexer (A0: 17, A1: 16, A2: 15, MUX_OUT: 7, IR_EN: 6)
- A visible, downward-facing LED for the color sensor on pin 5
- 8 LEDS using NeoPixel library, on pin 4.

After editing my code, do not build my project.
