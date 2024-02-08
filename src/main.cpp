#include "Arduino.h"
//#include "heltec.h"
#include <ESPAsyncWebServer.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include "WiFi.h"
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLE2902.h>

const int iteracoes = 50;
#define SERVICE_UUID "e41f0284-a538-4fee-bdc5-3813f657a3f4"
#define CHARACTERISTIC_UUID "5f96279b-d61b-4ac1-ac96-56dc0d3afab9"
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharDireita = NULL;
BLECharacteristic* pCharEsquerda = NULL;
BLECharacteristic* pCharFreio = NULL;
BLECharacteristic* pCharFreioDeMao = NULL;
BLECharacteristic* pCharEmbreagem = NULL;
BLECharacteristic* pCharPorta = NULL;
BLECharacteristic* pCharCinto = NULL;
bool deviceConnected = false;

// Configuração do módulo GPS
HardwareSerial gpsSerial(1);
//AsyncWebServer server(80);
// The TinyGPS++ object
TinyGPSPlus gps;
int channel;

int sppTotalCharsSent = 0;
uint8_t mac[6];

const char* ssid = "WEB-GPS";
const char* password = "esp32password";
const int pinSetaEsquerda = 13; //em teoria tem pulldown
const int pinSetaDireita = 15;  //em teoria tem pulldown
const int pinCintoSeguranca = 34; //em teoria nao tem pulldown
const int pinFreioDeMao = 35; //em teoria nao tem pulldown
const int pinEmbreagem = 32; //em teoria tem pulldown
const int pinPortaAberta = 33; //em teoria tem pulldown
const int pinFreio = 25; //em teoria tem pulldown
const int pinAcelerador = 4; //em teoria tem pulldown
// talvez acelerador?
// ignição vai ser apenas ver quando ele estiver ligado

boolean estadoInicialCinto;
boolean estadoInicialFreioDeMao;
boolean estadoInicialSetaEsquerda;
boolean estadoInicialSetaDireita;
boolean estadoInicialPortaAberta;
boolean estadoInicialFreio;
boolean estadoInicialEmbreagem;

// The remote service we wish to connect to.
static BLEUUID serviceUUID("e41f0284-a538-4fee-bdc5-3813f657a3f4");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("5f96279b-d61b-4ac1-ac96-56dc0d3afab9");
//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

String estadoSetaEsquerda = "desligada";
String estadoSetaDireita = "desligada";
String estadoCintoSeguranca = "desligado";
String estadoFreioDeMao = "desligado";
String estadoPortaAberta = "fechada";
String estadoFreio = "desligado";
String estadoEmbreagem = "livre"; // Inicializado com um valor que não é possível para garantir a primeira impressão
int estadoAcelerador = -1; // Inicializado com um valor que não é possível para garantir a primeira impressão

String estadoAnteriorSetaEsquerda = "desligada";
String estadoAnteriorSetaDireita = "desligada";
String estadoAnteriorCintoSeguranca = "desligado";
String estadoAnteriorFreioDeMao = "desligado";
String estadoAnteriorPortaAberta = "fechada";
String estadoAnteriorFreio = "desligado";
String estadoAnteriorEmbreagem = "desligado";

String latitude = "0.0";
String longitude = "0.0";
String velocidade = "0.0";
String data = "00/00/00";
String hora = "00:00:00";

//criar uma função de média movel para filtrar ruido
int mediaMilivolts(int pin)
{
  int media = 0;
  for(int i = 0; i < iteracoes; i++)
  {
    media += analogReadMilliVolts(pin);
  }
  media = media / iteracoes;
  return media;
}

String formatFloat(float value, int decimalPlaces) {
  char buffer[30];
  dtostrf(value, 0, decimalPlaces, buffer);
  return String(buffer);
}

String printData()
{
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", gps.date.day(), gps.date.month(), gps.date.year());
    return String(sz);
}

