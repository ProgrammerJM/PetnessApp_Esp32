#include <Servo.h>

Servo foodTrayDoorPin;
Servo dispenseServoPin;
int foodTrayWeightPin = A0;
bool dispenseFood = false;
int foodTrayWeightValue;
int AmountToDispensePerServing = 500;

void setup() {
  Serial.begin(9600);
  dispenseServoPin.attach(8);
  foodTrayDoorPin.attach(7);
  pinMode(foodTrayWeightPin, INPUT);
  
  Serial.println("Initializing servo....");
  foodTrayDoorPin.write(0); 
  dispenseServoPin.write(0);
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("start")) {
      dispenseFood = true;
      closeFoodTray();
      dispense();
    } else if (input.equalsIgnoreCase("stop")) {
      dispenseFood = false;
    }
  }
  
  if (dispenseFood) {
    foodTrayRead();
    if (foodTrayWeightValue < AmountToDispensePerServing) {
      continueDispense();
    } else {
      stopDispense();
    }
  }
}

void closeFoodTray(){
  Serial.println("OPENING FOOD TRAY DOOR");
  foodTrayDoorPin.write(90); 
  delay(1000); 
}

void openFoodTray(){
  Serial.println("CLOSING FOOD TRAY DOOR");
  foodTrayDoorPin.write(0); 
  delay(1000); 
}

void dispense() {
  Serial.println("Dispensing...");
  for (int angle = 0; angle <= 180; angle += 5) { 
    dispenseServoPin.write(angle);
    delay(10); 
    foodTrayRead(); 
    if (foodTrayWeightValue >= AmountToDispensePerServing) {
      stopDispense();
      return;
    }
  }  
  delay(100); 
}

void continueDispense(){
  Serial.println("Continue Dispensing...");
  for (int angle = 180; angle >= 0; angle -= 5) { 
    dispenseServoPin.write(angle);
    delay(10); 
    foodTrayRead(); 
    if (foodTrayWeightValue >= AmountToDispensePerServing) {
      stopDispense();
      return;
    }
  }
  delay(100);
}

void foodTrayRead(){
  foodTrayWeightValue = analogRead(foodTrayWeightPin);
  Serial.print("Food Tray Amount: "); Serial.println(foodTrayWeightValue);
}

void stopDispense() {
  foodTrayRead();
  Serial.println("Stopped dispensing");
  openFoodTray();
  delay(100); 
  dispenseFood = false;
}
