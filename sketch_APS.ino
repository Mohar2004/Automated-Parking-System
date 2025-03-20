#include <Servo.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

// Define pins for sensors and actuators
#define CSP_T A2
#define CSP_E A3
#define BSP_T A4
#define BSP_E A5
#define EGSP 8
#define EXGSP 9

// Initialize LCD (adjust pins as per your setup)
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

const byte ROWS = 4; // Four rows
const byte COLS = 3; // Three columns

// Define the keymap for the keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Define combined row/column pin connections
byte rowPins[ROWS] = {10, 11, 12, 13}; // Connect combined pins here
byte colPins[COLS] = {10, 11, 12};    // Reuse the same combined pins for columns

// Create Keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// Servo objects for gates
Servo entryGateServo;
Servo exitGateServo;

// Parking slot variables
int totalCarSlots = 5;
int totalBikeSlots = 5;
int carSlotsAvailable = totalCarSlots;
int bikeSlotsAvailable = totalBikeSlots;

// Time tracking for parking slots
unsigned long parkingStartTime[10]; // Array to store start times for each slot
bool slotOccupied[10] = {false};    // Array to track occupied slots

// Define gate angles
const int GOA = 90;
const int GCA = 0;

// Payment rate per second in rupees
const float PR = 0.0167;

void setup() {
  pinMode(CSP_E, INPUT);
  pinMode(CSP_T, OUTPUT);
  pinMode(BSP_E, INPUT);
  pinMode(BSP_T, OUTPUT);
  
  
  entryGateServo.attach(EGSP);
  exitGateServo.attach(EXGSP);
  
  // Initialize gates in closed position
  closeEntryGate();
  closeExitGate();

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("Parking System");
  delay(2000);
}

void loop() {
  lcd.clear();
  long duration1;
  long duration2;
  float distance1;
  float distance2;
  
  // Display available slots
  lcd.setCursor(0, 0);
  lcd.print("Cars: ");
  lcd.print(carSlotsAvailable);
  
  lcd.setCursor(0, 1);
  lcd.print("Bikes: ");
  lcd.print(bikeSlotsAvailable);

  // Handle vehicle entry
  //Sensor1
  digitalWrite(CSP_T, LOW);
  delay(2);
  digitalWrite(CSP_T, HIGH);
  delay(10);
  digitalWrite(CSP_T, LOW);
  delay(2);
  duration1 = pulseIn(CSP_E, HIGH);
  distance1 = (duration*0.0343)/2;

  //Sensor2
  digitalWrite(BSP_T, LOW);
  delay(2);
  digitalWrite(BSP_T, HIGH);
  delay(10);
  digitalWrite(BSP_T, LOW);
  delay(2);
  duration2 = pulseIn(BSP_E, HIGH);
  distance2 = (duration*0.0343)/2;

  //The entry gate has two sensors one which will measure the distance between the gate and the top of vehicle
  //the other one will measure the lateral distance between the gate and the vehicle.
  
  if (distance1 < 100 && distance2 < 60) {
    handleVehicleEntry("Car");
    delay(2000); // Avoid multiple triggers
  } else if (distance1 >= 100 && distance2 >= 60) {
    handleVehicleEntry("Bike");
    delay(2000); // Avoid multiple triggers
  }
  else
  {
    lcd.print("NO VEHICLE")
  }

  // Handle vehicle exit slot number entered from keypad
  int exitingSlot = checkExitRequest(); 
  if (exitingSlot != -1) {
    handleVehicleExit(exitingSlot);
    delay(2000); // Avoid multiple triggers
  }
}

// Function to handle vehicle entry
void handleVehicleEntry(String vehicleType) {
  if (vehicleType == "Car" && carSlotsAvailable > 0) {
    assignSlot(vehicleType);
    
    openEntryGate();
    delay(5000); // Keep gate open for a while to avoid mishap
    closeEntryGate();

    carSlotsAvailable--;
    
 }
 else if (vehicleType == "Bike" && bikeSlotsAvailable > 0) {
    assignSlot(vehicleType);
    
    openEntryGate();
    delay(5000); // Keep gate open for a while to avouid mishap
    closeEntryGate();

    bikeSlotsAvailable--;
    
 }
  else {
    lcd.clear();
    lcd.print("No Slots Available");
    delay(2000);
  }
}

// Function to assign a parking slot
void assignSlot(String vehicleType) {
  int assignedSlot = -1;

  // Assign a slot based on vehicle type
  for (int i = (vehicleType == "Car" ? 0 : totalCarSlots); 
           i < (vehicleType == "Car" ? totalCarSlots : totalCarSlots + totalBikeSlots); i++) {
    if (!slotOccupied[i]) {
      assignedSlot = i;
      slotOccupied[i] = true;
      parkingStartTime[i] = millis(); //Time of parking starts and gets recorded 
      break;
    }
  }

  if (assignedSlot != -1) {
    lcd.clear();
    lcd.print(vehicleType + " Slot: ");
    lcd.print(assignedSlot + 1); //Original parking slot index from 1
    delay(2000);
  }
}

// Function to handle vehicle exit
void handleVehicleExit(int exitingSlot) {
  if (!slotOccupied[exitingSlot]) {
    lcd.clear();
    lcd.print("Invalid Slot!");
    delay(2000);
    return;
  }

  // Calculate parked time in seconds
  unsigned long parkedTimeMillis = millis() - parkingStartTime[exitingSlot];
  float parkedTimeSeconds = parkedTimeMillis / 1000.0;

  // Calculate payment amount
  float paymentAmount = calculatePayment(parkedTimeSeconds);

  // Display payment details
  lcd.clear();
  lcd.print("Pay: Rs ");
  lcd.print(paymentAmount);
  
  delay(5000); //payment process under process;

  // Mark slot as empty after payment
  slotOccupied[exitingSlot] = false;

  if (exitingSlot < totalCarSlots) {
    carSlotsAvailable++;
  } else {
    bikeSlotsAvailable++;
  }

  openExitGate();
  delay(2000); // Keep gate open for a while to avoid mishap
  closeExitGate();

  lcd.clear();
  lcd.print("Thank You!");
  delay(2000);
}

//Function to calculate payment based on parked time
float calculatePayment(float timeInSeconds) {
  return timeInSeconds * PR;
}

//Function to enter the slot number being emptied
int checkExitRequest() {
  
  char key = keypad.getKey(); //Keys are pressed for entering slotnumber
  int exitingSlot;
  if(key){
    exitingSlot = (int)key;
  }
  
  return exitingSlot;
}

//The gate opening and closing functions
void openEntryGate() {
  entryGateServo.write(GOA);
}

void closeEntryGate() {
  entryGateServo.write(GCA); 
}

void openExitGate() {
  exitGateServo.write(GOA);
}

void closeExitGate() {
  exitGateServo.write(GCA); 
}

