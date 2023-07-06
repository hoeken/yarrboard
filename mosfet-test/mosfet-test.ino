/*
  Fade

  This example shows how to fade an LED on pin 9 using the analogWrite()
  function.

  The analogWrite() function uses PWM, so if you want to change the pin you're
  using, be sure to use another PWM capable pin. On most Arduino, the PWM pins
  are identified with a "~" sign, like ~3, ~5, ~6, ~9, ~10 and ~11.

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Fade
*/

int led = 18;         // the PWM pin the LED is attached to
int mosfet = 16;         // the PWM pin the LED is attached to

const int sensorIn = 4;      // pin where the OUT pin from sensor is connected on Arduino
float amps = 0;
float watts = 0;

int i = 0;

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin (115200); 
  Serial.println ("ACS712 current sensor"); 

  pinMode(led, OUTPUT);
  pinMode(mosfet, OUTPUT);
}

// the loop routine runs over and over again forever:
void loop() {

  Serial.print(i);
  Serial.println("");
  i++;

  digitalWrite(led, HIGH);
  digitalWrite(mosfet, HIGH);
  delay(2000);

  amps = getAmperage();
  watts = amps * 28.8;

  Serial.print(amps);
  Serial.print(" Amps ---  ");
  Serial.print(watts);
  Serial.println(" Watts");
  
  digitalWrite(led, LOW);
  digitalWrite(mosfet, LOW);
  delay(2000);

  amps = getAmperage();
  watts = amps * 28.8;

  Serial.print(amps);
  Serial.print(" Amps ---  ");
  Serial.print(watts);
  Serial.println(" Watts");

}

// ***** function calls ******
float getAmperage()
{
  float AcsValue=0.0, Samples=0.0, AvgAcs=0.0, AcsValueF=0.0;
  for (int x = 0; x < 5; x++)
  {
    AcsValue = analogRead(sensorIn);     //Read current sensor values   
    Samples = Samples + AcsValue;  //Add samples together
    delay (3); // let ADC settle before next sample 3ms
  }
  AvgAcs=Samples/5.0;//Taking Average of Samples

  //AvgAcs = analogRead(sensorIn);     //Read current sensor values   

  float amps;

  Serial.print(AvgAcs);
  Serial.print(" ADC ---  ");

  float volts;
  volts = AvgAcs * (3.3 / 4096.0);
  
  Serial.print(volts);
  Serial.print(" V ---  ");

  //result = (readValue * 3.3) / 4096.0 / mVperAmp; //ESP32 ADC resolution 4096
  amps = (1.65 - (AvgAcs * (3.3 / 4096.0))) / 0.100;

  Serial.print(amps);
  Serial.print(" A ---  ");

  return amps;
 }
