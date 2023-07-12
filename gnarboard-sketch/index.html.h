const char index_html_data[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Gnarboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" type="text/css" href="style.css">
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.0/jquery.min.js"></script> 
</head>
<body>
  <script>
    let socket = new WebSocket("ws://" + window.location.host + "/ws");

    socket.onopen = function(e) {
      console.log("[open] Connection established");
    };

    socket.onmessage = function(event) {
      const msg = JSON.parse(event.data);
      console.log(msg);

      if (msg.msg == 'config')
      {
        $('#nameHeader').html(msg.name);
        $('#stats').html(`Uptime:${msg.uptime}<br/>Messages: ${msg.totalMessages}<br/>UUID: ${msg.uuid}`);

        $('#channels').html();
        for (ch of msg.channels)
        {
          $('#channels').append(`<div id="channel${ch.id}" class="channelRow"></div>`);
          $('#channel' + ch.id).append(`<span id="channelName${ch.id}" class="channelName">${ch.name}</span>`);
          $('#channel' + ch.id).append(`<span><button id="channelState${ch.id}" class="channelState"></button></span>`);
          $('#channel' + ch.id).append(`<span id="channelDutyCycle${ch.id}" class="channelDutyCycle"></span>`);
          $('#channel' + ch.id).append(`<span id="channelCurrent${ch.id}" class="channelCurrent"></span>`);
          $('#channel' + ch.id).append(`<span id="channelAmpHours${ch.id}" class="channelAmpHours"></span>`);
          $('#channel' + ch.id).append(`<span id="channelError${ch.id}" class="channelError"></span>`);
        }
      }
      
      if (msg.msg == 'update')
      {
        $('#time').html(msg.time);
        for (ch of msg.channels)
        {
          if (ch.state)
          {
            $('#channelState' + ch.id).html("ON");
            $('#channelState' + ch.id).removeClass("buttonOff");
            $('#channelState' + ch.id).addClass("buttonOn");
          }
          else
          {
            $('#channelState' + ch.id).html("OFF");
            $('#channelState' + ch.id).removeClass("buttonOn");
            $('#channelState' + ch.id).addClass("buttonOff");
          }

          $('#channelDutyCycle' + ch.id).html((ch.duty * 100) + "%");
          $('#channelCurrent' + ch.id).html(`${ch.current}A`);
          $('#channelAmpHours' + ch.id).html(`${ch.aH}aH`);

          if (msg.soft_fuse_tripped)
            $('#channelError' + ch.id).html("Soft Fuse Tripped");
        }
      }
    };

    socket.onclose = function(event) {
      if (event.wasClean) {
        console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
      } else {
        // e.g. server process killed or network down
        // event.code is usually 1006 in this case
        console.log('[close] Connection died');
      }
    };

    socket.onerror = function(error) {
      console.log(`[error]`);
    };
  </script>
  <h1 id="nameHeader">Gnarboard v1.0.0</h1>
  <p id="channels"></p>
  <p id="time"></p>
  <p id="stats"></p>
</body>
</html>
)rawliteral";