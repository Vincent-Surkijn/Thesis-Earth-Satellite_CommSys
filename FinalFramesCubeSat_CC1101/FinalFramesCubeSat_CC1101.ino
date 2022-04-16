#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define SYNC_WORD B11000000

//Global variables
short seqNr = 0;
char *authToken = "abcdefgh";
char Rxbuff[64];
char Txbuff[64];
String usr_pswd = "adminpassword";
bool conn = false;
bool encrypted = true;
unsigned long t1;

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
}

void loop() {
  
  // Send a bleep every 4s, unless a connection is made
  if(!conn && millis()-t1>=4000){
    sendBleep();
    // Update t value
    t1 = millis();
  }

  // Check the RX buffer
  checkReceiver();

  // To confirm Arduino is still running at all times
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);   
}


/******** Functions *********/
/*
 * This method checks the RX Fifo
 * Returns 0 if a message was received and decoded correctly
 *         1 if a retransmit should be requested
 *         2 if the token/usr/pswd was invalid
 *         3 if the Fifo was empty
 */
int checkReceiver(){
  //Serial.println("Checking receiver");
  if (ELECHOUSE_cc1101.CheckRxFifo(100)){
      if (ELECHOUSE_cc1101.CheckCRC()){    //CRC Check. If "setCrc(false)" crc returns always OK!
        
        if(encrypted) Serial.println("Received encrypted message");
        else Serial.println("Received unencrypted message");

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
        Serial.print("Sequence nr:");
        Serial.println(receivedSeqNr);

        // Check seq nr
        if(seqNr != receivedSeqNr){
          Serial.println("Error: unexpected seq nr");
          return -1;
        }

        // Check token if necessary & read payload      
        char payload[255];
        int i;
        // Contains token
        if(encrypted){
          if(!checkToken(l))  return 2;
          Serial.print("Payload:");
          for(i = 1; i<= strlen(Rxbuff)-strlen(authToken)-l; i++){
            payload[i-1] = Rxbuff[i+l+strlen(authToken)-1];
            Serial.print(Rxbuff[i+l+strlen(authToken)-1]);
          } 
        }
        // Does not contain token
        else{
          Serial.print("Tokenless Payload:");
          for(i = 1; i<= strlen(Rxbuff)-l; i++){
            payload[i-1] = Rxbuff[i+l-1];
            Serial.print(Rxbuff[i+l-1]);     
          } 
        }
        Serial.println();

        // Pass payload if connection & authentication already established
        if(conn){
          // Pass payload to handler
          payload[i-1] = '\0';
          payloadHandler(payload);
        }        
        // Check authentication if first message
        else{
          // If not correct, return 2
          if(!checkAuth(l))  return 2;
          // Else send token and stop transmitting bleeps
          else{
            transmitToken();
            conn = true;
          }
        }
        
        // Increment sequence number, loop back to 0 if 255
        if(seqNr<255){
          seqNr++;
        }
        else{
          seqNr = 0;
        }
        
        return 0;
      }
      else{
        Serial.print("CRC check failed on msg ");
        Serial.println(seqNr);
        return 1;
      }
    }

    return 3;
}

/*
 * This method reads the message payload and acts upon it
 * Parameter char *payload: contains the payload of the received message
 * Returns true if successful, false if not
 */
bool payloadHandler(char *payload){
  //Serial.print("Passed payload:");
  //Serial.println(payload);
  // Check payload
  if(strcmp(payload,"adminpassword")==0)  return true;
  if(strcmp(payload,"deploy panels")==0){
    if(deploySolarPanels()){
      // Transmit confirmation message
      String temp = buildControlFrame("Panels deployed");
      temp.toCharArray(Txbuff,64);
      ELECHOUSE_cc1101.SendData(Txbuff,64);
      Serial.print("Payload handler sent ");
      Serial.println(Txbuff);
      // Return true for success
      return true;
    }
    else return false;
  }
  if(strcmp(payload,"unencrypted mode")==0){
    // Unencrypted messages can't enable unencrypted mode
    if(!encrypted){
      Serial.println("Error: unencrypted message tried to enable unencrypted mode");
      return false;
    }
    else{
      // Transmit success message
      String temp = buildControlFrame("unencrypted mode enabled");
      temp.toCharArray(Txbuff,64);
      ELECHOUSE_cc1101.SendData(Txbuff,64);
      Serial.print("Payload handler sent ");
      Serial.println(Txbuff);
  
      // Disable encryption
      encrypted = false;
      
      // Return true for success
      return true;
    }
  }
  if(strcmp(payload,"battery level")==0){
    // Transmit battery level
    String temp = buildStatusFrame("50%");
    temp.toCharArray(Txbuff,64);
    ELECHOUSE_cc1101.SendData(Txbuff,64);
    Serial.print("Payload handler sent ");
    Serial.println(Txbuff);
    
    // Return true for success
    return true;
  }
  if(strcmp(payload,"END")==0){
    // Terminate connection
    if(endConnection()){
      // Transmit success message
      String temp = buildControlFrame("END");
      temp.toCharArray(Txbuff,64);
      ELECHOUSE_cc1101.SendData(Txbuff,64);
      Serial.print("Payload handler sent ");
      Serial.println(Txbuff);
      
      // Return true for success
      return true;
    }
    else  return false;
  }
  
  Serial.println("Error: unrecognised payload");
  return false;
}

