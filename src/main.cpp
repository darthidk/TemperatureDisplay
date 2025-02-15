/*
Plan:
Upload file from serial connection as well from GUI
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <ctype.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(5,6,7,8,12,13);

int greenLEDPin = 11;
int redLEDPin = 9;
int blueLEDPin = 10;
int tempPin = A4;
int switchLEDPin = 2;
int switchSettingsPin = 3;

int switchLEDState = 0;
int switchSettingsState = 0;

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

int light_on = 1;
int led_bounds[6];
int led_lock = 0;

bool startup_show_settings = true;

// Degree symbol
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

String serialmap[] = {
    "Temperature Sensor",
    "Settings Display Button",
    "LED On/Off Button",
    "LED Red",
    "LED Green",
    "LED Blue"
};

// Detemines the brightness of the colour for the LED based on the temperature and min/max temps and brightness
int tempLEDcalc(int start, int end, int cur, int ledmax) {
	int val = ledmax * (cur - start)/(end - start);
	if ((end - cur) < (end - start)/6) {
		val = min(ledmax, val * (3 + (end - cur)) / 6);
	}
	if (cur <= 0) {
		return 255;
	}
	return (val < 0 || val > ledmax) ? 0 : val;
}

// Updates the array containing the the 10 minute peak temperatures, moving each element up by 1
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

void splitter(String in_str, char splitter, int arr[], int total_splits) {
	int num_found = 0;
	for(int i = 0; i < in_str.length(); i++) {
		if (in_str.charAt(i) == splitter) {
			arr[num_found++] = i;
		}
	}
}

// * Use for debugging serial and checking what the arduino is receiving
void debuglcd(char in) {
	if (isalpha(in)) {
		lcd.clear();
		lcd.setCursor(0,0);
	}
	lcd.print(in);
	delay(1000);
}


int serialReadUpdate(int* ledlist, int* led_power, int* led_lock) {
	if (!Serial || Serial.available() == 0) {
		return 0;
	} else {
		char serial_classification = Serial.read();
		//debuglcd(serial_classification);
		if (serial_classification == 'S') {
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("Detected changes");
			lcd.setCursor(0,1);
			lcd.print("to settings");
			delay(1000);
			lcd.clear();
			String in_str;
			int eeprom_index = 0;
			// Reading in settings updates from serial using the format below
			// 	L|0|0|
			// 	M|0|0|
			// 	H|0|0|
			// 	P|1|0|1|
			//  ;
			while (Serial.available() != 0) {
				if ((isalpha(Serial.peek()) && ((in_str.length() != 0))) || Serial.peek() == ';') {
					int i = 2;
					String val;
					while(in_str[i] != '|') {
						val += in_str[i];
						i++;
					}
					EEPROM.update(eeprom_index++, val.toInt());
					i += 1;
					val.remove(0, val.length());
					while(in_str[i] != '|') {
						val += in_str[i];
						i++;
					}
					EEPROM.update(eeprom_index++, val.toInt());
					
					if (Serial.peek() == ';') {
						if (in_str.charAt(0) == 'P') {
							startup_show_settings = in_str[in_str.length() - 2] -  '0';
							EEPROM.update(14, startup_show_settings);
							in_str.remove(0, in_str.length());
							in_str.concat((char)Serial.read());
						} else {
							while (Serial.available() != 0) {
								Serial.read();
							}
							break;
						}
					} else {
						in_str.remove(0, in_str.length());
						in_str.concat((char)Serial.read());
					}
				} else {
					in_str.concat((char)Serial.read());
				}
			}
		} else if (serial_classification == 'P') {
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.print("Detected changes");
			lcd.setCursor(0,1);
			lcd.print("to pins");
			delay(1000);
			lcd.clear();
			while (Serial.available() != 0) {
				// Input String should be 'A5|4|', 'pin|int coded for use|'
				String in_str;

				int splits_found = 0;
				while (splits_found != 2) {
					if (Serial.peek() == '|') {
						splits_found++;
					}
					in_str += (char)Serial.read();
				}
				
				int num_splits = 2;
				int split_locations[num_splits];
				splitter(in_str, '|', split_locations, num_splits);

				String sub_str = in_str.substring(0, split_locations[0]);
				int pin;
				if (sub_str[0] == 'A') {
					switch (static_cast<int> (sub_str[1]) - '0') {
						case 0:
							pin = (uint8_t)A0;
							break;
						case 1:
							pin = (uint8_t)A1;
							break;
						case 2:
							pin = (uint8_t)A2;
							break;
						case 3:
							pin = (uint8_t)A3;
							break;
						case 4:
							pin = (uint8_t)A4;
							break;
						case 5:
							pin = (uint8_t)A5;
							break;
						default:
							lcd.print("N");
							pin = (uint8_t)A5;
							break;
					}
				} else if (sub_str[0] == 'D') {
					if (sub_str[sub_str.length() - 1] == '~') {
						pin = sub_str.substring(1, sub_str.length() - 1).toInt();
					} else {
						pin = sub_str.substring(1, sub_str.length()).toInt();
					}
				}

				sub_str = in_str.substring(split_locations[0] + 1, split_locations[1]);
				if (sub_str == "0") {
					tempPin = pin;
					EEPROM.update(8, tempPin);
				} else if (sub_str == "1") {
					switchSettingsPin = pin;
					EEPROM.update(9, switchSettingsPin);
				} else if (sub_str == "2") {
					switchLEDPin = pin;
					EEPROM.update(10, switchLEDPin);
				} else if (sub_str == "3") {
					redLEDPin = pin;
					EEPROM.update(11, redLEDPin);
				} else if (sub_str == "4") {
					greenLEDPin = pin;
					EEPROM.update(12, greenLEDPin);
				}  else if (sub_str == "5") {
					blueLEDPin = pin;
					EEPROM.update(13, blueLEDPin);
				}
			}
		} else {
			lcd.clear();
			lcd.print("Clearing serial");
			while(Serial.available() != 0) {
				Serial.read();

			}
		}
		lcd.clear();
		lcd.print("Serial read");
		delay(1000);
		return 1;
	}
}

/* EEPROM Number Values:
	0-5: values for low and high bounds of led light settings
	Resulting default array: [0, 23, 18, 31, 25, 33]
	6: led power on (def: 1)
	7: lock led setting/disable button (def: 0)
	8-10: Temperature/Settings Switch/LED Switch Pins
	11-13: RGB Pins, in order
	14: Show Settings on startup
	*/
