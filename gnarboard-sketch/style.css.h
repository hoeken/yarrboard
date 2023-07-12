const char stylesheet_css_data[] PROGMEM = R"rawliteral(
html {
  font-family: Helvetica;
  display: inline-block;
  margin: 0px auto;
  text-align: center;
}
h1{
  color: #0F3376;
  padding: 2vh;
}
p{
  font-size: 1.5rem;
  padding: 0.2rem;
}

span {
  padding: 0.5rem;
  text-align: left;
}

button {
  display: inline-block;
  border: none;
  border-radius: 4px;
  color: white;
  padding: 16px 40px;
  text-decoration: none;
  font-size: 30px;
  margin: 2px;
  cursor: pointer;
}

.buttonOn {
  background-color: #008CBA;
}

.buttonOff {
  background-color: #D3D3D3;
}
)rawliteral";