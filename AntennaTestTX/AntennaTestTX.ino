#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define SYNC_WORD B11000000

void setup() {

    Serial.begin(9600);
    while(!Serial);  // Wait for communications to be running
    
    if (ELECHOUSE_cc1101.getCC1101()){        // Check the CC1101 Spi connection.
    Serial.println("Connection OK");
    }else{
    Serial.println("Connection Error");
    }
 
    ELECHOUSE_cc1101.Init();              // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setCCMode(1);       // set config for internal transmission mode.
    ELECHOUSE_cc1101.setModulation(0);  // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(433.92);   // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setSyncMode(2);  // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setPA(12);      // set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    ELECHOUSE_cc1101.setCrc(1);     // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.  ELECHOUSE_cc1101.Init();              // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setSyncWord(SYNC_WORD, SYNC_WORD); // set sync word to 1100 0000 1100 0000

    Serial.println("Tx Mode");
}

int count = 0;

void loop() {
  count++;
  char buff[100];
  sprintf(buff,"Message %d",count);
  Serial.print("Buff: ");
  Serial.println(buff);
  ELECHOUSE_cc1101.SendData(buff,100);
  delay(2000);
}
