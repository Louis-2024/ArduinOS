#include <Arduino_FreeRTOS.h>
#include <U8x8lib.h>
#include <EEPROM.h>

#define BUZZER 5
#define BUTTON 4
#define DIAL A0

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

TaskHandle_t TurnOn_Handler;
TaskHandle_t Unlock_Handler;
TaskHandle_t Menu_Handler;
TaskHandle_t Setting_Handler;
TaskHandle_t ResetPassword_Handler;

bool prevButtonState = 0;
String passwordInput = "";

void TaskTurnOn(void *pvParameters){
  while(1){
    byte buttonState = digitalRead(BUTTON);
    if(!prevButtonState && buttonState){
      u8x8.draw2x2UTF8(0, 3, "ArduinOS");
      vTaskDelay(2000/portTICK_PERIOD_MS);
      u8x8.clear();
      xTaskCreate(TaskUnlock, "Unlock", 256, NULL, 1, &Unlock_Handler);
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

    if(Serial.available()>0){
      char input = Serial.read();
      if(input >= '0' && input <= '9' && passwordInput.length() < 4){
        passwordInput = passwordInput + input;
      }
    }

    if(buttonState && !prevButtonState){  //presssing button
      if(dialLevel == 0 && passwordInput.length() == 4){
        if((EEPROM.read(0) > 9 && EEPROM.read(1) > 9 && EEPROM.read(2) > 9 && EEPROM.read(3) > 9 && passwordInput == "0000") || ((EEPROM.read(0) == passwordInput[0] - '0') && (EEPROM.read(1) == passwordInput[1] - '0') && (EEPROM.read(2) == passwordInput[2] - '0') && (EEPROM.read(3) == passwordInput[3] - '0'))){ //default password: 0000
          //unlock
          Serial.end();
          prevButtonState = buttonState;
          passwordInput = "";
          u8x8.clear();
          xTaskCreate(TaskMenu, "Menu", 256, NULL, 2, &Menu_Handler);
          vTaskDelete(NULL);
        }else{
          passwordInput = "";
        }
      }else if(dialLevel == 3){
        Serial.end();
        prevButtonState = buttonState;
        passwordInput = "";
        u8x8.clear();
        xTaskCreate(TaskTurnOn, "TurnOn", 256, NULL, 3, &TurnOn_Handler);
        vTaskDelete(NULL);
      }
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


void TaskMenu(void *pvParameters){
  while(1){
    byte buttonState = digitalRead(BUTTON);
    byte dialLevel = round((float) analogRead(DIAL)/1023 * 3);  //4 levels

    switch(dialLevel) {
      case 0:
        u8x8.drawUTF8(2, 2, "FILES");
        u8x8.drawUTF8(2, 4, "PROGRAMS");
        u8x8.drawUTF8(2, 6, "SETTING");
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(13, 0, "OFF");
        u8x8.setInverseFont(0);
        break;
      case 1:
        u8x8.drawUTF8(2, 2, "FILES");
        u8x8.drawUTF8(2, 4, "PROGRAMS");
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(2, 6, "SETTING");
        u8x8.setInverseFont(0);
        u8x8.drawUTF8(13, 0, "OFF");
        break;
      case 2:
        u8x8.drawUTF8(2, 2, "FILES");
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(2, 4, "PROGRAMS");
        u8x8.setInverseFont(0);
        u8x8.drawUTF8(2, 6, "SETTING");
        u8x8.drawUTF8(13, 0, "OFF");
        break;      
      case 3:
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(2, 2, "FILES");
        u8x8.setInverseFont(0);
        u8x8.drawUTF8(2, 4, "PROGRAMS");
        u8x8.drawUTF8(2, 6, "SETTING");
        u8x8.drawUTF8(13, 0, "OFF");
        break;
    }

    if(buttonState && !prevButtonState){
      switch(dialLevel) {
        case 0: //off
          prevButtonState = buttonState;
          u8x8.clear();
          xTaskCreate(TaskTurnOn, "TurnOn", 256, NULL, 3, &TurnOn_Handler);
          vTaskDelete(NULL);
          break;
        case 1: //setting
          prevButtonState = buttonState;
          u8x8.clear();
          xTaskCreate(TaskSetting, "Setting", 256, NULL, 1, &Setting_Handler);
          vTaskSuspend(NULL);
          break;
        case 2:
          //
          break;      
        case 3:
          //
          break;
      }
    }
    prevButtonState = buttonState;
    vTaskDelay(20/portTICK_PERIOD_MS);
  }
}


void TaskSetting(void *pvParameters){
  while(1){
    byte buttonState = digitalRead(BUTTON);
    byte dialLevel = round((float) analogRead(DIAL)/1023 * 4);  //5 levels

    switch(dialLevel) {
      case 0:
        u8x8.drawUTF8(0, 0, "BACK");
        u8x8.drawUTF8(2, 2, "ABOUT");
        u8x8.drawUTF8(2, 4, "RESET TIME");
        u8x8.drawUTF8(2, 6, "RESET PASSWORD");
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(13, 0, "OFF");
        u8x8.setInverseFont(0);
        break;
      case 1:
        u8x8.drawUTF8(0, 0, "BACK");
        u8x8.drawUTF8(2, 2, "ABOUT");
        u8x8.drawUTF8(2, 4, "RESET TIME");
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(2, 6, "RESET PASSWORD");
        u8x8.setInverseFont(0);
        u8x8.drawUTF8(13, 0, "OFF");
        break;
      case 2:
        u8x8.drawUTF8(0, 0, "BACK");
        u8x8.drawUTF8(2, 2, "ABOUT");
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(2, 4, "RESET TIME");
        u8x8.setInverseFont(0);
        u8x8.drawUTF8(2, 6, "RESET PASSWORD");
        u8x8.drawUTF8(13, 0, "OFF");
        break;      
      case 3:
        u8x8.drawUTF8(0, 0, "BACK");
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(2, 2, "ABOUT");
        u8x8.setInverseFont(0);
        u8x8.drawUTF8(2, 4, "RESET TIME");
        u8x8.drawUTF8(2, 6, "RESET PASSWORD");
        u8x8.drawUTF8(13, 0, "OFF");
        break;
      case 4:
        u8x8.setInverseFont(1);
        u8x8.drawUTF8(0, 0, "BACK");
        u8x8.setInverseFont(0);
        u8x8.drawUTF8(2, 2, "ABOUT");
        u8x8.drawUTF8(2, 4, "RESET TIME");
        u8x8.drawUTF8(2, 6, "RESET PASSWORD");
        u8x8.drawUTF8(13, 0, "OFF");
        break;
    }

    if(buttonState && !prevButtonState){
      switch(dialLevel) {
        case 0: //off
          prevButtonState = buttonState;
          u8x8.clear();
          xTaskCreate(TaskTurnOn, "TurnOn", 256, NULL, 3, &TurnOn_Handler);
          vTaskDelete(NULL);
          break;
        case 1: //reset password
          prevButtonState = buttonState;
          u8x8.clear();
          xTaskCreate(TaskResetPassword, "ResetPassword", 256, NULL, 3, &ResetPassword_Handler);
          vTaskDelete(NULL);
          break;
        case 2: //reset time
          //
          break;      
        case 3: //about
          //
          break;
        case 4: //back
          prevButtonState = buttonState;
          u8x8.clear();
          vTaskResume(Menu_Handler);
          vTaskDelete(NULL);
          break;
      }
    }

    prevButtonState = buttonState;
    vTaskDelay(20/portTICK_PERIOD_MS);
  }
}

void TaskResetPassword(void *pvParameters){
  while(1){
    //
  }
}

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(DIAL, INPUT);

  u8x8.begin();
  u8x8.setFlipMode(1);
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);

  xTaskCreate(TaskTurnOn, "TurnOn", 256, NULL, 3, &TurnOn_Handler);
  vTaskStartScheduler();
}

void loop() { }
