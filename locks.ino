/* RC522:
 * SDA  to  GPIO02 - D4
 * SCK  to  GPIO14 - D5
 * MOSI to  GPIO13 - D7
 * MISO to  GPIO12 - D6
 * IRQ      N/A
 * GND  to  GND
 * RST  to  GPIO00 - D3
 * 3.3V to  3.3V
 * 
 * 
 * Relay:
 * +  to  3.3V
 * -  to  GND
 * S  to  D2
 */

#include <MFRC522.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Hash.h>

//#define DEBUG

#ifdef DEBUG
  #define PRINTDEBUG(STR) Serial.println(STR)
#else
  #define PRINTDEBUG(STR) /*NOTHING*/
#endif

//WiFi
const char* ssid = "WIFI_SSID";
const char* password = "PASSWORD";

#define PLACE "CUSTOM_LOCK_IDENTIFIER"

const char* host = "api.example.com";
const int httpsPort = 443;
#define API "/lock.php?place="

//SHA1 fingerprint of the certificate
const char* fingerprint = "‎B4 79 37 7F 73 F8 38 B9 45 74 E2 FB 4B 02 F1 4B F6 AA 1D 11";

//RC522 RFID Shield
#define RST_PIN 0  //RST
#define SS_PIN  2  //SDA
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

//Relay
#define RELAY_PIN 4 // Signal pin for relay

//Min length of UID to be considered secure
#define AUTHORIZED_LENGTH 7
//const byte authorized[AUTHORIZED_LENGTH] = {1, 2, 3, 4, 5, 6, 7}; //Authorized UID for the non-WiFi version

//Salt to prepend and append to UID before hashing
#define SALT1 "RANDOM1"
#define SALT2 "RANDOM2"





// Helper routine to dump a byte array as hex values to Serial
void dump_byte_array_hex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; ++i) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}





// Helper routine to dump a byte array to Serial
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; ++i) {
    Serial.print(buffer[i] < 100 ? buffer[i] < 10 ? " 00" : " 0" : " ");
    Serial.print(buffer[i]);
  }
}





// UID string
String uid_string(byte *buffer, byte bufferSize) {
  String uid = "";
  
  for (byte i = 0; i < bufferSize; ++i) {
    uid += buffer[i];
  }

  return uid;
}





// UID hashing
String uid_hash(byte *buffer, byte bufferSize) {
  if(bufferSize < AUTHORIZED_LENGTH)
    return String("The UID is too short!");
  
  return sha1(SALT1 + uid_string(buffer, bufferSize) + SALT2);
}





// Inifinite loop - Reset self
void resetSelf() {
  PRINTDEBUG("Reseting");
  while(1);
}





// Function to connect WiFi
void connectWifi(const char* ssid, const char* password) {
  int WiFiCounter = 0;
  PRINTDEBUG("Connecting to:");
  PRINTDEBUG(ssid);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);                                            // Disable AP mode
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED && WiFiCounter < 30) {      // Wait for connection up to 30 seconds
    delay(1000);
    WiFiCounter++;
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }

  PRINTDEBUG();
  PRINTDEBUG("WiFi connected");
  PRINTDEBUG("IP address:");
  PRINTDEBUG(WiFi.localIP());
}





//// Helper routine to check authorization without WiFi connection
//bool check_auth(byte *buffer, byte bufferSize) {
//  if(bufferSize < AUTHORIZED_LENGTH)
//    return false;
//  for (byte i = 0; i < AUTHORIZED_LENGTH; ++i) {
//    if(authorized[i] != buffer[i])
//      return false;
//  }
//  return true;
//}

