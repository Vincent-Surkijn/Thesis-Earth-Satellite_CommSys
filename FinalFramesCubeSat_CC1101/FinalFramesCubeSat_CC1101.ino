#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define SYNC_WORD B11000000

//Global variables
char *authToken = "00000000";
short seqNr = 0;
char Rxbuff[255];
bool conn = false;
unsigned long t1;
unsigned long t2;

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
  ELECHOUSE_cc1101.setSyncWord(SYNC_WORD, SYNC_WORD); // set sync word to 1100 0000 1100 0000

  Serial.println("FinalFramesCubeSat_CC1101");

  t1 = millis();
  t2 = millis();
}

void loop() {
  // When a connection is made, the bleeps stop
  /*if(!conn && millis()-t1>=4000){
    sendBleep();
    // Update t value
    t1 = millis();
  }*/

  // Check Rx buffer every 50ms
  if(millis()-t2>=50){
    checkReceiver();
    // Update t value
    t2 = millis();
  }
}


/******** Functions *********/
void checkReceiver(){
  //Serial.println("Checking receiver");
  if (ELECHOUSE_cc1101.CheckRxFifo(100)){
      if (ELECHOUSE_cc1101.CheckCRC()){    //CRC Check. If "setCrc(false)" crc returns always OK!
        // Stop transmitting bleeps
        conn = true;

        //Receive and store message
        short len = ELECHOUSE_cc1101.ReceiveData(Rxbuff);
        Rxbuff[len] = '\0';
        Serial.print("Message:");
        Serial.println((char *) Rxbuff);

        // Decode sequence number
        byte l;
        if(seqNr<10)  l = 1;
        else if (seqNr<100) l = 2;
        else  l = 3;
        char temp[l+1];
        //Serial.print("Rxbuff:");
        for(int i = 1; i <= l; i++){
          //Serial.print(Rxbuff[i-1]);
          temp[i-1] = Rxbuff[i-1];
        }
        temp[l] = '\0';
        int receivedSeqNr = atoi(temp);
        Serial.print("Decoded nr:");
        Serial.println(receivedSeqNr);

        // Check values
        if(seqNr != receivedSeqNr)  Serial.println("Error: unexpected seq nr");
        //Serial.print("|");

        for(int i = 1; i<= strlen(authToken); i++)  Serial.print(Rxbuff[i+l-1]);//
        //Serial.print("|");
        
        for(int i = 1; i<= strlen(Rxbuff)-strlen(authToken)-l; i++)  Serial.print(Rxbuff[i+l+strlen(authToken)-1]);
        //Serial.println("|");
        //Serial.println("----END---");
        
        // Increment sequence number, loop back to 0 if 255
        if(seqNr<255){
          seqNr++;
        }
        else{
          seqNr = 0;
        }
        //Serial.print("seqNr:");
        //Serial.println(seqNr);
      }
      else{
        Serial.print("CRC check failed on msg ");
        Serial.println(seqNr);
      }
    }
}

void sendBleep(){
  // Build frame
  char bleep[300]; 
  (buildStatusFrame("Battery:50%")).toCharArray(bleep,300);

  // Send data
  ELECHOUSE_cc1101.SendData(bleep,255);

  // Error check
  //Serial.print("Bleep: ");
  //Serial.println(bleep);
  Serial.println("Message sent");
}

// Control frame: |preamble|sync|length|Seq Nr|Token|Payload|CRC|
String buildControlFrame(char *msg){
  String temp;
  char TxBuffCtrl[64];
  
  // Assign sequence number
  temp = String(seqNr);

  // Assign authentication token
  temp.concat(authToken);

  // Assign message
  temp.concat(msg);

  // Error check
  //Serial.print("temp:");
  //Serial.println(temp);

  return temp;
}

// Status frame: |preamble|sync|length|Seq Nr|Payload|CRC|
String buildStatusFrame(char* msg){  
  String temp;
  char TxBuffStatus[255];

  // Assign sequence number
  temp = String(seqNr);

  // Assign message
  temp.concat(msg);

  // Error check
  Serial.print("temp:");
  Serial.println(temp);

  return temp;
}