/*
 * This method terminates the connection
 * Returns true if successful, false if not
 */
bool endConnection(){
  if(encrypted){
    // Connection ended
    conn = false;
    // Re-enable encryption
    encrypted = true;
  
    Serial.println("Connection terminated");

    return true;
  }
  else{
    Serial.println("Error: unencrypted message tried to terminate connection");
    return false;
  }
}

/*
 * This method mimics the deploying of the solar panels
 * Returns true if successful, false if not
 */
bool deploySolarPanels(){
  if(!encrypted){
    Serial.println("Error: unencrypted message tried to deploy solar panels!");
    return false;
  }
  else{
    Serial.println("Deployed solar panels");
    return true;
  }
}

/*
 * This method transmits the authentication token in a Control frame
 */
void transmitToken(){
  // Build frame
  String msg = buildControlFrame("");
  msg.toCharArray(Txbuff,64);
  // Send data
  ELECHOUSE_cc1101.SendData(Txbuff,64);
  Serial.print("Sent token:");
  Serial.println(Txbuff);
}

/*
 * This method checks the authentication frame
 * Parameter byte l: contains length of expected sequence number
 * Returns true if correct usr&pswd, false if incorrect
 */
bool checkAuth(byte l){
    Serial.print("Auth:");
    char auth[strlen(Rxbuff)-strlen(authToken)-l];
    for(int i = 1; i<= strlen(Rxbuff)-strlen(authToken)-l; i++){
      auth[i-1] = Rxbuff[i+l+strlen(authToken)-1];
      Serial.print(Rxbuff[i+l+strlen(authToken)-1]);
    }
    auth[strlen(Rxbuff)-strlen(authToken)-l]='\0';
    Serial.println();
    
    char temp[50];
    usr_pswd.toCharArray(temp,50);
    //Serial.print("Temp:");
    //Serial.println(temp);
    if(strcmp(auth,temp)!=0){
      Serial.println("Error: invalid authentication");
      return false;
    }

    return true;
}

/*
 * This method checks the authentication token of a frame
 * Parameter byte l: contains length of expected sequence number
 * Returns true if correct token, false if incorrect
 */
bool checkToken(byte l){
    Serial.print("Token:");
    char token[8];
    for(int i = 1; i<= strlen(authToken); i++){
      token[i-1] = Rxbuff[i+l-1];
      Serial.print(Rxbuff[i+l-1]);
    }
    token[8] = '\0';
    Serial.println();
    // If first message, token is all zeros
    if(!conn){
      if(strcmp(token,"00000000")!=0){
        Serial.println("Error: invalid token");
        return false;
      }
      else return true;
    }
    if(strcmp(token,authToken)!=0){
      Serial.println("Error: invalid token");
      return false;
    }

    return true;
}

/*
 * This method sends the bleep message
 */
void sendBleep(){
  // Build frame
  char bleep[20]; 
  // Set sequence number to -1 because it is incremented in buildStatusFrame()
  seqNr = -1;
  (buildStatusFrame("Status update")).toCharArray(bleep,20);

  // Send data
  ELECHOUSE_cc1101.SendData(bleep,20);

  // Error check
  Serial.print("Bleep:");
  Serial.println(bleep);
  Serial.println("Sent unencrypted bleep");
}

// Control frame: |preamble|sync|length|Seq Nr|Token|Payload|CRC|
String buildControlFrame(char *msg){
  String temp;

  // Error check
  if(!encrypted)  Serial.println("Error: unencrypted Control frame");
  
  // Increment seq Nr before building
  seqNr++;
  
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
  
  // Increment seq Nr before building
  seqNr++;
  
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
