
//---------------------------------------- Libraries 
//--------- FreeRTOS
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

//--------- ADS1115
#include<ADS1115_WE.h> 
#include<Wire.h>


//--------- MQTT
#include <Ethernet.h>
#include <PubSubClient.h>


//---------------------------------------- Global Variables
//--------- FreeRTOS
QueueHandle_t QueueRawDatas[2];
QueueHandle_t QueuePayloadMQTT[2];

TaskHandle_t TaskSensor0;
TaskHandle_t TaskSensor1;
TaskHandle_t TaskSensor2;
TaskHandle_t TaskSensor3;

TaskHandle_t TaskFilter0;
TaskHandle_t TaskFilter1;
TaskHandle_t HTask_PubMQTT; 


typedef struct 
{
  int8_t sender;
  float  filtered_data;
}payload;

float RawData;
#define Period_Task_Read_Sensor 2500
#define MAX_Ticks_Queue Period_Task_Read_Sensor/2

//--------- ADS1115
#define I2C_ADDRESS 0x48
ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);

//--------- MQTT
// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(172, 16, 0, 100);
IPAddress server(172, 16, 0, 2);

EthernetClient ethClient;
PubSubClient client(server, 1883,ethClient);

//---------------------------------------- Protype of tasks and readChannel function
void Task_Read_Sensor_0(void *pvParameters);
void Task_Read_Sensor_1(void *pvParameters);
void Task_Read_Sensor_2(void *pvParameters);
void Task_Read_Sensor_3(void *pvParameters);
void Task_Filter       (void *pvParameters);

float readChannel(ADS1115_MUX channel, int i) {
  
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  voltage = adc.getResult_V();

  return voltage;
}

//---------------------------------------- Void Setup
void setup() {
//--------- ADS1115
{

  Wire.begin();
  Serial.begin(9600);
  if(!adc.init()){
    Serial.println("ADS1115 not connected!");
  }

  adc.setVoltageRange_mV(ADS1115_RANGE_6144); //comment line/change parameter to change range
  adc.setCompareChannels(ADS1115_COMP_0_GND); //comment line/change parameter to change channel
  adc.setMeasureMode(ADS1115_CONTINUOUS); //comment line/change parameter to change mode
  Serial.println("ADS1115 Example Sketch - Continuous Mode");
  Serial.println("All values in volts");
  Serial.println();
}

//--------- MQTT
{
  Ethernet.begin(mac, ip);
  // Note - the default maximum packet size is 128 bytes. If the
  // combined length of clientId, username and password exceed this use the
  // following to increase the buffer size:
  // client.setBufferSize(255);
  client.connect("ESP32");
}

//--------- FreeRTOS
  {
 

 
//---------- Creation of Queues
  //--------Core 0
    QueueRawDatas[0] = xQueueCreate(8,sizeof(float)); //Queue of 8 floats
    QueuePayloadMQTT[0] = xQueueCreate(1,sizeof(payload)); //Queue of 1 Payload


  //--------Core 1
    QueueRawDatas[1] = xQueueCreate(8,sizeof(float)); //Queue of 8 floats
    QueuePayloadMQTT[1] = xQueueCreate(1,sizeof(payload)); //Queue of 1 Payload


//--------------------------------------- Creation of Tasks     

//------------------------------------Core 0

        //--------------Create Task Read Sensor 0
    xTaskCreatePinnedToCore(Task_Read_Sensor, // Task function
                "Task_Read_Sensor_0", // Task name
                4096,  // Stack size
                (int8_t*)0, 
                1, // Priority
                &TaskSensor0,
                0);
    

          //--------------Create Task Read Sensor 1
    xTaskCreatePinnedToCore(Task_Read_Sensor, // Task function
                "Task_Read_Sensor_1", // Task name
                4096,  // Stack size
                (int8_t*)1, 
                1, // Priority
                &TaskSensor1,
                0);
            
    vTaskSuspend(TaskSensor1);


    //--------------Create Task Filter
    xTaskCreatePinnedToCore(
                Task_Filter, // Task function
                "Task_Filter0", // A name just for humans
                4096,  // Stack size
                (bool*)0, 
                1, // Priority
                &TaskFilter0,
                0 //
                );


    //--------------Create Task Pub
    xTaskCreatePinnedToCore(
                Task_PubMQTT, // Task function
                "Task_PubMQTT", // A name just for humans
                4096,  // Stack size
                NULL, 
                1, // Priority
                &HTask_PubMQTT,
                0 //
                );
    vTaskSuspend(HTask_PubMQTT);



//---------------------------------------- Core 1
    xTaskCreatePinnedToCore(
                Task_Filter, // Task function
                "Task_Filter1", // A name just for humans
                4096,  // Stack size
                (bool*)1, 
                1, // Priority
                &TaskFilter1,
                1 //
                );

          //--------------Create Task Read Sensor 2
    xTaskCreatePinnedToCore(Task_Read_Sensor, // Task function
                "Task_Read_Sensor_2", // Task name
                4096,  // Stack size
                (int8_t*)2, 
                1, // Priority
                &TaskSensor2,
                1);
    


          //--------------Create Task Read Sensor 3
    xTaskCreatePinnedToCore(Task_Read_Sensor, // Task function
                "Task_Read_Sensor_3", // Task name
                4096,  // Stack size
                (int8_t*)3, 
                1, // Priority
                &TaskSensor3,
                1);
    vTaskSuspend(TaskSensor3);
   


  }
}
//---------------------------------------- Void loop
void loop(){}


