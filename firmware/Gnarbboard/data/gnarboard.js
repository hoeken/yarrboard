var socket = false;
var current_page;
var current_config;
var firstload = true;
var app_username;
var app_password;
var network_config;

const ChannelControlRow = (id, name) => `
<tr id="channel${id}" class="channelRow">
  <td class="text-center"><button id="channelState${id}" type="button" class="btn btn-sm" onclick="toggle_state(${id})"></button></td>
  <td class="channelName">${name}</td>
  <td class="text-end"><button id="channelDutyCycle${id}" type="button" class="btn btn-sm btn-light" onclick="toggle_duty_cycle(${id})">???</button></td>
  <td id="channelCurrent${id}" class="text-end"></td>
</tr>
<tr id="channelDutySliderRow${id}" style="display:none">
  <td colspan="4">
    <input type="range" class="form-range" min="0" max="100" id="channelDutySlider${id}">
  </td>
</tr>
`;

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

const AlertBox = (message, type) => `
<div>
  <div class="mt-3 alert alert-${type} alert-dismissible" role="alert">
    <div>${message}</div>
    <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
  </div>
</div>`;

function start_gnarboard()
{
  socket = new WebSocket("ws://" + window.location.host + "/ws");

  socket.onopen = function(e) {
    console.log("[open] Connection established");
  
    //auto login?
    if (Cookies.get("username") && Cookies.get("password")){
      socket.send(JSON.stringify({
        "cmd": "login",
        "user": Cookies.get("username"),
        "pass": Cookies.get("password")
      }));
    }
  
    //load our config... will also trigger login
    socket.send(JSON.stringify({
      "cmd": "get_config"
    }));

    //load our network config
    socket.send(JSON.stringify({
      "cmd": "get_network_config"
    }));
    
    //check to see if we want a certain page
    if (window.location.hash)
    {
      let pages = ["control", "config", "stats", "network", "firmware"];
      let page = window.location.hash.substring(1);
      if (pages.includes(page))
        open_page(page);
    }  
  };
  
  socket.onmessage = function(event)
  {
    const msg = JSON.parse(event.data);
  
    console.log(msg);
    
    if (msg.msg == 'config')
    {
      current_config = msg;
      //console.log(msg);
  
      //is it our first boot?
      if (msg.first_boot && current_page != "network")
        show_alert(`Welcome to Gnarboard, head over to <a href="#network" onclick="open_page('network')">Network</a> to setup your WiFi.`, "primary");
  
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
          $('#channelTableBody').append(ChannelControlRow(ch.id, ch.name));
          $('#channelDutySlider' + ch.id).change(set_duty_cycle);
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
  
        //duty is a bit of a special case.
        let duty = Math.round(ch.duty * 100);
        if (current_config.channels[ch.id].isDimmable)
        {
          $('#channelDutySlider' + ch.id).val(duty); 
          $('#channelDutyCycle' + ch.id).html(`${duty}%`);
          $('#channelDutyCycle' + ch.id).show();
        }
        else
        {
          $('#channelDutyCycle' + ch.id).hide();
        }
  
        let current = ch.current.toFixed(2);
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
    //load up our network config.
    else if (msg.msg == "network_config")
    {
      //save our config.
      network_config = msg;

      //update login stuff.
      if (msg.require_login)
        $('#logoutNav').show();
      else
        $('#logoutNav').hide();

      //console.log(msg);
      $("#wifi_mode").val(msg.wifi_mode);
      $("#wifi_ssid").val(msg.wifi_ssid);
      $("#wifi_pass").val(msg.wifi_pass);
      $("#local_hostname").val(msg.local_hostname);
      $("#app_user").val(msg.app_user);
      $("#app_pass").val(msg.app_pass);
      $("#require_login").prop("checked", msg.require_login);
    }
    else if (msg.error)
    {
      //keep the u gotta login to the login page.
      if (msg.error == "You must be logged in.")
      {
        if (window.location.pathname != "/login.html")
          window.location.href = "/login.html";
      }
      else
        show_alert(msg.error);
    }
    else if (msg.success)
    {
      //keep the login success stuff on the login page.
      if (msg.success == "Login successful.")
      {
        if (window.location.pathname == "/login.html")
        {
          show_alert(msg.success, "success");
    
          if (app_username && app_password)
          {
            Cookies.set('username', app_username, { expires: 365 });
            Cookies.set('password', app_password, { expires: 365 });
          }
    
          //this is super fast otherwise.
          setTimeout(function (){
            window.location.href = "/";
          }, 1000);
        }
      }
      else
        show_alert(msg.success, "success");
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
}

function show_alert(message, type = 'danger')
{
  $('#liveAlertPlaceholder').append(AlertBox(message, type))
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

function toggle_duty_cycle(id)
{
  $(`#channelDutySliderRow${id}`).toggle();
}

function open_page(page)
{
  current_page = page;

  //load up our config
  if (page == "network")
    socket.send(JSON.stringify({
      "cmd": "get_network_config",
    }));

  //sad to see you go.
  if (page == "logout")
  {
    show_alert("Logging out.", "success");

    Cookies.remove("username");
    Cookies.remove("password");

    //this is super fast otherwise.
    setTimeout(function (){
      window.location.href = "/login.html";
    }, 1000);
  }
  else
  {
    $('.nav-link').removeClass("active");
    $(`#${page}Nav a`).addClass("active");
  
    $("div.pageContainer").hide();
    $(`#${page}Page`).show();
  
    //request our stats.
    if (page == "stats")
      get_stats_data();
    }
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

function set_duty_cycle(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.value;

  //must be realistic.
  if (value >= 0 && value <= 100)
  {
    //update our button
    $(`#channelDutyCycle${id}`).html(Math.round(value) + '%');

    //we want a duty value from 0 to 1
    value = value / 100;
  
    //set our new channel name!
    socket.send(JSON.stringify({
      "cmd": "set_duty",
      "id": id,
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

function do_login(e)
{
  app_username = $('#username').val();
  app_password = $('#password').val();

  socket.send(JSON.stringify({
    "cmd": "login",
    "user": app_username,
    "pass": app_password
  }));
}

function save_network_settings()
{
  //get our data
  let wifi_mode = $("#wifi_mode").val();
  let wifi_ssid = $("#wifi_ssid").val();
  let wifi_pass = $("#wifi_pass").val();
  let local_hostname = $("#local_hostname").val();
  let app_user = $("#app_user").val();
  let app_pass = $("#app_pass").val();
  let require_login = $("#require_login").prop("checked");

  //we should probably do a bit of verification here

  //app login?
  if (require_login)
  {
    $('#logoutNav').show();
    Cookies.set('username', app_user, { expires: 365 });
    Cookies.set('password', app_pass, { expires: 365 });
  }
  else
  {
    $('#logoutNav').hide();
    Cookies.remove("username");
    Cookies.remove("password");    
  }

  //if they are changing from client to client, we can't show a success.
  show_alert("Gnarboard may be unresponsive while changing WiFi settings. Make sure you connect to the right network after updating.", "primary");

  //okay, send it off.
  socket.send(JSON.stringify({
    "cmd": "set_network_config",
    "wifi_mode": wifi_mode,
    "wifi_ssid": wifi_ssid,
    "wifi_pass": wifi_pass,
    "local_hostname": local_hostname,
    "app_user": app_user,
    "app_pass": app_pass,
    "require_login": require_login
  }));
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