var socket = false;
var current_page;
var current_config;
var app_username;
var app_password;
var network_config;
var app_config;

var socket_retries = 0;
var retry_time = 0;
var last_heartbeat = 0;
const heartbeat_rate = 1000;
var ota_started = false;

var page_list = ["control", "config", "stats", "network", "settings", "system"];
var page_ready = {
  "control": false,
  "config":  false,
  "stats":   false,
  "network": false,
  "settings": false,
  "system":  true,
  "login":  true
};

const BoardNameEdit = (name) => `
<div class="col-12">
  <label for="fBoardName" class="form-label">Board Name</label>
  <input type="text" class="form-control" id="fBoardName" value="${name}">
  <div class="valid-feedback">Saved!</div>
  <div class="invalid-feedback">Must be 30 characters or less.</div>
</div>
`;

const PWMControlRow = (id, name) => `
<tr id="pwm${id}" class="pwmRow">
  <td class="text-center"><button id="pwmState${id}" type="button" class="btn btn-sm" onclick="toggle_state(${id})" style="width: 60px"></button></td>
  <td class="pwmName">${name}</td>
  <td class="text-end"><button id="pwmDutyCycle${id}" type="button" class="btn btn-sm btn-light" onclick="toggle_duty_cycle(${id})" style="width: 60px">???</button></td>
  <td id="pwmCurrent${id}" class="text-end"></td>
</tr>
<tr id="pwmDutySliderRow${id}" style="display:none">
  <td colspan="4">
    <input type="range" class="form-range" min="0" max="100" id="pwmDutySlider${id}">
  </td>
</tr>
`;

const PWMEditRow = (id, name, soft_fuse) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fPWMEnabled${id}">
      <label class="form-check-label" for="fPWMEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fPWMName${id}" value="${name}">
      <label for="fPWMName${id}">Name</label>
    </div>
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
  </div>
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fPWMDimmable${id}">
      <label class="form-check-label" for="fPWMDimmable${id}">
        Dimmable?
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <div class="form-floating mb-3">
      <input type="text" class="form-control" id="fPWMSoftFuse${id}" value="${soft_fuse}">
      <label for="fPWMSoftFuse${id}">Soft Fuse (Amps)</label>
    </div>
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be a number between 0 and 20</div>
  </div>
</div>
`;

const SwitchControlRow = (id, name) => `
<tr id="switch${id}" class="switchRow">
  <td class="text-center"><button id="switchState${id}" type="button" class="btn btn-sm" style="width: 80px"></button></td>
  <td class="switchName">${name}</td>
</tr>
`;

const SwitchEditRow = (id, name) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fSwitchEnabled${id}">
      <label class="form-check-label" for="fSwitchEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <input type="text" class="form-control" id="fSwitchName${id}" value="${name}">
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
  </div>
</div>
`;

const RGBControlRow = (id, name) => `
<tr id="rgb${id}" class="rgbRow">
  <td class="text-center"><input id="rgbPicker${id}" type="text"></td>
  <td class="rgbName">${name}</td>
</tr>
`;

const RGBEditRow = (id, name) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fRGBEnabled${id}">
      <label class="form-check-label" for="fRGBEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <input type="text" class="form-control" id="fRGBName${id}" value="${name}">
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
  </div>
</div>
`;

const ADCControlRow = (id, name) => `
<tr id="adc${id}" class="adcRow">
  <td class="adcName">${name}</td>
  <td class="text-center" id="adcReading${id}"></td>
  <td class="text-center" id="adcVoltage${id}"></td>
  <td class="text-center" id="adcPercentage${id}"></td>
</tr>
`;

const ADCEditRow = (id, name) => `
<div class="row mt-2 align-items-center">
  <div class="col-auto">
    <div class="form-check form-switch">
      <input class="form-check-input" type="checkbox" id="fADCEnabled${id}">
      <label class="form-check-label" for="fADCEnabled${id}">
        Enabled
      </label>
    </div>
    <div class="valid-feedback">Saved!</div>
  </div>
  <div class="col-auto">
    <input type="text" class="form-control" id="fADCName${id}" value="${name}">
    <div class="valid-feedback">Saved!</div>
    <div class="invalid-feedback">Must be 30 characters or less.</div>
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

let currentPWMSliderID = -1;
let currentRGBPickerID = -1;

//our heartbeat timer.
function send_heartbeat()
{
  //did we not get a heartbeat?
  if (Date.now() - last_heartbeat > heartbeat_rate * 2)
  {
    console.log("Missed heartbeat: " + (Date.now() - last_heartbeat))
    socket.close();
    retry_connection();
  }

  //only send it if we're already open.
  if (socket.readyState == WebSocket.OPEN)
  {
    immediateSend({"cmd": "ping"});
    setTimeout(send_heartbeat, heartbeat_rate);
  }
  else if (socket.readyState == WebSocket.CLOSING)
  {
    console.log("she closing " + socket.readyState);
    //socket.close();
    retry_connection();
  }
  else if (socket.readyState == WebSocket.CLOSED)
  {
    console.log("she closed " + socket.readyState);
    //socket.close();
    retry_connection();
  }
}

