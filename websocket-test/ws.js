#!/usr/bin/env node
const WebSocket = require('ws');

const client = new WebSocket('ws://gnarboard.local/ws');

client.on('error', console.error);
client.on('open', onConnect);
//client.on('ping', heartbeat);
client.on('close', function clear() {
  console.log("closed");
});
client.on('message', console.log);

function onConnect() {
    console.log('WebSocket Client Connected');

    this.send(JSON.stringify({
        "cmd": "login",
        "user": "admin",
        "pass": "admin"
    }));

    setTimeout(fadePin, 1000);
    //setTimeout(togglePin, 1000);
    //setTimeout(speedTest, 1000);
}

function onMessage(data) {
    console.log(data);
    if (typeof data === 'string') {
        let json = JSON.parse(data);
        //if (data.msg != "update")
        console.log(json);
    }
}

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
    let d = 20;
    let channel = 6;
    let max_duty = 1;

    client.send(JSON.stringify({
        "cmd": "set_state",
        "id": channel,
        "value": true
    }));

    while (true) {
        for (let i=0; i<=steps; i++)
        {
            client.send(JSON.stringify({
                "cmd": "set_duty",
                "id": channel,
                "value": (i / steps) * max_duty
            }));

            await delay(d)
        }

        for (let i=steps; i>=0; i--)
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
