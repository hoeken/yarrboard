var socket = new WebSocket("ws://" + window.location.host + "/ws");
var current_page;
var current_config;
var firstload = true;

const ChannelNameEdit = (name) => `
<div class="col-12">
  <label for="fBoardName" class="form-label">Board Name</label>
  <input type="text" class="form-control" id="fBoardName" value="${name}">
  <div class="valid-feedback">Saved!</div>
  <div class="invalid-feedback">Must be 30 characters or less.</div>
</div>
`;

const ChannelEditRow = (id, name, soft_fuse) => `
<div class="col-md-8">
  <label for="fChannelName${id}" class="form-label">Channel ${id} Name</label>
  <input type="text" class="form-control" id="fChannelName${id}" value="${name}">
  <div class="valid-feedback">Saved!</div>
  <div class="invalid-feedback">Must be 30 characters or less.</div>
</div>
<div class="col-md-2">
  <label for="fDimmable${id}" class="form-label">Dimmable?</label>
  <select id="fDimmable${id}" class="form-select">
    <option value="0">No</option>
    <option value="1">Yes</option>
  </select>
  <div class="valid-feedback">Saved!</div>
</div>
<div class="col-md-2">
  <label for="fSoftFuse${id}" class="form-label">Soft Fuse</label>
  <div class="input-group mb-3">
    <input type="text" class="form-control" id="fSoftFuse${id}" value="${soft_fuse}">
    <span class="input-group-text">A</span>
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be a number between 0 and 20</div>
  </div>
</div>
`;

socket.onopen = function(e) {
  console.log("[open] Connection established");

  //check to see if we want a certain page
  if (window.location.hash)
  {
    let pages = ["control", "config", "stats", "network", "firmware", "login"];
    let page = window.location.hash.substring(1);
    if (pages.includes(page))
      open_page(page);
  }  
};

