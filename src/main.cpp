#include "Arduino.h"
//#include "heltec.h"
#include <ESPAsyncWebServer.h>
#include <BluetoothSerial.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>

#define BT_DISCOVER_TIME  6000
esp_spp_sec_t sec_mask=ESP_SPP_SEC_NONE; // or ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE to request pincode confirmation
esp_spp_role_t role=ESP_SPP_ROLE_SLAVE; // or ESP_SPP_ROLE_MASTER

// Configuração do módulo GPS
HardwareSerial gpsSerial(1);
//AsyncWebServer server(80);
// The TinyGPS++ object
TinyGPSPlus gps;
BTAddress addr;  // Certifique-se de que BTAddress seja o tipo correto para representar endereços Bluetooth
int channel;
BluetoothSerial SerialBT;

int sppTotalCharsSent = 0;

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

String estadoSetaEsquerda = "desligada";
String estadoSetaDireita = "desligada";
String estadoCintoSeguranca = "desligado";
String estadoFreioDeMao = "desligado";
String estadoPortaAberta = "fechada";
String estadoFreio = "desligado";
int estadoEmbreagem = -1; // Inicializado com um valor que não é possível para garantir a primeira impressão
int estadoAcelerador = -1; // Inicializado com um valor que não é possível para garantir a primeira impressão

String estadoAnteriorSetaEsquerda = "desligada";
String estadoAnteriorSetaDireita = "desligada";
String estadoAnteriorCintoSeguranca = "desligado";
String estadoAnteriorFreioDeMao = "desligado";
String estadoAnteriorPortaAberta = "fechada";
String estadoAnteriorFreio = "desligado";

String latitude = "0.0";
String longitude = "0.0";
String velocidade = "0.0";
String data = "00/00/00";
String hora = "00:00:00";

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
  Serial.println("Iniciando calibração dos sensores. Por favor, aguarde...");

  // Calibração do sensor de cinto de segurança
  int leituraCinto = analogReadMilliVolts(pinCintoSeguranca);
  estadoInicialCinto = (leituraCinto > 3000);
  printf("Cinto de Segurança: %s\n", estadoInicialCinto ? "ligado" : "desligado");

  // Calibração do sensor de freio de mão
  int leituraFreioMao = analogReadMilliVolts(pinFreioDeMao);
  estadoInicialFreioDeMao = (leituraFreioMao > 3000);
  printf("Freio de Mão: %s\n", estadoInicialFreioDeMao ? "ligado" : "desligado");

  // Calibração do sensor de seta esquerda
  int leituraSetaEsquerda = analogReadMilliVolts(pinSetaEsquerda);
  estadoInicialSetaEsquerda = (leituraSetaEsquerda > 3000);
  printf("Seta Esquerda: %s\n", estadoInicialSetaEsquerda ? "ligada" : "desligada");
//
  // Calibração do sensor de seta direita
  int leituraSetaDireita = analogReadMilliVolts(pinSetaDireita);
  estadoInicialSetaDireita = (leituraSetaDireita > 3000);
  printf("Seta Direita: %s\n", estadoInicialSetaDireita ? "ligada" : "desligada");

  // Calibração do sensor de porta aberta
  int leituraPortaAberta = analogReadMilliVolts(pinPortaAberta);
  estadoInicialPortaAberta = (leituraPortaAberta > 3000);
  printf("Porta: %s\n", estadoInicialPortaAberta ? "aberta" : "fechada");

  // Calibração do sensor de freio
  int leituraFreio = analogReadMilliVolts(pinFreio);
  estadoInicialFreio = (leituraFreio > 3000);
  printf("Freio: %s\n", estadoInicialFreio ? "ligado" : "desligado");

  Serial.println("Calibração concluída.\n\n\n");
}