String printHora()
{
    char sa[32];
    sprintf(sa, "%02d:%02d:%02d ", gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(sa);
}


String readLatitude() {
  if (gps.location.isValid()) {
    return formatFloat(gps.location.lat(), 6);
  }
  return "Invalid";
}

String readLongitude() {
  if (gps.location.isValid()) {
    return formatFloat(gps.location.lng(), 6);
  }
  return "Invalid";
}

String readSpeed() {
  if (gps.speed.isValid()) {
    return formatFloat(gps.speed.kmph(), 2);
  }
  return "Invalid";
}

String readData() {
  if (gps.date.isValid()) {
    return printData();
  }
  return "Invalid";
}

String readHora() {
  if (gps.time.isValid()) {
    return printHora();
  }
  return "Invalid"; 
}

void calibrarSensores() {
  Serial.println("\nIniciando calibração dos sensores. Por favor, aguarde...");

  // Calibração do sensor de cinto de segurança
  int leituraCinto = mediaMilivolts(pinCintoSeguranca);
  estadoInicialCinto = (leituraCinto > 3000);
  printf("Cinto de Segurança: %s\n", estadoInicialCinto ? "ligado" : "desligado");

  // Calibração do sensor de freio de mão
  int leituraFreioMao = mediaMilivolts(pinFreioDeMao);
  estadoInicialFreioDeMao = (leituraFreioMao > 3000);
  printf("Freio de Mão: %s\n", estadoInicialFreioDeMao ? "ligado" : "desligado");

  // Calibração do sensor de seta esquerda
  int leituraSetaEsquerda = mediaMilivolts(pinSetaEsquerda);
  estadoInicialSetaEsquerda = (leituraSetaEsquerda > 3000);
  printf("Seta Esquerda: %s\n", estadoInicialSetaEsquerda ? "ligada" : "desligada");
//
  // Calibração do sensor de seta direita
  int leituraSetaDireita = mediaMilivolts(pinSetaDireita);
  estadoInicialSetaDireita = (leituraSetaDireita > 3000);
  printf("Seta Direita: %s\n", estadoInicialSetaDireita ? "ligada" : "desligada");

  // Calibração do sensor de porta aberta
  int leituraPortaAberta = mediaMilivolts(pinPortaAberta);
  estadoInicialPortaAberta = (leituraPortaAberta > 3000);
  printf("Porta: %s\n", estadoInicialPortaAberta ? "aberta" : "fechada");

  // Calibração do sensor de freio
  int leituraFreio = mediaMilivolts(pinFreio);
  estadoInicialFreio = (leituraFreio > 3000);
  printf("Freio: %s\n", estadoInicialFreio ? "ligado" : "desligado");

  // Calibração do sensor de embreagem
  int leituraEmbreagem = mediaMilivolts(pinEmbreagem);
  estadoInicialEmbreagem = (leituraEmbreagem > 3000);
  printf("Embreagem: %s\n", estadoInicialEmbreagem ? "pressionada" : "livre");

  Serial.println("Calibração concluída.\n\n\n");
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
      Serial.println("Device Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device Disconnected");
    }

    void onWrite(BLECharacteristic *pCharacteristic) {
      Serial.printf("Write value: %s\n", pCharacteristic->getValue().c_str());
    }

    void onNotify(BLECharacteristic *pCharacteristic) {
      Serial.printf("Notify value: %s\n", pCharacteristic->getValue().c_str());
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.println("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
      }
    }

    void onNotify(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        Serial.print("Notify value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
      }
      Serial.println();
    }
}; 

void initBT(String content)
{
    // Create the BLE Device
  BLEDevice::init(std::string(content.c_str()));

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  //callbacks

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setNotifyProperty(true);
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 23, 22); // RX, TX

  String macAddress;

  WiFi.macAddress(mac);
  // Converte o endereço MAC para uma string hexadecimal
  for (int i = 0; i < 6; i++)
  {
    macAddress += String(mac[i], HEX);
    if (i < 5)
    {
      macAddress += ":";
    }
  }
  Serial.printf("MAC Address: %s\n", macAddress.c_str());

  pinMode(pinSetaEsquerda, INPUT_PULLDOWN);
  pinMode(pinSetaDireita, INPUT_PULLDOWN);
  pinMode(pinCintoSeguranca, INPUT_PULLDOWN);
  pinMode(pinFreioDeMao, INPUT_PULLDOWN);
  pinMode(pinEmbreagem, INPUT_PULLDOWN);
  pinMode(pinPortaAberta, INPUT_PULLDOWN);
  pinMode(pinFreio, INPUT_PULLDOWN);
  analogSetAttenuation(ADC_11db);

  initBT("Telemetria_" + macAddress);
  delay(3000); // Aguarda 1 segundo antes de iniciar a calibração
  calibrarSensores();
}

