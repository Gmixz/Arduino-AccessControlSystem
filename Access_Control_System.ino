*// Include required libraries
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <SPI.h>
#include <Adafruit_Fingerprint.h>

// Create instances
SoftwareSerial SIM900(19,18); // SoftwareSerial SIM900(Rx, Tx)
MFRC522 mfrc522(53, 5); // MFRC522(SDA Pin, RST Pin)
LiquidCrystal_I2C lcd(0x3F, 16, 2);
Servo sg90;

// Initialize Pins for led, servo and buzzer
constexpr uint8_t greenLed = 7;
constexpr uint8_t redLed = 6;
constexpr uint8_t buzzerPin = 9;
constexpr uint8_t servoPin = 8;

char initial_password[4] = { '1','5','4','2'}; // Variable to store initial password
String tagUID = "99 2E C6 99"; // String to store UID tag
char password[4]; // Variable to store users password

// Start with RFID scan (card). Step 1.
boolean IsRFIDMode = true;

// Step 2.
boolean IsFingerPrintMode = false;

// Step 3.
boolean IsKeyPadMode = false;

//Variable that dictates whether the system should stop progressing through the steps.
boolean IsRunModeHalted = false;

char key_pressed = 0; // Variable to store incoming keys

uint8_t i = 0;  // Variable used for counter

// defining how many rows and columns our keypad have
const byte rows = 4;
const byte columns = 4;

// Keypad pin map
char hexaKeys[rows][columns] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Initialize pins for keypad
byte row_pins[rows] = {35, 34, 33, 32};
byte column_pins[columns] = {39, 38, 37, 36};

// Create instance for keypad
Keypad Keypad_key = Keypad( makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);

//Fingerprint instance.
SoftwareSerial fingerPrintSerial(10, 11);
Adafruit_Fingerprint adafruitFingerprint = Adafruit_Fingerprint(&fingerPrintSerial);

