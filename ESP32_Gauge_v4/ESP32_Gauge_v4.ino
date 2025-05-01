/*

ESP32_Gauge_V4.ino

From v2 version
This version has all the LVGL components in place and the right color scheme.
Read data from UART, coming from Arduino NANO
Emergency blinking 
UART Error detection

From V3
This version will take the new format for indepenent messages R, T and V.
The amount of buffer lines has been increased to buf[240 * 20]
A second buffer has been added.

This version
Get rid of PID and use just average


*/
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <ui.h>

#define RXD1 27
#define TXD1 26

bool activateLogOutputSerial = false;

// Global Objects
TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
lv_disp_draw_buf_t draw_buf;
lv_color_t buf[240 * 20]; // Buffer de 10 líneas
lv_color_t buf2[240 * 20]; // Buffer de 10 líneas
lv_obj_t *scr;//Screen Background
lv_obj_t *temp_meter; //temp Meter

//variables to hold data from CAN bus
int rpm = 0;
float temperature = 0.0;
float voltage = 0.0;
char tempStr[10];
char voltStr[10];

//To avoid update T and V if nothing changes.
float lastTemperature = -1.0;
float lastVoltage = -1.0;


// PID conf and read
const int readIntervalMs = 50;       // Reading interval in milliseconds
const int rpmBufferSize = 5;       // buffer size for calculating the average

// RPM Buffer (average in rpmBufferSize reads)
int rpmBuffer[rpmBufferSize];
int rpmIndex = 0;
bool bufferFull = false;

unsigned long lastReadTime = 0;

// Emergency boundaries
float tempThreshold = 105.0;
float voltThreshold = 12;


//Red blink for emergency status
bool isEmergency = false;
bool toggleRed = false;
lv_timer_t *emergencyTimer = NULL;


//  UART error control
unsigned long lastDataReceivedTime = 0;
const unsigned long dataTimeoutMs = 1000;
bool dataTimedOut = false;

// Flush callback: This is invoked when LVGL wants to draw
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  int w = area->x2 - area->x1 + 1;
  int h = area->y2 - area->y1 + 1;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)color_p, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

void make_temp_meter(void){
  temp_meter = lv_arc_create(lv_scr_act());
  lv_obj_set_size(temp_meter, 100, 100);
  lv_arc_set_rotation(temp_meter, 0);
  lv_arc_set_bg_angles(temp_meter, 0, 50);
  lv_arc_set_range(temp_meter, 0, 100);
  lv_obj_center(temp_meter);
  lv_arc_set_mode(temp_meter, LV_ARC_MODE_REVERSE);

  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
}


void ReadSerial2Data() {
  static String inputString = "";

  while (Serial2.available()) {
    char inChar = (char)Serial2.read();
    if (inChar == '\n') {
      inputString.trim(); // Clean spaces and \r
      if (activateLogOutputSerial) {
        Serial.println("Data received: " + inputString);
      }
      

      lastDataReceivedTime = millis();  // Actualiza la marca de tiempo
      dataTimedOut = false;             // Resets flag

      if (inputString.indexOf('R') != -1 || 
          inputString.indexOf('T') != -1 || 
          inputString.indexOf('V') != -1 ) {

        String dataType = inputString.substring(0, 1);
        String dataValue = inputString.substring(1);

        if (dataType == "R") {
          rpm = dataValue.toInt();
          if (activateLogOutputSerial) {
            Serial.print("RPM: ");
            Serial.println(rpm);
          }          
        }

        if (dataType == "T") {
          float newTemp = dataValue.toFloat();
          if (newTemp != temperature) {
            temperature = newTemp;
            if (activateLogOutputSerial) {
              Serial.print("Temperatura: ");
              Serial.println(temperature);
            }     
          }
        }

        if (dataType == "V") {
          float newVolt = dataValue.toFloat();
          if (newVolt != voltage) {
            voltage = newVolt;
            if (activateLogOutputSerial) {
              Serial.print("Voltaje: ");
              Serial.println(voltage);
            }     
          }
        }

      } else {
        if (activateLogOutputSerial) {
          Serial.println("Error: Badly formed frame");
        }
      }

      inputString = ""; // Reset for the next line
    } else {
      inputString += inChar;
    }
  }
}