//---------------------------------------- Tasks Declarations


//------------------------------------------------------------
void Task_Read_Sensor(void *pvParameters)
{
  int8_t numero = (int)pvParameters;
  while(1){ 

printf("Task %d comecou\n",numero);
/*
For 8 times read sensor and send to queue

OBSERVATION: I did not used "for" coding structure because
once i know the number of times i want to repeat the structure and it is small.
is not needed use "for", once this one use Type-B,Type-J, and Type-R operations at Assembly Level.
Therefore, this part of the project was optimazed like that.

And the keys are for hide the whole structure once wanted
*/
if (numero==0 || numero==1)
{
/*1*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 1 da queue 0\n",numero,RawData);
/*2*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 2 da queue 0\n",numero,RawData);
/*3*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 3 da queue 0\n",numero,RawData);
/*4*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 4 da queue 0\n",numero,RawData);
/*5*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 5 da queue 0\n",numero,RawData);
/*6*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 6 da queue 0\n",numero,RawData);
/*7*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 7 da queue 0\n",numero,RawData);
/*8*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[0], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 8 da queue 0\n",numero,RawData);
}
else
{
/*1*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 1 da queue 1\n",numero,RawData);
/*2*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 2 da queue 1\n",numero,RawData);
/*3*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 3 da queue 1\n",numero,RawData);
/*4*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 4 da queue 1\n",numero,RawData);
/*5*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 5 da queue 1\n",numero,RawData);
/*6*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 6 da queue 1\n",numero,RawData);
/*7*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 7 da queue 1\n",numero,RawData);
/*8*/RawData = readChannel(ADS1115_COMP_0_GND,0);xQueueSend(QueueRawDatas[1], &RawData, MAX_Ticks_Queue);printf("Task %d enviou %f para o elemento 8 da queue 1\n",numero,RawData);
}

// Instead of use "switch" structure was used if/else for code optimization
 if      (numero==0) {vTaskResume(TaskSensor1);puts("Suspende T0 chama T1\n\n");}
 else if (numero==1) {vTaskResume(TaskSensor0);puts("Suspende T1 chama T0\n\n");}
 else if (numero==2) {vTaskResume(TaskSensor3);puts("Suspende T2 chama T3\n\n");}
 else                {vTaskResume(TaskSensor2);puts("Suspende T3 chama T2\n\n");}
  
  vTaskSuspend(NULL);

}}




