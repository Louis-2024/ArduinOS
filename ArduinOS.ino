#include <Arduino_FreeRTOS.h>
#include <U8x8lib.h>
#include <EEPROM.h>

#define BUZZER 5
#define BUTTON 4
#define DIAL A0

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

TaskHandle_t TurnOn_Handler;
TaskHandle_t Unlock_Handler;

bool prevButtonState = 0;
String passwordInput = "";

void DisplayUpperTab(byte dialLevel){
  switch(dialLevel) {
    case 0:
      u8x8.drawUTF8(0, 0, "BACK");
      u8x8.setInverseFont(1);
      u8x8.drawUTF8(12, 0, "NEXT");
      u8x8.setInverseFont(0);
      break;
    case 3:
      u8x8.setInverseFont(1);
      u8x8.drawUTF8(0, 0, "BACK");
      u8x8.setInverseFont(0);
      u8x8.drawUTF8(12, 0, "NEXT");
      break;
    default:
      u8x8.drawUTF8(0, 0, "BACK");
      u8x8.drawUTF8(12, 0, "NEXT");
  }
}

void TaskTurnOn(void *pvParameters){
  while(1){
    byte buttonState = digitalRead(BUTTON);
    if(!prevButtonState && buttonState){
      u8x8.draw2x2UTF8(0, 3, "ArduinOS");
      vTaskDelay(2000/portTICK_PERIOD_MS);
      u8x8.clear();
      xTaskCreate(TaskUnlock, "Unlock", 160, NULL, 1, &Unlock_Handler); //max measured stack size: 145
      vTaskDelete(NULL);
    }
    prevButtonState = buttonState;
    vTaskDelay(20/portTICK_PERIOD_MS);
  }
}

void TaskUnlock(void *pvParameters){
  Serial.begin(9600);
  while(1){
    byte buttonState = digitalRead(BUTTON);
    byte dialLevel = round((float) analogRead(DIAL)/1023 * 3);  //4 levels
    DisplayUpperTab(dialLevel);

    if(Serial.available()>0){
      char input = Serial.read();
      if(input >= '0' && input <= '9' && passwordInput.length() < 4){
        passwordInput = passwordInput + input;
      }
    }

    if(buttonState && !prevButtonState && dialLevel == 0 && passwordInput.length() == 4){  // 4 digits && presssing button
      if(EEPROM.read(0) > 9 && EEPROM.read(1) > 9 && EEPROM.read(2) > 9 && EEPROM.read(3) > 9 && passwordInput == "0000"){ //default password: 0000
        //unlock
        Serial.end();
        passwordInput = "";
      }else if(EEPROM.read(0) == passwordInput[0] && EEPROM.read(1) == passwordInput[1] && EEPROM.read(2) == passwordInput[2] && EEPROM.read(3) == passwordInput[3]){
        //unlock
        Serial.end();
        passwordInput = "";
      }else{
        //unlock
        passwordInput = "";
      }
    }

    if(buttonState && !prevButtonState && dialLevel == 3){  //back
      Serial.end();
      prevButtonState = buttonState;
      passwordInput = "";
      u8x8.clear();
      xTaskCreate(TaskTurnOn, "TurnOn", 160, NULL, 3, &TurnOn_Handler);
      vTaskDelete(NULL);
    }

    u8x8.drawUTF8(1, 4, "Password:");
    for(byte i = 0; i < passwordInput.length(); i++){
      u8x8.drawUTF8(11 + i, 4, "*");
    }
    for(byte j = 0; j < 4 - passwordInput.length(); j++){
      u8x8.drawUTF8(14 - j, 4, "_");
    }
    prevButtonState = buttonState;
    vTaskDelay(20/portTICK_PERIOD_MS);
  }
}

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(DIAL, INPUT);

  u8x8.begin();
  u8x8.setFlipMode(1);
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);

  xTaskCreate(TaskTurnOn, "TurnOn", 160, NULL, 3, &TurnOn_Handler);
}

void loop() { }