int getCurrentRPM() {
  return lv_arc_get_value(ui_RPM);  // Current value in arc
}

float getRPMaverage() {
  long sum = 0;
  int elements = bufferFull ? rpmBufferSize : rpmIndex;

  for (int i = 0; i < elements; i++) {
    sum += rpmBuffer[i];
  }

  return (elements > 0) ? ((float)sum / elements) : 0;
}


void emergencyFlashCallback(lv_timer_t * timer) {
  toggleRed = !toggleRed;

  lv_color_t flashColor = toggleRed 
    ? lv_color_hex(0x000000)   // Black
    : lv_color_hex(0xFF0000);  // Dark red

  lv_obj_set_style_bg_color(ui_Screen1, flashColor, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_RPM, flashColor, LV_PART_MAIN | LV_STATE_DEFAULT);
}


void checkEmergencyConditions() {
  bool tempHigh = temperature >= tempThreshold;
  bool voltLow = voltage <= voltThreshold && voltage != 0;

  bool emergencyDetected = tempHigh || voltLow;

  if (emergencyDetected && !isEmergency) {
    isEmergency = true;
    toggleRed = false;
    emergencyTimer = lv_timer_create(emergencyFlashCallback, 500, NULL); // every 500 ms
  }
  else if (!emergencyDetected && isEmergency) {
    isEmergency = false;
    lv_timer_del(emergencyTimer);
    emergencyTimer = NULL;
    // Resets original color 
    lv_color_t normalColor = lv_color_hex(_ui_theme_color_orangeBG[0]);
    lv_obj_set_style_bg_color(ui_Screen1, normalColor, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_RPM, normalColor, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}






void setup() {
  Serial.begin(115200);   // To print output log
  Serial2.begin(115200, SERIAL_8N1, RXD1, TXD1);  // To read from UART2 
  delay(1000);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  lv_init();

  //Initialize a display buffer
  lv_disp_draw_buf_init(&draw_buf, buf, buf2, sizeof(buf) / sizeof(buf[0]));

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 240;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  ui_init();

}

void loop() {
  // Read Serial every X ms
  if (millis() - lastReadTime >= readIntervalMs) {
    lastReadTime = millis();
    ReadSerial2Data();

    // adds to buffer
    rpmBuffer[rpmIndex++] = rpm;
    if (rpmIndex >= rpmBufferSize) {
      rpmIndex = 0;
      bufferFull = true;
    }    
  }

  if (millis() - lastDataReceivedTime > dataTimeoutMs) {
    if (!dataTimedOut) {
      lv_arc_set_value(ui_RPM, 0);
      lv_label_set_text(ui_RPMtext, " ");
      lv_label_set_text(ui_TEMPlabel, "ERR");  //Mostrar error
      lv_label_set_text(ui_VOLTlabel, " ");
      dataTimedOut = true;
    }
  } else {
    // Only updates in case of timeout
    
    // average calculation
    float rpmAverage = getRPMaverage();
    int newRPM = round(rpmAverage);




    if ( rpmAverage==0 ) { // Engine off
      lv_arc_set_value(ui_RPM, 0);
      lv_label_set_text(ui_RPMtext, "ENG OFF");

      dtostrf(temperature, 4, 1, tempStr);  // min width set to 4, 1 decimal
      lv_label_set_text_fmt(ui_TEMPlabel, "%sC", tempStr);
      dtostrf(voltage, 4, 1, voltStr);      // min width set to 4, 2 decimals
      lv_label_set_text_fmt(ui_VOLTlabel, "%sv", voltStr);

    } else {
      lv_arc_set_value(ui_RPM, newRPM);
      lv_label_set_text_fmt(ui_RPMtext, "%d", newRPM);

      dtostrf(temperature, 4, 1, tempStr);  // min width set to 4, 1 decimal
      lv_label_set_text_fmt(ui_TEMPlabel, "%sC", tempStr);
      dtostrf(voltage, 4, 1, voltStr);      // min width set to 4, 2 decimals
      lv_label_set_text_fmt(ui_VOLTlabel, "%sv", voltStr);
      
      checkEmergencyConditions();
    }
  }

  lv_tick_inc(5);
  lv_timer_handler();
}