//------------------------------------------------------------
void Task_Filter(void *pvParameters)
{ 
  
  bool numero = (bool)pvParameters;
  bool  sender=1;
  while(1){

  sender=!sender;
  float valueFromQueue=0;
  float FinalvalueFromQueue=0;

/*
----------------- Start Filtering 
For 8 times read sensor and send to queue

OBSERVATION: I did not used "for" coding structure because
once i know the number of times i want to repeat the structure and it is small.
is not needed use "for", once this one use Type-B,Type-J, and Type-R operations at Assembly Level.
Therefore, this part of the project was optimazed like that.

And the keys are for hide the whole structure once wanted.
*/

// If parameter is zero, then act like filter0, else act like filter1
if(!numero)
{  
/*1*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*2*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*3*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*4*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*5*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*6*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*7*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*8*/xQueueReceive(QueueRawDatas[0], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;

// Send filtered value from ADC to MQTT Queue 
FinalvalueFromQueue/=8.0;
payload pacote;
pacote.filtered_data=FinalvalueFromQueue;


    if(!sender) pacote.sender=0;
    else{pacote.sender=1;}

    puts("*****************************************************************************");
    printf("Pacote enviado para Queue MQTT:\nSensor: %d   Dado Filtrado: %f\n",pacote.sender,pacote.filtered_data);
    puts("*****************************************************************************");
    puts("");
    xQueueSend(QueuePayloadMQTT[0], &pacote, MAX_Ticks_Queue);


//********************************************** Call for Task Pub MQTT **********************************

//If both elements from queue are full, then publish call task PubMQTT
if (!uxQueueSpacesAvailable(QueuePayloadMQTT[0]) && !uxQueueSpacesAvailable(QueuePayloadMQTT[1])) {vTaskResume(HTask_PubMQTT);}
else{puts("Nao chamou Pub MQTT");}
    
}
else
{
/*1*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*2*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*3*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*4*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*5*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*6*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*7*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;
/*8*/xQueueReceive(QueueRawDatas[1], &valueFromQueue, MAX_Ticks_Queue);FinalvalueFromQueue+=valueFromQueue;

// Send filtered value from ADC to MQTT Queue 
FinalvalueFromQueue/=8.0;
payload pacote;
pacote.filtered_data=FinalvalueFromQueue;


    if(!sender) pacote.sender=2;
    else{pacote.sender=3;}
    puts("*****************************************************************************");
    printf("Pacote enviado para Queue MQTT:\nSensor: %d   Dado Filtrado: %f\n",pacote.sender,pacote.filtered_data);
    puts("*****************************************************************************");
    puts("");
    xQueueSend(QueuePayloadMQTT[1], &pacote, MAX_Ticks_Queue);

}
    
}}


//------------------------------------------------------------

void Task_PubMQTT(void * pvParameters)
{while(1){

puts("**************************************************************");
puts("Entrou na Pub MQTT");
puts("**************************************************************");

vTaskSuspend(TaskSensor0);
vTaskSuspend(TaskSensor1);
vTaskSuspend(TaskSensor2);
vTaskSuspend(TaskSensor3);
vTaskSuspend(TaskFilter0);
vTaskSuspend(TaskFilter1);

//------- starting MQTT's cycle

client.loop();

payload FinalvalueFromQueue;

//--------------------------------------------------------------------------------- Core 0
  xQueueReceive(QueuePayloadMQTT[0], &FinalvalueFromQueue, MAX_Ticks_Queue);

printf("Valor QueuePayloadMQTT[0]: Sensor %d Dado Filtrado %f\n", FinalvalueFromQueue.sender,FinalvalueFromQueue.filtered_data);

  if (!FinalvalueFromQueue.sender)
        {client.publish("Sensor 0", String(FinalvalueFromQueue.filtered_data).c_str());printf("\n\nSensor 0  valor final %f\n\n",FinalvalueFromQueue.filtered_data);vTaskResume(TaskSensor0);}
  
  else if (FinalvalueFromQueue.sender)  
        {client.publish("Sensor 1", String(FinalvalueFromQueue.filtered_data).c_str());printf("\n\nSensor 1  valor final %f\n\n",FinalvalueFromQueue.filtered_data);vTaskResume(TaskSensor1);}

 vTaskResume(TaskFilter0); 

//--------------------------------------------------------------------------------- Core 1
  xQueueReceive(QueuePayloadMQTT[1], &FinalvalueFromQueue, MAX_Ticks_Queue);

printf("Valor QueuePayloadMQTT[1]: Sensor %d Dado Filtrado %f \n", FinalvalueFromQueue.sender,FinalvalueFromQueue.filtered_data);

  if (FinalvalueFromQueue.sender==2)
        {client.publish("Sensor 2", String(FinalvalueFromQueue.filtered_data).c_str());printf("\n\nSensor 2  valor final %f\n\n",FinalvalueFromQueue.filtered_data);vTaskResume(TaskSensor2);}
  
  else if (FinalvalueFromQueue.sender==3)
        {client.publish("Sensor 3", String(FinalvalueFromQueue.filtered_data).c_str());printf("\n\nSensor 3  valor final %f\n\n",FinalvalueFromQueue.filtered_data);vTaskResume(TaskSensor3);}

  vTaskResume(TaskFilter1);
    
    puts("**************************************************************");
    puts("Saindo da PubMQTT");
    puts("**************************************************************");
    puts("");

  vTaskSuspend(NULL); 
    
}}

