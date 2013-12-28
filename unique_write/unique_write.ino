#include <avr/pgmspace.h>
#include "/home/mike/compile/nfc-voting/unique-string-gen/uniqueSet.h"
// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

unsigned int uniqInt;
unsigned int counter = 0;
// the loop routine runs over and over again forever:
void loop() {
  
  Serial.println("Hello World!");
  delay(1000);        // delay in between reads for stability
  
  uniqInt = pgm_read_word_near(uniqueSet + counter);
  Serial.println(uniqInt);
  Serial.println("hackaday.com/LA2014?" + String(uniqInt, HEX));
  
  if (++counter >= uniqueSetLen) { counter = 0; }

}
