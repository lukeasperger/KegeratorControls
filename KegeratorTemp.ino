#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Wire.h>
#include <ILI9341_t3.h>
#include <Adafruit_STMPE610.h>



/*---------------Module Defines-----------------------------*/
#define TEMP_IN                       3
#define RELAY_PIN                     4
#define FAN_PIN                       23
#define UP_BUTTON                     19
#define DOWN_BUTTON                   20
#define DEFAULT_TEMP                  37 // Degrees Farenheit
#define STOP_COMPRESSOR_THRESHOLD     2 // Degrees Farenheit
#define START_COMPRESSOR_THRESHOLD    2 // Degrees Farenheit
#define COMPRESSOR_DELAY              60*10 // seconds
#define MIN_TEMP                      35
#define MAX_TEMP                      45

/*---------------Define Display Parameters (240 x 320)------*/
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define SET_CURSOR_X                  50
#define SET_CURSOR_Y                  50
#define CUR_CURSOR_X                  50
#define CUR_CURSOR_Y                  150
#define TEXT_SIZE                     3

#define FRAME_X 210
#define FRAME_Y 180
#define FRAME_W 100
#define FRAME_H 50

#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
#define TFT_CS 10
#define TFT_DC  9
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

/*---------------Module Function Prototypes-----------------*/
float tempControl();
float readTemp();
void updateDisplay();
void checkButtons();
void timerActions();

/*---------------State Definitions--------------------------*/
typedef enum {
  STARTUP_STATE, COOLING_STATE, IDLING_STATE
} States_t;

/*---------------Module Variables---------------------------*/
static int set_temp;
static bool up_pressed = false;
static bool down_pressed = false;
States_t state;
unsigned long last_cycle_end;
OneWire oneWire(TEMP_IN);
DallasTemperature dTemp(&oneWire);
static IntervalTimer myTimer;
static IntervalTimer buttonTimer;

void setup() {

  Serial.begin(9600);
  myTimer.begin(timerActions, 1000000);
  buttonTimer.begin(checkButtons, 100000);
  
  // set pins
  pinMode(TEMP_IN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(UP_BUTTON, INPUT);
  pinMode(DOWN_BUTTON, INPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(FAN_PIN,LOW);

  // start screen
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(1); 
  delay(2000); // temporary fix to ignore first temp reading which is usually garbage

  // set variables
  set_temp = DEFAULT_TEMP;
  state = STARTUP_STATE;
  last_cycle_end = 0;

}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("starting loop");
  
}

void timerActions() {
  float cur_temp = tempControl();
  updateDisplay(cur_temp);
}

float tempControl() {
  Serial.println("temp ctr");
  float cur_temp = readTemp();

  if (state == COOLING_STATE || state == STARTUP_STATE) {
    if ((set_temp - cur_temp) > STOP_COMPRESSOR_THRESHOLD) {
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(FAN_PIN, LOW);
      state = IDLING_STATE;
      last_cycle_end = millis();
    }
  } 
  if (state == IDLING_STATE || state == STARTUP_STATE) {
    if ((cur_temp - set_temp) > START_COMPRESSOR_THRESHOLD) {
      if ((millis() - last_cycle_end) < COMPRESSOR_DELAY * 1000 && state != STARTUP_STATE) {
        return cur_temp; 
      }
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(FAN_PIN, HIGH);
      state = COOLING_STATE;
    }
  }

  return cur_temp;
}

float readTemp() {
  dTemp.requestTemperatures();
  float tempC = dTemp.getTempCByIndex(0);
  float tempF = tempC * 1.8 + 32.0;
  Serial.println("temp read");

  // Check if reading was successful
  if(tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature sensor reading is ");
    Serial.print(tempF);
    Serial.println(" degrees Farenheit");
  } else {
    Serial.println("Error: Could not read temperature data");
  }
  return tempF;
}

void updateDisplay(float cur_temp) {
  //tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, ILI9341_BLACK);

  tft.fillRect(0,0, 320, 240, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLUE);
  tft.setTextSize(TEXT_SIZE);
  tft.setCursor(0,0);
  tft.print("Set Temp: ");
  tft.println(set_temp);
  tft.print("Cur Temp: ");
  tft.println(cur_temp);
  tft.println();
  if (state == COOLING_STATE) {
    tft.println("Cooling State\n");
  } else if (state == IDLING_STATE) {
    tft.println("Idling State\n");
  }
  tft.println("Set to +/- 2 deg");
  if (state == IDLING_STATE) {
    int cycle_time_min = (millis() - last_cycle_end) / (1000*60);
    int delay_time = COMPRESSOR_DELAY/60;
    if (delay_time > cycle_time_min) {
      tft.print("Delay: ");
      tft.print(delay_time - cycle_time_min);
      tft.println(" min");
    }
    
  }
  
  return;
}

void checkButtons() {

  // check up button
  if (digitalRead(UP_BUTTON) && !up_pressed) {
    up_pressed = true;
    if (set_temp < MAX_TEMP) set_temp++;
  } else if (!digitalRead(UP_BUTTON) && up_pressed) {
    up_pressed = false;
  }

  // check down button
  if (digitalRead(DOWN_BUTTON) && !down_pressed) {
    down_pressed = true;
   if (set_temp > MIN_TEMP)  set_temp--;
  } else if (!digitalRead(DOWN_BUTTON) && down_pressed) {
    down_pressed = false;
  }
}

