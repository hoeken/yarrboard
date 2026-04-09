# Yarrboard

A collection of open hardware projects for automation projects on your boat.

The project is broken into a few different repositories:

* [FrothFET](https://github.com/hoeken/frothfet) - 8 Channel Digital Switching board with pwm + load monitoring
* [FrothFET Firmware](https://github.com/hoeken/frothfet-firmware) - Firmware for the FrothFET board
* [Brineomatic](https://github.com/hoeken/brineomatic) - Watermaker Controller
* [Brineomatic Firwmare](https://github.com/hoeken/brineomatic) - Firmware for the Brineomatic board
* [SendIt](https://github.com/hoeken/sendit) - 8 channel sensor multitool board
* [SendIt Firwmare](https://github.com/hoeken/sendit-firmware) - Firmware for the SendIt board
* [Yarrboard Framework](https://github.com/hoeken/YarrboardFramework) - Common framework that all the boards run on
* [SignalK Plugin](https://github.com/hoeken/signalk-yarrboard-plugin) - Plugin for SignalK integration
* [Yarrboard Client](https://github.com/hoeken/yarrboard-client) - JS client for controlling a board over websocket

## Interface

Since it is based on the esp32, the main communication method is over WiFi.  Setup, config, and control are very easy using the built-in web portal.  Just point your phone to the url of the board and you're good to go.  Typically this is http://yarrboard.local or http://yarrboard.  You can change this hostname in the settings, or use the IP address instead.

![Screenshot of UI](/assets/images/ui-screenshot.png)

## SignalK

There is also a plugin for integration with SignalK, so you can have your data all in one place.  It supports two way comms, so you can use SignalK and Node-RED to turn your loads on or off.

The plugin for SignalK is called [signalk-yarrboard-plugin](https://github.com/hoeken/signalk-yarrboard-plugin)

## How To Get One

No production as of right now, but possibly in the future.  It is 100% open source, so its possible to order PCBs, parts, and DIY your own board if you like.  There is the potential of partnering with the right person to take care of the manufacturing side of things, or you could make and sell your own - its open source after all!
