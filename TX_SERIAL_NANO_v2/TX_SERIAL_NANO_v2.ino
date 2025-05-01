/*********************************************************************
TX_SERIAL_NANO_v2
This sketch grabs data from CAN Bus and sends it through UART TX

In demo mode you can send new values via UART. i.e. R5000, V13 or T70



********************************************************************/

#include <SPI.h>
#include <mcp2515.h>

bool isDemoMode = false;
bool activateLogOutputSerial = false;

// MCP2515 Config
struct can_frame canMsg;
MCP2515 mcp2515(10); // CS en pin 10

// Data variables
volatile uint16_t rpm = 0;
volatile float temperature = 0.0;
volatile float voltage = 0.0;

volatile float previous_temperature = 0.0;
volatile float previous_voltage = 0.0;

float TEMP_SLOPE = 1.024;
float TEMP_OFFSET = -40;
float VOLTAGE_FACTOR = 0.104; 

const int writeIntervalMs = 200;

void sendSerial1(){
  if (activateLogOutputSerial) {
    Serial.print("R");
    Serial.println(rpm);

    if (previous_temperature != temperature){
      Serial.print("T");
      Serial.println(temperature, 1);
    }

    if (previous_voltage != voltage){
      Serial.print("V");
      Serial.println(voltage, 2);
    }
  }

  Serial1.print("R");
  Serial1.println(rpm);

  if (previous_temperature != temperature){
    Serial1.print("T");
    Serial1.println(temperature, 1);
    previous_temperature = temperature;
  }

  if (previous_voltage != voltage){
    Serial1.print("V");
    Serial1.println(voltage, 2);
    previous_voltage = voltage;
  }
}

void setup() {
  Serial.begin(115200);    
  Serial1.begin(115200);    

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); 
  mcp2515.setNormalMode();
  
  if (activateLogOutputSerial) {
    Serial.println("MCP2515 Inicializado.");
  }
}

void loop() {
  // Input RPM override from Serial when in demo mode
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (isDemoMode && input.startsWith("R")) {
      int newRpm = input.substring(1).toInt();
      if (newRpm > 0) {
        rpm = newRpm;
        if (activateLogOutputSerial) {
          Serial.print("RPM manually set to: ");
          Serial.println(rpm);
        }
      }
    }
    if (isDemoMode && input.startsWith("T")) {
      int newTemp = input.substring(1).toInt();
      if (newTemp > 0) {
        temperature = newTemp;
        if (activateLogOutputSerial) {
          Serial.print("Temp manually set to: ");
          Serial.println(temperature);
        }
      }
    }   
    if (isDemoMode && input.startsWith("V")) {
      int newVolt = input.substring(1).toInt();
      if (newVolt > 0) {
        voltage = newVolt;
        if (activateLogOutputSerial) {
          Serial.print("Volt manually set to: ");
          Serial.println(voltage);
        }
      }
    }    
  }

  if (isDemoMode) {
    static unsigned long lastSend = 0;
    if (millis() - lastSend > writeIntervalMs) {
      lastSend = millis();

      // Solo actualizamos si no se ha sobreescrito por Serial
      
      //rpm = 1000+(lastSend/100);
      //temperature = 1.1+(lastSend/1000);
      //voltage = 15.2+(lastSend/1000);

      sendSerial1();
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
    if (millis() - lastSend > writeIntervalMs) {
      lastSend = millis();
      sendSerial1();
    }
  }
}
