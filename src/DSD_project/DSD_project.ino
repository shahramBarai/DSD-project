#include "GyverButton.h"
#include "messages.h"
#include "Telegram.h"
#include "Accelerometer.h"
#include "BCG.h"

//----------Push button----------
const int BUTTON_PIN = 18;
GButton touch(BUTTON_PIN, HIGH_PULL, NORM_OPEN);
uint32_t tmr, tmr_bcg, tmr_tone; const int WAITING_DELAY = 10000;
bool fall_flag, BCG_flag, tone_flag;
int vBPM, toneMode;

void setup() {
  Serial.begin(115200);
  //pinMode(BUTTON_PIN, INPUT_PULLUP);
  touch.setStepTimeout(100);
  touch.setClickTimeout(500);

  pinMode (LED_red, OUTPUT);
  pinMode (LED_green, OUTPUT);
  
  mpu_setup();            //Accelerometer setup (MPU-6050)
  telegram_setup();       //Telegram setup
  telegram_send_message(START_MESSAGE);
  //---------------BUZZER-----------------
  ledcSetup(0,1E5, 12);
  ledcAttachPin(4  ,0);

  digitalWrite (LED_green, HIGH);
  digitalWrite (LED_red, LOW);
}

void loop() {  
  touch.tick();
  
  if (fall && touch.isDouble()) { fall_flag = true;
  } else { fall_flag = false; }

  if (touch.isTriple()) {
    telegram_send_message("The BCG measurement system is activated. Please, take a rest position and try to avoid any movement.");
    BCG_flag = true;
    tmr_bcg = millis();
  }

  if (fall == true && millis() - tmr_tone >= 500 && toneMode == 1){ //in event of a fall detection
    tone_flag = !tone_flag;
    if (tone_flag) {
      ledcWriteTone(0,800);
    } else {
      ledcWriteTone(0,0);
    }
    digitalWrite (LED_red, tone_flag);
    tmr_tone = millis();
  }

  if (!fall && millis() - tmr >= 100) {
    mpu_calculation();
    tmr = millis();
    tmr_tone = millis(); toneMode = 1;
  } else if (fall && fall_flag) {
    Serial.println("Cancelled");
    fall = false; fall_flag = false; tone_flag = false;
    ledcWriteTone(0,0); digitalWrite (LED_red, tone_flag); // stop alarm
    tmr = millis();
  } else if (fall && millis() - tmr >= WAITING_DELAY && !fall_flag){
    Serial.println("Fall detected!!!");
    telegram_send_message("Fall detected!!");
    fall_flag = false;
    if (toneMode == 1) {
      digitalWrite (LED_red, HIGH);
      ledcWriteTone(0,0);
      delay(10); 
      ledcWriteTone(0,800);
    }
    toneMode = 2;
    tmr = millis();
  } else if (BCG_flag && millis() - tmr_bcg >= bcgMeasuringTime) {
    vBPM = get_vBPM();
    Serial.println("vBPM" + String(vBPM));
    telegram_send_message(String(vBPM) + ".0 BPM (measurement error is " + String(get_flaw()) + ".0%)");

    if (vBPM < 60) telegram_send_message(HR_LOW_MESSAGE);
    else if (vBPM > 100) telegram_send_message(HR_HIGH_MESSAGE);
    else telegram_send_message(HR_NORMAL_MESSAGE);
    
    BCG_flag = false;
  }
}
