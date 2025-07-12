#include <Keypad.h>             // Used for handling keypad input.
#include <LiquidCrystal_I2C.h>  // Used for interfacing with I2C LCD displays.
#include <Wire.h>               // Necessary for I2C communication.
#include <Servo.h>              // Used for controlling servo motors.
#include <SPI.h>                // Include the SPI library
#include <MFRC522.h>            // Include the MFRC522 RFID library

// Define pin numbers
#define WARNING A0
#define GREEN A2
#define SERVO 1
#define RST_PIN 2
#define SS_PIN 10


// Initialize LCD and RFID
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);
byte ID[4] = {51,46,65,168};

// Define keypad layout
const byte ROWS = 4;
const byte COLS = 3;
char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

// Define keypad pin configuration
byte rowPins[ROWS] = { 9, 8, 7, 6 };
byte colPins[COLS] = { 5, 4, 3 };

// Create Keypad object
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
int wrong_counter = 0;

//Create password variable
int password = 1234;

// Create Servo object
Servo Servo1;

void setup() {
  pinMode(WARNING, OUTPUT);  // Set the WARNING pin as an output to control the red LED and the buzzer
  pinMode(GREEN, OUTPUT);    // Set the GREEN pin as an output to control the green LED signal
  Servo1.attach(SERVO);      // Start the Servo motor
  lcd.begin();               // Initialize the LCD display
  SPI.begin();               // Initialize the SPI communication. This will be used by the RFID controller
  rfid.PCD_Init();           // Start the RFID module
  set_password();            // The password will be defined by the user. When the safe is powered, it asks for the user to create password
}

void loop() {
  // Move the servo motor to the initial position (180 degrees)
  Servo1.write(180); //180 degrees locks the safe

  // Set the cursor at position (0, 0) on the LCD and print the message
  print("Enter Password:", 0, 0);

  // Check if the entered password is correct
  if (check_password(password)) {
    print("CORRECT PASSWORD", 0, 0);
    print("Lock Opened", 0, 1);

    // Move the servo motor to the unlocked position (90 degrees)
    Servo1.write(90);

    // Reset the wrong password counter
    wrong_counter = 0;

    // Flash the green signal
    digitalWrite(GREEN, HIGH);
    delay(2000);
    digitalWrite(GREEN, LOW);

    // After the safe is opened, the program waits for the user to relock again by pressing #
    while (relock()) {
      print("Safe Locked", 0, 0);
      
      // Move the servo motor to the locked position (180 degrees)
      Servo1.write(180);

      // Wait for 0.5 seconds
      delay(500);

      // Break out of the loop
      break;
    }
  }
  
  else
  {
    print("WRONG PASSWORD", 0, 0);

    // Set cursor at (0, 1) and print remaining attempts
    lcd.setCursor(0, 1);
    lcd.print(3 - (++wrong_counter));

    if (3-wrong_counter == 1)
      print("attempt left.", 3, 1);

    else
      print("attempts left.", 3, 1);
  }

  // Check if the maximum wrong attempts (3) have been reached
  if (wrong_counter == 3) {
    print("SAFE LOCKED", 0, 0);

    // Set cursor at (0, 1) and print swipe card message
    print("Swipe Card", 0, 1);
    
    // Lock the safe until the authorized card gets scanned
    while (1)
    {
      // Flash the red led and the buzzer
      digitalWrite(WARNING, HIGH);
      delay(500);
      digitalWrite(WARNING, LOW);
      delay(500);
      
      // Check if a card is getting scanned. If not, restart the loop
      if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
      {
        continue;
      }

      // Compare the scanned card's UID with the authorized UID number
      if ((rfid.uid.uidByte[0] == ID[0]) && (rfid.uid.uidByte[1] == ID[1]) && (rfid.uid.uidByte[2] == ID[2]) && (rfid.uid.uidByte[3] == ID[3]))
      {
        print("Safe unlocked", 0,0);
        Servo1.write(90); // Unlock the safe

        // Reset the incorrect attempt counter
        wrong_counter = 0;

        delay(100);

        // Wait for the user to press # to lock the safe 
        while(!relock())
        {
        // This while loop is to make the program wait until the user presses '#'
        }
        
        // Lock the safe and print it into display
        Servo1.write(180);
        print("Safe locked", 0,0);
        delay(100);
        break;
      }

      else
      {
        print("Unvalid card", 0,0);
        delay(1500);
        print("Safe locked", 0, 0);
        print("Scan card", 0,1);
      }

      // Stop the communication between the scanner and the card until new card is scanned
      rfid.PICC_HaltA();
    }
    
  }

  // Wait for 3 seconds before clearing the LCD screen and repeating the loop
  delay(3000);
  lcd.clear();
}

// Function to check password
bool check_password(int password) {
  int input = 0;  // for password input
  input = take_input();

  // Set the LCD cursor for the result display
  lcd.setCursor(0, 1);

  // Check if the entered password matches the expected password
  if (input == password)
    return true;
  
  else
    return false;
}

// Set the cursor at position (i, j) on the LCD and print the message
void print(char *message, int i, int j) {
  if (i == 0 && j == 0) {
    // Clear the LCD screen if the cursor is at the top left corner
    lcd.clear();
  }

  // Set cursor at (i, j) and print the message
  lcd.setCursor(i, j);
  lcd.print(message);
}

int take_input(){
  // Declare variables for keypad input and password entry
  char customKey; // for store the pressed keypad key.

  int input = 0;  // for password input
  int i = 0;      // for loop counters
  int j = 1000;   // Because the password is a four-digit number
  int temp;       // a temporary variable for digit conversion

  // Loop to read 4 digits of the password
  while (i < 4) {
    // Waits for a key press and assigns the pressed key to customKey.
    customKey = customKeypad.getKey();

    // Check if a key is pressed
    if (customKey) {
      // Set the LCD cursor to display the entered key
      lcd.setCursor(i, 1);
      lcd.print(customKey);

      // Convert char 'customKey' to integer 'input'
      temp = customKey - '0';  // Convert the character to an integer
      temp = temp * j;         // for positioning the digit correctly within the overall number.
      input += temp;           // Adds the calculated temp value to the overall input
      j /= 10;                 // Divides j by 10 to adjust its value for the next digit

      // Increment the digit counter
      i++;
    }
  }
  return(input);
}

void set_password()
{
  print("Set Password", 0, 0);
  password = take_input();
}

bool relock()
{
  lcd.clear();
  print("Press # to lock", 0, 0);
  char customKey;

  do //Program stays in the loop until the user presses #
  {
    customKey = customKeypad.getKey();
  
    if (customKey)
    {
      if (customKey == '#') //Compare the given input with '#'
        return true;
    }
  } while (customKey != '#');
}