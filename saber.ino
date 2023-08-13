#define DEBUG 1           // debug information in Serial (1 - allow, 0 - disallow)


#include "FastLED.h"        // addressable LED library
#include <avr/pgmspace.h>   // PROGMEM library
//#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "FastLED.h"        // addressable LED library
#include <EEPROM.h>


// SETTINGS 
#define NUM_LEDS 60         // number of microcircuits WS2812 on LED strip
#define BTN_TIMEOUT 800     // button hold delay, ms
#define STRIKE_THR 150      // hit acceleration threshold



//PINS
#define LED_PIN 12
#define SPEAKER_PIN 10
#define BUTTON_PIN 20

CRGB leds[NUM_LEDS];
MPU6050 accelgyro;


//VARIABLES 
boolean SABER_ON = false;
boolean gyro_on;
enum LED_STATES { RED, BLUE, GREEN, RAINBOW, LAST_LIGHT };
int CURRENT_LED_STATE;

//MPU 6050 Variab;es
int16_t ax, ay, az;
int16_t gx, gy, gz;
int gyroX, gyroY, gyroZ, accelX, accelY, accelZ, freq, freq_f = 20;
unsigned long ACC, GYR, COMPL;
unsigned long humTimer = -9000, mpuTimer, nowTimer;
float k = 0.2;



void setup() {
//  Wire.begin();
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("SETUP BEGINNING");

  // LED Setup
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(100);  // ~40% of LED strip brightness
  setAll(0, 0, 0);             // and turn it off


  //Button Setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //IMU MPU Initialize.
    // IMU initialization
  accelgyro.initialize();
  accelgyro.setFullScaleAccelRange(MPU6050_ACCEL_FS_16);
  accelgyro.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  if (DEBUG) {
    if (accelgyro.testConnection()) Serial.println(F("MPU6050 OK"));
    else Serial.println(F("MPU6050 fail"));
  }
  Serial.println("SETUP INITIALIZED");
}

void loop() {
 btnTick();
 gyroTick();
 ledTick();
 strikeTick();
 // Sound Generation
 tone(SPEAKER_PIN,freq_f + 50);
}

void ledTick(){
    if (!SABER_ON){
      setAll(0,0,0);
      return;
    }
    // Different Lights and States
    switch (CURRENT_LED_STATE) {
    case RED:
      setAll(255,0,0);
      break;
    case GREEN:
      setAll(0,255,0);
      break;
    case BLUE:
      setAll(0,0,255);
      break;
    case RAINBOW:
      rainbowTick();
      break;
    default:
      // statements
      break;
}
  }


void gyroTick() {
  if (millis() - mpuTimer > 500) {                            
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);       

    // find absolute and divide on 100
    gyroX = abs(gx / 100);
    gyroY = abs(gy / 100);
    gyroZ = abs(gz / 100);
    accelX = abs(ax / 100);
    accelY = abs(ay / 100);
    accelZ = abs(az / 100);

    // vector sum
    ACC = sq((long)accelX) + sq((long)accelY) + sq((long)accelZ);
    ACC = sqrt(ACC);
    GYR = sq((long)gyroX) + sq((long)gyroY) + sq((long)gyroZ);
    GYR = sqrt((long)GYR);
    COMPL = ACC + GYR;
    
    freq = (long)COMPL * COMPL / 1500;                        // parabolic tone change
    freq = constrain(freq, 18, 300);                          
    freq_f = freq * k + freq_f * (1 - k);                     // smooth filter
    mpuTimer = micros();                                     
  }
}

void strikeTick() {
  if (ACC > STRIKE_THR) {
    hit_flash();
    Serial.println("HIT");
  }
}


int btn_counter;
boolean btn_pressed, btn_action_init, btn_hold_flag , btn_prev_pressed;
unsigned long btn_timer;

//Used to differentiate Button holds and clicking the button twice.
void btnTick() {
  Serial.println(btn_action_init);
  btn_prev_pressed = btn_pressed;
  btn_pressed = !digitalRead(BUTTON_PIN);
  // button-action init defines the first n action and starts the button the button timer.
  if (btn_pressed && !btn_action_init) {
    if (DEBUG) Serial.println(F("BTN PRESS"));
    btn_action_init = true;
    btn_hold_flag = true;
    btn_counter = 0;
    btn_timer = millis();
  }
  
  if (!btn_pressed && btn_hold_flag) {
    btn_hold_flag = false;
  }

  if(!btn_prev_pressed && btn_pressed){
    btn_counter += 1;
   }
  if ((millis() - btn_timer > BTN_TIMEOUT) && btn_action_init){
    Serial.println("DONE BTN_ACTION");
    Serial.println(btn_counter);
    // BUTTON HOLD
    if(btn_hold_flag){
      SABER_ON = !SABER_ON;
      if (SABER_ON){
         light_up();
        }
        else{
         light_down();
          }
      }
    // LIGHT MODE CHANGE
    else if(btn_counter == 2 && SABER_ON){
        CURRENT_LED_STATE += 1;
      if (CURRENT_LED_STATE == LAST_LIGHT)
        CURRENT_LED_STATE = 0;
      }
    else if(btn_counter == 5 && SABER_ON){
      Serial.println("3 presses");
     }
    btn_action_init = false;
    }
}




void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue);
  }
  FastLED.show();
}
void setPixel(int Pixel, byte red, byte green, byte blue) {
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}

void light_up() {
  for (char i = 0; i <= (NUM_LEDS / 2 - 1); i++) {        
    setPixel(i, 255, 255, 255);
    setPixel((NUM_LEDS - 1 - i), 255, 255, 255);
    FastLED.show();
    delay(25);
  }
}

void light_down() {
  for (char i = (NUM_LEDS / 2 - 1); i >= 0; i--) {      
    setPixel(i, 0, 0, 0);
    setPixel((NUM_LEDS - 1 - i), 0, 0, 0);
    FastLED.show();
    delay(25);
  }
}

unsigned long rainbow_timer;
int counter;
int rainbow_delay = 10;
void rainbowTick(){
    if(millis() - rainbow_timer > rainbow_delay){
       rainbow_timer = millis();
       counter +=1;
      }
     if (counter == 512){
      counter = 0;
     }
    rainbow(counter);
  }

//Tick Between 0 and 256
void rainbow(int tick) {
  uint16_t i, j;
    for(i=0; i<NUM_LEDS; i++) {
      int r;
      int g;
      int b;
      Wheel((i*1+tick) & 255, &r,&g,&b);
      setPixel(i, g,r,b);
    }
    FastLED.show();
}

void Wheel(byte WheelPos , int*r,int*g,int*b) {
  if(WheelPos < 85) {
    *g = WheelPos * 3;
    *r = 255 - WheelPos * 3;
    *b = 0;
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    *b = WheelPos * 3;
    *g = 255 - WheelPos * 3;
    *r = 0;
  } 
  else {
    WheelPos -= 170;
    *r = WheelPos * 3;
    *b = 255 - WheelPos * 3;
    *g = 0;
  }
}


int FLASH_DELAY = 300;
void hit_flash() {
  setAll(255, 255, 255);            
  delay(FLASH_DELAY);                
}
