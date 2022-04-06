#include <ELECHOUSE_CC1101_SRC_DRV.h>

//Global variables
byte authToken[8] = {0,0,0,0,0,0,0,0};
byte seqNr = 0;
char res[64];

void setup() {

    Serial.begin(9600);
    while(!Serial);  // Wait for communications to be running
    
    if (ELECHOUSE_cc1101.getCC1101()){        // Check the CC1101 Spi connection.
      Serial.println("Connection OK");
    }else{
      Serial.println("Connection Error");
    }
 
    ELECHOUSE_cc1101.Init();              // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setCCMode(1);       // CRC enabled, var pckt length mode, also baud rate and BW
    ELECHOUSE_cc1101.setModulation(0);  // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(433.92);   // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setSyncMode(2);  // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setPA(12);      // set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    ELECHOUSE_cc1101.setCrc(1);     // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.

    Serial.println("Setup done");
}

void loop() {
  //Serial.print("number:");
  //Serial.println(seqNr);
  buildControlFrame("Hello World!");

  Serial.print("Res:");
  Serial.print(byte(res[0]));
  Serial.print("|");
  
  for(int i = 0; i< 8; i++)  Serial.print(byte(res[i+1]));//
  Serial.print("|");
  
  Serial.print(strlen(res));
  //for(int i = 0; i< strlen(res)-(8+1); i++)  Serial.print(res[i+8+1]);  // when receiving length will be returned by rx function
  Serial.println("|");
  
  delay(3000);
}

/******** Functions *********/
// Control frame: |preamble|sync|length|Seq Nr|Token|Payload|CRC|
char *buildControlFrame(char *msg){
  // Assign sequence number
  if(sizeof(seqNr)==1){
    res[0] = seqNr;
  }
  else{
    Serial.println("Error: sequence number is larger than one byte");
    return -1; 
  }
  // Assign authentication token
  if(sizeof(authToken)==8){
    for(int i = 0; i< sizeof(authToken); i++)  res[i+1]=authToken[i];
  }
  else{
    Serial.println("Error: authentication token is larger than 8 bytes");  
    return -1; 
  }
  // Assign message
  if(sizeof(msg)<255){
    for(int i = 0; i< strlen(msg); i++)  res[i+8+1]=msg[i];
  }
  else{
    Serial.println("Error: message is larger than 255 bytes");  
    return -1; 
  }
/*
  // Error check
  Serial.print("Res:");
  Serial.print(byte(res[0]));
  Serial.print("|");
  
  for(int i = 0; i< sizeof(authToken); i++)  Serial.print(byte(res[i+1]));//
  Serial.print("|");
  
  //Serial.print("Size of msg:");
  //Serial.println(sizeof(msg));
  for(int i = 0; i< strlen(msg); i++)  Serial.print(res[i+8+1]);
  Serial.println("|");
 */
  return res;
}

// Status frame: |preamble|sync|length|Seq Nr|Payload|CRC|
char* buildStatusFrame(char* msg){
  char *res = msg;
    
  return res;
}