// void desenharSetaDireita(int x, int y)
// {
//   int tamanhoSeta = 8; // Ajuste o tamanho da seta conforme necessário
//   // Desenha o corpo da seta apontando para a direita
//   Heltec.display->drawLine(x, y - 4, x + tamanhoSeta, y - 4);
//   Heltec.display->drawLine(x, y + 4, x + tamanhoSeta, y + 4);
//   Heltec.display->drawLine(x + tamanhoSeta, y + 4, x + tamanhoSeta, y + 8);
//   Heltec.display->drawLine(x + tamanhoSeta, y - 4, x + tamanhoSeta, y - 8);
//   //ponta
//   Heltec.display->drawLine(x + tamanhoSeta, y + 8, x + tamanhoSeta + 8, y);
//   Heltec.display->drawLine(x + tamanhoSeta, y - 8, x + tamanhoSeta + 8, y);
// }

// void desenharSetaEsquerda(int x, int y)
// {
//   int tamanhoSeta = 8; // Ajuste o tamanho da seta conforme necessário
//   // Desenha o corpo da seta apontando para a esquerda
//   Heltec.display->drawLine(x, y - 4, x + tamanhoSeta, y - 4);
//   Heltec.display->drawLine(x, y + 4, x + tamanhoSeta, y + 4);
//   Heltec.display->drawLine(x, y + 4, x, y + 8);
//   Heltec.display->drawLine(x, y - 4, x, y - 8);
//   //ponta
//   Heltec.display->drawLine(x, y + 8, x - 8, y);
//   Heltec.display->drawLine(x, y - 8, x - 8, y);
// }

// void desenharCintoSeguranca(int x, int y)
// {
//   //usar o heltec text pra escrever cinto
//   Heltec.display->drawString(x, y, "Cinto");
// }

// void desenharFreioDeMao(int x, int y)
// {
//   //usar o heltec text pra escrever cinto
//   Heltec.display->drawString(x, y, "Freio Mão");
// }

// void desenharPorta(int x, int y)
// {
//   //usar o heltec text pra escrever cinto
//   Heltec.display->drawString(x, y, "Porta");
// }

// void desenharFreio(int x, int y)
// {
//   //usar o heltec text pra escrever cinto
//   Heltec.display->drawString(x, y, "Freio");
// }

// void desenharEmbreagem(int x, int y, int nivel)
// {
//   // Ajusta o nível para garantir que esteja dentro do intervalo [0, 100]
//   nivel = constrain(nivel, 0, 100);

//   // Desenha a barra de progresso para representar o nível da embreagem
//   Heltec.display->drawProgressBar(x, y, 40, 7, nivel);
// }

// void limparTela()
// {
//   Heltec.display->clear();
//   Heltec.display->display();
// }

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms) {
	unsigned long start = millis();
	do {
		while (gpsSerial.available()) {
			gps.encode(gpsSerial.read());
		}
	} while (millis() - start < ms);
}

void On_ESP_SPP_WRITE(esp_spp_cb_param_t *param)
{
  Serial.println("571 - ESP_SPP_WRITE_EVT: Bluetooth SPP write operation completed");
  Serial.print("572 - Status = ");
  switch (param->write.status)
  {
  case ESP_SPP_SUCCESS: // Successful operation
    Serial.println("ESP_SPP_SUCCESS");
    break;

  case ESP_SPP_FAILURE: // Generic failure
    Serial.println("ESP_SPP_FAILURE");
    break;

  case ESP_SPP_BUSY: // Temporarily can not handle this request
    Serial.println("ESP_SPP_BUSY");
    break;

  case ESP_SPP_NO_DATA: // no data
    Serial.println("ESP_SPP_NO_DATA");
    break;

  case ESP_SPP_NO_RESOURCE: // No more set pm control block
    Serial.println("ESP_SPP_NO_RESOURCE");
    break;

  default:
    Serial.println("UNKNOWN");
    break;
  }

  Serial.print("595 - Bytes transmitted = ");
  Serial.println(param->write.len, DEC);
  sppTotalCharsSent += param->write.len;
  Serial.print("598 - Total Bytes transmitted = ");
  Serial.println(sppTotalCharsSent, DEC);

  Serial.print("598 - Congestion = ");
  if (param->write.cong)
    Serial.println("TRUE");
  else
    Serial.println("FALSE");
}

