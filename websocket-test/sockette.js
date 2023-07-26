#!/usr/bin/env node
global.WebSocket = require('ws');
const Sockette = require('sockette');

const client = new Sockette('ws://gnarboard.local/ws', {
  timeout: 1000,
  onopen: onOpen,
  onmessage: onMessage,
  onreconnect: message => console.log('[socket] reconnecting'),
  onmaximum: message => console.log('[socket] connection failed!'),
  onclose: message => console.log('[socket] connection closed!', message.code, message.reason),
  onerror: error => console.log("[socket] connection error: " + error.toString())
});

function onOpen(message)
{
    console.log('WebSocket Client Connected');

    //doLogin("admin", "admin");

    setTimeout(fadePin, 1000);
    //setTimeout(togglePin, 1000);
    //setTimeout(speedTest, 1000);
}

function doLogin(username, password)
{
    sendMessage({
        "cmd": "login",
        "user": username,
        "pass": password
    });
}

function onMessage(message)
{
    if (typeof message.data === 'string') {
        let data = JSON.parse(message.data);

        if (data.msg == "update")
            console.log("update: " + data.time)
        else
            console.log(data);
    }
}

function sendMessage(message)
{  
    try {
      reTrySafe(() => client.json(message), 3)
    } catch (error) {
        console.error("Send: " + error);
    }
}


async function speedTest()
{
    while(true) {
        sendMessage({"cmd": "get_config"});
        await delay(8)
    }
}

async function exercisePins()
{
    while (true) {
        for (i=0; i<8; i++)
        {
            sendMessage({
                "cmd": "set_duty",
                "id": i,
                "value": Math.random()
            });
            sendMessage({
                "cmd": "set_state",
                "id": i,
                "value": true
            });

            await delay(200)
        }

        for (i=0; i<8; i++)
        {
            sendMessage({
                "cmd": "set_state",
                "id": i,
                "value": false
            });
           	await delay(200)
        }
    }
}

async function togglePin()
{
    sendMessage({
        "cmd": "set_duty",
        "id": 0,
        "value": 1
    });

    while (true) {
        sendMessage({
            "cmd": "set_state",
            "id": 0,
            "value": true
        });

        await delay(1000)

        sendMessage({
            "cmd": "set_state",
            "id": 0,
            "value": false
        });

        await delay(2000)
    }
}

async function fadePin()
{
    let steps = 50;
    let d = 10;
    let channel = 2;
    let max_duty = 1;

    while (true)
    {
        sendMessage({
            "cmd": "set_state",
            "id": channel,
            "value": true
        });

        for (i=0; i<=steps; i++)
        {
            sendMessage({
                "cmd": "set_duty",
                "id": channel,
                "value": (i / steps) * max_duty
            });

            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            sendMessage({
                "cmd": "set_duty",
                "id": channel,
                "value": (i / steps) * max_duty
            });

            await delay(d)
        }

        sendMessage({
            "cmd": "set_state",
            "id": channel,
            "value": false
        });
    }
}

//function to make an easy delay.
const delay = (millis) => new Promise(resolve => setTimeout(resolve, millis));

//function to handle retrying
async function reTryCatch(callback, times = 10) {
  try {
    return await callback()
  } catch (error) {
    if (times > 0) {
      return await reTryCatch(callback, times - 1)
    } else {
      throw error
    }
  }
}

// Helper
// Tries and ignores errors if any occur
const reTrySafe = async (cb, times) => {
  try {
    return await reTryCatch(cb, times)
  } catch(error) {
    return null
  } 
}