void loop() {

  // Leitura do estado dos sensores
  int valorSetaEsquerda = mediaMilivolts(pinSetaEsquerda);
  int valorSetaDireita = mediaMilivolts(pinSetaDireita);
  int valorCintoSeguranca = mediaMilivolts(pinCintoSeguranca);
  int valorFreioDeMao = mediaMilivolts(pinFreioDeMao);
  int valorEmbreagem = mediaMilivolts(pinEmbreagem);
  int valorPortaAberta = mediaMilivolts(pinPortaAberta);
  int valorFreio = mediaMilivolts(pinFreio);
  //int valorAcelerador = mediaMilivolts(pinAcelerador);
  
  if (deviceConnected) 
  {
    if(estadoInicialCinto == true)
    {
      estadoCintoSeguranca = "Cinto: ";
      estadoCintoSeguranca += (valorCintoSeguranca > 3000) ? "desligado" : "ligado";
      if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
      {
        Serial.println(estadoCintoSeguranca.c_str());
        pCharacteristic->setValue(estadoCintoSeguranca.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
      }
    }
    else
    {
      estadoCintoSeguranca = "Cinto: ";
      estadoCintoSeguranca += (valorCintoSeguranca > 3000) ? "ligado" : "desligado";
      if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
      {
        Serial.println(estadoCintoSeguranca.c_str());
        pCharacteristic->setValue(estadoCintoSeguranca.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
      }
    } 
    if(estadoInicialFreioDeMao == true)
    {
      estadoFreioDeMao = "Freio de Mão: ";
      estadoFreioDeMao += (valorFreioDeMao > 3000) ? "desligado" : "ligado";
      if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
      {
        Serial.println(estadoFreioDeMao.c_str());
        pCharacteristic->setValue(estadoFreioDeMao.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorFreioDeMao = estadoFreioDeMao;
      }
    }
    else
    {
      estadoFreioDeMao = "Freio de Mão: ";
      estadoFreioDeMao += (valorFreioDeMao > 3000) ? "ligado" : "desligado";
      if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
      {
        Serial.println(estadoFreioDeMao.c_str());
        pCharacteristic->setValue(estadoFreioDeMao.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorFreioDeMao = estadoFreioDeMao;
      }
    }

    if(estadoInicialEmbreagem == true)
    {
      estadoEmbreagem = "Embreagem: ";
      estadoEmbreagem += (valorEmbreagem > 3000) ? "livre" : "pressionada";
      if(estadoEmbreagem != estadoAnteriorEmbreagem)
      {
        Serial.println(estadoEmbreagem.c_str());
        pCharacteristic->setValue(estadoEmbreagem.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorEmbreagem = estadoEmbreagem;
      }
    }
    else
    {
      estadoEmbreagem = "Embreagem: ";
      estadoEmbreagem += (valorEmbreagem > 3000) ? "pressionada" : "livre";
      if(estadoEmbreagem != estadoAnteriorEmbreagem)
      {
        Serial.println(estadoEmbreagem.c_str());
        pCharacteristic->setValue(estadoEmbreagem.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorEmbreagem = estadoEmbreagem;
      }
    }

    if(estadoInicialSetaEsquerda == true)
    {
      estadoSetaEsquerda = "Seta Esquerda: ";
      estadoSetaEsquerda += (valorSetaEsquerda > 3000) ? "desligada" : "ligada";
      if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
      {
        Serial.println(estadoSetaEsquerda.c_str());
        pCharacteristic->setValue(estadoSetaEsquerda.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
      }
    }
    else
    {
      estadoSetaEsquerda = "Seta Esquerda: ";
      estadoSetaEsquerda += (valorSetaEsquerda > 3000) ? "ligada" : "desligada";
      if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
      {
        Serial.println(estadoSetaEsquerda.c_str());
        pCharacteristic->setValue(estadoSetaEsquerda.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
      }
    }
    if(estadoInicialSetaDireita == true)
    {
      estadoSetaDireita = "Seta Direita: ";
      estadoSetaDireita += (valorSetaDireita > 3000) ? "desligada" : "ligada";
      if(estadoAnteriorSetaDireita != estadoSetaDireita)
      {
        Serial.println(estadoSetaDireita.c_str());
        pCharacteristic->setValue(estadoSetaDireita.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorSetaDireita = estadoSetaDireita;
      }
    }
    else
    {
      estadoSetaDireita = "Seta Direita: ";
      estadoSetaDireita += (valorSetaDireita > 3000) ? "ligada" : "desligada";
      if(estadoAnteriorSetaDireita != estadoSetaDireita)
      {
        Serial.println(estadoSetaDireita.c_str());
        pCharacteristic->setValue(estadoSetaDireita.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorSetaDireita = estadoSetaDireita;
      }
    }
    if(estadoInicialPortaAberta == true)
    {
      estadoPortaAberta = "Porta: ";
      estadoPortaAberta += (valorPortaAberta > 3000) ? "fechada" : "aberta";
      if(estadoAnteriorPortaAberta != estadoPortaAberta)
      {
        Serial.println(estadoPortaAberta.c_str());
        pCharacteristic->setValue(estadoPortaAberta.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorPortaAberta = estadoPortaAberta;
      }
    }
    else
    {
      estadoPortaAberta = "Porta: ";
      estadoPortaAberta += (valorPortaAberta > 3000) ? "aberta" : "fechada";
      if(estadoAnteriorPortaAberta != estadoPortaAberta)
      {
        Serial.println(estadoPortaAberta.c_str());
        pCharacteristic->setValue(estadoPortaAberta.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorPortaAberta = estadoPortaAberta;
      }
    }
    if (estadoInicialFreio == true)
    {
      estadoFreio = "Freio: ";
      estadoFreio += (valorFreio > 3000) ? "desligado" : "ligado";
      if(estadoAnteriorFreio != estadoFreio)
      {
        Serial.println(estadoFreio.c_str());
        pCharacteristic->setValue(estadoFreio.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorFreio = estadoFreio;
      }
    }
    else
    {
      estadoFreio = "Freio: ";
      estadoFreio += (valorFreio > 3000) ? "ligado" : "desligado";
      if(estadoAnteriorFreio != estadoFreio)
      {
        Serial.println(estadoFreio.c_str());
        pCharacteristic->setValue(estadoFreio.c_str());
        pCharacteristic->notify();
        delay(30);
        estadoAnteriorFreio = estadoFreio;
      }
    } 
  }
  delay(3000);
}