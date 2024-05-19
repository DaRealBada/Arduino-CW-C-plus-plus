#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <TimeLib.h>
#include <MemoryFree.h>
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
int i = 0;

int index = 0;
const int vehicle_len = 5;
int scrolling_index = 0;


enum state {
  Synchronisation,
  Main
};
struct vehicle {
  String regNumber;
  bool Payment_Status;
  char Type;
  String Parking_Location;
  String Entry_time;
  String Exit_time;
};

byte ArrowDown[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100
};
byte ArrowUp[8] = {
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100
};

vehicle vehicles[vehicle_len];

int get_vehicle_position(String regNumber) {

  for (int i = 0; i < vehicle_len; i++) {
    if (vehicles[i].regNumber == regNumber) {

      return i;
    }
  }
  return -1; //returns -1 if the vehicle doesn't currently exist in the system
}

int get_new_position() {
  for (int i = 0; i < vehicle_len; i++) {
    if (vehicles[i].regNumber == "") {
      return i;
    }
  }
  return -1;//returns -1 if there is no available space
}


char change_vehicle_type(char current_type) {
  for (int i = 0; i < vehicle_len; i++) {
    if (vehicles[i].Type == current_type) {
      return -1;
    } else if (vehicles[i].Type != current_type) {
      vehicles[i].Type == current_type;
      return vehicles[i].Type;
    }
  }
}




char row1[16], row2[16];

unsigned long lastChecked;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setBacklight(7);
  lastChecked = millis();
  setTime(0, 0, 0, 1, 1, 2023);  //millis()
}

void lcd_display() {
  if (i >= 0) {
    lcd.createChar(0, ArrowUp);
    lcd.createChar(1, ArrowDown);
    lcd.setCursor(0, 0);
    lcd.write((uint8_t)0);
    for (i = 0; i < index; i++) {
      lcd.setCursor(1, 0);
      lcd.print(vehicles[scrolling_index].regNumber);
    }

  }
}


