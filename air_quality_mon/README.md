Compact air quality monitor using the [Seeed Grove Air Quality Sensor](https://www.seeedstudio.com/Grove-Air-Quality-Sensor-v1-3-Arduino-Compatible.html), an Arduino Nano, a 128x64 SSH1306 I2C oled, and a generic TM1637 seven segment display.

The seven segment display provides a number and a letter indicating F: fresh, L: low pollution, H: high, or a message "High pollution! Force signal active!!!"

These four levels are detailed on the [wiki page for the sensor](https://wiki.seeedstudio.com/Grove-Air_Quality_Sensor_v1.3/#play-with-arduino)

The oled gives more verbose output and the last 7 readings (128x64 SSH1306)


The sensor has to be warmed up in relatively clean air.

The sensor gets warm/hot in operation; the included sensor protector is optional but will keep you from touching it or damaging the sensor.

The board can be powered by usb or by VIN (the "nano top vin cutout.stl" has a larger cutout by the USB port for a 1x2 2.54 connector.  It should sit in there fine, but you may want some glue)

If you are planning on using 2 monitors for comparison it seems that the input voltage has an effect on the reading from the sensor (~5-10 difference from my experience)

  Parts:

    1 x Arduino Nano
  
    1 x Seeed Grove Air Quality Sensor

    1 x SSH1306 128x64 oled 

    1 x TM1637 seven segment display
  
  Screws:
  
    2 x 5mm M3 standoffs (if you use the protector)
    
    2 x M3x5/M3x6 (also for protector)
    
    4 x M1.6x8 (for nano mounting)
    
    8 x M2x6 (for 7 segment & sensor top mounting)
    
    4 x M2x3 (for oled mounting)
    
    3 x M2x4 (for sensor mounting)
    

Assembly notes:

The sensor and both displays require 5v.  

The ground and 5v on the ICSP header do not work as expected, so avoid those

The white wire of the sensor does not connect to anything.

The screw head on the left side of the sensor is rather close to bridging contacts, recommended to put some insulating tape there

The back clips in, no screws.  It will require some force depending on what filament is used.