function start_yarrboard()
{
  //main data connection
  start_websocket();

  //let the rest of the site load first.
  setTimeout(check_for_updates, 1000);

  //check to see if we want a certain page
  if (window.location.hash)
  {
    let page = window.location.hash.substring(1);
    if (page_list.includes(page))
      open_page(page);
  }
  else
    open_page("control");  
}

function load_configs()
{
  //load our config... will also trigger login
  immediateSend({
    "cmd": "get_config"
  });

  //load our network config
  setTimeout(function (){
    immediateSend({
      "cmd": "get_network_config"
    });  
  }, 50);

  //load our network config
  setTimeout(function (){
    immediateSend({
      "cmd": "get_app_config"
    });  
  }, 100);

  //load our stats config
  setTimeout(function (){
    immediateSend({
      "cmd": "get_stats"
    });  
  }, 150);
}

function start_websocket()
{
  //close any old connections
  if (socket)
    socket.close();
  
  //do we want ssl?
  let protocol = "ws://";
  if (document.location.protocol == 'https:')
    protocol = "wss://";

  //open it.
  socket = new WebSocket(protocol + window.location.host + "/ws");
  
  console.log("Opening new websocket");

  socket.onopen = function(e)
  {
    console.log("[socket] Connected");

    //we are connected, reload
    socket_retries = 0;
    retry_time = 0;
    last_heartbeat = Date.now();

    //ticker checker
    setTimeout(send_heartbeat, heartbeat_rate);

    //our connection status
    $(".connection_status").hide();
    $("#connection_good").show();

    //auto login?
    if (Cookies.get("username") && Cookies.get("password")){
      console.log("auto login");
      immediateSend({
        "cmd": "login",
        "user": Cookies.get("username"),
        "pass": Cookies.get("password")
      });
    }
  
    //get our basic info
    load_configs();
  };

  socket.onmessage = function(event)
  {
    const msg = JSON.parse(event.data);

    if (msg.msg == 'config')
    {
      // console.log("config");
      // console.log(msg);
      // console.log(event.data);
      // console.log(event.data.length);

      current_config = msg;

      //is it our first boot?
      if (msg.first_boot && current_page != "network")
        show_alert(`Welcome to Yarrboard, head over to <a href="#network" onclick="open_page('network')">Network</a> to setup your WiFi.`, "primary");

      //did we get a crash?
      if (msg.has_coredump)
        show_alert(`
          <p>Oops, looks like Yarrboard crashed.</p>
          <p>Please download the <a href="/coredump.txt" target="_blank">coredump</a> and report it to our <a href="https://github.com/hoeken/yarrboard/issues">Github Issue Tracker</a> along with the following information:</p>
          <ul><li>Firmware: ${msg.firmware_version}</li><li>Hardware: ${msg.hardware_version}</li></ul>
        `, "danger");

      //let the people choose their own names!
      $('#boardName').html(msg.name);
      document.title = msg.name;
  
      //update our footer automatically.
      $('#projectName').html("Yarrboard v" + msg.firmware_version);

      //stats info
      $("#firmware_version").html(msg.firmware_version);
      $("#hardware_version").html(msg.hardware_version);

      //show some info about restarts
      if (msg.last_restart_reason)
        $("#last_reboot_reason").html(msg.last_restart_reason);
      else
        $("#last_reboot_reason").parent().hide();
  
      //populate our pwm control table
      $('#pwmControlDiv').hide();
      $('#pwmStatsDiv').hide();
      if (msg.pwm)
      {
        $('#pwmTableBody').html("");
        for (ch of msg.pwm)
        {
          if (ch.enabled)
          {
            $('#pwmTableBody').append(PWMControlRow(ch.id, ch.name));
            $('#pwmDutySlider' + ch.id).change(set_duty_cycle);

            //update our duty when we move
            $('#pwmDutySlider' + ch.id).on("input", set_duty_cycle);

            //stop updating the UI when we are choosing a duty
            $('#pwmDutySlider' + ch.id).on('focus', function(e) {
              let ele = e.target;
              let id = ele.id.match(/\d+/)[0];
              currentPWMSliderID = id;
            });

            //stop updating the UI when we are choosing a duty
            $('#pwmDutySlider' + ch.id).on('touchstart', function(e) {
              let ele = e.target;
              let id = ele.id.match(/\d+/)[0];
              currentPWMSliderID = id;
            });
            
            //restart the UI updates when slider is closed
            $('#pwmDutySlider' + ch.id).on("blur", function (e) {
              currentPWMSliderID = -1;
            });

            //restart the UI updates when slider is closed
            $('#pwmDutySlider' + ch.id).on("touchend", function (e) {
              currentPWMSliderID = -1;
            });
          }
        }

        $('#pwmStatsTableBody').html("");
        for (ch of msg.pwm)
        {
          if (ch.enabled)
          {
            $('#pwmStatsTableBody').append(`<tr id="pwmStats${ch.id}" class="pwmRow"></tr>`);
            $('#pwmStats' + ch.id).append(`<td class="pwmName">${ch.name}</td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmAmpHours${ch.id}" class="text-end"></td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmWattHours${ch.id}" class="text-end"></td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmOnCount${ch.id}" class="text-end"></td>`);
            $('#pwmStats' + ch.id).append(`<td id="pwmTripCount${ch.id}" class="text-end"></td>`);
          }
        }

        $('#pwmControlDiv').show();
        $('#pwmStatsDiv').show();  
      }

      //populate our switch control table
      $('#switchControlDiv').hide();
      $('#switchStatsDiv').hide();
      if (msg.switches)
      {
        $('#switchTableBody').html("");
        for (ch of msg.switches)
        {
          if (ch.enabled)
            $('#switchTableBody').append(SwitchControlRow(ch.id, ch.name));
        }

        $('#switchStatsTableBody').html("");
        for (ch of msg.switches)
        {
          if (ch.enabled)
          {
            $('#switchStatsTableBody').append(`<tr id="switchStats${ch.id}" class="switchRow"></tr>`);
            $('#switchStats' + ch.id).append(`<td class="switchName">${ch.name}</td>`);
            $('#switchStats' + ch.id).append(`<td id="switchOnCount${ch.id}" class="text-end"></td>`);
          }
        }
  
        $('#switchControlDiv').show();
        $('#switchStatsDiv').show();
      }

      //populate our rgb control table
      $('#rgbControlDiv').hide();
      if (msg.rgb)
      {
        $('#rgbTableBody').html("");
        for (ch of msg.rgb)
        {
          if (ch.enabled)
          {
            $('#rgbTableBody').append(RGBControlRow(ch.id, ch.name));

            //init our color picker
            $('#rgbPicker' + ch.id).spectrum({
              color: "#000",
              showPalette: true,
              palette: [
                ["#000","#444","#666","#999","#ccc","#eee","#f3f3f3","#fff"],
                ["#f00","#f90","#ff0","#0f0","#0ff","#00f","#90f","#f0f"],
                ["#f4cccc","#fce5cd","#fff2cc","#d9ead3","#d0e0e3","#cfe2f3","#d9d2e9","#ead1dc"],
                ["#ea9999","#f9cb9c","#ffe599","#b6d7a8","#a2c4c9","#9fc5e8","#b4a7d6","#d5a6bd"],
                ["#e06666","#f6b26b","#ffd966","#93c47d","#76a5af","#6fa8dc","#8e7cc3","#c27ba0"],
                ["#c00","#e69138","#f1c232","#6aa84f","#45818e","#3d85c6","#674ea7","#a64d79"],
                ["#900","#b45f06","#bf9000","#38761d","#134f5c","#0b5394","#351c75","#741b47"],
                ["#600","#783f04","#7f6000","#274e13","#0c343d","#073763","#20124d","#4c1130"]
            ]
            });

            //update our color on change
            $('#rgbPicker' + ch.id).change(set_rgb_color);

            //update our color when we move
            $('#rgbPicker' + ch.id).on("move.spectrum", set_rgb_color);

            //stop updating the UI when we are choosing a color
            $('#rgbPicker' + ch.id).on('show.spectrum', function(e) {
              let ele = e.target;
              let id = ele.id.match(/\d+/)[0];
              currentRGBPickerID = id;
            });

            //restart the UI updates when picker is closed
            $('#rgbPicker' + ch.id).on("hide.spectrum", function (e) {
              currentRGBPickerID = -1;
            });
          }
        }

        $('#rgbControlDiv').show();
      }

      //populate our adc control table
      $('#adcControlDiv').hide();
      if (msg.adc)
      {
        $('#adcTableBody').html("");
        for (ch of msg.adc)
        {
          if (ch.enabled)
            $('#adcTableBody').append(ADCControlRow(ch.id, ch.name));
        }

        $('#adcControlDiv').show();
      }
      
      //only do it as needed
      if (!page_ready.config || current_page != "config")
      {
        //populate our pwm edit table
        $('#boardConfigForm').html(BoardNameEdit(msg.name));

        //validate + save control
        $("#fBoardName").change(validate_board_name);

        //edit controls for each pwm
        $('#pwmConfig').hide();
        if (msg.pwm)
        {
          $('#pwmConfigForm').html("");
          for (ch of msg.pwm)
          {
            $('#pwmConfigForm').append(PWMEditRow(ch.id, ch.name, ch.softFuse));
            $(`#fPWMDimmable${ch.id}`).prop("checked", ch.isDimmable);
            $(`#fPWMEnabled${ch.id}`).prop("checked", ch.enabled);
  
            //enable/disable other stuff.
            $(`#fPWMName${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMDimmable${ch.id}`).prop('disabled', !ch.enabled);
            $(`#fPWMSoftFuse${ch.id}`).prop('disabled', !ch.enabled);
  
            //validate + save
            $(`#fPWMEnabled${ch.id}`).change(validate_pwm_enabled);
            $(`#fPWMName${ch.id}`).change(validate_pwm_name);
            $(`#fPWMDimmable${ch.id}`).change(validate_pwm_dimmable);
            $(`#fPWMSoftFuse${ch.id}`).change(validate_pwm_soft_fuse);
          }  
          $('#pwmConfig').show();
        }

        //edit controls for each switch
        $('#switchConfig').hide();
        if (msg.switches)
        {
          $('#switchConfigForm').html("");
          for (ch of msg.switches)
          {
            $('#switchConfigForm').append(SwitchEditRow(ch.id, ch.name));
            $(`#fSwitchEnabled${ch.id}`).prop("checked", ch.enabled);
  
            //enable/disable other stuff.
            $(`#fSwitchName${ch.id}`).prop('disabled', !ch.enabled);
  
            //validate + save
            $(`#fSwitchEnabled${ch.id}`).change(validate_switch_enabled);
            $(`#fSwitchName${ch.id}`).change(validate_switch_name);
          }  
          $('#switchConfig').show();
        }

        //edit controls for each rgb
        $('#rgbConfig').hide();
        if (msg.rgb)
        {
          $('#rgbConfigForm').html("");
          for (ch of msg.rgb)
          {
            $('#rgbConfigForm').append(RGBEditRow(ch.id, ch.name));
            $(`#fRGBEnabled${ch.id}`).prop("checked", ch.enabled);
  
            //enable/disable other stuff.
            $(`#fRGBName${ch.id}`).prop('disabled', !ch.enabled);
  
            //validate + save
            $(`#fRGBEnabled${ch.id}`).change(validate_rgb_enabled);
            $(`#fRGBName${ch.id}`).change(validate_rgb_name);
          }
          $('#rgbConfig').show();
        }

        //edit controls for each rgb
        $('#adcConfig').hide();
        if (msg.adc)
        {
          $('#adcConfigForm').html("");
          for (ch of msg.adc)
          {
            $('#adcConfigForm').append(ADCEditRow(ch.id, ch.name));
            $(`#fADCEnabled${ch.id}`).prop("checked", ch.enabled);
  
            //enable/disable other stuff.
            $(`#fADCName${ch.id}`).prop('disabled', !ch.enabled);
  
            //validate + save
            $(`#fADCEnabled${ch.id}`).change(validate_adc_enabled);
            $(`#fADCName${ch.id}`).change(validate_adc_name);
          }

          $('#adcConfig').show();
        }
      }

      //ready!
      page_ready.config = true;
    }
    else if (msg.msg == 'update')
    {
      // console.log("update");
      // console.log(msg);
      // console.log(event.data);
      // console.log(event.data.length);

      //we need a config loaded.
      if (!current_config)
        return;

      //update our clock.
      // let mytime = Date.parse(msg.time);
      // if (mytime)
      // {
      //   let mydate = new Date(mytime);
      //   $('#time').html(mydate.toLocaleString());
      //   $('#time').show();
      // }
      // else
      //   $('#time').hide();

      if (msg.uptime)
      {
        $("#uptime_footer").html("Uptime: " + secondsToDhms(Math.round(msg.uptime/1000)));
        $("#uptime_footer").show();
      }
      else
        $("#uptime_footer").hide();

      //or maybe voltage
      // if (msg.bus_voltage)
      // {
      //   $('#bus_voltage_main').html("Bus Voltage: " + msg.bus_voltage.toFixed(2) + "V");
      //   $('#bus_voltage_main').show();
      // }
      // else
      //   $('#bus_voltage_main').hide();

      //our pwm info
      if (msg.pwm)
      {
        for (ch of msg.pwm)
        {
          if (current_config.pwm[ch.id].enabled)
          {
            if (ch.state)
            {
              $('#pwmState' + ch.id).html("ON");
              $('#pwmState' + ch.id).removeClass("btn-danger");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
              $('#pwmState' + ch.id).addClass("btn-success");
            }
            else if(ch.soft_fuse_tripped)
            {
              $('#pwmState' + ch.id).html("TRIP");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-secondary");
              $('#pwmState' + ch.id).addClass("btn-danger");
            }
            else
            {
              $('#pwmState' + ch.id).html("OFF");
              $('#pwmState' + ch.id).removeClass("btn-success");
              $('#pwmState' + ch.id).removeClass("btn-danger");
              $('#pwmState' + ch.id).addClass("btn-secondary");
            }
      
            //duty is a bit of a special case.
            let duty = Math.round(ch.duty * 100);
            if (current_config.pwm[ch.id].isDimmable)
            {
              if (currentPWMSliderID != ch.id)
              {
                $('#pwmDutySlider' + ch.id).val(duty); 
                $('#pwmDutyCycle' + ch.id).html(`${duty}%`);
                $('#pwmDutyCycle' + ch.id).show();
              }
            }
            else
            {
              $('#pwmDutyCycle' + ch.id).hide();
            }
      
            let current = ch.current.toFixed(1);
            let currentHTML = `${current}&nbsp;A`;

            if (msg.bus_voltage)
            {
              let wattage = ch.current * msg.bus_voltage;
              wattage = wattage.toFixed(0);
              
              currentHTML += `&nbsp;/&nbsp${wattage}&nbsp;W`;
            }

            $('#pwmCurrent' + ch.id).html(currentHTML);
          }
        }
      }

      //our switch info
      if (msg.switches)
      {
        for (ch of msg.switches)
        {
          if (current_config.switches[ch.id].enabled)
          {
            if (ch.isOpen)
            {
              $('#switchState' + ch.id).html("OPEN");
              $('#switchState' + ch.id).removeClass("btn-success");
              $('#switchState' + ch.id).addClass("btn-secondary");
            }
            else
            {
              $('#switchState' + ch.id).html("CLOSED");
              $('#switchState' + ch.id).removeClass("btn-secondary");
              $('#switchState' + ch.id).addClass("btn-success");
            }
          }
        }
      }

      //our rgb info
      if (msg.rgb)
      {
        for (ch of msg.rgb)
        {
          if (current_config.rgb[ch.id].enabled && currentRGBPickerID != ch.id)
          {
            let _red = Math.round(255 * ch.red);
            let _green = Math.round(255 * ch.green);
            let _blue = Math.round(255 * ch.blue);

            $("#rgbPicker" + ch.id).spectrum("set", `rgb(${_red}, ${_green}, ${_blue}`);
          }
        }
      }

      //our adc info
      if (msg.adc)
      {
        for (ch of msg.adc)
        {
          if (current_config.adc[ch.id].enabled)
          {
            let reading = Math.round(ch.reading);
            let voltage = ch.voltage.toFixed(2);
            let percentage = ch.percentage.toFixed(1);

            $("#adcReading" + ch.id).html(reading);
            $("#adcVoltage" + ch.id).html(voltage + "V")
            $("#adcPercentage" + ch.id).html(percentage + "%")
          }
        }
      }
      
      page_ready.control = true;
    }
    else if (msg.msg == "stats")
    {
      //console.log("stats");

      //we need this
      if (!current_config)
        return;

      $("#uptime").html(secondsToDhms(Math.round(msg.uptime/1000)));
      $("#messages").html(msg.messages.toLocaleString("en-US"));
      if (msg.fps)
        $("#fps").html(msg.fps.toLocaleString("en-US"));
      $("#heap_size").html(formatBytes(msg.heap_size));
      $("#free_heap").html(formatBytes(msg.free_heap));
      $("#min_free_heap").html(formatBytes(msg.min_free_heap));
      $("#max_alloc_heap").html(formatBytes(msg.max_alloc_heap));
      $("#rssi").html(msg.rssi + "dBm");
      $("#uuid").html(msg.uuid);
      $("#ip_address").html(msg.ip_address);

      if (msg.bus_voltage)
        $("#bus_voltage").html(msg.bus_voltage.toFixed(1) + "V");
      else
        $("#bus_voltage_row").remove();

      if (msg.fans)
        $("#fan_rpm").html(msg.fans.map((a) => a.rpm + "RPM").join(", "));
      else
        $("#fan_rpm_row").remove();

      if (msg.pwm)
      {
        for (ch of msg.pwm)
        {
          if (current_config.pwm[ch.id].enabled)
          {
            $('#pwmAmpHours' + ch.id).html(formatAmpHours(ch.aH));
            $('#pwmWattHours' + ch.id).html(formatWattHours(ch.wH));
            $('#pwmOnCount' + ch.id).html(ch.state_change_count.toLocaleString("en-US"));
            $('#pwmTripCount' + ch.id).html(ch.soft_fuse_trip_count.toLocaleString("en-US"));
          }
        }
      }

      if (msg.switches)
      {
        for (ch of msg.switches)
        {
          if (current_config.switches[ch.id].enabled)
          {
            $('#switchOnCount' + ch.id).html(ch.state_change_count.toLocaleString("en-US"));
          }
        }
      }    

      page_ready.stats = true;
    }
    //load up our network config.
    else if (msg.msg == "network_config")
    {
      //console.log("network config");

      //save our config.
      network_config = msg;

      //console.log(msg);
      $("#wifi_mode").val(msg.wifi_mode);
      $("#wifi_ssid").val(msg.wifi_ssid);
      $("#wifi_pass").val(msg.wifi_pass);
      $("#local_hostname").val(msg.local_hostname);

      page_ready.network = true;    
    }
    //load up our network config.
    else if (msg.msg == "app_config")
    {
      //console.log("network config");

      //save our config.
      app_config = msg;

      //update login stuff.
      if (msg.require_login)
        $('#logoutNav').show();
      else
        $('#logoutNav').hide();

      //enabled/disable user/pass fields
      $(`#app_user`).prop('disabled', !msg.require_login);
      $(`#app_pass`).prop('disabled', !msg.require_login);
      $(`#require_login`).change(function (){
        $(`#app_user`).prop('disabled', !$("#require_login").prop("checked"))
        $(`#app_pass`).prop('disabled', !$("#require_login").prop("checked"))
      });

      //console.log(msg);
      $("#app_user").val(msg.app_user);
      $("#app_pass").val(msg.app_pass);
      $("#require_login").prop("checked", msg.require_login);
      $("#app_enable_api").prop("checked", msg.app_enable_api);
      $("#app_enable_serial").prop("checked", msg.app_enable_serial);

      //for our ssl stuff
      $("#app_enable_ssl").prop("checked", msg.app_enable_ssl);
      $("#server_cert").val(msg.server_cert);
      $("#server_key").val(msg.server_key);

      //hide/show these guys
      if (msg.app_enable_ssl)
      {
        $("#server_cert_container").show();
        $("#server_key_container").show();
      }

      //make it dynamic too
      $("#app_enable_ssl").change(function (){
        if ($("#app_enable_ssl").prop("checked"))
        {
          $("#server_cert_container").show();
          $("#server_key_container").show();  
        }
        else
        {
          $("#server_cert_container").hide();
          $("#server_key_container").hide();  
        }
      });

      page_ready.settings = true;    
    }
    //load up our network config.
    else if (msg.msg == "ota_progress")
    {
      //console.log("ota progress");

      //OTA is blocking... so update our heartbeat
      last_heartbeat = Date.now();

      let progress = Math.round(msg.progress);

      let prog_id = `#firmware_progress`;
      $(prog_id).css("width", progress + "%").text(progress + "%");
      if (progress == 100)
      {
        $(prog_id).removeClass("progress-bar-animated");
        $(prog_id).removeClass("progress-bar-striped");
      }

      //was that the last?
      if (progress == 100)
      {
        show_alert("Firmware update successful.", "success");

        //reload our page
        setTimeout(function (){
          location.reload(true);
        }, 2500); 
      }
    }
    else if (msg.status == "error")
    {
      //did we turn login off?
      if (msg.message == "Login not required.")
      {
        Cookies.remove("username");
        Cookies.remove("password");    
      }

      //keep the u gotta login to the login page.
      if (msg.message == "You must be logged in.")
      {
        console.log("you must log in");
        open_page("login");
      }
      else
        show_alert(msg.message);
    }
    else if (msg.status == "success")
    {
      //keep the login success stuff on the login page.
      if (msg.message == "Login successful.")
      {
        //only needed for login page, otherwise its autologin
        if (current_page == "login")
        {
          //save user/pass to cookies.
          if (app_username && app_password)
          {
            Cookies.set('username', app_username, { expires: 365 });
            Cookies.set('password', app_password, { expires: 365 });
          }

          //need this to show everything
          load_configs();

          //let them nav
          $("#navbar").show();

          //this is super fast otherwise.
          open_page("control");
        }
      }
      else
        show_alert(msg.message, "success");
    }
    else if (msg.pong)
    {
      //we are connected still
      //console.log("pong: " + msg.pong);

      //we got the heartbeat
      last_heartbeat = Date.now();
    }
    else
    {
      console.log("[socket] Unknown message: ");
      console.log(msg);
    }
  };
  
  socket.onclose = function(event)
  {
    //console.log(`[socket] Connection closed code=${event.code} reason=${event.reason}`);
  };
  
  socket.onerror = function(error)
  {
    //console.log(`[socket] error`);
    //console.log(error);
  };
}

