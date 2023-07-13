var socket = new WebSocket("ws://" + window.location.host + "/ws");
var current_page = "control";
var current_config;

function start_gnarboard()
{
    //check to see if we want a certain page
    if (window.location.hash)
    {
        const pages = ["control", "config", "stats", "network", "firmware", "login"]
        console.log(window.location.hash);
        let page = window.location.hash.substring(1);
        if (pages.includes(page))
            open_page(page);
    }

    socket.onopen = function(e) {
        console.log("[open] Connection established");
    };
      
    socket.onmessage = function(event)
    {
        const msg = JSON.parse(event.data);

      
        if (msg.msg == 'config')
        {
            current_config = msg;

            $("#boardName").html(msg.name);
            $("#uptime").html(msg.uptime);
            $("#messages").html(msg.totalMessages);
            $("#uuid").html(msg.uuid);
        
            $('#channelTableBody').html("");
            for (ch of msg.channels)
            {
                $('#channelTableBody').append(`<tr id="channel${ch.id}" class="channelRow"></tr>`);
                $('#channel' + ch.id).append(`<td class="text-center"><button id="channelState${ch.id}" type="button" class="btn btn-sm" onclick="toggle_state(${ch.id})"></button></td>`);
                $('#channel' + ch.id).append(`<td id="channelName${ch.id}" class="channelName">${ch.name}</td>`);
                $('#channel' + ch.id).append(`<td id="channelDutyCycle${ch.id}" class="text-end"></td>`);
                $('#channel' + ch.id).append(`<td id="channelCurrent${ch.id}" class="text-end"></td>`);
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
              $('#channelState' + ch.id).removeClass("btn-secondary");
              $('#channelState' + ch.id).addClass("btn-success");
            }
            else
            {
              $('#channelState' + ch.id).html("OFF");
              $('#channelState' + ch.id).removeClass("btn-success");
              $('#channelState' + ch.id).addClass("btn-secondary");
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
      
}

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

function open_page(page)
{
    current_page = page;

    $('.nav-link').removeClass("active");
    $(`#${page}Nav a`).addClass("active");

    $("div.pageContainer").hide();
    $(`#${page}Page`).show();
}