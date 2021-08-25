//===================================================================================================================

//#define DISPLAY_SSD1306Wire
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

#define SERVICE_UUID        "042bd80f-14f6-42be-a45c-a62836a4fa3f"
#define CHARACTERISTIC_UUID "065de41b-79fb-479d-b592-47caf39bfccb"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

#define NOTIFY_CHARACTERISTIC

bool estConnecte = false;
bool etaitConnecte = false;
int valeur = 0; // le compteur
// the number of the LED pin
const int ledPin = 16;  // 16 corresponds to GPIO16

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;
Servo myservo;
const int servoPin = 16;
const int nbFeux = 4;
const int basePinsFeux = 2;
enum EtatFeu {rouge,orange,vert,clignotant};
class UnFeu {
  public :
  UnFeu(int pinRouge,int pinOrange,int pinVert) {
     _pinRouge=pinRouge;
     _pinOrange=pinOrange;
     _pinVert=pinVert;
     pinMode(_pinRouge,OUTPUT);
     pinMode(_pinOrange,OUTPUT);
     pinMode(_pinVert,OUTPUT);
     setEtat(clignotant);
  };
  
  void gereClignotant() {
      if (_etat!=clignotant) return;
      orangeOn=!orangeOn;
      digitalWrite(_pinOrange,orangeOn);
  }

  void setEtat(EtatFeu etat) {
     _etat = etat;
     switch (_etat) {
       case rouge : {
         digitalWrite(_pinRouge,true);
         digitalWrite(_pinOrange,false);
         digitalWrite(_pinVert,false);
         break;
       };
       case orange : {
         digitalWrite(_pinRouge,false);
         digitalWrite(_pinOrange,true);
         digitalWrite(_pinVert,false);
         orangeOn=true;
         break;
       };       
       case vert : {
         digitalWrite(_pinRouge,false);
         digitalWrite(_pinOrange,false);
         digitalWrite(_pinVert,true);
         break;
       }; 
       case clignotant : {
         digitalWrite(_pinRouge,false);
         digitalWrite(_pinOrange,true);
         digitalWrite(_pinVert,false);
         orangeOn=true;
         break;
       }        
     }
   }
  private:
   EtatFeu _etat;
   int _pinRouge;
   int _pinOrange;
   int _pinVert; 
   bool orangeOn;
};

UnFeu feux[4]={{1,2,3},{4,5,6},{7,8,9},{10,11,12}};
TaskHandle_t feuHandle;
void taskFeux(void *pvParameters) {
   (void) pvParameters;
   while(true) {
     for (int i=0;i<nbFeux;i++) feux[i].gereClignotant();
     vTaskDelay(500);
   }
}

void initFeux() {
  
    xTaskCreatePinnedToCore(
      taskFeux, "TaskFeux" // A name just for humans
      ,
      40 * 1024 // This stack size can be checked & adjusted by reading the Stack Highwater
      ,
      NULL,
      2 // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
      ,
      &feuHandle, 0);
      
}
class NotifyCaracteristic : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    //std::string value=pCharacteristic->getValue();
    int value=*(pCharacteristic->getData());
    myservo.write(value);
    Serial.print("Write event :");
    Serial.println(value);
    //Serial.printf("Value %d",*(pCharacteristic->getData()));
  }
};


class EtatServeur : public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) 
    {
      estConnecte = true;
    }

    void onDisconnect(BLEServer* pServer) 
    {
      estConnecte = false;
    }
};

//===================================================================================================================

#ifdef DISPLAY_SSD1306Wire
#include "SSD1306Wire.h"

SSD1306Wire  display(0x3c, 5, 4);
void initDisplay();
void afficherMessage(String msg, int duree);
void afficherDatas(String msg, String datas, int duree);
#endif

//===================================================================================================================

void setup() 
{
  Serial.begin(115200);
  initFeux();
  //ESP32PWM::allocateTimer(0);
	//ESP32PWM::allocateTimer(1);
	//ESP32PWM::allocateTimer(2);
	//ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);// Standard 50hz servo
  myservo.attach(servoPin, 500, 2400);
    // configure LED PWM functionalitites
  //ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  //ledcAttachPin(ledPin, ledChannel);
  Serial.println("Test BLE init");

  #ifdef DISPLAY_SSD1306Wire
  initDisplay();

  afficherMessage("Test BLE init server", 0);
  #endif

  Serial.println("Test BLE init server");  

  BLEDevice::init("MonESP32");
  //BLEDevice::getAddress(); // Retrieve our own local BD BLEAddress
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new EtatServeur());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  #ifdef NOTIFY_CHARACTERISTIC
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE  | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // CrÃ©e un descripteur : Client Characteristic Configuration (pour les indications/notifications)
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new NotifyCaracteristic);
  #else
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  #endif

  pService->start();

  //pServer->getAdvertising()->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
  Serial.println("Test BLE start advertising");

  Serial.println("Test BLE wait connection");

  #ifdef DISPLAY_SSD1306Wire
  afficherMessage("Test BLE wait", 1000);
  #endif
}


void loop() 
{
  
  
  bool fini = false;

  while(!fini)
  { 
    
    /*myservo.write(0);
    delay(5000);
    myservo.write(180);
    delay(5000);
    */
    // notification 
    if (estConnecte) 
    { 
      pCharacteristic->setValue(valeur); // la nouvelle valeur du compteur
      #ifdef NOTIFY_CHARACTERISTIC
      pCharacteristic->notify();  
      #endif      
      delay(1000); // bluetooth stack will go into congestion, if too many packets are sent

      String datas(valeur);
      //Serial.println("BLE notify");
      Serial.printf("BLE notify : %d\n", valeur);
      #ifdef DISPLAY_SSD1306Wire
      afficherDatas("BLE notify", datas, 500);
      #endif
      
      valeur++; // on compte ...
    }
    // dÃ©connectÃ© ?
    if (!estConnecte && etaitConnecte) 
    {
      Serial.println("BLE deconnection");
      #ifdef DISPLAY_SSD1306Wire
      afficherMessage("BLE deconnecte", 500);
      #else
      delay(500); // give the bluetooth stack the chance to get things ready
      #endif
      
      pServer->startAdvertising(); // restart advertising
      Serial.println("BLE restart advertising");

      Serial.println("Test BLE wait connection");
      #ifdef DISPLAY_SSD1306Wire
      afficherMessage("Test BLE wait", 0);
      #endif
      
      etaitConnecte = estConnecte;
    }
    // connectÃ© ?
    if (estConnecte && !etaitConnecte) 
    {
      Serial.println("BLE connection");
      #ifdef DISPLAY_SSD1306Wire
      afficherMessage("BLE connecte", 0);
      #endif
      
      etaitConnecte = estConnecte;
    }
  }
}

//===================================================================================================================

#ifdef DISPLAY_SSD1306Wire
void initDisplay()
{
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
}

void afficherMessage(String msg, int duree)
{
  //char ligne1[64];  
  //sprintf(ligne1, "%s", );
  //display.drawString(0, 10, String(ligne1));
  
  display.clear();  
  display.drawString(0, 10, msg);
  display.drawString(0, 20, "====================");  
  display.display();
  delay(duree);
}

void afficherDatas(String msg, String datas, int duree)
{  
  display.clear();  
  display.drawString(0, 10, msg);
  display.drawString(0, 20, "====================");  
  display.drawString(0, 40, datas);  
  display.display();
  delay(duree);
}

#endif
