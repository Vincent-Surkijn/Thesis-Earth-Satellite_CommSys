/*
   RadioLib AX.25 Transmit Example

   This example sends AX.25 messages using
   SX1278's FSK modem.

   Other modules that can be used for AX.25:
    - SX127x/RFM9x
    - RF69
    - SX1231
    - CC1101
    - SX126x
    - nRF24
    - Si443x/RFM2x

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

/*************
 * 
 * Niet vergeten D2(NANO) aan D0(CC1101) te verbinden!
 * 
 *********************/

// include the library
#include <RadioLib.h>

// NSS pin:   10
// DIO0 pin:  2
CC1101 radio = new Module(10, 2, RADIOLIB_NC, RADIOLIB_NC);

// create AX.25 client instance using the FSK module
AX25Client ax25(&radio);

void setup() {
  Serial.begin(9600);

  // initialize CC1101
  Serial.print(F("[CC1101] Initializing ... "));
  int state = radio.begin();

  // when using one of the non-LoRa modules for AX.25
  // (RF69, CC1101,, Si4432 etc.), use the basic begin() method
  // int state = radio.begin();

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // initialize AX.25 client
  Serial.print(F("[AX.25] Initializing ... "));
  // source station callsign:     "N7LEM"
  // source station SSID:         0
  // preamble length:             8 bytes
  state = ax25.begin("N7LEM");
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

}

void loop() {
  // send AX.25 unnumbered information frame
  Serial.print(F("[AX.25] Sending UI frame ... "));
  // destination station callsign:     "NJ7P"
  // destination station SSID:         0
  int state = ax25.transmit("Hello World!", "NJ7P");
  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

  } else {
    // some error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  delay(1000);
}