function retry_connection()
{
  //bail if its good to go
  if (socket.readyState == WebSocket.OPEN)
    return;

  //keep watching if we are connecting
  if (socket.readyState == WebSocket.CONNECTING)
  {
    console.log("Waiting for connection");
    
    retry_time++;
    $("#retries_count").html(retry_time);

    //tee it up.
    setTimeout(retry_connection, 1000);

    return;
  }

  //keep track of stuff.
  retry_time = 0;
  socket_retries++;
  console.log("Reconnecting... " + socket_retries);

  //our connection status
  $(".connection_status").hide();
  $("#retries_count").html(retry_time);
  $("#connection_retrying").show();

  //reconnect!
  start_websocket();

  //set some bounds
  let my_timeout = 500;
  my_timeout = Math.max(my_timeout, socket_retries * 1000);
  my_timeout = Math.min(my_timeout, 60000);

  //tee it up.
  setTimeout(retry_connection, my_timeout);

  //infinite retees
  //our connection status
  //  $(".connection_status").hide();
  //  $("#connection_failed").show();
}

function show_alert(message, type = 'danger')
{
  //we only need one alert at a time.
  $('#liveAlertPlaceholder').html(AlertBox(message, type))

  //make sure we can see it.
  $('html').animate({
      scrollTop: 0
    },
    750 //speed
  );
}

