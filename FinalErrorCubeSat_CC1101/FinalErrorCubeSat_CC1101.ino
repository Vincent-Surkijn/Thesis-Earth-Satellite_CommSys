#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define SYNC_WORD B11000000
#define Min_Control_Size 9  // 8 byte token + 1 byte sequence number
#define Min_Status_Size 1   // 1 byte sequence number

/*
 * TODO
 * missed msg -1
 * if rx seq nr is smaller then expected missed message numbers are negative
 * if rx seq nr has more digits then expected (problem only because format is not byte)
 */

//Global variables
short seqNr = 0;
char *authToken = "abcdefgh";
char Rxbuff[64];
char Txbuff[64];
bool encrypted = true;

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
}

void loop() {
  int8_t amount = checkReceiver();
  if(amount < 0){
    //Serial.println("Retransmission requested");
  }
}


/******** Functions *********/
/*
 * This method checks the RX Fifo
 * Returns 0 if a message was received and decoded correctly
 *         1 if the Fifo was empty
 *         -1 retransmission requested
 */
int8_t checkReceiver(){
  if (ELECHOUSE_cc1101.CheckRxFifo(100)){
      if (ELECHOUSE_cc1101.CheckCRC()){    //CRC Check. If "setCrc(false)" crc returns always OK!
        //Receive and store message
        short len = ELECHOUSE_cc1101.ReceiveData(Rxbuff);
        Rxbuff[len] = '\0';
        Serial.print("Message:");
        Serial.println((char *) Rxbuff);
        
        // Check minimum frame size, request retransmit if too short
        if( (!encrypted && len < Min_Status_Size) || (encrypted && len < Min_Control_Size) ) {
          // Increment sequence number, loop back to 0 if 255
          if(seqNr<255) seqNr++;
          else seqNr = 0;
          // Request retransmission of current message
          requestRetransmission(0);
          return -1;
        }

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
        Serial.print("Sequence nr:");
        Serial.println(receivedSeqNr);

        // Check seq nr
        if(seqNr != receivedSeqNr){
          Serial.println("Error: unexpected seq nr");
          // Check amount of missed messages
          signed short amount = receivedSeqNr - seqNr;
          // If amount = negative (expected seq nr > received seq nr), alternative method to check amount
          if(amount<=0){
            if(256 + amount > 50) Serial.println("Missed more than 50 messages");
          }
          if(amount > 50) Serial.println("Missed more than 50 messages");
          
          // Now update sequence number
          if(receivedSeqNr + 1 < 255) seqNr = receivedSeqNr + 1;
          else seqNr = 0;

          // Request retransmission of missed messages
          requestRetransmission(amount);
          
          return -1;
        }
        
        // Increment sequence number, loop back to 0 if 255
        if(seqNr<255) seqNr++;
        else seqNr = 0;
        
        return 0;
      }
      else{
        Serial.print("CRC check failed on msg ");
        Serial.println(seqNr);
        // Increment sequence number, loop back to 0 if 255
        if(seqNr<255) seqNr++;
        else seqNr = 0;
        // Request retransmission of current message
        requestRetransmission(0);
        return -1;
      }
    }
    return 1;
}

void requestRetransmission(short amount){
  if(amount==0){  // If amount is 0, current sequence number should be retransmitted
    // Build message
    String buff = "Missed message Nr:";
    buff.concat(seqNr - 1);
    char msg[25];
    buff.toCharArray(msg,25);
    String temp = buildControlFrame(msg);
    temp.toCharArray(Txbuff,64);
    // Send message
    ELECHOUSE_cc1101.SendData(Txbuff,64);
    Serial.print("Outgoing message:");
    Serial.println(Txbuff);
    // Increment sequence number, loop back to 0 if 255
    if(seqNr<255) seqNr++;
    else seqNr = 0;
    return;
  }
  else if(amount > 0){
    // Save initial received sequence number before sending frames
    short initSeqNr = seqNr;
    for(short i = amount; i > 0; i--){
      // Build message
      String buff = "Missed message Nr:";
      buff.concat(initSeqNr - i - 1);
      char msg[100];
      buff.toCharArray(msg,100);
      String temp = buildControlFrame(msg);
      temp.toCharArray(Txbuff,64);
      // Send message
      ELECHOUSE_cc1101.SendData(Txbuff,64);
      Serial.print("Outgoing message:");
      Serial.println(Txbuff);
      // Update sequence number
      // Increment sequence number, loop back to 0 if 255
      if(seqNr<255) seqNr++;
      else seqNr = 0;
      // Give receiver some time
      delay(50);
    }   
  }
  else{
    // Save initial received sequence number before sending frames
    short initSeqNr = seqNr;
    // Retrieve actual amount of missed messages
    amount = 256 + amount;
    Serial.println("Negative");
    for(short i = 1; i <= amount; i++){
      // Build message
      String buff = "Missed message Nr:";
      if(initSeqNr - 1 - i >= 0)  buff.concat(initSeqNr - 1 - i);
      else  buff.concat(initSeqNr - 1 - i + 256);
      char msg[100];
      buff.toCharArray(msg,100);
      String temp = buildControlFrame(msg);
      temp.toCharArray(Txbuff,64);
      // Send message
      ELECHOUSE_cc1101.SendData(Txbuff,64);
      Serial.print("Outgoing message:");
      Serial.println(Txbuff);
      // Update sequence number
      // Increment sequence number, loop back to 0 if 255
      if(seqNr<255) seqNr++;
      else seqNr = 0;
      // Give receiver some time
      delay(50);
    }
  }
}

// Control frame: |preamble|sync|length|Seq Nr|Token|Payload|CRC|
String buildControlFrame(char *msg){
  String temp;

  // Error check
  if(!encrypted)  Serial.println("Error: unencrypted Control frame");
  
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

  // Status frames should never be encrypted
  encrypted = false;
  
  // Assign sequence number
  temp = String(seqNr);

  // Assign message
  temp.concat(msg);

  // Re-enable encryption after Status frame is built
  encrypted = true;

  // Error check
  //Serial.print("temp:");
  //Serial.println(temp);

  return temp;
}