// Lista de parametros para o callback
//  1 para captura
//  2 para requisicao de dados do GPS
//  3 para enviar dados de reconhecimento facial
//  4 para reiniciar o esp32
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("Bluetooth Conectado!");
  }
  else if (event == ESP_SPP_CLOSE_EVT)
  {
    Serial.println("Bluetooth Desconectado!");
  }
  else if (event == ESP_SPP_WRITE_EVT)
  {
    Serial.println("Bluetooth Enviando Dados!");
    On_ESP_SPP_WRITE(param);
  }
  else if (event == ESP_SPP_DATA_IND_EVT)

  {
    Serial.printf("ESP_SPP_DATA_IND_EVT len=%d, handle=%d\n\n", param->data_ind.len, param->data_ind.handle);
    String stringRead = String(*param->data_ind.data);
    int paramInt = stringRead.toInt() - 48;
    Serial.printf("paramInt: %d\n", paramInt);
    // trocar todos esses if elses por um unico switch case
    switch (paramInt)
    {
    case 1:
      Serial.println("Caso 1");
      break;
    case 2:
      Serial.println("Caso 2");
      break;
    case 3:
      Serial.println("Caso 3");
      break;
    case 4:
      Serial.println("Reiniciando o ESP32");
      ESP.restart();
      break;
    default:
      break;
    }
  }
}

  void initBT(String content)
{
  if (!SerialBT.begin(content, true))
  {
    Serial.println("An error occurred initializing Bluetooth");
    ESP.restart();
  }
  else
  {
    Serial.println("Bluetooth initialized");
  }

  Serial.println("Starting discoverAsync...");
  BTScanResults* btDeviceList = SerialBT.getScanResults();  // maybe accessing from different threads!
  if (SerialBT.discoverAsync([](BTAdvertisedDevice* pDevice) {
      Serial.printf("Found a new device asynchronously: %s\n", pDevice->toString().c_str());
    } )
    ) {
    delay(BT_DISCOVER_TIME);
    Serial.print("Stopping discoverAsync... ");
    SerialBT.discoverAsyncStop();
    Serial.println("discoverAsync stopped");
    delay(5000);
    if (btDeviceList->getCount() > 0) {
      for (int i = 0; i < btDeviceList->getCount(); i++) {
        BTAdvertisedDevice *device = btDeviceList->getDevice(i);
        // Verificar se o nome do dispositivo inicia com "SIGEAUTO"
        if (device->getName().find("SIGEAUTO") == 0) {
          Serial.println("Found a device with name starting with 'SIGEAUTO'");
          // Obter os canais associados ao endereço Bluetooth
          std::map<int, std::string> channels = SerialBT.getChannels(device->getAddress());
          // Conectar apenas a este dispositivo
          BTAddress addr = device->getAddress();
          if (!channels.empty()) {
            int channel = channels.begin()->first;
            Serial.printf("Connecting to %s - %d\n", addr.toString().c_str(), channel);
            if(SerialBT.connect(addr, channel, sec_mask, role))
            {
              Serial.println("Connected to device");
            }
            else
            {
              Serial.println("Failed to connect to device");
            }
          } else {
            Serial.println("No channels available for the device.");
          }
          break;  // Sair do loop após encontrar o dispositivo desejado
        }
      }
    } else {
      Serial.println("No devices found during discoverAsync");
    }
  }

  SerialBT.register_callback(btCallback);
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 23, 22); // RX, TX
  initBT("TELEMETRIA");

  //Setting the ESP as an access point
  //Serial.print("Setting AP (Access Point)…");
  //Remove the password parameter, if you want the AP (Access Point) to be open
  // WiFi.softAP(ssid, password);

  // IPAddress IP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(IP);

  // server.on("/latitude", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readLatitude().c_str());
  // });
  // server.on("/longitude", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readLongitude().c_str());
  // });
  // server.on("/velocidade", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readSpeed().c_str());
  // });
  // server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readData().c_str());
  // });
  // server.on("/hora", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readHora().c_str());
  // });
  // server.on("/setaEsquerda", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", estadoSetaEsquerda.c_str());
  // });
  // server.on("/setaDireita", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", estadoSetaDireita.c_str());
  // });
  // server.on("/cintoSeguranca", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", estadoCintoSeguranca.c_str());
  // });
  // server.on("/freioDeMao", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", estadoFreioDeMao.c_str());
  // });
  // server.on("/portaAberta", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", estadoPortaAberta.c_str());
  // });
  // server.on("/freio", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", estadoFreio.c_str());
  // });
  // server.on("/embreagem", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", String(estadoEmbreagem).c_str());
  // });
  
  //server.begin();
  // Heltec.begin(true, false, true);
  pinMode(pinSetaEsquerda, INPUT_PULLDOWN);
  pinMode(pinSetaDireita, INPUT_PULLDOWN);
  pinMode(pinCintoSeguranca, INPUT_PULLDOWN);
  pinMode(pinFreioDeMao, INPUT_PULLDOWN);
  pinMode(pinEmbreagem, INPUT_PULLDOWN);
  pinMode(pinPortaAberta, INPUT_PULLDOWN);
  pinMode(pinFreio, INPUT_PULLDOWN);
  analogSetAttenuation(ADC_11db);
  // Heltec.display->setContrast(255);
  // //mudar o tamanho da fonte
  // Heltec.display->setFont(ArialMT_Plain_10);
  // limparTela();
  // Heltec.display->flipScreenVertically();

  delay(1000); // Aguarda 1 segundo antes de iniciar a calibração
  calibrarSensores();
  //limparTela();
}