void loop() {

  static state(current_state) = Synchronisation;
  switch (current_state) {
    case Synchronisation:
      {
        lcd.setBacklight(5);
        delay(1000);
        Serial.print("Q");
        if (Serial.available() > 0) {
          char incoming_char = Serial.read();
          if (incoming_char == 'X') {
            lcd.setBacklight(7);
            Serial.print("BASIC");
            current_state = Main;
          }
        }
      }

    case Main:
      {
        uint8_t ButtonsPressed = lcd.readButtons();
        static unsigned long Select_Start = 0;
        static bool hold_select_button = false;
        if (ButtonsPressed && BUTTON_SELECT) {
          if (Select_Start == 0) {
            Select_Start = millis();
          }
          if (millis() - Select_Start > 1000) {
            if (!hold_select_button) {
              lcd.setBacklight(5);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print(" F312908");
              lcd.print(" ");
              lcd.setCursor(0, 1);
              lcd.print(" Free SRAM: ");
              lcd.print(freeMemory());
              delay(1000);
              hold_select_button = true;
              Select_Start == 0;
            }
          } else {
            if (hold_select_button) {
              lcd.clear();
              lcd.setBacklight(7);
              hold_select_button = false;
              Select_Start = 0;
            }
          }
        }

        if (ButtonsPressed && BUTTON_DOWN) {
          if (scrolling_index != index - 1) {
            scrolling_index++;
            Serial.println(scrolling_index);
            lcd_display();
          }
        }
        if (ButtonsPressed && BUTTON_UP) {
          if (scrolling_index != 0) {
            scrolling_index--;
            Serial.println(scrolling_index);
            lcd_display();
          }
        }
        if (Serial.available() > 0) {
          String input = Serial.readString();
          input.trim();
          if (input.charAt(0) == 'A') {

            int current_position = get_vehicle_position(input.substring(2, 9));
            if (current_position == -1) {
              //if vehicle doesnt exist then get new position
              //Serial.println(input);
              int new_position = get_new_position();
              //Serial.println("DEBUG);
              


              if (new_position < 0) {
                // if there is no space
                Serial.println("ERROR: NO EMPTY SPACES. REMOVE A VEHICLE");
              } else {
                //if there is space, is it paid for
                if (current_position == -1) {
                  int current_position = get_vehicle_position(input.substring(2, 9));
                  vehicles[new_position].regNumber = input.substring(2, 9);
                  vehicles[new_position].Type = input.charAt(10);
                  vehicles[new_position].Parking_Location = input.substring(12);
                  vehicles[current_position].Payment_Status = false;
                  vehicles[current_position].Entry_time = "0000";
                  vehicles[current_position].Exit_time = "0000";
                  lcd.print(" ");
                  lcd.print(input.substring(2, 9));
                  lcd.print(" ");
                  lcd.print(input.substring(12, input.length()));
                  lcd.setCursor(0, 1);
                  lcd.print(" ");
                  lcd.print(input.charAt(10));
                  lcd.print(" ");
                  if (vehicles[current_position].Payment_Status) {
                    lcd.print("PD");
                  }
                  else {
                    lcd.print("NPD");
                  }

                  lcd.setCursor(6, 1); 
                  lcd.print(" ");
                  lcd.print(vehicles[current_position].Entry_time);
                  lcd.print(" ");
                  lcd.print(vehicles[current_position].Exit_time);

                }
              }
            }
          }
          if (input.charAt(0) == 'S') {
            int current_position = get_vehicle_position(input.substring(2, 9));
            if (current_position >= 0) {

              if (input.substring(10, input.length()) == "PD") {
                if (vehicles[current_position].Payment_Status != true) {
                  vehicles[current_position].Payment_Status = true;
                  String hours_passed;
                  if (hour() < 10) {
                    hours_passed += "0";
                  }
                  hours_passed += String(hour());
                  String minutes_passed;
                  if (minute() < 10) {
                    minutes_passed += "0";
                  }
                  minutes_passed += String(minute());
                  vehicles[current_position].Exit_time = hours_passed + minutes_passed;
                  Serial.println("Exit time= " + hours_passed + minutes_passed);  // can remove
                } else {
                  Serial.println("ERROR: Vehicle already payed for");
                }
              } else if (input.substring(10, input.length()) == "NPD") {
                if (vehicles[current_position].Payment_Status) {
                  vehicles[current_position].Payment_Status = false;
                  String hours_passed;
                  if (hour() < 10) {
                    hours_passed += "0";
                  }
                  hours_passed += String(hour());
                  String minutes_passed;
                  if (minute() < 10) {
                    minutes_passed += "0";
                  }
                  minutes_passed += String(minute());
                  vehicles[current_position].Entry_time = hours_passed + minutes_passed;
                  Serial.println("Entry time= " + hours_passed + minutes_passed);  // can remove
                } else {
                  Serial.println("ERROR: Vehicle is already not paid for");
                }
              } else {
                if (input.substring(10, input.length()) == "NPD" || input.substring(10, input.length()) == "PD") {
                  Serial.println("ERROR: Payment status entered is the same as stored value"); 
                } else {
                  Serial.print("not valid");
                }
              }


            } else {
              Serial.println("ERROR: Can't indicate payment status. Vehicle must be added to database.");
            }
          }



          if (input.charAt(0) == 'T') {                                          //if the first character is T
            int current_position = get_vehicle_position(input.substring(2, 9));  // the current_position variabe stores the registration number
            char current_type = input.charAt(10);                                // The vehicle's type is the third character
            if (current_position >= 0) {                                         //if the vehicle's registration number has been added to the system
              if (vehicles[current_position].Payment_Status = false) {
                Serial.println("ERROR: Payment status is not paid.");
              } else {
                if (vehicles[current_position].Type == current_type) {
                  Serial.println("ERROR: Vehicle is already of that type");
                } else if (vehicles[current_position].Type != current_type) {
                  vehicles[current_position].Type = current_type;
                  Serial.print(vehicles[current_position].Type);
                }
              }
            } else {
              Serial.println("ERROR: Vehicle must be added to database");
            }
          }

          if (input.charAt(0) == 'L') {
            int current_position = get_vehicle_position(input.substring(2, 9));
            if (current_position >= 0) {
              if (vehicles[current_position].Payment_Status) {
                String input_location = input.substring(10, input.length());
                if (vehicles[current_position].Parking_Location == input_location) {
                  Serial.println("ERROR: Location entered is already in use");
                } else if (vehicles[current_position].Parking_Location != input_location) {
                  vehicles[current_position].Parking_Location = input_location;
                  Serial.println(vehicles[current_position].Parking_Location);

                } else {
                  input_location == vehicles[current_position].Parking_Location;
                  Serial.println("New parking location");
                }
              } else {
                Serial.println("ERROR: Can't change location because vehicle has not been paid for");
              }
            } else {
              Serial.println("ERROR: Vehicle must be added to system");
            }
          }
          if (input.charAt(0) == 'R') {
            int current_position = get_vehicle_position(input.substring(2, 9));
            if (current_position >= 0) {
              if (vehicles[current_position].Payment_Status) {                
                vehicles[scrolling_index].regNumber == "";
              } else {
                Serial.println("ERROR: Vehicle must be added to system");
              }
            } else {
              Serial.println("vehicle must be added ");
            }
          }
        }
      }
  }
}
