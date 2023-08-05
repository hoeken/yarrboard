#!/usr/bin/env node

var W3CWebSocket = require('websocket').w3cwebsocket;
var client;
const delay = (millis) => new Promise(resolve => setTimeout(resolve, millis)) 

var socket_retries = 0;
var retry_time = 0;
var last_heartbeat = 0;
const heartbeat_rate = 1000;
let messageCount = 0;

function main()
{
    createWebsocket();
}

function createWebsocket()
{
    client = new W3CWebSocket('ws://yarrboard.local/ws');

    client.onerror = function() {
        console.log('[socket] Connection error');
    };

    client.onopen = function() {
        console.log('[socket] Connected');

        //we are connected, reload
        socket_retries = 0;
        retry_time = 0;
        last_heartbeat = Date.now();

        //our connection watcher
        setTimeout(sendHeartbeat, heartbeat_rate);

        doLogin("admin", "admin");

        setTimeout(fadePin, 100);
        //setTimeout(togglePin, 100);
        
        let cmd;
        //cmd = {"cmd":"ping"}
        //cmd = {"cmd":"toggle_channel","id": 0};
        //cmd = {"cmd":"set_channel","id": 0, "state":true};
        cmd = {"cmd":"set_channel","id": 0, "duty":0.5};
        
        //setTimeout(function (){speedTest(cmd, 10)}, 100);
    };

    client.onclose = function() {
        console.log("[socket] Connection closed");
    };

    client.onmessage = onMessage
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
        messageCount++;
        last_heartbeat = Date.now();
        
        if (data.msg == "update")
            true;
            //console.log("update");
        else if (data.pong)
        {
            true;
            //console.log(data.pong);
            //last_heartbeat = Date.now();
        }
        else if (data.ok)
            true;
        else
            console.log(data);

        console.log(`Messages: ${messageCount}`);
    }
}

function sendMessage(message)
{
    if (client.readyState == W3CWebSocket.OPEN) {
        try {
            //console.log(message.cmd);
            client.send(JSON.stringify(message));
        } catch (error) {
            console.error("Send: " + error);
        }
    }
}

//our heartbeat timer.
function sendHeartbeat()
{
  //did we not get a heartbeat?
  if (Date.now() - last_heartbeat > heartbeat_rate * 3)
  {
    console.log("[socket] Missed heartbeat: " + (Date.now() - last_heartbeat))
    client.close();
    retryConnection();
  }

  //only send it if we're already open.
  if (client.readyState == W3CWebSocket.OPEN)
  {
    sendMessage({"cmd": "ping"});
    setTimeout(sendHeartbeat, heartbeat_rate);
  }
  else if (client.readyState == W3CWebSocket.CLOSING)
  {
    console.log("[socket] she closing " + client.readyState);
    retryConnection();
  }
  else if (client.readyState == W3CWebSocket.CLOSED)
  {
    console.log("[socket] she closed " + client.readyState);
    retryConnection();
  }
}

function retryConnection()
{
  //disable this for now.
  return;

  //bail if its good to go
  if (client.readyState == W3CWebSocket.OPEN)
    return;

  //keep watching if we are connecting
  if (client.readyState == W3CWebSocket.CONNECTING)
  {
    console.log("[socket] Waiting for connection");
    
    retry_time++;

    //tee it up.
    setTimeout(retryConnection, 1000);

    return;
  }

  //keep track of stuff.
  retry_time = 0;
  socket_retries++;
  console.log("[socket] Reconnecting... " + socket_retries);

  //reconnect!
  createWebsocket();

  //set some bounds
  let my_timeout = 500;
  my_timeout = Math.max(my_timeout, socket_retries * 1000);
  my_timeout = Math.min(my_timeout, 60000);

  //tee it up.
  setTimeout(retryConnection, my_timeout);
}

async function speedTest(msg, delay_ms = 10)
{
    while(true) {
        sendMessage(msg);
        await delay(delay_ms);
    }
}

async function exercisePins()
{
    while (true) {
        for (i=0; i<8; i++)
        {
            sendMessage({
                "cmd": "set_channel",
                "id": i,
                "duty": Math.random()
            });
            sendMessage({
                "cmd": "set_channel",
                "id": i,
                "state": true
            });

            await delay(200)
        }

        for (i=0; i<8; i++)
        {
            sendMessage({
                "cmd": "set_channel",
                "id": i,
                "state": false
            });
           	await delay(200)
        }
    }
}

async function togglePin()
{
    sendMessage({
        "cmd": "set_channel",
        "id": 0,
        "duty": 1
    });

    while (true) {
        sendMessage({
            "cmd": "set_channel",
            "id": 0,
            "state": true
        });

        await delay(1000)

        sendMessage({
            "cmd": "set_channel",
            "id": 0,
            "state": false
        });

        await delay(2000)
    }
}

async function fadePin(d = 5)
{
    let steps = 25;
    let channel = 0;
    let max_duty = 1;

    while (true)
    {
        sendMessage({
            "cmd": "set_channel",
            "id": channel,
            "state": true
        });
        await delay(d)

        for (i=0; i<=steps; i++)
        {
            sendMessage({
                "cmd": "set_channel",
                "id": channel,
                "duty": (i / steps) * max_duty
            });

            await delay(d)
        }

        for (i=steps; i>=0; i--)
        {
            sendMessage({
                "cmd": "set_channel",
                "id": channel,
                "duty": (i / steps) * max_duty
            });

            await delay(d)
        }
    }
}

main();
