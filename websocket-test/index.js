#!/usr/bin/env node

var W3CWebSocket = require('websocket').w3cwebsocket;

//var client = new W3CWebSocket('ws://192.168.1.229:8080');
var client = new W3CWebSocket('ws://192.168.1.229/ws');

const delay = (millis) => new Promise(resolve => setTimeout(resolve, millis)) 

client.onerror = function() {
    console.log('Connection Error');
};

client.onopen = function() {
    console.log('WebSocket Client Connected');

    client.send(JSON.stringify({
        "cmd": "login",
        "user": "admin",
        "pass": "admin"
    }));

    setTimeout(fadePin, 1000);
    //setTimeout(togglePin, 1000);
    //setTimeout(speedTest, 1000);
};

client.onclose = function() {
    console.log('echo-protocol Client Closed');
};

client.onmessage = function(e) {
    if (typeof e.data === 'string') {
        let data = JSON.parse(e.data);
        //if (data.msg != "update")
        console.log(data);
    }
};

async function speedTest()
{
    while(true) {
        client.send(JSON.stringify({
            "cmd": "get_config"
        }));
        await delay(8)
    }
}

async function exercisePins()
{
    while (true) {
        for (i=0; i<8; i++)
        {
            client.send(JSON.stringify({
                "cmd": "set_duty",
                "id": i,
                "value": Math.random()
            }));
            client.send(JSON.stringify({
                "cmd": "set_state",
                "id": i,
                "value": true
            }));

            await delay(200)
        }

        for (i=0; i<8; i++)
        {
            client.send(JSON.stringify({
                "cmd": "set_state",
                "id": i,
                "value": false
            }));
           	await delay(200)
        }
    }
}

async function togglePin()
{
    client.send(JSON.stringify({
        "cmd": "set_duty",
        "id": 0,
        "value": 1
    }));

    while (true) {
        client.send(JSON.stringify({
            "cmd": "set_state",
            "id": 0,
            "value": true
        }));

        await delay(1000)

        client.send(JSON.stringify({
            "cmd": "set_state",
            "id": 0,
            "value": false
        }));

        await delay(2000)
    }
}

async function fadePin()
{
    let steps = 50;
    let d = 40;
    let channel = 6;
    let max_duty = 1;

    client.send(JSON.stringify({
        "cmd": "set_state",
        "id": channel,
        "value": true
    }));

    while (true) {
        for (i=0; i<=steps; i++)
        {
            client.send(JSON.stringify({
                "cmd": "set_duty",
                "id": channel,
                "value": (i / steps) * max_duty
            }));

            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            client.send(JSON.stringify({
                "cmd": "set_duty",
                "id": channel,
                "value": (i / steps) * max_duty
            }));

            await delay(d)
        }
    }
}
