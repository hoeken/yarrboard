
let socket = new WebSocket("ws://" + window.location.host + "/ws");

socket.onopen = function(e) {
  console.log("[open] Connection established");
};

socket.onmessage = function(event) {
  const msg = JSON.parse(event.data);
  //console.log(msg);

  if (msg.msg == 'config')
  {
    $('#nameHeader').html(msg.name);
    $('#stats').html(`Uptime:${msg.uptime}<br/>Messages: ${msg.totalMessages}<br/>UUID: ${msg.uuid}`);

    $('#channelTable').html("<tr><th>State</th><th>Name</th><th>Duty</th><th>Current</th></tr>");
    for (ch of msg.channels)
    {
      $('#channelTable').append(`<tr id="channel${ch.id}" class="channelRow"></tr>`);
      $('#channel' + ch.id).append(`<td><button id="channelState${ch.id}" class="channelState" onclick="toggle_state(${ch.id})"></button></td>`);
      $('#channel' + ch.id).append(`<td id="channelName${ch.id}" class="channelName">${ch.name}</td>`);
      $('#channel' + ch.id).append(`<td id="channelDutyCycle${ch.id}" class="channelDutyCycle"></td>`);
      $('#channel' + ch.id).append(`<td id="channelCurrent${ch.id}" class="channelCurrent"></td>`);
      //$('#channel' + ch.id).append(`<td id="channelAmpHours${ch.id}" class="channelAmpHours"></td>`);
      $('#channel' + ch.id).append(`<td id="channelError${ch.id}" class="channelError"></td>`);
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

      let duty = Math.round(ch.duty * 100);
      //let current = Math.round(ch.current * 100) / 100;
      let current = ch.current.toFixed(2);
      let aH = Math.round(ch.aH * 100) / 100;
      $('#channelDutyCycle' + ch.id).html(`${duty}%`);
      $('#channelCurrent' + ch.id).html(`${current}A`);
      //$('#channelAmpHours' + ch.id).html(`${aH}aH`);

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

function toggle_state(id)
{
  let new_state = false;
  if ($("#channelState" + id).text() == "OFF")
    new_state = true;

  socket.send(JSON.stringify({
    "cmd": "state",
    "id": id,
    "value": new_state
  }));
}