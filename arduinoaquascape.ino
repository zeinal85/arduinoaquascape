String menuItems[] = {"Set LED State", "Feed Now", "Pump In State", "Pump Out State", "Set Filter", "LCD Setting", "Auto Feeder", "Auto LED On","Auto LED Off"};

byte downArrow[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};

byte upArrow[8] = {
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100 
};

byte menuCursor[8] = {
  B01000,
  B00100,
  B00010,
  B00001,
  B00010,
  B00100,
  B01000,
  B00000
};

#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <ServoTimer2.h>
#include "RTClib.h"

// EEPROM address
int lcdBrightnessAddress = 0;
int relayLedAddress = 1;
int feederHoursAddress = 2;
int feederMinutesAddress = 3;
int nextFeedHoursAddress = 4;
int nextFeedMinutesAddress = 5;
int filterPumpAddress = 6;
int AutoLedOnHoursAddress = 7;
int AutoLedOnMinutesAddress = 8;
int AutoLedOffHoursAddress = 9;
int AutoLedOffMinutesAddress = 10;

// Navigation button variables
int readKey;
int key;
int savedDistance = 0;

// Menu control variables
int menuPage = 0;
int maxMenuPages = round(((sizeof(menuItems) / sizeof(String)) / 2) + .5)+2;
int cursorPosition = 0;

// Setting the LCD shields pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int brightness = 50;
int lcdBrightnessPin = 10;

// Servo Setting
ServoTimer2 servo;
int servo_pin= 11;
int max_spin = 3;

// RTC Setting
RTC_DS1307 rtc;

// Relay Pin
int relayLed = 2;
int relayFilterPump = 3;
int relayWaterPumpIn = 12;
int relayWaterPumpOut = 13;

// Feed Setting
int feedHrs=1;
int feedMin=0;

// Auto Led 
int autoLedOnHrs = 8;
int autoLedOnMin = 0;
int autoLedOffHrs = 16;
int autoLedOffMin = 0;

// Water Pump In/Out State
int waterPumpInState = 1;
int waterPumpOutState = 1;

void setup() {

  // Initializes serial communication
  Serial.begin(9600);

  // Initializes and clears the LCD screen
  lcd.begin(16, 2);
  lcd.clear();
  lcd.createChar(0, menuCursor);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);
  brightness = getLcdBrightness();
  analogWrite(lcdBrightnessPin, brightness);

  // RTC setup
  // The following code will set the RTC same as computer time, Just use this code once and if your RTC is not set before.
  // if your RTC was set then just ignore this code.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (! rtc.begin()) {
    while (1);
  }

  // Relay pin configuration (set as output)
  pinMode(relayLed, OUTPUT);
  pinMode(relayFilterPump, OUTPUT);
  pinMode(relayWaterPumpIn, OUTPUT);
  pinMode(relayWaterPumpOut, OUTPUT);

  // Relay LED read EEPROM configuration and set the pin on or off based on configuration
  if( getRelayLedState() == 0 ){
    setRelayLedOn();
  }else if( getRelayLedState() == 1){
    setRelayLedOff();    
  }

  // Get feeder data from EEPROM
  feedHrs = getFeederHours();
  feedMin = getFeederMinutes();

  // Get Led On data from EEPROM
  autoLedOnHrs = getAutoLedOnHours();
  autoLedOnMin = getAutoLedOnMinutes();
  autoLedOffHrs = getAutoLedOffHours();
  autoLedOffMin = getAutoLedOffMinutes();

  digitalWrite(relayFilterPump,getFilterPumpState());
  digitalWrite(relayWaterPumpIn,waterPumpInState);
  digitalWrite(relayWaterPumpOut,waterPumpOutState);
}

void loop() {
  autoCheck();
  mainMenuDraw();
  drawCursor();
  operateMainMenu();
}

void mainMenuDraw() {
  Serial.print(menuPage);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(menuItems[menuPage]);
  lcd.setCursor(1, 1);
  lcd.print(menuItems[menuPage + 1]);
  if (menuPage == 0) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
  } else if (menuPage > 0 and menuPage < maxMenuPages) {
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  } else if (menuPage == maxMenuPages) {
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
  }
}

