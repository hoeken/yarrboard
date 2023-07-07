#!/usr/bin/env node

var W3CWebSocket = require('websocket').w3cwebsocket;

var client = new W3CWebSocket('ws://192.168.1.229:80');

const delay = (millis) => new Promise(resolve => setTimeout(resolve, millis)) 

client.onerror = function() {
    console.log('Connection Error');
};

client.onopen = function() {
    console.log('WebSocket Client Connected');

    setTimeout(exercisePins, 1000);
};

client.onclose = function() {
    console.log('echo-protocol Client Closed');
};

client.onmessage = function(e) {
    if (typeof e.data === 'string') {
        let data = JSON.parse(e.data);
        console.log(data);
    }
};

async function exercisePins()
{
    while (true) {
        for (i=0; i<8; i++)
        {
            client.send(JSON.stringify({
                "cmd": "duty",
                "id": i,
                "value": Math.random()
            }));
            client.send(JSON.stringify({
                "cmd": "state",
                "id": i,
                "value": true
            }));

            await delay(100)
        }

        for (i=0; i<8; i++)
        {
            client.send(JSON.stringify({
                "cmd": "state",
                "id": i,
                "value": false
            }));
           	await delay(100)
        }
    }
}
