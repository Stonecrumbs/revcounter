/*********************************************************************
TX_SERIAL_NANO_v2
This sketch grabs data from CAN Bus and sends it through UART TX

Based on Working version TX_SERIAL_NANO_v1. 

Upgrades to be:
  - RPM, Temp and Volt will be separated into three messages. They will
  no longer be part of the same csv line.
  R99999 is an integer where R stands for RPM.
  T99.9 Same as RPM but T stands for Temperature
  V99.99 you know the drill.

  - Each value will only be sent when differs from the previous read.

********************************************************************/

#include <SPI.h>
#include <mcp2515.h>

bool isDemoMode = false;
bool activateLogOutputSerial = true;

// MCP2515 Config
struct can_frame canMsg;
MCP2515 mcp2515(10); // CS en pin 10

// Data variables
volatile uint16_t rpm = 0;
volatile float temperature = 0.0;
volatile float voltage = 0.0;

// for controlling the previous value. RPM is allways changing
volatile float previous_temperature = 0.0;
volatile float previous_voltage = 0.0;

// Adjust constants to temp curve and voltage
float TEMP_SLOPE = 1.024;
float TEMP_OFFSET = -40;
float VOLTAGE_FACTOR = 0.104; 


void sendSerial1(){
  Serial1.print("R");
  Serial1.println(rpm);

  if (previous_temperature != temperature){
    Serial1.print("T");
    Serial1.println(temperature, 1); // 1 decimal for temperature
    previous_temperature = temperature;
  }

  if (previous_voltage != voltage){
    Serial1.print("V");
    Serial1.println(voltage, 2);   // 2 decimals for voltage
    previous_voltage = voltage;
  }
}

//Serial output for logging
void sendSerial(){
  Serial.print("R");
  Serial.println(rpm);

  Serial.print("T");
  Serial.println(temperature, 1); // 1 decimal for temperature
  previous_temperature = temperature;

  Serial.print("V");
  Serial.println(voltage, 2);   // 2 decimals for voltage
  previous_voltage = voltage;
}

void setup() {
  Serial.begin(115200);    // for output log
  Serial1.begin(115200);    // For UART 

  // Inicialise MCP2515
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); 
  mcp2515.setNormalMode();
  
  if (activateLogOutputSerial) {
    Serial.println("MCP2515 Inicializado.");
  }
}

void loop() {
  if (isDemoMode) {
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 100) { // send every 100 ms
      lastSend = millis();
      rpm = 5000+(lastSend/100);
      temperature = 1.1+(lastSend/1000);
      voltage = 15.2+(lastSend/1000);
      
      sendSerial1();

      if (activateLogOutputSerial) {
        sendSerial();
      }
    }
  }
  else {  
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      uint32_t canId = canMsg.can_id;

      if (canId == 0x212) { // RPM
        uint16_t rpm_raw = (canMsg.data[2] << 8) | canMsg.data[3];
        rpm = rpm_raw;
      }
      else if (canId == 0x608) { // Temperature
        uint8_t temp_raw = canMsg.data[0];
        temperature = (TEMP_SLOPE * temp_raw) + TEMP_OFFSET;
      } 
      else if (canId == 0x151) { // Voltage
        uint8_t volt_raw = canMsg.data[5];
        voltage = volt_raw * VOLTAGE_FACTOR;
      }
    }

    static unsigned long lastSend = 0;
    if (millis() - lastSend > 200) { // send every 200 ms

      sendSerial1();

      if (activateLogOutputSerial) {
        sendSerial();
      }
    }
  }
}
