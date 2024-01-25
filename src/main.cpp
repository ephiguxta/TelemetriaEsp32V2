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

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static BLEAddress *pServerAddress;
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
  char buffer[10];
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

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("onConnect");
  }


  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
 
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID1");
    return false;
  }

  Serial.println(" - Found our characteristics");
 
  if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  // The target substring to match in the device name
  const char* targetSubstring = "SIGEAUTO";

  /** EuVouMeAproximar
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    String deviceName = advertisedDevice.getName().c_str();
    if (deviceName.length() > 0 && deviceName.indexOf(targetSubstring) != -1 && advertisedDevice.getRSSI() > -30) {
      Serial.print("Found our device! Address: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      //doScan = true;
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void initBT(String content)
{
  std::string contentStr = content.c_str(); // Convert String to std::string
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init(contentStr);

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
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

  initBT("Telemetria" + macAddress);
  delay(1000); // Aguarda 1 segundo antes de iniciar a calibração
  calibrarSensores();
}

void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // Leitura do estado dos sensores
  int valorSetaEsquerda = mediaMilivolts(pinSetaEsquerda);
  int valorSetaDireita = mediaMilivolts(pinSetaDireita);
  int valorCintoSeguranca = mediaMilivolts(pinCintoSeguranca);
  int valorFreioDeMao = mediaMilivolts(pinFreioDeMao);
  int valorEmbreagem = mediaMilivolts(pinEmbreagem);
  int valorPortaAberta = mediaMilivolts(pinPortaAberta);
  int valorFreio = mediaMilivolts(pinFreio);
  int valorAcelerador = mediaMilivolts(pinAcelerador);
  
  if (connected) 
  {
    if(estadoInicialCinto == true)
    {
      estadoCintoSeguranca = "Cinto: ";
      estadoCintoSeguranca += (valorCintoSeguranca > 3000) ? "desligado" : "ligado";
      if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
      {
        Serial.printf(estadoCintoSeguranca.c_str());
        pRemoteCharacteristic->writeValue(estadoCintoSeguranca.c_str(), estadoCintoSeguranca.length());
        estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
      }
    }
    else
    {
      estadoCintoSeguranca = "Cinto: ";
      estadoCintoSeguranca += (valorCintoSeguranca > 3000) ? "ligado" : "desligado";
      if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
      {
        Serial.printf(estadoCintoSeguranca.c_str());
        pRemoteCharacteristic->writeValue(estadoCintoSeguranca.c_str(), estadoCintoSeguranca.length());
        estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
      }
    } 
    if(estadoInicialFreioDeMao == true)
    {
      estadoFreioDeMao = "Freio de Mão: ";
      estadoFreioDeMao += (valorFreioDeMao > 3000) ? "desligado" : "ligado";
      if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
      {
        Serial.printf(estadoFreioDeMao.c_str());
        pRemoteCharacteristic->writeValue(estadoFreioDeMao.c_str(), estadoFreioDeMao.length());
        estadoAnteriorFreioDeMao = estadoFreioDeMao;
      }
    }
    else
    {
      estadoFreioDeMao = "Freio de Mão: ";
      estadoFreioDeMao += (valorFreioDeMao > 3000) ? "ligado" : "desligado";
      if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
      {
        Serial.printf(estadoFreioDeMao.c_str());
        pRemoteCharacteristic->writeValue(estadoFreioDeMao.c_str(), estadoFreioDeMao.length());
        estadoAnteriorFreioDeMao = estadoFreioDeMao;
      }
    }

    if(estadoInicialEmbreagem == true)
    {
      estadoEmbreagem = "Embreagem: ";
      estadoEmbreagem += (valorEmbreagem > 3000) ? "livre" : "pressionada";
      if(estadoEmbreagem != estadoAnteriorEmbreagem)
      {
        Serial.printf(estadoEmbreagem.c_str());
        pRemoteCharacteristic->writeValue(estadoEmbreagem.c_str(), estadoEmbreagem.length());
        estadoAnteriorEmbreagem = estadoEmbreagem;
      }
    }
    else
    {
      estadoEmbreagem = "Embreagem: ";
      estadoEmbreagem += (valorEmbreagem > 3000) ? "pressionada" : "livre";
      if(estadoEmbreagem != estadoAnteriorEmbreagem)
      {
        Serial.printf(estadoEmbreagem.c_str());
        pRemoteCharacteristic->writeValue(estadoEmbreagem.c_str(), estadoEmbreagem.length());
        estadoAnteriorEmbreagem = estadoEmbreagem;
      }
    }

    if(estadoInicialSetaEsquerda == true)
    {
      estadoSetaEsquerda = "Seta Esquerda: ";
      estadoSetaEsquerda += (valorSetaEsquerda > 3000) ? "desligada" : "ligada";
      if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
      {
        Serial.printf(estadoSetaEsquerda.c_str());
        pRemoteCharacteristic->writeValue(estadoSetaEsquerda.c_str(), estadoSetaEsquerda.length());
        estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
      }
    }
    else
    {
      estadoSetaEsquerda = "Seta Esquerda: ";
      estadoSetaEsquerda += (valorSetaEsquerda > 3000) ? "ligada" : "desligada";
      if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
      {
        Serial.printf(estadoSetaEsquerda.c_str());
        pRemoteCharacteristic->writeValue(estadoSetaEsquerda.c_str(), estadoSetaEsquerda.length());
        estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
      }
    }

    if(estadoInicialSetaDireita == true)
    {
      estadoSetaDireita = "Seta Direita: ";
      estadoSetaDireita += (valorSetaDireita > 3000) ? "desligada" : "ligada";
      if(estadoAnteriorSetaDireita != estadoSetaDireita)
      {
        Serial.printf(estadoSetaDireita.c_str());
        pRemoteCharacteristic->writeValue(estadoSetaDireita.c_str(), estadoSetaDireita.length());
        estadoAnteriorSetaDireita = estadoSetaDireita;
      }
    }
    else
    {
      estadoSetaDireita = "Seta Direita: ";
      estadoSetaDireita += (valorSetaDireita > 3000) ? "ligada" : "desligada";
      if(estadoAnteriorSetaDireita != estadoSetaDireita)
      {
        Serial.printf(estadoSetaDireita.c_str());
        pRemoteCharacteristic->writeValue(estadoSetaDireita.c_str(), estadoSetaDireita.length());
        estadoAnteriorSetaDireita = estadoSetaDireita;
      }
    }

    if(estadoInicialPortaAberta == true)
    {
      estadoPortaAberta = "Porta: ";
      estadoPortaAberta += (valorPortaAberta > 3000) ? "fechada" : "aberta";
      if(estadoAnteriorPortaAberta != estadoPortaAberta)
      {
        Serial.printf(estadoPortaAberta.c_str());
        pRemoteCharacteristic->writeValue(estadoPortaAberta.c_str(), estadoPortaAberta.length());
        estadoAnteriorPortaAberta = estadoPortaAberta;
      }
    }
    else
    {
      estadoPortaAberta = "Porta: ";
      estadoPortaAberta += (valorPortaAberta > 3000) ? "aberta" : "fechada";
      if(estadoAnteriorPortaAberta != estadoPortaAberta)
      {
        Serial.printf(estadoPortaAberta.c_str());
        pRemoteCharacteristic->writeValue(estadoPortaAberta.c_str(), estadoPortaAberta.length());
        estadoAnteriorPortaAberta = estadoPortaAberta;
      }
    }
    
    if (estadoInicialFreio == true)
    {
      estadoFreio = "Freio: ";
      estadoFreio += (valorFreio > 3000) ? "desligado" : "ligado";
      if(estadoAnteriorFreio != estadoFreio)
      {
        Serial.printf(estadoFreio.c_str());
        pRemoteCharacteristic->writeValue(estadoFreio.c_str(), estadoFreio.length());
        estadoAnteriorFreio = estadoFreio;
      }
    }
    else
    {
      estadoFreio = "Freio: ";
      estadoFreio += (valorFreio > 3000) ? "ligado" : "desligado";
      if(estadoAnteriorFreio != estadoFreio)
      {
        Serial.printf(estadoFreio.c_str());
        pRemoteCharacteristic->writeValue(estadoFreio.c_str(), estadoFreio.length());
        estadoAnteriorFreio = estadoFreio;
      }
    }
  }
  else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(3000);
}