void setup() {
  // Arduino Pin configuration
  pinMode(buzzerPin, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  sg90.attach(servoPin);  //Declare pin 8 for servo
  sg90.write(0); // Set initial position at 0 degrees

  lcd.begin();   // LCD screen
  lcd.backlight();
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  // Arduino communicates with SIM900 GSM shield at a baud rate of 19200
  // Make sure that corresponds to the baud rate of your module
  SIM900.begin(19200);

  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r");
  delay(100);
  // Set module to send SMS data to serial out upon receipt
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(100);

  lcd.clear(); // Clear LCD screen
}

void loop() {

  // Workflow => User must Scan RFID card -> Insert FingerPring -> Insert KeyPad Password.
  // However, if user fails to provide on of the requirements above we must push him to step 1.
  if (IsRunModeHalted == false) {
   
    if (IsRFIDMode == true)
{
      // Function to receive message
      //receive_message();

      lcd.setCursor(0, 0);
      lcd.print(" Door Lock");
      lcd.setCursor(0, 1);
      lcd.print(" Scan Your Tag ");

      // Look for new cards
      if ( ! mfrc522.PICC_IsNewCardPresent()) {
        return;
      }

      // Select one of the cards
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        return;
      }

      //Reading from the card
      String tag = "";
      for (byte j = 0; j < mfrc522.uid.size; j++)
      {
        tag.concat(String(mfrc522.uid.uidByte[j] < 0x10 ? " 0" : " "));
        tag.concat(String(mfrc522.uid.uidByte[j], HEX));
      }
      tag.toUpperCase();

      //Checking the card
      if (tag.substring(1) == tagUID)
      {
        // If UID of tag is matched.
        lcd.clear();
        lcd.print("Tag Matched");
        digitalWrite(greenLed, HIGH);
        delay(3000);
        digitalWrite(greenLed, LOW);
        IsRFIDMode = false;
IsFingerPrintMode = true;
      }

      else
      {
        // If UID of tag is not matched.
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wrong Tag Shown");
        lcd.setCursor(0, 1);
        lcd.print("Access Denied");
        digitalWrite(buzzerPin, HIGH);
        digitalWrite(redLed, HIGH);
        //send_message("Someone Tried with the wrong tag \nType 'close' to halt the system.");
        delay(3000);
        digitalWrite(buzzerPin, LOW);
        digitalWrite(redLed, LOW);
        lcd.clear();
      }
    }

if (IsFingerPrintMode == true)
{
lcd.clear();
lcd.setCursor(0, 0);
lcd.print(" Scan FingerPrint ");
int counter = 0;
int fingerprintId = -1;

while(counter <= 5)
{
fingerprintId = getFingerprintIdByScanningCurrentFingerprint();

if (fingerprintId >= 0) {
break;
}

counter++;
delay(1000);
}

if (fingerprintId >= 0) //If ID greater than -1 means we found a good ID.
{
lcd.clear();
lcd.print("Fingerprint Matched");
digitalWrite(greenLed, HIGH);
delay(3000);
digitalWrite(greenLed, LOW);
IsKeyPadMode = true;
}
else
{
lcd.clear();
            lcd.print("Wrong FingerPrint");
            digitalWrite(buzzerPin, HIGH);
            digitalWrite(redLed, HIGH);
            //send_message("Someone Tried with the wrong Fingerprint \nType 'close' to halt the system.");
            delay(3000);
            digitalWrite(buzzerPin, LOW);
            digitalWrite(redLed, LOW);
IsRFIDMode = true;
}

IsFingerPrintMode = false;
}
   
    if (IsKeyPadMode == true)
{
      key_pressed = keypad_key.getKey(); // Storing keys
      if (key_pressed)
      {
        password[i++] = key_pressed; // Storing in password variable
        lcd.print("*");
      }
      if (i == 4) // If 4 keys are completed
      {
        delay(200);
        if (!(strncmp(password, initial_password, 4))) // If password is matched
        {
          lcd.clear();
          lcd.print("Pass Accepted");
          sg90.write(90); // Door Opened
          digitalWrite(greenLed, HIGH);
          //send_message("Door Opened \nIf it was't you, type 'close' to halt the system.");
          delay(3000);
          digitalWrite(greenLed, LOW);
          sg90.write(0); // Door Closed
          lcd.clear();
          i = 0;          
        }
        else // If password is not matched
        {
          lcd.clear();
          lcd.print("Wrong Password");
          digitalWrite(buzzerPin, HIGH);
          digitalWrite(redLed, HIGH);
          //send_message("Someone Tried with the wrong Password \nType 'close' to halt the system.");
          delay(3000);
          digitalWrite(buzzerPin, LOW);
          digitalWrite(redLed, LOW);
          lcd.clear();
          i = 0;    
        }

IsRFIDMode = true; //Go back to step 1.
IsKeyPadMode = false; //User only gets one chance to do this right. If right move on, if wrong move on too.

      }
    }

  }
  else {
    receive_message();
delay(100);
  }
}

// Receiving the message
void receive_message()
{
  char incoming_char = 0; //Variable to save incoming SMS characters
  String incomingData;   // for storing incoming serial data
 
  if (SIM900.available() > 0)
  {
    incomingData = SIM900.readString(); // Get the incoming data.
    delay(10);
  }


  // if received command is to open the door
  if (incomingData.indexOf("open") >= 0)
  {
    sg90.write(90);
    IsRunModeHalted = false;
    send_message("Opened");
    delay(10000);
    sg90.write(0);
  }

  // if received command is to halt the system
  if (incomingData.indexOf("close") >= 0)
  {
    IsRunModeHalted = true;
    send_message("Closed");
  }
  incomingData = "";
}

// Function to send the message
void send_message(String message)
{
  SIM900.println("AT+CMGF=1");    //Set the GSM Module in Text Mode
  delay(100);
  SIM900.println("AT+CMGS=\"+##########\""); // Replace it with your mobile number
  delay(100);
  SIM900.println(message);   // The SMS text you want to send
  delay(100);
  SIM900.println((char)26);  // ASCII code of CTRL+Z
  delay(100);
  SIM900.println();
  delay(1000);
}

// returns -1 if failed, otherwise works.
int getFingerprintIdByScanningCurrentFingerprint()
{
 if (adafruitFingerprint.getImage() != FINGERPRINT_OK ||
adafruitFingerprint.image2Tz() != FINGERPRINT_OK ||
adafruitFingerprint.fingerFastSearch() != FINGERPRINT_OK)
  {
return -1;
  }
  else
  {
Serial.print(1);
return 1;
  }
}
