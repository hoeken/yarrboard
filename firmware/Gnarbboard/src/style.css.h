const char stylesheet_css_data[] PROGMEM = R"rawliteral(
html {
  font-family: Helvetica;
  display: inline-block;
  margin: 0px auto;
  text-align: center;
}

h1 {
  color: #0F3376;
  padding: 2vh;
}

p {
  font-size: 1.5rem;
  padding: 0.2rem;
}

span {
  padding: 0.5rem;
}

table {
  margin-left: auto;
  margin-right: auto;
}

td.channelName {
  font-size: 1.25rem;
}

td.channelAmpHours, td.channelCurrent, td.channelDutyCycle {
  font-size: 1.0rem;
  text-align: right;
}

button {
  display: inline-block;
  border: none;
  border-radius: 4px;
  color: white;
  padding: 10px;
  text-decoration: none;
  font-size: 20px;
  margin: 2px;
  cursor: pointer;
  width: 80px;
}

.buttonOn {
  background-color: #0288D1;
}

.buttonOff {
  background-color: #8094A4;
}
)rawliteral";