void drawCursor() {
  for (int x = 0; x < 2; x++) {
    lcd.setCursor(0, x);
    lcd.print(" ");
  }

  if (menuPage % 2 == 0) {
    if (cursorPosition % 2 == 0) {
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
  }
  if (menuPage % 2 != 0) {
    if (cursorPosition % 2 == 0) {
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }
    if (cursorPosition % 2 != 0) {
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    }
  }
}

void operateMainMenu() {
  int activeButton = 0;
  int showHomePage = 0;
  while (activeButton == 0) {
    autoCheck();
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 0:
        break;
      case 1: 
        button = 0;
        switch (cursorPosition) {
          case 0:
            setLedStateManually();            
            break;
          case 1:
            feedNowMenu();
            break;
          case 2:
            waterPumpInStateMenu();
            break;
          case 3:
            waterPumpOutStateMenu();
            break;
          case 4:
            setFilterPumpState();
            break;
          case 5:
            setBrightnessMenu();
            break;
          case 6:
            setFeederMenu();
            break;
          case 7:
            setAutoLedOn();
            break;
          case 8:
            setAutoLedOff();
            break;
        }
        activeButton = 1;
        mainMenuDraw();
        drawCursor();
        break;
      case 2:
        button = 0;
        if (menuPage == 0) {
          cursorPosition = cursorPosition - 1;
          cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        }
        if (menuPage % 2 == 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition - 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));

        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
      case 3:
        button = 0;
        if (menuPage % 2 == 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        cursorPosition = cursorPosition + 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
      case 4:
        showHomePage = 1;
        while( showHomePage == 1 ){
          autoCheck();
          int homePageButton;
          int pressedKey;
          pressedKey = analogRead(0);
          if (pressedKey < 790) {
            delay(100);
            pressedKey = analogRead(0);
          }
          homePageButton = evaluateButton(pressedKey);
          switch (homePageButton) {
            case 0 :
              showStatus(10000,5000,5000,5000);
              break;
            case 2:
              showHomePage = 0;
              break;
          }
        }
        break;
    }
  }
}

int evaluateButton(int x) {
  int result = 0;
  if (x < 50) {
    result = 1; // right
  } else if (x < 195) {
    result = 2; // up
  } else if (x < 380) {
    result = 3; // down
  } else if (x < 790) {
    result = 4; // left
  }
  return result;
}

