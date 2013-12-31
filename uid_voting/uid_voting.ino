/**************************************************************************/
/*! 
    NTAG203 voting proof of concept
    
*/
/**************************************************************************/

//Includes used for NFC reader
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

#include <EEPROM.h>

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);

unsigned int uniqueSetLen = 3;

uint8_t uniqueSet[21] PROGMEM= {
  0x04, 0x88, 0x78, 0xD2, 0x85, 0x32, 0x80,

  0x04, 0x8D, 0x78, 0xD2, 0x85, 0x32, 0x80,

  0x04, 0x81, 0x78, 0xD2, 0x85, 0x32, 0x80,

};

//Need no more than 500 booleans so just use 64 bytes
uint8_t hasVoted[64]; 

void setup(void) {
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  pinMode(3, INPUT);
  digitalWrite(3, HIGH);
  
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
    
  Serial.println("Waiting for an ISO14443A card");
}

void uid_output(void) {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    //Serial.println("Found a card!");
    //Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    if (uidLength == 7) {
      Serial.print(" ");
      for (uint8_t i=0; i < uidLength; i++) 
      {
        if (uid[i] <= 0x0F) { Serial.print(" 0x0"); }
        else { Serial.print(" 0x"); }Serial.print(uid[i], HEX);Serial.print(",");
      }
      Serial.println("");      
    }
    //TODO: Insert beep here
    
    //Serial.print("UID Value: ");
    //for (uint8_t i=0; i < uidLength; i++) 
    //{
    //  Serial.print(" 0x");Serial.print(uid[i], HEX); 
    //}
    //Serial.println("");
    // Wait 1 second before continuing
    delay(1000);
  }
  else
  {
    // PN532 probably timed out waiting for a card
    //Serial.println("Timed out waiting for a card");
  }
}

void castBallot(uint8_t hwID[7]) {
  boolean found = false;
  for (unsigned int i=0; i<uniqueSetLen; i++) {
    for (unsigned char d=0; d<7; d++) {
      if (hwID[d] != pgm_read_byte(uniqueSet+(7*i)+d)) {
        //Byte doesn't match
        break;
      }
      if (d == 6) {
        //Need a flag to get out of nested loops
        found = true;
      }
    }
    if (found) {
      Serial.print(" Found at index: ");Serial.println(i);
      
      //This tag is a valid voting token, now make sure it hasn't already been used to vote
      if ((1<<(i%8)) & (hasVoted[i/8])) {
        //This tag has already voted -- VOTER FRAUD!
        Serial.println("CHEATER -- You've already voted");
        return;
      }
      else {
        //This is a valid vote
        Serial.println("Vote has been cast.");
        //TODO: Record votes
        
        //Set hasVoted bit and write to EEPROM for persistent value
        hasVoted[i/8] |= 1<<(i%8);
        EEPROM.write(i/8, hasVoted[i/8]);
        return;
      }      
    }
  }
  //If we made it this far the UID isn't in the stored list of valid tokens
  Serial.println("Not a valid voting token");
  return;
}

void loop(void) {
  if (digitalRead(2) == LOW) {
    //Jumper is on pin2, enter UID output mode
    Serial.println("UID output mode:");
    while(1) {
      uid_output();
      delay(1000);
    } 
  }
  else {
    //Use normal voting mode
    
    //Test to see if jumper for EEPROM erase is in place
    if (digitalRead(3) == LOW) {
      //Jumper is on pin3, blank 64 bytes of EEPROM
      Serial.println("Blanking first 64 bytes of EEPROM");
      for (unsigned char i=0; i<64; i++) {
        EEPROM.write(i,0x00);
      }
      Serial.println("EEPROM lanking done");
    }
    
    //Fill hasVoted{} boolean array from EEPROM
    for (unsigned char i=0; i<64; i++) {
      hasVoted[i] = EEPROM.read(i);
    }
    
    //Loop forever handling voting events as needed
    while(1) {
      boolean success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
      uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
      
      // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
      // 'uid' will be populated with the UID, and uidLength will indicate
      // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
      
      if (success) {
        Serial.println("Found a card!");
        //NTAG203 has 7-byte UID, ignore all others
        if (uidLength == 7) {
          Serial.print(uid[0], HEX);Serial.print(uid[1], HEX);Serial.print(uid[1], HEX);Serial.print(uid[2], HEX);Serial.print(uid[3], HEX);Serial.print(uid[4], HEX);Serial.print(uid[5], HEX);Serial.println(uid[6], HEX);
          
          castBallot(uid);

          
  
          // Wait 1 second before continuing to avoid multiple reads
          delay(1000);
        }
        else { 
          Serial.println("Wrong UID Length");
          delay(1000);
        }
      }
    }
  }
}