// Helper routine to check authorization
bool check_auth(byte *buffer, byte bufferSize) {
  if(bufferSize < AUTHORIZED_LENGTH)
  {
    PRINTDEBUG("UID is too short");
    return false;
  }
  
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  PRINTDEBUG("Connecting to:");
  PRINTDEBUG(host);
  delay(10);
  // Check connection
  if(!client.connect(host, httpsPort))
  {
    PRINTDEBUG("Connection failed");
    return false;
  }

  // Check fingerprint
  if(client.verify(fingerprint, host))
  {
    PRINTDEBUG("Certificate matches");
  }
  else
  {
    PRINTDEBUG("Certificate does not match");
    return false;
  }

  // URL request
  String url = String(API) + String(PLACE) + String("&key=") + uid_hash(buffer, bufferSize);
  PRINTDEBUG("Requesting URL:");
  PRINTDEBUG(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266Lock\r\n" +
               "Connection: close\r\n\r\n");

  PRINTDEBUG("Request sent");

//  Debugging - Read all the lines of the reply from server and print them to Serial
//  PRINTDEBUG("Response:");
//  PRINTDEBUG("=========");
//  while(client.connected()){
//    String line = client.readStringUntil('\n');
//    PRINTDEBUG(line);
//  }
//  return false;
  
  PRINTDEBUG("Headers:");
  PRINTDEBUG("========");
  while(client.connected())
  {
    String line = client.readStringUntil('\n');
    PRINTDEBUG(line);
    if(line == "\r")
    {
      PRINTDEBUG(line);
      PRINTDEBUG("========");      
      PRINTDEBUG("Headers received");
      break;
    }
  }
  
  String line = client.readStringUntil('\n');

  PRINTDEBUG("Reply was:");
  PRINTDEBUG("==========");
  PRINTDEBUG(line);
  PRINTDEBUG("==========");
  PRINTDEBUG("Closing connection");
  
  if(line == "{\"" + String(PLACE) + "\":true}")    //Check the server response
  {
    PRINTDEBUG("UID is authorized");
    return true;
  }
  else
  {
    PRINTDEBUG("UID is not authorized");
    return false;
  }
}





////////////////////////// SETUP //////////////////////////////////////////////////////
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);     // Initialize serial communications
    delay(100);
    Serial.setDebugOutput(true);
  #endif
  PRINTDEBUG();             // Break line after gibberish

  configTime(1 * 3600, 1 * 3600, "pool.ntp.org", "time.nist.gov");
  delay(5000);
  
  SPI.begin();                          // Init SPI bus
  delay(100);
  mfrc522.PCD_Init();                   // Init MFRC522 
  delay(1000);
//  PRINTDEBUG("Performing Self-Test");
//  PRINTDEBUG(String(mfrc522.PCD_PerformSelfTest()));
//  delay(100);
  PRINTDEBUG("Getting Antenna Gain");
  PRINTDEBUG(String(mfrc522.PCD_GetAntennaGain()));
  delay(100);
  PRINTDEBUG("Setting Max Antenna Gain");
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  delay(100);
  PRINTDEBUG("Getting Antenna Gain");
  PRINTDEBUG(String(mfrc522.PCD_GetAntennaGain()));
  delay(100);
  
  pinMode(BUILTIN_LED, OUTPUT);         // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BUILTIN_LED, HIGH);      // Turn the LED off by making the voltage HIGH
  delay(10);
  
  pinMode(RELAY_PIN, OUTPUT);           // Initialize the RELAY_PIN pin as an output
  digitalWrite(RELAY_PIN, HIGH);         // Turns Relay Off
  delay(10);
  
  connectWifi(ssid, password);          // Try to connect WiFi for 30 seconds
  delay(10);
}





////////////////////////// LOOP //////////////////////////////////////////////////////
void loop() {
  int connectFails = 0;

  //WiFi check
  while(WiFi.status() != WL_CONNECTED)
  {
    connectWifi(ssid, password);          // Try to connect WiFi for 30 seconds
    connectFails++;
    if (connectFails > 4)
      resetSelf();                        // If 2.5 minutes passed with no connection - Reset Self
  }
  
  // Look for new cards
  if(!mfrc522.PICC_IsNewCardPresent())
  {
    delay(50);
    return;
  }
  
  // Select one of the cards
  if(!mfrc522.PICC_ReadCardSerial())
  {
    PRINTDEBUG("Selection failed");
    delay(50);
    return;
  }
  
  // Show some details of the PICC (that is: the tag/card)
  PRINTDEBUG();
  PRINTDEBUG("Card UID:");
  #ifdef DEBUG
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    delay(10);
  #endif
  PRINTDEBUG();
  PRINTDEBUG("UID SHA1 Hash:");
  PRINTDEBUG(" " + uid_hash(mfrc522.uid.uidByte, mfrc522.uid.size));
  
  delay(10);
  if(check_auth(mfrc522.uid.uidByte, mfrc522.uid.size))
  {
    PRINTDEBUG("Authorized");
    digitalWrite(RELAY_PIN, LOW);          // Turns Relay ON
    PRINTDEBUG("Unlock ON");
    delay(3000);                         // Wait 3 seconds
    digitalWrite(RELAY_PIN, HIGH);          // Turns Relay Off
    PRINTDEBUG("Unlock OFF");
  }
  else
  {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on
    PRINTDEBUG("Not Authorized");
    delay(2500);
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH        
  }
}