void readSettings() {
	for(int i = 0; i < 6; i++) {
		led_bounds[i] = EEPROM.read(i);
		i++;
		led_bounds[i] = EEPROM.read(i);
	}
	light_on = EEPROM.read(6);
	led_lock = EEPROM.read(7);
	tempPin = EEPROM.read(8);
	switchSettingsPin = EEPROM.read(9);
	switchLEDPin = EEPROM.read(10);
	redLEDPin = EEPROM.read(11);
	greenLEDPin = EEPROM.read(12);
	blueLEDPin = EEPROM.read(13);
	startup_show_settings = EEPROM.read(14);
}

// Displays the current settings on the led screen
void displaySettings(int delay_time) {
	lcd.clear();
	lcd.setCursor(0,0);
	
	readSettings();

	lcd.write("Displaying Settings");
	delay(delay_time);

	for(int i = 0; i < 6; i++) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.write("LED Bound ");
		lcd.print(i);
		lcd.write(": ");
		lcd.print(led_bounds[i++]);

		lcd.setCursor(0,1);
		lcd.write("LED Bound ");
		lcd.print(i);
		lcd.write(": ");
		lcd.print(led_bounds[i]);

		delay(delay_time);
	}
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.write("LED On: ");
	lcd.print(light_on);

	lcd.setCursor(0,1);
	lcd.write("LED Lock On: ");
	lcd.print(led_lock);
	delay(delay_time);

	lcd.clear();
	lcd.setCursor(0,0);
	lcd.write("Temperature: ");
	lcd.print(tempPin);
	lcd.setCursor(0,1);
	lcd.write("Settings SW: ");
	lcd.print(switchSettingsPin);

	delay(delay_time);

	lcd.clear();
	lcd.setCursor(0,0);
	lcd.write("LED SW: ");
	lcd.print(switchLEDPin);
	lcd.setCursor(0,1);
	lcd.write("Red LED: ");
	lcd.print(redLEDPin);

	delay(delay_time);

	lcd.clear();
	lcd.setCursor(0,0);
	lcd.write("Green LED: ");
	lcd.print(greenLEDPin);
	lcd.setCursor(0,1);
	lcd.write("Blue LED: ");
	lcd.print(blueLEDPin);

	delay(delay_time);

	lcd.clear();
	lcd.setCursor(0,0);
	lcd.write("Show Settings: ");
	lcd.print(startup_show_settings);
}