void setBrightnessMenu() { 
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Brightness");
  lcd.setCursor(0, 1);
  lcd.print("Value : ");
  lcd.setCursor(8, 1);
  lcd.print(brightness);
  
  while (activeButton == 0) {
    int button;

    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    
    switch (button) {
      case 2:
        if( (brightness + 10) < 255)
        {
          brightness += 10;
        }else if( brightness >= 255 ){
          brightness = 255;
        }else{
          brightness = 255;
        }
        
        saveLcdBrightness(brightness);
        analogWrite(lcdBrightnessPin, brightness);
        lcd.setCursor(8, 1);
        lcd.print("    ");
        lcd.setCursor(8, 1);
        lcd.print(brightness);
        break;
      
      case 3:
        if( (brightness - 10) > 0)
        {
          brightness -= 10;
        }else if( brightness <= 0 ){
          brightness = 0;
        }else{
          brightness = 0;
        }
        
        saveLcdBrightness(brightness);
        analogWrite(lcdBrightnessPin, brightness);
        lcd.setCursor(8, 1);
        lcd.print("   ");
        lcd.setCursor(8, 1);
        lcd.print(brightness);
        break;
      case 4: 
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void setFilterPumpState() { 
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Filter State");
  lcd.setCursor(0, 1);
  lcd.print("State:");

  if( getFilterPumpState() == 0 ){
    lcd.setCursor(7, 1);
    lcd.print("On");
  }else if( getFilterPumpState() == 1 ){
    lcd.setCursor(7, 1);
    lcd.print("Off");
  }

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        if( getFilterPumpState() == 0 ){
          setFilterPumpOff();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( getFilterPumpState() == 1){
          setFilterPumpOn();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 3: 
        if( getFilterPumpState() == 0 ){
          setFilterPumpOff();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( getFilterPumpState() == 1){
          setFilterPumpOn();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 4: 
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void waterPumpInStateMenu() { 
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pump In State");
  lcd.setCursor(0, 1);
  lcd.print("State:");

  if( waterPumpInState == 0 ){
    lcd.setCursor(7, 1);
    lcd.print("On");
  }else if( waterPumpInState == 1 ){
    lcd.setCursor(7, 1);
    lcd.print("Off");
  }

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        if( waterPumpInState == 0 ){
          waterPumpInState = 1;
          digitalWrite(relayWaterPumpIn,waterPumpInState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( waterPumpInState == 1){
          waterPumpInState = 0;
          digitalWrite(relayWaterPumpIn,waterPumpInState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 3: 
        if( waterPumpInState == 0 ){
          waterPumpInState = 1;
          digitalWrite(relayWaterPumpIn,waterPumpInState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( waterPumpInState == 1){
          waterPumpInState = 0;
          digitalWrite(relayWaterPumpIn,waterPumpInState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 4: 
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void waterPumpOutStateMenu() { 
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pump Out State");
  lcd.setCursor(0, 1);
  lcd.print("State:");

  if( waterPumpOutState == 0 ){
    lcd.setCursor(7, 1);
    lcd.print("On");
  }else if( waterPumpOutState == 1 ){
    lcd.setCursor(7, 1);
    lcd.print("Off");
  }

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        if( waterPumpOutState == 0 ){
          waterPumpOutState = 1;
          digitalWrite(relayWaterPumpOut,waterPumpOutState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( waterPumpOutState == 1){
          waterPumpOutState = 0;
          digitalWrite(relayWaterPumpOut,waterPumpOutState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 3: 
        if( waterPumpOutState == 0 ){
          waterPumpOutState = 1;
          digitalWrite(relayWaterPumpOut,waterPumpOutState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( waterPumpOutState == 1){
          waterPumpOutState = 0;
          digitalWrite(relayWaterPumpOut,waterPumpOutState);
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 4: 
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void feedNowMenu() {
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Feed The Fish! ");
  lcd.setCursor(0, 1);
  lcd.print(" Press Right! ");

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 1:
        moveServo();
        break;
      case 4:
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void setLedStateManually() { 
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set LED State");
  lcd.setCursor(0, 1);
  lcd.print("State:");

  if( getRelayLedState() == 0 ){
    lcd.setCursor(7, 1);
    lcd.print("On");
  }else if( getRelayLedState() == 1 ){
    lcd.setCursor(7, 1);
    lcd.print("Off");
  }

  while (activeButton == 0) {
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 2:
        if( getRelayLedState() == 0 ){
          saveRelayLedState(1);
          setRelayLedOff();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( getRelayLedState() == 1){
          saveRelayLedState(0);
          setRelayLedOn();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 3: 
        if( getRelayLedState() == 0 ){
          saveRelayLedState(1);
          setRelayLedOff();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("Off");
        }else if( getRelayLedState() == 1){
          saveRelayLedState(0);
          setRelayLedOn();
          lcd.setCursor(7, 1);
          lcd.print("    ");
          lcd.setCursor(7, 1);
          lcd.print("On");
        }
        break;
      case 4: 
        button = 0;
        activeButton = 1;
        break;
    }
  }
}

void setFeederMenu() {
  int activeButton = 0;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Feed Interval");
  lcd.setCursor(1, 1);
  lcd.print("Hrs:");
  lcd.setCursor(5, 1);
  lcd.print(feedHrs);
  lcd.setCursor(8, 1);
  lcd.print("Min:");
  lcd.setCursor(12, 1);
  lcd.print(feedMin);

  while (activeButton == 0) {
    int button;
    int activeButtonFeed = 0;

    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 1:

        int buttonFeed;
        while (activeButtonFeed == 0) {
          readKey = analogRead(0);
          if (readKey < 790) {
            delay(100);
            readKey = analogRead(0);
          }
          buttonFeed = evaluateButton(readKey);
          switch (buttonFeed) {
            case 2:
              if( (feedMin + 1) < 59){
                feedMin += 1;
              }else if( feedMin >= 59 ){
                feedMin = 59;
              }else{
                feedMin = 59;
              }
              
              saveFeederMinutes(feedMin);
              calcNextFeedTime(feedHrs, feedMin);
              lcd.setCursor(12, 1);
              lcd.print("  ");
              lcd.setCursor(12, 1);
              lcd.print(feedMin);

              if(feedMin < 10){
                lcd.setCursor(12, 1);
                lcd.blink();
              }else{
                lcd.setCursor(13, 1);
                lcd.blink();
              }    
              break;

            case 3:
              if( (feedMin - 1) > 0){
                feedMin -= 1;
              }else if( feedMin <= 0 ){
                feedMin = 0;
              }else{
                feedMin = 0;
              }
              
              saveFeederMinutes(feedMin);
              calcNextFeedTime(feedHrs, feedMin);
              lcd.setCursor(12, 1);
              lcd.print("  ");
              lcd.setCursor(12, 1);
              lcd.print(feedMin);

              if(feedMin < 10){
                lcd.setCursor(12, 1);
                lcd.blink();
              }else{
                lcd.setCursor(13, 1);
                lcd.blink();
              }
              break;

            case 4:
              buttonFeed = 0;
              activeButtonFeed = 1;
              break;
          }

        }

        break;

      case 2:
        if( (feedHrs + 1) < 12){
          feedHrs += 1;
        }else if( feedHrs >= 12 ){
          feedHrs = 12;
        }else{
          feedHrs = 12;
        }
        
        saveFeederHours(feedHrs);
        calcNextFeedTime(feedHrs, feedMin);
        lcd.setCursor(5, 1);
        lcd.print("  ");
        lcd.setCursor(5, 1);
        lcd.print(feedHrs);

        if(feedHrs < 10){
          lcd.setCursor(5, 1);
          lcd.blink();
        }else{
          lcd.setCursor(6, 1);
          lcd.blink();
        }
        break;

      case 3:
        if( (feedHrs - 1) > 0){
          feedHrs -= 1;
        }else if( feedHrs <= 0 ){
          feedHrs = 0;
        }else{
          feedHrs = 0;
        }
        
        saveFeederHours(feedHrs);
        calcNextFeedTime(feedHrs, feedMin);
        lcd.setCursor(5, 1);
        lcd.print("  ");
        lcd.setCursor(5, 1);
        lcd.print(feedHrs);

        if(feedHrs < 10){
          lcd.setCursor(5, 1);
          lcd.blink();
        }else{
          lcd.setCursor(6, 1);
          lcd.blink();
        }

        break;
      case 4: 
        button = 0;
        activeButton = 1;
        lcd.noBlink();
        break;
    }
  }
}

void setAutoLedOn() {
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set LED On Time");
  lcd.setCursor(1, 1);
  lcd.print("Hrs:");
  lcd.setCursor(5, 1);
  lcd.print(autoLedOnHrs);
  lcd.setCursor(8, 1);
  lcd.print("Min:");
  lcd.setCursor(12, 1);
  lcd.print(autoLedOnMin);

  while (activeButton == 0) {
    int activeButtonAutoLedOn = 0;
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 1:
        int buttonAutoLed;
        while (activeButtonAutoLedOn == 0) {
          readKey = analogRead(0);
          if (readKey < 790) {
            delay(100);
            readKey = analogRead(0);
          }
          buttonAutoLed = evaluateButton(readKey);
          switch (buttonAutoLed) {
            case 2:
              if( (autoLedOnMin + 1) < 59){
                autoLedOnMin += 1;
              }else if( autoLedOnMin >= 59 ){
                autoLedOnMin = 59;
              }else{
                autoLedOnMin = 59;
              }
              
              saveAutoLedOnMinutes(autoLedOnMin);
              lcd.setCursor(12, 1);
              lcd.print("  ");
              lcd.setCursor(12, 1);
              lcd.print(autoLedOnMin);

              if(autoLedOnMin < 10){
                lcd.setCursor(12, 1);
                lcd.blink();
              }else{
                lcd.setCursor(13, 1);
                lcd.blink();
              }    
              break;

            case 3:
              if( (autoLedOnMin - 1) > 0){
                autoLedOnMin -= 1;
              }else if( autoLedOnMin <= 0 ){
                autoLedOnMin = 0;
              }else{
                autoLedOnMin = 0;
              }
              
              saveAutoLedOnMinutes(autoLedOnMin);
              lcd.setCursor(12, 1);
              lcd.print("  ");
              lcd.setCursor(12, 1);
              lcd.print(autoLedOnMin);

              if(autoLedOnMin < 10){
                lcd.setCursor(12, 1);
                lcd.blink();
              }else{
                lcd.setCursor(13, 1);
                lcd.blink();
              }
              break;

            case 4:
              buttonAutoLed = 0;
              activeButtonAutoLedOn = 1;
              break;
          }

        }

        break;
      case 2:
        if( (autoLedOnHrs + 1) < 24){
          autoLedOnHrs += 1;
        }else if( autoLedOnHrs >= 23 ){
          autoLedOnHrs = 23;
        }else{
          autoLedOnHrs = 23;
        }
        
        saveAutoLedOnHours(autoLedOnHrs);
        lcd.setCursor(5, 1);
        lcd.print("  ");
        lcd.setCursor(5, 1);
        lcd.print(autoLedOnHrs);

        if(autoLedOnHrs < 10){
          lcd.setCursor(5, 1);
          lcd.blink();
        }else{
          lcd.setCursor(6, 1);
          lcd.blink();
        }
        break;
      case 3:
        if( (autoLedOnHrs - 1) >= 0){
          autoLedOnHrs -= 1;
        }else if( autoLedOnHrs < 0 ){
          autoLedOnHrs = 0;
        }else{
          autoLedOnHrs = 0;
        }
        
        saveAutoLedOnHours(autoLedOnHrs);
        lcd.setCursor(5, 1);
        lcd.print("  ");
        lcd.setCursor(5, 1);
        lcd.print(autoLedOnHrs);

        if(autoLedOnHrs < 10){
          lcd.setCursor(5, 1);
          lcd.blink();
        }else{
          lcd.setCursor(6, 1);
          lcd.blink();
        }

        break;
      case 4: 
        button = 0;
        activeButton = 1;
        lcd.noBlink();
        break;
    }
  }
}

void setAutoLedOff() {
  int activeButton = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set LED Off Time");
  lcd.setCursor(1, 1);
  lcd.print("Hrs:");
  lcd.setCursor(5, 1);
  lcd.print(autoLedOffHrs);
  lcd.setCursor(8, 1);
  lcd.print("Min:");
  lcd.setCursor(12, 1);
  lcd.print(autoLedOffMin);

  while (activeButton == 0) {
    int activeButtonAutoLedOff = 0;
    int button;
    readKey = analogRead(0);
    if (readKey < 790) {
      delay(100);
      readKey = analogRead(0);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 1:
        int buttonAutoLed;
        while (activeButtonAutoLedOff == 0) {
          readKey = analogRead(0);
          if (readKey < 790) {
            delay(100);
            readKey = analogRead(0);
          }
          buttonAutoLed = evaluateButton(readKey);
          switch (buttonAutoLed) {
            case 2:
              if( (autoLedOffMin + 1) < 59){
                autoLedOffMin += 1;
              }else if( autoLedOffMin >= 59 ){
                autoLedOffMin = 59;
              }else{
                autoLedOffMin = 59;
              }
              
              saveAutoLedOffMinutes(autoLedOffMin);
              saveAutoLedOffHours(autoLedOffHrs);
              lcd.setCursor(12, 1);
              lcd.print("  ");
              lcd.setCursor(12, 1);
              lcd.print(autoLedOffMin);

              if(autoLedOffMin < 10){
                lcd.setCursor(12, 1);
                lcd.blink();
              }else{
                lcd.setCursor(13, 1);
                lcd.blink();
              }    
              break;

            case 3:
              if( (autoLedOffMin - 1) > 0){
                autoLedOffMin -= 1;
              }else if( autoLedOffMin <= 0 ){
                autoLedOffMin = 0;
              }else{
                autoLedOffMin = 0;
              }
              
              saveAutoLedOffMinutes(autoLedOffMin);
              saveAutoLedOffHours(autoLedOffHrs);
              lcd.setCursor(12, 1);
              lcd.print("  ");
              lcd.setCursor(12, 1);
              lcd.print(autoLedOffMin);

              if(autoLedOffMin < 10){
                lcd.setCursor(12, 1);
                lcd.blink();
              }else{
                lcd.setCursor(13, 1);
                lcd.blink();
              }
              break;

            case 4:
              buttonAutoLed = 0;
              activeButtonAutoLedOff = 1;
              break;
          }

        }

        break;
      case 2:
        if( (autoLedOffHrs + 1) < 24){
          autoLedOffHrs += 1;
        }else if( autoLedOffHrs >= 23 ){
          autoLedOffHrs = 23;
        }else{
          autoLedOffHrs = 23;
        }
        
        saveAutoLedOffMinutes(autoLedOffMin);
        saveAutoLedOffHours(autoLedOffHrs);
        lcd.setCursor(5, 1);
        lcd.print("  ");
        lcd.setCursor(5, 1);
        lcd.print(autoLedOffHrs);

        if(autoLedOffHrs < 10){
          lcd.setCursor(5, 1);
          lcd.blink();
        }else{
          lcd.setCursor(6, 1);
          lcd.blink();
        }
        break;
      case 3:
        if( (autoLedOffHrs - 1) >= 0){
          autoLedOffHrs -= 1;
        }else if( autoLedOffHrs < 0 ){
          autoLedOffHrs = 0;
        }else{
          autoLedOffHrs = 0;
        }
        
        saveAutoLedOffMinutes(autoLedOffMin);
        saveAutoLedOffHours(autoLedOffHrs);
        lcd.setCursor(5, 1);
        lcd.print("  ");
        lcd.setCursor(5, 1);
        lcd.print(autoLedOffHrs);

        if(autoLedOffHrs < 10){
          lcd.setCursor(5, 1);
          lcd.blink();
        }else{
          lcd.setCursor(6, 1);
          lcd.blink();
        }

        break;
      case 4: 
        button = 0;
        activeButton = 1;
        lcd.noBlink();
        break;
    }
  }
}

void saveLcdBrightness(int val){
  EEPROM.write(lcdBrightnessAddress, val);
}

int getLcdBrightness(){
  return EEPROM.read(lcdBrightnessAddress);
}

void moveServo(){
  servo.attach(servo_pin);
  for (int i = 0; i < max_spin; i++){
    servo.write(700);
    delay(300);
    servo.write(2400);
    delay(300);
  }
  servo.detach();
}

void showTime(){
  DateTime now = rtc.now();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Aquaduino   ");

  lcd.setCursor(0, 1);
  
  if( now.day() < 10 ){
    lcd.print("0");
    lcd.print( now.day() );
    lcd.print('/');
  }else{
    lcd.print( now.day() );
    lcd.print('/');
  }

  if( now.month() < 10 ){
    lcd.print("0");
    lcd.print( now.month() );
    lcd.print('/');
  }else{
    lcd.print( now.month() );
    lcd.print('/');
  }

  lcd.print( now.year() );
  lcd.print(' ');
  
  if( now.hour() < 10 ){
    lcd.print("0");
    lcd.print( now.hour() );
    lcd.print(':');
  }else{
    lcd.print( now.hour() );
    lcd.print(':');
  }
  if( now.minute() < 10 ){
    lcd.print("0");
    lcd.print( now.minute() );
  }else{
    lcd.print( now.minute() );
  }

}

void showNextFeeder(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Next Feeder Time");
  lcd.setCursor(0, 1);
  lcd.print("Hrs:");
  lcd.print(getNextFeederHours());
  lcd.print(" ");
  lcd.print("Min:");
  lcd.print(getNextFeederMinutes());
}

void showTimeLedOn(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Led On Time");
  lcd.setCursor(0, 1);
  lcd.print("Hrs:");
  lcd.print(getAutoLedOnHours());
  lcd.print(" ");
  lcd.print("Min:");
  lcd.print(getAutoLedOnMinutes());
}

void showTimeLedOff(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Led Off Time");
  lcd.setCursor(0, 1);
  lcd.print("Hrs:");
  lcd.print(getAutoLedOffHours());
  lcd.print(" ");
  lcd.print("Min:");
  lcd.print(getAutoLedOffMinutes());
}

void showStatus(int timeDelay,int nextFeedDelay, int timeLedOnDelay, int timeLedOffDelay){
  autoCheck();
  showTime();
  delay(timeDelay);
  autoCheck();
  showNextFeeder();
  delay(nextFeedDelay);
  autoCheck();
  showTimeLedOn();
  delay(timeLedOnDelay);
  autoCheck();
  showTimeLedOff();
  delay(timeLedOffDelay);
  autoCheck();
}

void setRelayLedOn(){
  digitalWrite(relayLed,LOW);
}

void setRelayLedOff(){
  digitalWrite(relayLed,HIGH);
}

void saveRelayLedState(int val){
  EEPROM.write(relayLedAddress, val);
}

int getRelayLedState(){
  return EEPROM.read(relayLedAddress);
}

void saveAutoLedOnHours(int val){
  EEPROM.write(AutoLedOnHoursAddress, val);
}

int getAutoLedOnHours(){
  return EEPROM.read(AutoLedOnHoursAddress);
}

void saveAutoLedOnMinutes(int val){
  EEPROM.write(AutoLedOnMinutesAddress, val);
}

int getAutoLedOnMinutes(){
  return EEPROM.read(AutoLedOnMinutesAddress);
}

void saveAutoLedOffHours(int val){
  EEPROM.write(AutoLedOffHoursAddress, val);
}

int getAutoLedOffHours(){
  return EEPROM.read(AutoLedOffHoursAddress);
}

void saveAutoLedOffMinutes(int val){
  EEPROM.write(AutoLedOffMinutesAddress, val);
}

int getAutoLedOffMinutes(){
  return EEPROM.read(AutoLedOffMinutesAddress);
}

void setFilterPumpOn(){
  digitalWrite(relayFilterPump,LOW);
  saveFilterPumpState(0);
}

void setFilterPumpOff(){
  digitalWrite(relayFilterPump,HIGH);
  saveFilterPumpState(1);
}

void saveFilterPumpState(int val){
  EEPROM.write(filterPumpAddress, val);
}

int getFilterPumpState(){
  return EEPROM.read(filterPumpAddress);
}

void setWaterPumpInOn(){
  digitalWrite(relayWaterPumpIn,LOW);
}

void setWaterPumpInOff(){
  digitalWrite(relayWaterPumpIn,HIGH);
}

void setWaterPumpOutOn(){
  digitalWrite(relayWaterPumpOut,LOW);
}

void setWaterPumpOutOff(){
  digitalWrite(relayWaterPumpOut,HIGH);
} 

void saveFeederHours(int val){
  EEPROM.write(feederHoursAddress, val);
}

int getFeederHours(){
  return EEPROM.read(feederHoursAddress);
}

void saveFeederMinutes(int val){
  EEPROM.write(feederMinutesAddress, val);
}

int getFeederMinutes(){
  return EEPROM.read(feederMinutesAddress);
}

void saveNextFeederHours(int val){
  EEPROM.write(nextFeedHoursAddress, val);
}

int getNextFeederHours(){
  return EEPROM.read(nextFeedHoursAddress);
}

void saveNextFeederMinutes(int val){
  EEPROM.write(nextFeedMinutesAddress, val);
}

int getNextFeederMinutes(){
  return EEPROM.read(nextFeedMinutesAddress);
}

void calcNextFeedTime(int hours, int minutes){
  DateTime now = rtc.now();
  hours = now.hour() + hours;
  minutes = now.minute() + minutes;
  
  if( minutes > 59 ){
    int tempMinutes = minutes % 60;
    int hoursFromMinutes = (minutes - tempMinutes)/60; 
    hours += hoursFromMinutes;
    minutes = tempMinutes;
  }
  if( hours > 12 ){
    hours = hours%12;
  }
  saveNextFeederMinutes(minutes);
  saveNextFeederHours(hours);
}

void autoFeeder(){
  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();
  int nextHour = getNextFeederHours();
  int nextMinute = getNextFeederMinutes();

  if( nextHour > 0 ){
    if( hour > 12){
      hour = hour-12;
    }
    if( hour == nextHour ){
      if( minute == nextMinute ){
        calcNextFeedTime( getFeederHours(), getFeederMinutes() );
        moveServo();
      }
    }
  }else{
    if( minute == nextMinute ){
      calcNextFeedTime( getFeederHours(), getFeederMinutes() );
      moveServo();
    }
  }
  
}

void autoLed(){
  DateTime now = rtc.now();
  if( now.hour() == getAutoLedOnHours() && now.minute() == getAutoLedOnMinutes() ){
    saveRelayLedState(0);
    setRelayLedOn();
  }else if( now.hour() == getAutoLedOffHours() && now.minute() == getAutoLedOffMinutes() ){
    saveRelayLedState(1);
    setRelayLedOff();
  }
}

void autoCheck(){
  autoFeeder();
  autoLed();
}