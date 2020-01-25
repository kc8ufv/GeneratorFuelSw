// Automatic Fuel Switch and fuel low indicator alarm
// Chrissy Hart, KC8UFV

// Device is set up like an Arduino Lilypad, and should be programmed with an Arduino-compatible ICSP programmer

// Theory of operation

// Senses fuel levels with standard fuel sender units connected to analog inputs, for longevity, reed based float senders are recommended - TRIGGER VALUES CURRENTLY ESTIMATED
// RED led for the tank (attached to the raw LED pins on the board) will flash slow at low fuel, and fast when sender reads empty, and solid if disconnected
// Buttons have built in LEDs that will illuminate when that tank is selected. 
// When a tank initially shows empty, and the other tank shows fuel, a timer will be started to run a few minutes 15? before switchover to use any remaining fuel.
// At automatic switchover, the alarm pin will buzz briefly to notify users of an empty tank, so it may be refilled for continuous operation.
// If both tanks reads empty, the alarm pin will be pulsed intermittently to warn of impending fuel loss.
// On power failure, obviously all indicators will be out. The fuel valve will automatically default to the first tank. 
// The first tank is also the default tank at power up.

#include<NoDelay.h>

void FUEL1blink(); // Timer functions need to be declared before everything else
void FUEL2blink(); 
void FUELSWITCH();
void ALARM();

noDelay FUEL1low(1000, FUEL1blink);    // Sets nodelay variable for fuel tank 1 slow blink
noDelay FUEL1empty(250, FUEL1blink);   // And fast blink

noDelay FUEL2low(1000, FUEL2blink);    // Same for tank 2
noDelay FUEL2empty(250, FUEL2blink);  

noDelay FUELSWtime(10, FUELSWITCH);    // Initialize a timer for fuel tank switching. This will be reset later in code when a tank starts to read empty. This timer may be reset multiple times at the low/empty threshold due to sloshing or switch bounce. 
noDelay FUELALARM(20000, ALARM);       // Initialize a 20 second timer for the alarm buzzer for when fuel is low

  int BUTTON1pin = 0;         // Define all IO pins - Defining all variables globally, so they are all available in all functions
  int BUTTON2pin = 1;
  int BUTTON1LEDpin = 2;
  int BUTTON2LEDpin = 3;
  int LED1pin = 4;
  int LED2pin = 5;
  int ALARMpin = 6;
  int VALVEpin = 7;
  int FUEL1pin = A0;
  int FUEL2pin = A1;

  int FUEL1lvl = 0;           // Initialize variable for reading the fuel levels. Lower numbers indicate higher tank levels
  int FUEL2lvl = 0;
  int FUELempty = 320;       // Calculated value of 336 with a 230 ohm low tank reading and 470 ohm pull up into a voltage divider. Lowered slightly to account for tolerances
  int FUELdiscon = 700;      // Way higher than any fuel sender would generate with a 470 ohm pull up - basically this is to be a minimum value read if the sender is disconnected
  int FUELlow = 275;         // Full tank value of 33 ohms calculated a value of 136. Lowered slightly to allow for tolerances, and should trigger at about 1/8 tank
  int ACTTANK = 1;            // variable to store active tank 
  int FUEL1STATUS = 1;        // Initial fuel tank status - assume full at startup, will read shortly.. 1=full, 2=low, 3=empty, 4=disconnected
  int FUEL2STATUS = 1;



void setup() {
  // put your setup code here, to run once:
  
  pinMode(BUTTON1pin, INPUT_PULLUP);  // Set all pinmodes
  pinMode(BUTTON2pin, INPUT_PULLUP);
  pinMode(FUEL1pin, INPUT);
  pinMode(FUEL2pin, INPUT);
  pinMode(BUTTON1LEDpin, OUTPUT);
  pinMode(BUTTON2LEDpin, OUTPUT);
  pinMode(LED1pin, OUTPUT);
  pinMode(LED2pin, OUTPUT);
  pinMode(ALARMpin, OUTPUT);
  pinMode(VALVEpin, OUTPUT);

  delay(50);                          // very brief delay after initial startup and setting of modes as main loop starts with an analog read, and we want to make sure the ADCs are stable
  
  

}