// Initial code execution 
void setup() {
	lcd.begin(16,2);
	Serial.begin(9600);
	
	lcd.createChar(0, deg);

	readSettings();

	pinMode(greenLEDPin, OUTPUT);
	pinMode(redLEDPin, OUTPUT);
	pinMode(blueLEDPin, OUTPUT);
	pinMode(switchLEDPin, INPUT);
	pinMode(switchSettingsPin, INPUT);

	analogWrite(greenLEDPin, 150 * light_on);
	analogWrite(redLEDPin, 150 * light_on);
	analogWrite(blueLEDPin, 150 * light_on);

	if (startup_show_settings == 1) {
		displaySettings(500);
	}

	// * Code which can be used for testing each colour of the LED individually
	// for(int k = 0; k < 3; k++) {
	// analogWrite(greenLEDPin, 0);
	// analogWrite(redLEDPin, 0);
	// analogWrite(blueLEDPin, 0);
	// lcd.clear();
	// lcd.setCursor(0,0);
	// if (k == 0) {
	// 	lcd.print("green");
	// } else if (k == 1) {
	// 	lcd.print("red");
	// } else if (k == 2)  {
	// 	lcd.print("blue");
	// }
	// 	for(int i = 0; i < 100; i++) {
	// 		lcd.setCursor(0,1);
	// 		if (k == 0) {
	// 			analogWrite(greenLEDPin, i);
	// 			lcd.print(i);
	// 		} else if (k == 1) {
	// 			analogWrite(redLEDPin, i);
	// 			lcd.print(i);
	// 		} else if (k == 2)  {
	// 			analogWrite(blueLEDPin, i);
	// 			lcd.print(i);
	// 		}
	// 		delay(50);
	// 	}
	// }
}

// Repeated code
void loop() {
	switchLEDState = digitalRead(switchLEDPin);
	switchSettingsState = digitalRead(switchSettingsPin);
	switchLEDState = digitalRead(switchLEDPin);
	switchSettingsState = digitalRead(switchSettingsPin);
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

	// Writing to LCD Screen
	// Current temp
	lcd.clear();
	lcd.leftToRight();
	lcd.setCursor(0,0);
	lcd.print(temp);
	lcd.write(byte(0));
	lcd.print("C");

	// Rotating every 5 seconds  between overall peak and low and hourly peak and low
	// Rotating every 5 seconds  between overall peak and low and hourly peak and low
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


	// Reading in new settings from serial and updating if found
	if (serialReadUpdate(led_bounds, &light_on, &led_lock) == 1) {
		lcd.setCursor(0,0);
		lcd.clear();
		lcd.print("Settings updated");
	}

	// Button to enable LED being on and off
	if (switchLEDState == HIGH && led_lock == 0) {
		lcd.clear();
		lcd.setCursor(0,0);
		if (light_on == 0) {
			light_on = 1;
			lcd.print("Light On");
		} else {
			light_on = 0;
			lcd.print("Light Off");
		}
	}

	//Button to display settings
	if (switchSettingsState == HIGH) {
		displaySettings(1000);
	}

	// Setting LED colour and brightness
	analogWrite(blueLEDPin, 0);
	analogWrite(greenLEDPin, 0);
	analogWrite(redLEDPin, 0);
	if (light_on == 1) {
		analogWrite(blueLEDPin, tempLEDcalc(led_bounds[0], led_bounds[1], temp, 100));
		analogWrite(greenLEDPin, tempLEDcalc(led_bounds[2], led_bounds[3], temp, 100));
		analogWrite(redLEDPin, tempLEDcalc(led_bounds[4], led_bounds[5], temp, 100));
		if (temp >= led_bounds[5]) {
			analogWrite(redLEDPin, 255);
		}
	}
	

	// Incrementing time and other related values
	time++;
	highTime++;
	lowTime++;


	delay(1000);
}