function toggle_state(id)
{
  //OFF or TRIP both switch it to on.
  let new_state = true;
  if ($("#pwmState" + id).text() == "ON")
    new_state = false;

  throttledSend({
    "cmd": "set_pwm_channel",
    "id": id,
    "state": new_state
  });
}

function toggle_duty_cycle(id)
{
  $(`#pwmDutySliderRow${id}`).toggle();
}

function open_page(page)
{
  if (page == current_page)
    return;

  current_page = page;

  console.log(`Opening page ${page}`);

  //request our stats.
  if (page == "stats")
    get_stats_data();

  //hide all pages.
  $("div.pageContainer").hide();

  //special stuff
  if (page == "login")
  {
    //hide our nav bar
    $("#navbar").hide();

    //enter triggers login
      $(document).on('keypress',function(e) {
        if(e.which == 13)
            do_login();
    });
  }

  //sad to see you go.
  if (page == "logout")
  {
    Cookies.remove("username");
    Cookies.remove("password");

    open_page("login");
  }
  else
  {
    //update our nav
    $('.nav-link').removeClass("active");
    $(`#${page}Nav a`).addClass("active");

    //is our new page ready?
    on_page_ready();
  }
}

function on_page_ready()
{
  //is our page ready yet?
  if (page_ready[current_page])
  {
    $("#loading").hide();
    $(`#${current_page}Page`).show();
  }
  else
  {
    $("#loading").show();
    setTimeout(on_page_ready, 100);
  }
}

