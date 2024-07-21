#include <SoftwareSerial.h>
#include <TinyGPS.h>

// Create software serial object to communicate with SIM900 and GPS
SoftwareSerial SIM900(7, 8); // SIM900 Tx & Rx is connected to Arduino #7 & #8
SoftwareSerial gpsSerial(4, 3); // GPS Tx & Rx is connected to Arduino #4 & #3

TinyGPS gps;

bool messageProcessed = false; // Flag to track whether message has been processed
String messageBuffer = ""; // Buffer to store the received message

void setup() {
  // led high for indicating vehicle turned ON
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  
  // Begin serial communication with Arduino and SIM900
  SIM900.begin(9600);

  // Begin serial communication with GPS module
  gpsSerial.begin(9600);

  Serial.println("Initializing Autoshield...");
  delay(1000);

  initializeGSM();
  configureSMSMode();
  configureSMSNotification();
  sendAlertSMS();
  Serial.write("SMS sent!");
}

void loop() {
  // Check if there is any incoming data
  if (SIM900.available() > 0) {
    readIncomingData();
  }

  // Process the message if it has been received and not processed yet
  if (messageBuffer.length() > 0 && !messageProcessed) {
    processMessage(messageBuffer);
    messageBuffer = ""; // Clear the buffer after processing
  }
}

void sendATCommand(const char* command, int waitTime) {
  SIM900.println(command);
  delay(waitTime); // Delay in milliseconds
  while (SIM900.available()) {
    Serial.write(SIM900.read()); // Forward what Software Serial received to Serial Port
  }
}

void initializeGSM() {
  sendATCommand("AT", 1000);
  sendATCommand("AT+CSQ", 1000);   // Signal quality test
  sendATCommand("AT+CCID", 1000);  // Read SIM information
  sendATCommand("AT+CREG?", 1000); // Check network registration
}

void configureSMSMode() {
  sendATCommand("AT+CMGF=1\r", 1000); // Set SIM900 to SMS mode
}

void sendAlertSMS() {
  float flat, flon;
  unsigned long age;

  // Get GPS coordinates
  smartDelay(5000); // Wait for GPS data to be available
  gps.f_get_position(&flat, &flon, &age);

  // Construct message with GPS coordinates
  String message = "Theft alert! ";
  if (age == TinyGPS::GPS_INVALID_AGE) {
    message += "No GPS data available.";
  } else {
    message += "Location: ";
    message += "Lat: " + String(flat, 6) + ", ";
    message += "Lon: " + String(flon, 6);
  }

  sendSMS("+923025231476", message.c_str());
}

void sendSMS(const char* phoneNumber, const char* message) {
  configureSMSMode();
  SIM900.print("AT+CMGS=\"");
  SIM900.print(phoneNumber);
  SIM900.println("\"");
  delay(1000); // Short delay to process command
  
  SIM900.print(message);
  delay(1000); // Short delay to process command

  // End AT command with a ^Z (ASCII code 26)
  SIM900.write(26);
  delay(500); // Wait for the message to be sent and processed
}

void configureSMSNotification() {
  sendATCommand("AT+CNMI=2,2,0,0,0\r", 100); // Set module to send SMS data to serial out upon receipt
}

void readIncomingData() {
  String message = ""; // Variable to store the received message

  // Read incoming data until there's nothing left
  while (SIM900.available()) {
    char incoming_char = SIM900.read();
    message += incoming_char; // Append each character to the message string
    delay(10); // Small delay to allow the next character to be received
  }

  if (message.length() > 0) {
    messageBuffer = message; // Save the message to the buffer
  }
}

void processMessage(String msg) {
  // Do something with the received message
  Serial.println("Received Message: " + msg);

  // Check if the received message contains "Turn Off"
  if (msg.indexOf("Turn Off") != -1) {
    digitalWrite(LED_BUILTIN, LOW); // Set the built-in LED pin to LOW to turn it off
  } else if (msg.indexOf("Turn On") != -1) {
    digitalWrite(LED_BUILTIN, HIGH); // Set the built-in LED pin to HIGH to turn it on
  }
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (gpsSerial.available())
      gps.encode(gpsSerial.read());
  } while (millis() - start < ms);
}
