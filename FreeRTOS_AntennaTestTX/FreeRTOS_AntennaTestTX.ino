#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <Arduino_FreeRTOS.h>

// define FreeRTOS tasks
void TaskTransmit( void *pvParameters );
void TaskBlink( void *pvParameters );

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
    ELECHOUSE_cc1101.setCrc(1);     // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.

  // Now set up tasks
  xTaskCreate(
    TaskTransmit
    ,  "Transmit"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

    xTaskCreate(
    TaskBlink
    ,  "Blink"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

    Serial.println("Tx Mode");
}


void loop() {

}

void TaskTransmit(void *pvParameters)  // This is a task.
{
  // Init task variables
  (void) pvParameters;
  int count = 0;  // for counting messages
  
  for(;;){  // Infinite loop
    count++;  // increment counter
    // Concatenate counter and message in String datatype (because it has .concat method)
    String buff = "Message ";
    String temp = String(count);
    buff.concat(temp);
    //Serial.print("Buff: ");
    //Serial.println(buff);
    // Convert to char array for SendData method
    char c[50];
    buff.toCharArray(c,50);
    //Serial.print("c: ");
    //Serial.println(c);
    // Send data
    ELECHOUSE_cc1101.SendData(c,20);
    // wait for one second
    vTaskDelay(1000 / portTICK_PERIOD_MS );
  }
}


// For testing
void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  for (;;) // A Task shall never return or exit.
  {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
  }
}