void loop() {

  // latitude = readLatitude();
  // longitude = readLongitude();
  // velocidade = readSpeed();
  // data = readData();
  // hora = readHora();

  // Serial.print("Latitude: ");
  // Serial.println(latitude);
  // Serial.print("Longitude: ");
  // Serial.println(longitude);
  // Serial.print("Velocidade: ");
  // Serial.println(velocidade);
  // Serial.print("Data: ");
  // Serial.println(data);
  // Serial.print("Hora: ");
  // Serial.println(hora);

  // Leitura do estado dos sensores
  int valorSetaEsquerda = analogReadMilliVolts(pinSetaEsquerda);
  int valorSetaDireita = analogReadMilliVolts(pinSetaDireita);
  int valorCintoSeguranca = analogReadMilliVolts(pinCintoSeguranca);
  int valorFreioDeMao = analogReadMilliVolts(pinFreioDeMao);
  int valorEmbreagem = analogRead(pinEmbreagem);
  int valorPortaAberta = analogReadMilliVolts(pinPortaAberta);
  int valorFreio = analogReadMilliVolts(pinFreio);
  
  if(estadoInicialCinto == true)
  {
    estadoCintoSeguranca = (valorCintoSeguranca > 3000) ? "desligado" : "ligado";
    if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
    {
      Serial.printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
      estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
    }
  }
  else
  {
    estadoCintoSeguranca = (valorCintoSeguranca > 3000) ? "ligado" : "desligado";
    if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
    {
      Serial.printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
      estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
    }
  }

  if(estadoInicialFreioDeMao == true)
  {
    estadoFreioDeMao = (valorFreioDeMao > 3000) ? "desligado" : "ligado";
    if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
    {
      Serial.printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
      estadoAnteriorFreioDeMao = estadoFreioDeMao;
    }
  }
  else
  {
    estadoFreioDeMao = (valorFreioDeMao > 3000) ? "ligado" : "desligado";
    if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
    {
      Serial.printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
      estadoAnteriorFreioDeMao = estadoFreioDeMao;
    }
  }

  if (valorEmbreagem != estadoEmbreagem) {
    estadoEmbreagem = valorEmbreagem;
    Serial.printf("Embreagem: %d\n", estadoEmbreagem);
  }

  if(estadoInicialSetaEsquerda == true)
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 3000) ? "desligada" : "ligada";
    if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
    {
      Serial.printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
      estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
    }
  }
  else
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 3000) ? "ligada" : "desligada";
    if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
    {
      Serial.printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
      estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
    }
  }

  if(estadoInicialSetaDireita == true)
  {
    estadoSetaDireita = (valorSetaDireita > 3000) ? "desligada" : "ligada";
    if(estadoAnteriorSetaDireita != estadoSetaDireita)
    {
      Serial.printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
      estadoAnteriorSetaDireita = estadoSetaDireita;
    }
  }
  else
  {
    estadoSetaDireita = (valorSetaDireita > 3000) ? "ligada" : "desligada";
    if(estadoAnteriorSetaDireita != estadoSetaDireita)
    {
      Serial.printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
      estadoAnteriorSetaDireita = estadoSetaDireita;
    }
  }

  if(estadoInicialPortaAberta == true)
  {
    estadoPortaAberta = (valorPortaAberta > 3000) ? "fechada" : "aberta";
    if(estadoAnteriorPortaAberta != estadoPortaAberta)
    {
      Serial.printf("Porta: %s\n", estadoPortaAberta.c_str());
      estadoAnteriorPortaAberta = estadoPortaAberta;
    }
  }
  else
  {
    estadoPortaAberta = (valorPortaAberta > 3000) ? "aberta" : "fechada";
    if(estadoAnteriorPortaAberta != estadoPortaAberta)
    {
      Serial.printf("Porta: %s\n", estadoPortaAberta.c_str());
      estadoAnteriorPortaAberta = estadoPortaAberta;
    }
  }
  
  if (estadoInicialFreio == true)
  {
    estadoFreio = (valorFreio > 3000) ? "desligado" : "ligado";
    if(estadoAnteriorFreio != estadoFreio)
    {
      Serial.printf("Freio: %s\n", estadoFreio.c_str());
      estadoAnteriorFreio = estadoFreio;
    }
  }
  else
  {
    estadoFreio = (valorFreio > 3000) ? "ligado" : "desligado";
    if(estadoAnteriorFreio != estadoFreio)
    {
      Serial.printf("Freio: %s\n", estadoFreio.c_str());
      estadoAnteriorFreio = estadoFreio;
    }
  }

  // limparTela();

  // if (estadoSetaEsquerda == "ligada")
  //   desenharSetaEsquerda(10, 32);

  // if (estadoSetaDireita == "ligada")
  //   desenharSetaDireita(110, 32);

  // if (estadoCintoSeguranca == "ligado")
  //   desenharCintoSeguranca(50, 25);

  // if (estadoFreioDeMao == "ligado")
  //   desenharFreioDeMao(38, 35);

  // if (estadoPortaAberta == "aberta")
  //   desenharPorta(2, 50);

  // if (estadoFreio == "ligado")
  //   desenharFreio(100, 50);
    
  //imprimir na tela os dados do GPS
  //Heltec.display->drawString(2, 0, data.c_str());
  //Heltec.display->drawString(2, 8, hora.c_str());
  //Heltec.display->drawString(70, 0, longitude.c_str());
  //Heltec.display->drawString(70, 8, latitude.c_str());
  //Heltec.display->drawString(70, 16, velocidade.c_str());

  //desenharEmbreagem(41, 53, map(estadoEmbreagem, 0, 3300, 0, 100));

  //Heltec.display->display();

  smartDelay(3000);
}