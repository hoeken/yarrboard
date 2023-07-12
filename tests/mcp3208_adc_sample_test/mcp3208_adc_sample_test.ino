/**
 * Basic ADC reading example.
 * - connects to ADC
 * - reads value from channel
 * - converts value to analog voltage
 */

#include <SPI.h>
#include <Mcp320x.h>

#define SPI_CS    	5 		   // SPI slave select
#define ADC_VREF    3300     // 3.3V Vref
#define ADC_CLK     1600000  // SPI clock 1.6MHz


MCP3208 adc(ADC_VREF, SPI_CS);

void setup() {

  // configure PIN mode
  pinMode(SPI_CS, OUTPUT);

  // set initial PIN state
  digitalWrite(SPI_CS, HIGH);

  // initialize serial
  Serial.begin(115200);

  // initialize SPI interface for MCP3208
  SPISettings settings(ADC_CLK, MSBFIRST, SPI_MODE0);
  SPI.begin();
  SPI.beginTransaction(settings);
}

int lastMillis = 0;
int samples = 0;

void loop()
{
  uint16_t raw = 0;
  raw = adc.read(MCP3208::Channel::SINGLE_0);
  raw = adc.read(MCP3208::Channel::SINGLE_1);
  raw = adc.read(MCP3208::Channel::SINGLE_2);
  raw = adc.read(MCP3208::Channel::SINGLE_3);
  raw = adc.read(MCP3208::Channel::SINGLE_4);
  raw = adc.read(MCP3208::Channel::SINGLE_5);
  raw = adc.read(MCP3208::Channel::SINGLE_6);
  raw = adc.read(MCP3208::Channel::SINGLE_7);
  //uint16_t val = adc.toAnalog(raw);

  samples++;

  if (millis() - lastMillis > 1000)
  {
    Serial.print("Samples: ");
    Serial.println(samples);

    samples = 0;
    lastMillis = millis();
  }
}