function get_stats_data()
{
  if (socket.readyState == WebSocket.OPEN)
  {
    immediateSend({
      "cmd": "get_stats",
    });
  }

  //keep loading it while we are here.
  if (current_page == "stats")
    setTimeout(get_stats_data, 1000);
}

//drops messages if sent too fast.
var lastSentTime = Date.now();
function throttledSend(jdata)
{
  //ota is blocking... stop sending
  if (ota_started)
    return;

  //rate limit
  if (Date.now() > lastSentTime + 50)
  {
    immediateSend(jdata);
    lastSentTime = Date.now();
  }
  else
    console.log("message dropped, too fast");
}

function immediateSend(jdata)
{
  //ota is blocking... stop sending
  if (ota_started)
    return;

  socket.send(JSON.stringify(jdata));
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
    immediateSend({
      "cmd": "set_boardname",
      "value": value
    });
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
    $(`#pwmDutyCycle${id}`).html(Math.round(value) + '%');

    //we want a duty value from 0 to 1
    value = value / 100;
  
    //set our new pwm name!
    throttledSend({
      "cmd": "set_pwm_channel",
      "id": id,
      "duty": value
    });
  }
}

function validate_pwm_name(e)
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

    //set our new pwm name!
    immediateSend({
      "cmd": "set_pwm_channel",
      "id": id,
      "name": value
    });
  }
}

