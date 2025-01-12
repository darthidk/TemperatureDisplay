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
float peakTemp = 0;
float lowTemp = 99999;
float peakHourTemp = 0;
float peakTempPrev[6] = {0,0,0,0,0,0};
float lowHourTemp = 99999;
float lowTempPrev[6] = {99999,99999,99999,99999,99999,99999};
int peakTime = 0;
int lowTime = 0;
int interval = 60*60;

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

void setup() {
  Serial.begin(9600);
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
  // If the 10 min peak is changed, also check the hourly peak and then the overall peak as well
  if (temp > peakTempPrev[5]) {
    peakTempPrev[5] = temp;
    if (temp > peakHourTemp) {
      peakHourTemp = temp;
      peakTime = 0;
      if (temp > peakTemp) {
        peakTemp = temp;
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



  // Reset the timers when the timers is over an hour old
  // Also update the list at the same time
  if (peakTime/(interval) == 1) {
    Serial.print("Peak temperature over an hour old. Old temperature: ");
    Serial.print(peakHourTemp);
    peakHourTemp = 0;
    for(int i = 1; i < 6; i++) {
      if (peakHourTemp < peakTempPrev[i]) {
        peakHourTemp = peakTempPrev[i];
        peakTime = 0;
      }
    }
    for(int i = 0; i < 5; i++) {
      peakTempPrev[i] = peakTempPrev[i+1];
    }
    peakTempPrev[5] = 0;
    Serial.print(" New temperature: ");
    Serial.println(peakHourTemp);
    Serial.print(" Time: ");
    Serial.print(time);
    Serial.print(" Peak time: ");
    Serial.println(peakTime);
  } 

  if (lowTime/(interval) == 1) {
    Serial.print("Low temperature over an hour old. Old temperature: ");
    Serial.print(lowHourTemp);
    lowHourTemp = 9999;
    for(int i = 1; i < 6; i++) {
      if (lowHourTemp > lowTempPrev[i]) {
        lowHourTemp = lowTempPrev[i];
        lowTime = 0;
      }
    }
    Serial.print("Updating list now: ");
    for(int i = 0; i < 5; i++) {
      lowTempPrev[i] = lowTempPrev[i+1];
      Serial.print(lowTempPrev[i]);
      Serial.print(" ");
    } 
    lowTempPrev[5] = 99999;
    Serial.print(" New temperature: ");
    Serial.print(lowHourTemp);
    Serial.print(" Time: ");
    Serial.print(time);
    Serial.print(" Low time: ");
    Serial.println(lowTime);
  }



  
  
  // Every 10 minutes, update list containing the peak temperatures of last 10 minutes
  if (peakTime%(interval/6) == 0 && peakTime != 0) {
    if (peakTempPrev[0] == peakHourTemp) {
      Serial.print("10 minute check, getting new peakHourTemp, old temp: ");
      Serial.print(peakHourTemp);
      peakHourTemp = 0;
      for(int i = 1; i < 6; i++) {
        if (peakHourTemp < peakTempPrev[i]) {
          peakHourTemp = peakTempPrev[i];
          peakTime = 0;
        }
      }
      Serial.print(" New temperature: ");
      Serial.println(peakHourTemp);
    }
    Serial.print("Updating list now: ");
    for(int i = 0; i < 5; i++) {
      peakTempPrev[i] = peakTempPrev[i+1];
      Serial.print(peakTempPrev[i]);
      Serial.print(" ");
    }
    Serial.print("");
    Serial.print(" Time: ");
    Serial.print(time);
    Serial.print(" Peak time: ");
    Serial.println(peakTime);
    peakTempPrev[5] = 0; 
  }

  if (lowTime%(interval/6) == 0 && lowTime != 0) {
    if (lowTempPrev[0] == lowHourTemp) {
      Serial.print("10 minute check, getting new lowHourTemp, old temp: ");
      Serial.print(lowHourTemp);
      lowHourTemp = 99999;
      for(int i = 1; i < 6; i++) {
        if (lowHourTemp > lowTempPrev[i]) {
          lowHourTemp = lowTempPrev[i];
          lowTime = 0;
        }
      }
      Serial.print(" New temperature: ");
      Serial.println(lowHourTemp);
    }
    Serial.print("Updating list now: ");
    for(int i = 0; i < 5; i++) {
      lowTempPrev[i] = lowTempPrev[i+1];
      Serial.print(lowTempPrev[i]);
      Serial.print(" ");
    } 
    Serial.print(" Time: ");
    Serial.print(time);
    Serial.print(" Low time: ");
    Serial.println(lowTime);
    lowTempPrev[5] = 99999;
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
  if (time%60/5%4 == 0) {
    lcd.setCursor(9,0);
    lcd.print("H:");
    lcd.print(peakTemp);
  } else if (time%60/5%4 == 1) {
    lcd.setCursor(9,0);
    lcd.print("L:");
    lcd.print(lowTemp);
  } else if (time%60/5%4 == 2) {
    lcd.setCursor(8,0);
    lcd.print("H1:");
    lcd.print(peakHourTemp);
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
  peakTime++;
  lowTime++;
  delay(1000);
}