void loop() {
  // Start by reading fuel levels in each tank
  FUEL1lvl = analogRead(FUEL1pin);
  FUEL2lvl = analogRead(FUEL2pin);

  if (FUEL1lvl > FUELdiscon) {         // See if fuel tank is disconnected
    digitalWrite(LED1pin, HIGH);        // if so, light the fuel tank disconnected light
    FUEL1STATUS = 4;
  } else if (FUEL1lvl > FUELempty) {   // See if empty
    FUEL1empty.fupdate();               // Check if it's time to blink
    if (ACTTANK == 1) {                 // If tank 1 is the active tank, it's time for the switchover process
      if (FUEL1STATUS < 3 ) {             // If the sensor just switched to reading empty from reading full or low
        FUELSWtime.setdelay(900000);      // Set a 15 minute delay until switchover
      } else {
        FUELSWtime.fupdate();             // ELSE check if the timer expired, and switch tanks - tank switching logic makes sure the other tank isn't empty or disconnected.
      }
    }
    FUEL1STATUS = 3;                     
  } else if (FUEL1lvl > FUELlow) {     // Check if tank reads low
    FUEL1STATUS = 2;                      
    FUEL1low.fupdate();                 // If low, check if it's time for a slow blink
  } else {                              // If we've made it this far, the tank is full
    digitalWrite(LED1pin, LOW);         // Turn off the fuel LED
    FUEL1STATUS = 1;                      
  }

                                        // REPEAT SAME STEPS FOR TANK 2

  if (FUEL2lvl > FUELdiscon) {         // See if fuel tank is disconnected
    digitalWrite(LED1pin, HIGH);        // if so, light the fuel tank disconnected light
    FUEL2STATUS = 4;
  } else if (FUEL2lvl > FUELempty) {   // See if empty
    FUEL2empty.fupdate();               // Check if it's time to blink
    if (ACTTANK == 2) {                 // See if tank 2 is the active tank
      if (FUEL2STATUS < 3) {                // If the sensor just switched to reading empty from reading full or low
        FUELSWtime.setdelay(900000);      // Set a 15 minute delay until switchover
      } else {
        FUELSWtime.fupdate();             // ELSE check if the timer expired, and switch tanks - tank switching logic makes sure the other tank isn't empty or disconnected.
      }
    }
    FUEL2STATUS = 3;                     
  } else if (FUEL2lvl > FUELlow) {     // Check if tank reads low
    FUEL2STATUS = 2;                      
    FUEL2low.fupdate();                 // If low, check if it's time for a slow blink
  } else {                              // If we've made it this far, the tank is full
    digitalWrite(LED2pin, LOW);         // Turn off the fuel LED
    FUEL2STATUS = 1;                      
  }  


  if (digitalRead(BUTTON1pin) == LOW) {   // Check if Manual Switch to tank 1 button is pressed
    if (FUEL1STATUS < 3) {                // Make sure there is fuel in the tank
        ACTTANK = 1;                      // And set the active tank variable
        
    } else if (FUEL2STATUS > 2) {         // Or, if tank 2 reads empty or disconnected
        ACTTANK = 1;                      // set the active tank variable
      
    }
  }

  
  if (digitalRead(BUTTON2pin) == LOW) {   // Check if Manual Switch to tank 2 button is pressed
    if (FUEL2STATUS < 3) {                // Make sure there is fuel in the tank
        ACTTANK = 2;                      // And set the active tank variable
        
    } else if (FUEL2STATUS > 2) {         // Or, if tank 2 reads empty or disconnected
        ACTTANK = 2;                      //  set the active tank variable
      
    }
  }

  if (ACTTANK == 1) {                     // Set status lights, and set valve to correct tank
    digitalWrite(BUTTON1LEDpin, HIGH);
    digitalWrite(BUTTON2LEDpin, LOW);
    digitalWrite(VALVEpin, LOW);
  } else {
    digitalWrite(BUTTON1LEDpin, LOW);
    digitalWrite(BUTTON2LEDpin, HIGH);
    digitalWrite(VALVEpin, HIGH);
  }

  if (FUEL1STATUS > 2) {
    if (FUEL2STATUS > 2) {                // If both tanks are low
      FUELALARM.fupdate();                // Send intermittent fuel alarm
    }
  }
  
}

void FUEL1blink() {
   if (digitalRead(LED1pin) == LOW) {
    digitalWrite(LED1pin, HIGH);
   } else {
    digitalWrite(LED1pin, LOW);
   }
}

void FUEL2blink() {
   if (digitalRead(LED2pin) == LOW) {
    digitalWrite(LED2pin, HIGH);
   } else {
    digitalWrite(LED2pin, LOW);
   }
} 


void FUELSWITCH() {                        // Check new tank has fuel and switch if appropriate. Lights and tanks will switch at the end of the main loop... 
  if (ACTTANK == 1) {
    if (FUEL2STATUS < 3) {                 // Make sure there is fuel in tank 2
        ACTTANK = 2;                       // And set the active tank variable
        ALARM();                           // Also, a single buzz of the alarm to notify of the tank switch
    } else {
      FUELSWtime.setdelay(10000);           // If tank 2 not ready, try again in 10 seconds
    }
  } else {
     if (FUEL1STATUS < 3) {                // Make sure there is fuel in tank 1
        ACTTANK = 1;                       // And set the active tank variable
        ALARM();                           // Also, a single buzz of the alarm to notify of the tank switch  
    } else {
      FUELSWtime.setdelay(10000);           // If tank 1 not ready, try again in 10 seconds
    }
  }
  
}

void ALARM() {
  digitalWrite(ALARMpin, HIGH);          // Initiate a 1 second buzz of the alarm
  delay(1000);                           // Blocking delay OK here due to the infrequency of this function being called.
  digitalWrite(ALARMpin, LOW);
}
