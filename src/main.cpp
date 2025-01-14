#include <Arduino.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(12,11,5,4,3,2);

const int greenLEDPin = 10;
const int redLEDPin = 8;
const int blueLEDPin = 9;
const int tempPin = A0;
const int switchPin = 13;

int switchState = 0;
int recent_switch = 0;
int light = 1;

unsigned long int time = 0;
float highTemp = 0;
float lowTemp = 99999;
float highHourTemp = 0;
float highTempPrev[6] = {0,0,0,0,0,0};
float lowHourTemp = 99999;
float lowTempPrev[6] = {99999,99999,99999,99999,99999,99999};
int highTime = 0;
int lowTime = 0;
unsigned int interval = 60*60;

byte deg[8] = {
  0b01110,
  0b01010,
  0b01110,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

int tempLEDcalc(int start, int end, int cur, int ledmax) {
  int val = ledmax * (cur - start)/(end - start);
  if (cur <= 0) {
    return 255;
  }
  return (val < 0 || val > ledmax) ? 0 : val;
}

void tempListUpdates(int* time, float* hourTemp, float* tempList) {
  if (tempList[0] == *hourTemp) {
    for(int i = 1; i < 6; i++) {
      if (*hourTemp < tempList[i]) {
        *hourTemp = tempList[i];
        *time = 0;
      }
    }
  }
  for(int i = 0; i < 5; i++) {
    tempList[i] = tempList[i+1];
  }
  tempList[5] = 0; 
}

void setup() {
  lcd.begin(16,2);

  pinMode(greenLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(blueLEDPin, OUTPUT);
  pinMode(switchPin, INPUT);

  analogWrite(greenLEDPin, 0);
  analogWrite(redLEDPin, 0);
  analogWrite(blueLEDPin, 0);

  lcd.createChar(0, deg);
}

void loop() {
  switchState = digitalRead(switchPin);
  int sensorVal = analogRead(tempPin);
  float temp = ((sensorVal/1024.0) * 5.0 - .5) * 100;

  // Updating the peak temperature over a 10 minute interval
  // If the 10 min high is changed, also check the hourly peak and then the overall peak as well
  if (temp > highTempPrev[5]) {
    highTempPrev[5] = temp;
    if (temp > highHourTemp) {
      highHourTemp = temp;
      highTime = 0;
      if (temp > highTemp) {
        highTemp = temp;
      } 
    }
  } else if (temp < lowTempPrev[5]) {
    lowTempPrev[5] = temp;
    if (temp < lowHourTemp) {
      lowHourTemp = temp;
      lowTime = 0;
      if (temp < lowTemp) {
        lowTemp = temp;
      }
    }
  }  
  
  // Every 10 minutes, update list containing the peak temperatures of last 10 minutes
  if (highTime%(interval/6) == 0 && highTime != 0) {
    tempListUpdates(&highTime, &highHourTemp, highTempPrev);
  }

  if (lowTime%(interval/6) == 0 && lowTime != 0) {
    tempListUpdates(&lowTime, &lowHourTemp, lowTempPrev);
  }

  // Button to enable LED being on and off
  if (switchState == HIGH) {
    if (light == 0) {
      light = 1;
    } else {
      light = 0;
    }
  }

  if (light == 1) {
    analogWrite(blueLEDPin, tempLEDcalc(0, 23, temp, 255));
    analogWrite(greenLEDPin, tempLEDcalc(18, 31, temp, 190));
    analogWrite(redLEDPin, tempLEDcalc(25, 33, temp, 255));
    if (temp >= 33) {
      analogWrite(redLEDPin, 255);
    }
  }

  // Writing to LCD Screen
  // Current temp
  lcd.clear();
  lcd.leftToRight();
  lcd.setCursor(0,0);
  lcd.print(temp);
  lcd.write(byte(0));
  lcd.print("C");

  // Rotating every 5 seconds between overall peak and low and hourly peak and low
  if (time%interval/5%4 == 0) {
    lcd.setCursor(9,0);
    lcd.print("H:");
    lcd.print(highTemp);
  } else if (time%interval/5%4 == 1) {
    lcd.setCursor(9,0);
    lcd.print("L:");
    lcd.print(lowTemp);
  } else if (time%interval/5%4 == 2) {
    lcd.setCursor(8,0);
    lcd.print("H1:");
    lcd.print(highHourTemp);
  } else {
    lcd.setCursor(8,0);
    lcd.print("L1:");
    lcd.print(lowHourTemp);
  }

  // Printing time since turned on
  lcd.setCursor(0,1);
  lcd.print("Time:   00:00:00");
  
  if (time%60 < 10) {
    lcd.setCursor(15, 1);
    lcd.print(time%60);
  } else {
    lcd.setCursor(14, 1);
    lcd.print(time%60);
  }

  if (time/60%60 < 10) {
    lcd.setCursor(12, 1);
    lcd.print(time/60%60);
  } else {
    lcd.setCursor(11, 1);
    lcd.print(time/60%60);
  }

  if (time/60/60 < 10) {
    lcd.setCursor(9, 1);
    lcd.print(time/60/60);
  } else {
    lcd.setCursor(8, 1);
    lcd.print(time/60/60);
  }

  // Incrementing time
  time++;
  highTime++;
  lowTime++;
  delay(1000);
}