function validate_pwm_dimmable(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  immediateSend({
    "cmd": "set_pwm_channel",
    "id": id,
    "isDimmable": value
  });
}

function validate_pwm_enabled(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fPWMName${id}`).prop('disabled', !value);
  $(`#fPWMDimmable${id}`).prop('disabled', !value);
  $(`#fPWMSoftFuse${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  immediateSend({
    "cmd": "set_pwm_channel",
    "id": id,
    "enabled": value
  });
}

function validate_pwm_soft_fuse(e)
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
    immediateSend({
      "cmd": "set_pwm_channel",
      "id": id,
      "softFuse": value
    });
  }
}

function validate_switch_name(e)
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

    //set our new pwm name!
    immediateSend({
      "cmd": "set_switch",
      "id": id,
      "name": value
    });
  }
}

function validate_switch_enabled(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fSwitchName${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  immediateSend({
    "cmd": "set_switch",
    "id": id,
    "enabled": value
  });
}

function set_rgb_color(e, color)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];

  let rgb = color.toRgb();

  let red = rgb.r / 255;
  let green = rgb.g / 255;
  let blue = rgb.b / 255;

  throttledSend({
    "cmd": "set_rgb",
    "id": id,
    "red": red.toFixed(4),
    "green": green.toFixed(4),
    "blue": blue.toFixed(4)
  });
}

function validate_rgb_name(e)
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

    //set our new pwm name!
    immediateSend({
      "cmd": "set_rgb",
      "id": id,
      "name": value
    });
  }
}

function validate_rgb_enabled(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fRGBName${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  immediateSend({
    "cmd": "set_rgb",
    "id": id,
    "enabled": value
  });
}

function validate_adc_name(e)
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

    //set our new pwm name!
    immediateSend({
      "cmd": "set_adc",
      "id": id,
      "name": value
    });
  }
}

function validate_adc_enabled(e)
{
  let ele = e.target;
  let id = ele.id.match(/\d+/)[0];
  let value = ele.checked;

  //enable/disable other stuff.
  $(`#fADCName${id}`).prop('disabled', !value);

  //nothing really to validate here.
  $(ele).addClass("is-valid");

  //save it
  immediateSend({
    "cmd": "set_adc",
    "id": id,
    "enabled": value
  });
}

function do_login(e)
{
  app_username = $('#username').val();
  app_password = $('#password').val();

  immediateSend({
    "cmd": "login",
    "user": app_username,
    "pass": app_password
  });
}

function save_network_settings()
{
  //get our data
  let wifi_mode = $("#wifi_mode").val();
  let wifi_ssid = $("#wifi_ssid").val();
  let wifi_pass = $("#wifi_pass").val();
  let local_hostname = $("#local_hostname").val();

  //we should probably do a bit of verification here

  //if they are changing from client to client, we can't show a success.
  show_alert("Yarrboard may be unresponsive while changing WiFi settings. Make sure you connect to the right network after updating.", "primary");

  //okay, send it off.
  immediateSend({
    "cmd": "set_network_config",
    "wifi_mode": wifi_mode,
    "wifi_ssid": wifi_ssid,
    "wifi_pass": wifi_pass,
    "local_hostname": local_hostname
  });

  //reload our page
  setTimeout(function (){
    location.reload();
  }, 2500);  
}

function save_app_settings()
{
  //get our data
  let app_user = $("#app_user").val();
  let app_pass = $("#app_pass").val();
  let require_login = $("#require_login").prop("checked");
  let app_enable_api = $("#app_enable_api").prop("checked");
  let app_enable_serial = $("#app_enable_serial").prop("checked");
  let app_enable_ssl = $("#app_enable_ssl").prop("checked");
  let server_cert = $("#server_cert").val();
  let server_key = $("#server_key").val();

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

  //okay, send it off.
  immediateSend({
    "cmd": "set_app_config",
    "app_user": app_user,
    "app_pass": app_pass,
    "require_login": require_login,
    "app_enable_api": app_enable_api,
    "app_enable_serial": app_enable_serial,
    "app_enable_ssl": app_enable_ssl,
    "server_cert": server_cert,
    "server_key": server_key
  });

  //if they are changing from client to client, we can't show a success.
  show_alert("App settings have been updated.", "success");
}

function restart_board()
{
  if (confirm("Are you sure you want to restart your Yarrboard?"))
  {
    //okay, send it off.
    immediateSend({
      "cmd": "restart",
    });

    show_alert("Yarrboard is now restarting, please be patient.", "primary");
    
    setTimeout(function (){
      location.reload();
    }, 5000);
  }
}

function reset_to_factory()
{
  if (confirm("WARNING! Are you sure you want to reset your Yarrboard to factory defaults?  This cannot be undone."))
  {
    //okay, send it off.
    immediateSend({
      "cmd": "factory_reset",
    });

    show_alert("Yarrboard is now resetting to factory defaults, please be patient.", "primary");
  }
}

function check_for_updates()
{
  //did we get a config yet?
  if (current_config)
  {
    $.ajax({
      url: "https://raw.githubusercontent.com/hoeken/yarrboard/main/firmware/firmware.json",
      cache: false,
      dataType: "json",
      success: function(jdata) {
        //did we get anything?
        let data;
        for (firmware of jdata)
          if (firmware.type == current_config.hardware_version)
            data = firmware;

        if (!data)
        {
          show_alert(`Could not find a firmware for this hardware.`, "danger");
          return;
        }

        $("#firmware_checking").hide();

        //do we have a new version?
        if (compareVersions(data.version, current_config.firmware_version))
        {
          if (data.changelog)
          {
            $("#firmware_changelog").append(marked.parse(data.changelog));
            $("#firmware_changelog").show();
          }

          $("#new_firmware_version").html(data.version);
          $("#firmware_bin").attr("href", `${data.url}`);
          $("#firmware_update_available").show();

          show_alert(`There is a <a  onclick="open_page('system')" href="/#system">firmware update</a> available (${data.version}).`, "primary");
        }
        else
          $("#firmware_up_to_date").show();
      }
    });
  }
  //wait for it.
  else
    setTimeout(check_for_updates, 1000);
}

function update_firmware()
{
  $("#btn_update_firmware").prop("disabled", true);
  $("#progress_wrapper").show();

  //okay, send it off.
  immediateSend({
    "cmd": "ota_start",
  });  

  ota_started = true;
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
  var sDisplay = (s > 0 && d == 0 && h == 0 && m == 0) ? s + (s == 1 ? " second" : " seconds") : "";

  return (dDisplay + hDisplay + mDisplay + sDisplay).replace(/,\s*$/, "");
}

function formatAmpHours(aH)
{
  //low amp hours?
  if (aH < 10)
    return aH.toFixed(3) + "&nbsp;Ah";
  else if (aH < 100)
    return aH.toFixed(2) + "&nbsp;Ah";
  else if (aH < 1000)
    return aH.toFixed(1) + "&nbsp;Ah";

  //okay, now its kilo time
  aH = aH / 1000;
  if (aH < 10)
    return aH.toFixed(3) + "&nbsp;kAh";
  else if (aH < 100)
    return aH.toFixed(2) + "&nbsp;kAh";
  else if (aH < 1000)
    return aH.toFixed(1) + "&nbsp;kAh";

  //okay, now its mega time
  aH = aH / 1000;
  if (aH < 10)
    return aH.toFixed(3) + "&nbsp;MAh";
  else if (aH < 100)
    return aH.toFixed(2) + "&nbsp;MAh";
  else if (aH < 1000)
    return aH.toFixed(1) + "&nbsp;MAh";
  else
    return Math.roud(aH) + "&nbsp;MAh";
}

function formatWattHours(wH)
{
  //low watt hours?
  if (wH < 10)
    return wH.toFixed(3) + "&nbsp;Wh";
  else if (wH < 100)
    return wH.toFixed(2) + "&nbsp;Wh";
  else if (wH < 1000)
    return wH.toFixed(1) + "&nbsp;Wh";

  //okay, now its kilo time
  wH = wH / 1000;
  if (wH < 10)
    return wH.toFixed(3) + "&nbsp;kWh";
  else if (wH < 100)
    return wH.toFixed(2) + "&nbsp;kWh";
  else if (wH < 1000)
    return wH.toFixed(1) + "&nbsp;kWh";

  //okay, now its mega time
  wH = wH / 1000;
  if (wH < 10)
    return wH.toFixed(3) + "&nbsp;MWh";
  else if (wH < 100)
    return wH.toFixed(2) + "&nbsp;MWh";
  else if (wH < 1000)
    return wH.toFixed(1) + "&nbsp;MWh";
  else
    return Math.roud(wH) + "&nbsp;MWh";
}

function formatBytes(bytes, decimals = 2) {
  if (!+bytes) return '0 Bytes'

  const k = 1024
  const dm = decimals < 0 ? 0 : decimals
  const sizes = ['Bytes', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB']

  const i = Math.floor(Math.log(bytes) / Math.log(k))

  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(dm))} ${sizes[i]}`
}

// return true if 'first' is greater than or equal to 'second'
function compareVersions(first, second)
{

    var a = first.split('.');
    var b = second.split('.');

    for (var i = 0; i < a.length; ++i) {
        a[i] = Number(a[i]);
    }
    for (var i = 0; i < b.length; ++i) {
        b[i] = Number(b[i]);
    }
    if (a.length == 2) {
        a[2] = 0;
    }

    if (a[0] > b[0]) return true;
    if (a[0] < b[0]) return false;

    if (a[1] > b[1]) return true;
    if (a[1] < b[1]) return false;

    if (a[2] > b[2]) return true;
    if (a[2] < b[2]) return false;

    return true;
}