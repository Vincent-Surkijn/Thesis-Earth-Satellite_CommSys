#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define SYNC_WORD B11000000

//Global variables
char *authToken = "00000000";
short seqNr = 0;
char Rxbuff[255];

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

  Serial.println("FinalFramesGroundStation_CC1101");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    /******Alternative method with buildFrame functions**********
    // Read input
    char first = Serial.read();
    String buff;
    // Build frame depending on input
    if(first == 'S')  buff = buildStatusFrame(Serial.readString());
    else if(first == 'C')  buff = buildControlFrame(Serial.readString());
    else Serial.println("Error: invalid input, must start with an S or C");
    */

    // Read input
    String buff = Serial.readString();
    
    // Transmit data
    char msg[100];
    buff.toCharArray(msg,100);
    // Error check
    Serial.print("Msg:");
    Serial.println(msg);
    ELECHOUSE_cc1101.SendData(msg,100);
  }

  // Check RX buffer
  checkReceiver();
}

void checkReceiver(){
  //Serial.println("Checking receiver");
  if (ELECHOUSE_cc1101.CheckRxFifo(100)){
      if (ELECHOUSE_cc1101.CheckCRC()){    //CRC Check. If "setCrc(false)" crc returns always OK!
        //Receive and store message
        short len = ELECHOUSE_cc1101.ReceiveData(Rxbuff);
        Rxbuff[len] = '\0';
        Serial.print("Message:");
        Serial.println((char *) Rxbuff);
      }
      else{
        Serial.print("CRC check failed on msg ");
        Serial.println(seqNr);
      }
    }
}

// Control frame: |preamble|sync|length|Seq Nr|Token|Payload|CRC|
String buildControlFrame(String msg){
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
String buildStatusFrame(String msg){  
  String temp;
  char TxBuffStatus[255];

  // Assign sequence number
  temp = String(seqNr);

  // Assign message
  temp.concat(msg);

  // Error check
  //Serial.print("temp:");
  //Serial.println(temp);

  return temp;
}
