# Yarrboard

Pronounced yarrrrrrr * brd.

An open hardware project for digital switching on boats, or anywhere with 12v-24v systems.

The project is based around the amazing esp32 and currently consists of one board, an 8-channel mosfet driver.  With this board, you can control almost any DC load on your boat. There are future plans to design a relay board, an I/O board, etc.

## 8 Channel Mosfet Driver

The main (and only) board available is for driving DC loads.  It is designed with common DC loads in mind, such as: lights, pumps, motors, relays, and other electronics.  Here are some specifications:

* 12v to 30v supply voltage (supports both lead acid and lithium battery systems in either 12v or 24v configuration)
* 8 separately controllable channels with PWM capability on each channel
* Each channel capable of up to 20A, with appropriate cooling
* Current monitoring accurate to about 0.01A on each channel
* Firmware supports soft fuses to protect from overcurrent
* Made with heavy duty 2oz copper and thick copper busbars to handle big loads
* Each channel is individually fused, with a manual bypass in case of MOSFET failure
* Potential expandability with QWIIC headers and a serial port for possible NMEA2000
* 3D printable case w/ design files

![Image of 8 channel mosfet driver](/assets/images/8ch-mosfet-driver.jpg)
![Closeup of 8 channel mosfet driver](/assets/images/8ch-mosfet-driver-closeup.jpg)

## Interface

Since it is based on the esp32, the main communication method is over WiFi using websockets.  This makes for very easy interfacing if you want to write your own scripts.  It also means that the user interface for setup, config, and control are very easy.  Just point your phone to the url of the board and you're good to go.

![Screenshot of UI](/assets/images/ui-screenshot.png)

## SignalK

There is also a plugin for integration with SignalK, so you can have your data all in one place.  It supports two way comms, so you can use SignalK and Node-RED to turn your loads on or off.

The plugin for SignalK is called [signalk-yarrboard-plugin](https://github.com/hoeken/signalk-yarrboard-plugin)

## How To Get One

No production as of right now, but possibly in the future.  It is 100% open source, so its possible to order PCBs, parts, and DIY your own board if you like.  Would be interested in partnering with someone to take care of the manufacturing side of things.

## Installation and Setup

* Flash the firmware
* Connect to Yarrboard wifi
* Open browser to http://yarrboard.local or http://yarrboard
* Update network settings to connect to your boat wifi
* Reconnect to boat wifi and 
* Open browser to http://yarrboard.local or http://yarrboard
* Update board settings, such as login info, channel names, soft fuses, etc.
* Install SignalK + signalk-yarrboard-plugin and configure
* Setup any Node-RED flows and custom logic you want.

## Protocol

The protocol for communicating with Yarrboard is entirely based on JSON. Each request to the server should be a single JSON object, and the server will respond with a JSON object.

### Websockets Protocol

Yarrboard provides a websocket server on http://yarrboard.local/ws

Clients communicating over websockets can send a **login** command, or include your username and password with each request.

### Web API Protocol

Yarrboard provides a POST API endpoint at **http://yarrboard.local/api/endpoint**

Examples of how to communicate with this endpoint:

```
curl -i -d '{"cmd":"ping"}'  -H "Content-Type: application/json"  -X POST http://yarrboard.local/api/endpoint
curl -i -d '{"cmd":"set_channel","user":"admin","pass":"admin","id":0,"state":true}'  -H "Content-Type: application/json"  -X POST http://yarrboard.local/api/endpoint
```
Note, you will need to pass your username/password in each request with the Web API.

Additionally, there are a few convenience urls to get basic info.  These are GET only and optionally accept the **user** and **pass** parameters.

* http://yarrboard.local/api/config
* http://yarrboard.local/api/stats
* http://yarrboard.local/api/update

Some example code:

```
curl -i -X GET http://yarrboard.local/api/config
curl -i -X GET http://yarrboard.local/api/config?user=admin&pass=admin
curl -i -X GET http://yarrboard.local/api/stats?user=admin&pass=admin
curl -i -X GET http://yarrboard.local/api/update?user=admin&pass=admin
```

### Serial API Protocol

Yarrboard provides a USB serial port that communicates at 115200 baud.  It uses the same JSON protocol as websockets and the web API.

Clients communicating over serial can send a **login** command, or include your username and password with each request.

Each command should end with a newline (\n) and each response will end with a newline (\n).

This port is also used for debugging, so make sure you check that each line parses into valid JSON before you try to use it.

## Links
