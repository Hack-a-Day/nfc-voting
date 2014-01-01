/**************************************************************************/
/*! 
    NTAG203 voting proof of concept
    
    NFC/NDEF Libraries:
    https://github.com/Seeed-Studio/PN532
    Rotary Encoder Code adapted from:
    http://www.circuitsathome.com/mcu/rotary-encoder-interrupt-service-routine-for-avr-micros
    Piezo Buzzer adapted from:
    http://arduino.cc/en/Tutorial/Tone  
*/
/**************************************************************************/

//*******************MISC Settings*************
#include <avr/eeprom.h>
#include "/home/mike/compile/nfc-voting/uid_voting/uniqueSet.h"

//Need no more than 500 booleans so just use 64 bytes
uint8_t hasVoted[64];

uint16_t ballotCount[3];

/* //Testing:
unsigned int uniqueSetLen = 3;

uint8_t uniqueSet[21] PROGMEM= {
  0x04, 0x88, 0x78, 0xD2, 0x85, 0x32, 0x80,

  0x04, 0x8D, 0x78, 0xD2, 0x85, 0x32, 0x80,

  0x04, 0x81, 0x78, 0xD2, 0x85, 0x32, 0x80,

};
*/
//***************End MISC Settings*************

//*******************NFC Reader Settings*************
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);
//***************End NFC Reader Settings*************


//*******************Rotary Encoder Settings*************
/* Rotary encoder */
#define ENC_A 0
#define ENC_B 1
#define ENC_PORT PINC

/* Atmega328 */
/* encoder port */
#define ENC_CTL	DDRC	//encoder port control
#define ENC_WR	PORTC	//encoder port write	
#define ENC_RD	PINC	//encoder port read
#define ENC_A 0	
#define ENC_B 1	

volatile uint8_t selected_option = 0;
//***************End Rotary Encoder Settings*************

//*******************Tone Settings*************

//***************End Tone Settings*************

//*******************HD44780 Settings*************

//***************End HD44780 Settings*************

//*******************LED Settings*************

//***************End LED Settings*************


/********************************
  Initialize the pin change interrupt for the rotary encoder
********************************/
void initEncoder(void) {
  /* Setup encoder pins as inputs */
  ENC_WR |= (( 1<<ENC_A )|( 1<<ENC_B ));    //turn on pullups
  PCMSK1 |= (( 1<<PCINT8 )|( 1<<PCINT9 ));  //enable encoder pins interrupt sources
  
  /* enable pin change interupts */
  PCICR |= ( 1<<PCIE1 );
}

/********************************
  Initialize pins for use as jumpers
********************************/
void initJumpers(void) {
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  pinMode(3, INPUT);
  digitalWrite(3, HIGH);
}

/********************************
  Initialize serial terminal output
********************************/
void initSerial(void) {
  Serial.begin(115200);
  Serial.println("Hello!");
}

/********************************
  Initialize the PN532 NFC reader
********************************/
void initNFC(void) {
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

/********************************
  uid_output is used to read in the UID of NTAG203 tags
  
  one C Header formatted line will be printed to the
    terminal for each tag that is read. This is the
    technique used to generate the uniqueSet.h file
********************************/
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

/********************************
  prints vote count to the terminal
********************************/
void printTally(void) {
  Serial.print("A: ");Serial.print(ballotCount[0]);Serial.print(" B: ");Serial.print(ballotCount[1]);Serial.print(" C: ");Serial.println(ballotCount[2]);
}

/********************************
  This function is called whenever a 7-byte UID is read
  
  -Checks to make sure UID is in the uniqueSet.h file
  -Uses index from the header file entry to verify
    the tag has not already voted using EEPROM array
  -If this is a valid ballot it updates the EEPROM
    array as well as EEPROM ballot total
********************************/
void castBallot(uint8_t hwID[7], uint8_t vote) {
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
        ballotCount[vote] += 1;
        //TODO: save ballotCount to EEPROM
        eeprom_write_word((uint16_t*)(64 + (2*vote)), ballotCount[vote]);
        printTally();        
        
        //Set hasVoted bit and write to EEPROM for persistent value
        hasVoted[i/8] |= 1<<(i%8);
        eeprom_write_byte((uint8_t*)(i/8), hasVoted[i/8]);
        return;
      }      
    }
  }
  //If we made it this far the UID isn't in the stored list of valid tokens
  Serial.println("Not a valid voting token");
  return;
}

/********************************
  Called if a jumper is shorted to ground
  
  Writes zeros to EEPROM for voting array
    as well as ballot totals
********************************/
void zeroEEPROM(void) {
  Serial.println("Blanking first 64 bytes of EEPROM");
  //Zero out space for tracking used token index
  for (uint8_t i=0; i<64; i++) {
    eeprom_write_byte((uint8_t*)i,0x00);
  }
  //Zero out space for tallying the ballots
  //Address 64 if the first free area after the token index array
  eeprom_write_word((uint16_t*)(64), 0x00);
  eeprom_write_word((uint16_t*)(66), 0x00);
  eeprom_write_word((uint16_t*)(68), 0x00);
  Serial.println("EEPROM blanking done");
}

/********************************
  Fill arrays in RAM from EEPROM values
********************************/
void fillArrays(void) {
  //Fill hasVoted{} boolean array from EEPROM
  for (unsigned char i=0; i<64; i++) {
    hasVoted[i] = eeprom_read_byte((uint8_t*)i);
  }
  
  //Fill ballotCount{} array from EEPROM
  for (unsigned char i=0; i<3; i++) {
    ballotCount[i] = eeprom_read_word((uint16_t*)(64 + (2*i)));; 
  }
}

/********************************
  Setup Function
********************************/
void setup(void) {
  initSerial();
  initEncoder();
  initJumpers();
  initNFC();
}

/********************************
  Main function
********************************/
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
      zeroEEPROM();
    }
    
    fillArrays();
    printTally();
    
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
          Serial.print(uid[0], HEX);Serial.print(uid[1], HEX);Serial.print(uid[2], HEX);Serial.print(uid[3], HEX);Serial.print(uid[4], HEX);Serial.print(uid[5], HEX);Serial.println(uid[6], HEX);
          
          castBallot(uid, selected_option);

          
  
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

/********************************
  Increments the voting selection
********************************/
void incSelOpt(void) {
  if (++selected_option > 2) { selected_option = 0; }
}

/********************************
  Decrements the voting selection
********************************/
void decSelOpt(void) {
  //Postfix checks before decrement
  if (selected_option-- == 0 ) { selected_option = 2; }
}

/********************************
  Pin Change interrupt service handler used for rotary encoder
********************************/
ISR(PCINT1_vect)
{
  static uint8_t old_AB = 3;  //lookup table index
  static int8_t encval = 0;   //encoder value  
  static const int8_t enc_states [] PROGMEM = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};  //encoder lookup table
  /**/
  old_AB <<=2;  //remember previous state
  old_AB |= ( ENC_RD & 0x03 );
  encval += pgm_read_byte(&(enc_states[( old_AB & 0x0f )]));
  /* post "Navigation forward/reverse" event */
  if( encval > 3 ) {  //four steps forward
    incSelOpt();
    Serial.print("Up ");Serial.println(selected_option);
    encval = 0;
  }
  else if( encval < -3 ) {  //four steps backwards
    decSelOpt();
    Serial.print("Dn ");Serial.println(selected_option);
    encval = 0;
  }
}