socket.onmessage = function(event)
{
  const msg = JSON.parse(event.data);

  if (msg.msg == 'config')
  {
    current_config = msg;
    console.log(msg);

    //let the people choose their own names!
    $('#boardName').html(msg.name);
    document.title = msg.name;

    //update our footer automatically.
    $('#projectName').html(msg.version);

    //populate our channel control table
    if (current_page != "control" || firstload)
    {
      $('#channelTableBody').html("");
      for (ch of msg.channels)
      {
        $('#channelTableBody').append(`<tr id="channel${ch.id}" class="channelRow"></tr>`);
        $('#channel' + ch.id).append(`<td class="text-center"><button id="channelState${ch.id}" type="button" class="btn btn-sm" onclick="toggle_state(${ch.id})"></button></td>`);
        $('#channel' + ch.id).append(`<td class="channelName">${ch.name}</td>`);
        $('#channel' + ch.id).append(`<td id="channelDutyCycle${ch.id}" class="text-end"></td>`);
        $('#channel' + ch.id).append(`<td id="channelCurrent${ch.id}" class="text-end"></td>`);
        $('#channel' + ch.id).append(`<td id="channelError${ch.id}" class="channelError"></td>`);
      }
    }

    if (current_page != "stats" || firstload)
    {
      //populate our channel stats table
      $('#channelStatsTableBody').html("");
      for (ch of msg.channels)
      {
        $('#channelStatsTableBody').append(`<tr id="channelStats${ch.id}" class="channelRow"></tr>`);
        $('#channelStats' + ch.id).append(`<td class="channelName">${ch.name}</td>`);
        $('#channelStats' + ch.id).append(`<td id="channelAmpHours${ch.id}" class="text-end"></td>`);
        $('#channelStats' + ch.id).append(`<td id="channelOnCount${ch.id}" class="text-end"></td>`);
        $('#channelStats' + ch.id).append(`<td id="channelTripCount${ch.id}" class="text-end"></td>`);
      }

    }

    if (current_page != "config" || firstload)
    {
      //populate our channel edit table
      $('#channelConfigForm').html(ChannelNameEdit(msg.name));

      //validate + save
      $("#fBoardName").change(validate_board_name);

      for (ch of msg.channels)
      {
        $('#channelConfigForm').append(ChannelEditRow(ch.id, ch.name, ch.softFuse));
        $(`#fDimmable${ch.id}`).val(ch.isDimmable ? "1" : "0");

        //validate + save
        $(`#fChannelName${ch.id}`).change(validate_channel_name);
        $(`#fDimmable${ch.id}`).change(validate_channel_dimmable);
        $(`#fSoftFuse${ch.id}`).change(validate_channel_soft_fuse);
      }
    }

    firstload = false;
  }
  else if (msg.msg == 'update')
  {
    $('#time').html(msg.time);
    for (ch of msg.channels)
    {
      if (ch.state)
      {
        $('#channelState' + ch.id).html("ON");
        $('#channelState' + ch.id).removeClass("btn-danger");
        $('#channelState' + ch.id).removeClass("btn-secondary");
        $('#channelState' + ch.id).addClass("btn-success");
      }
      else if(ch.soft_fuse_tripped)
      {
        $('#channelState' + ch.id).html("TRIP");
        $('#channelState' + ch.id).removeClass("btn-success");
        $('#channelState' + ch.id).removeClass("btn-secondary");
        $('#channelState' + ch.id).addClass("btn-danger");
      }
      else
      {
        $('#channelState' + ch.id).html("OFF");
        $('#channelState' + ch.id).removeClass("btn-success");
        $('#channelState' + ch.id).removeClass("btn-danger");
        $('#channelState' + ch.id).addClass("btn-secondary");
      }

      let duty = Math.round(ch.duty * 100);
      let current = ch.current.toFixed(2);
      $('#channelDutyCycle' + ch.id).html(`${duty}%`);
      $('#channelCurrent' + ch.id).html(`${current}A`);

      if (msg.soft_fuse_tripped)
        $('#channelError' + ch.id).html("Soft Fuse Tripped");
    }
  }
  else if (msg.msg == "stats")
  {
    $("#uptime").html(secondsToDhms(Math.round(msg.uptime/1000)));
    $("#messages").html(msg.messages.toLocaleString("en-US"));
    $("#heap_size").html(formatBytes(msg.heap_size));
    $("#free_heap").html(formatBytes(msg.free_heap));
    $("#min_free_heap").html(formatBytes(msg.min_free_heap));
    $("#max_alloc_heap").html(formatBytes(msg.max_alloc_heap));
    $("#rssi").html(msg.rssi + "dBm");
    $("#uuid").html(msg.uuid);

    for (ch of msg.channels)
    {
      let aH = Math.round(ch.aH * 100) / 100;
      $('#channelAmpHours' + ch.id).html(`${aH}aH`);
      $('#channelOnCount' + ch.id).html(ch.state_change_count.toLocaleString("en-US"));
      $('#channelTripCount' + ch.id).html(ch.soft_fuse_trip_count.toLocaleString("en-US"));
    }
  }
  else
  {
    console.log("Unknown message");
    console.log(msg);
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

function start_gnarboard()
{
  
}

function toggle_state(id)
{
  //OFF or TRIP both switch it to on.
  let new_state = true;
  if ($("#channelState" + id).text() == "ON")
    new_state = false;

  socket.send(JSON.stringify({
    "cmd": "set_state",
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

  //request our stats.
  if (page == "stats")
    get_stats_data();
}

function get_stats_data()
{
  socket.send(JSON.stringify({
    "cmd": "get_stats",
  }));

  //keep loading it while we are here.
  if (current_page == "stats")
    setTimeout(get_stats_data, 1000);
}

function validate_board_name(e)
{
  let ele = e.target;
  let value = ele.value;

  if (value.length <= 0 || value.length > 30)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
    $(ele).removeClass("is-invalid");
    $(ele).addClass("is-valid");

    //set our new board name!
    socket.send(JSON.stringify({
      "cmd": "set_boardname",
      "value": value
    }));
  }
}

function validate_channel_name(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  if (value.length <= 0 || value.length > 30)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
    $(ele).removeClass("is-invalid");
    $(ele).addClass("is-valid");

    //set our new channel name!
    socket.send(JSON.stringify({
      "cmd": "set_channelname",
      "id": id,
      "value": value
    }));
  }
}

function validate_channel_dimmable(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  //convert it
  if (value == "1")
    value = true;
  else
    value = false;

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  socket.send(JSON.stringify({
    "cmd": "set_dimmable",
    "id": id,
    "value": value
  }));
}

function validate_channel_soft_fuse(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = parseFloat(ele.value);

  //real numbers only, pal.
  if (isNaN(value) || value <= 0 || value > 20)
  {
    $(ele).removeClass("is-valid");
    $(ele).addClass("is-invalid");
  }
  else
  {
    $(ele).removeClass("is-invalid");
    $(ele).addClass("is-valid");

    console.log(value);

    //save it
    socket.send(JSON.stringify({
      "cmd": "set_soft_fuse",
      "id": id,
      "value": value
    }));
  }
}

function secondsToDhms(seconds)
{
  seconds = Number(seconds);
  var d = Math.floor(seconds / (3600*24));
  var h = Math.floor(seconds % (3600*24) / 3600);
  var m = Math.floor(seconds % 3600 / 60);
  var s = Math.floor(seconds % 60);
  
  var dDisplay = d > 0 ? d + (d == 1 ? " day, " : " days, ") : "";
  var hDisplay = h > 0 ? h + (h == 1 ? " hour, " : " hours, ") : "";
  var mDisplay = (m > 0 && d == 0) ? m + (m == 1 ? " minute, " : " minutes, ") : "";
  var sDisplay = (s > 0 && d == 0 && h == 0) ? s + (s == 1 ? " second" : " seconds") : "";

  return (dDisplay + hDisplay + mDisplay + sDisplay).replace(/,\s*$/, "");
}

function formatBytes(bytes, decimals = 2) {
  if (!+bytes) return '0 Bytes'

  const k = 1024
  const dm = decimals < 0 ? 0 : decimals
  const sizes = ['Bytes', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB']

  const i = Math.floor(Math.log(bytes) / Math.log(k))

  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(dm))} ${sizes[i]}`
}