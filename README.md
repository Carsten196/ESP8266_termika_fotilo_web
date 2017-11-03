# ESP8266_termika_fotilo_web
Web Version in Javascript with HTML5 Canvas, with support of Camera overlay to to thermal image

based on Example FSBrowser by Hristo Gochkov and Arduino Termika Fotilo.
I created a version without the 1,8" TFT. Instead it uses a Web Browser. So it should work with all Computers, smart phone, tablets...
But it will not work completely on iPhone devices, cause ther is no support for getUserMedia() to get access to the camera.
It supports three different Webservers with DHCP and fixed IPs, on connection fail is will a be a webserver itself.

The only parts you need are:
- Wemos D1 mini ($2,50 on Alibaba)
- Diode 1N4148 (less than 5 cents, to reduce sensor voltage)
- optional Zener-Diode Z2,7 (less than 5 cents, stabilization of sensor voltage)
- optional Resistor 820 ohms or any other (less than 5 cents, for the Zener-Diode)
- Melexis MLX90621 16x4 thermal Sensor (42€ with tax ans shipping  http://www.as-electronic.net)
- optional a 0.96" I2C OLED ($2,50 on Alibaba, to show IP-Address and something more)

connect the sensor to pins D1 and D2 (I2C Clock and Data),
connect sensor GND to G,
connect the unmarked side of 1N4148 to 3,3V and the marked side to sensor power supply.

optional connect Zener Diode in series with resistor marked side to sensor power supply, the other side to G,
optional connect A0 to sensoe power supply, too for Voltage measurement,
optional connect the OLED to 5V, G, D1 and D2.

now edit arduino File and enter SSID and password of your home router.
compile and upload Arduino File and SPIFFS Data with Arduino IDE.

now it should work :)

look on Display or serial Port for IP Adress.
Enter IP Address in your browser, the demo site will load. If not try IP-Adress/index.htm .

operating instructions:
- Camera: Test if your device supports camera captureing.
- get stream: Start your camera, you will be asked if want this.
- Start/Stop Start Stop Data Transfer, now camera will be captured with thermal picture overlay.
- with mintemp maxtemp +/- you can set the temp span.
- with zoom and up/down/left/right you can synchronize camara with thermal image.
- with transparency you can make the thermal image more transpartent to see the camra picture.
- default to set mintemp maxtemp and zoom to default values.

the other buttons don't care

for testing without sensor, the server will send some dummy data.

its the first experimental version, not perfect, but working.


by the way, it's a complete working webserver with SPIFFS(Flash) fileserver with web-based file-eplorer feature !!!












