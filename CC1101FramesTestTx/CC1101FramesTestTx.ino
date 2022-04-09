#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#define SYNC_WORD B11000000

/******** TODO ************/
/*
 * Seq nr check
 * Comparison ACK vs Seq Nr
 */

//Global variables
char *authToken = "00000000";
short seqNr = 100;
char TxBuffCtrl[64];
char TxBuffStatus[255];
char Rxbuff[255];

// define FreeRTOS tasks
void TaskTransmit( void *pvParameters );
void TaskReceive( void *pvParameters );
void TaskBlink( void *pvParameters );

TaskHandle_t xHandleTransmit;
TaskHandle_t xHandleReceive;

SemaphoreHandle_t  TxBuffMutex;

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

  TxBuffMutex = xSemaphoreCreateMutex();
  if (TxBuffMutex == NULL) {
    Serial.println("Mutex creation failed");
  }
  xSemaphoreGive(TxBuffMutex); // release mutex
  
  // Now set up tasks
  xTaskCreate(
    TaskTransmit
    ,  "Transmit"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &xHandleTransmit );

    /*xTaskCreate(
    TaskReceive
    ,  "Receive"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &xHandleReceive );*/

  Serial.println("Setup done");
}

void loop() {
  // Empty, everything is done in tasks
  
}

/******** Tasks *********/
/*
 * This task is responsible for regularly transmitting the Status frames
 */
void TaskTransmit(void *pvParameters)  // This is a task.
{
  // Init task variables
  (void) pvParameters;
  
  for(;;){  // Infinite loop
    Serial.println("TxTask");

    // Build frame
    buildControlFrame("Hello");
    // Send data
    ELECHOUSE_cc1101.SendData(TxBuffCtrl,255);
    
    // Increment sequence number, loop back to 0 if 255
    if(seqNr<255){
      seqNr++;
    }
    else{
      seqNr = 0;
    }
    Serial.println("Message sent");

    // wait for 4 seconds
    vTaskDelay(4000 / portTICK_PERIOD_MS );
  }
}


void TaskReceive(void *pvParameters)  // This is a task.
{
  // Init task variables
  (void) pvParameters;
  
  for(;;){  // Infinite loop
    Serial.println("RxTask");

    // While nothing is received, check every 100 ticks (100*15ms=1500ms=1.5s)
    while(!ELECHOUSE_cc1101.CheckRxFifo(100)){
      vTaskDelay(100);
    }
    //Checks whether something has been received.
    //When something is received we give some time (argument for function) to receive the message in full.(time in millis)
    if (ELECHOUSE_cc1101.CheckRxFifo(100)){
      if (ELECHOUSE_cc1101.CheckCRC()){    //CRC Check. If "setCrc(false)" crc returns always OK!
        Serial.print("Rssi: ");
        Serial.println(ELECHOUSE_cc1101.getRssi());
        Serial.print("LQI: ");
        Serial.println(ELECHOUSE_cc1101.getLqi());
        
        short len = ELECHOUSE_cc1101.ReceiveData(Rxbuff);
        Rxbuff[len] = '\0';
        Serial.println((char *) Rxbuff);
        Serial.println("----END---");
        
        // Increment sequence number, loop back to 0 if 255
        if(seqNr<255){
          seqNr++;
        }
        else{
          seqNr = 0;
        }
        Serial.print("seqNr:");
        Serial.println(seqNr);
      }
      else{
        Serial.print("CRC check failed on msg ");
        Serial.println(seqNr);
      }
    }
    
    // wait for 2 seconds
    vTaskDelay(1000 / portTICK_PERIOD_MS );
  } 
}


/******** Functions *********/
// Control frame: |preamble|sync|length|Seq Nr|Token|Payload|CRC|
void buildControlFrame(char *msg){
  String temp;
  // Clear transmit buffer
  memset(TxBuffCtrl,0,sizeof(TxBuffCtrl));
  
  // Assign sequence number
  temp = String(seqNr);

  // Assign authentication token
  temp.concat(authToken);

  // Assign message
  temp.concat(msg);

  // Assign everything to TxBuff
  temp.toCharArray(TxBuffCtrl,50);

  // Error check
  //Serial.print("temp:");
  //Serial.println(temp);
  Serial.print("TxBuffCtrl:");
  Serial.println(TxBuffCtrl);
}

// Status frame: |preamble|sync|length|Seq Nr|Payload|CRC|
void buildStatusFrame(char* msg){
  // Clear transmit buffer
  memset(TxBuffStatus,0,sizeof(TxBuffStatus));
  
}
