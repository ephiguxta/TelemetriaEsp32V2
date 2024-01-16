#include "Arduino.h"
//#include "heltec.h"
#include <ESPAsyncWebServer.h>
#include <BluetoothSerial.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include "WiFi.h"

const int iteracoes = 50;

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

// static void smartDelay(unsigned long ms) {
// 	unsigned long start = millis();
// 	do {
// 		while (gpsSerial.available()) {
// 			gps.encode(gpsSerial.read());
// 		}
// 	} while (millis() - start < ms);
// }

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

// Função para inicializar o bluetooth
void initBT(String content)
{
  if (!SerialBT.begin(content))
  {
    Serial.println("An error occurred initializing Bluetooth");
    ESP.restart();
  }
  else
  {
    Serial.println("Bluetooth initialized");
  }


  SerialBT.register_callback(btCallback);
  Serial.println("The device started, now you can pair it with bluetooth");

  delay(2000);

  //tentar conectar o bluetooth dentro de um if
  if(SerialBT.connect("Redmi Note 12"))
  {
    Serial.println("Conectado com sucesso!");
  }
  else
  {
    Serial.println("Falha ao conectar!");
  }
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

  initBT("TelemetriaESP32");

  delay(1000); // Aguarda 1 segundo antes de iniciar a calibração
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
  int valorAcelerador = mediaMilivolts(pinAcelerador);
  
  if(estadoInicialCinto == true)
  {
    estadoCintoSeguranca = (valorCintoSeguranca > 3000) ? "desligado" : "ligado";
    if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
    {
      Serial.printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
      SerialBT.println("Cinto de Segurança: " + estadoCintoSeguranca);
      estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
    }
  }
  else
  {
    estadoCintoSeguranca = (valorCintoSeguranca > 3000) ? "ligado" : "desligado";
    if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
    {
      Serial.printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
      SerialBT.println("Cinto de Segurança: " + estadoCintoSeguranca);
      estadoAnteriorCintoSeguranca = estadoCintoSeguranca;
    }
  }

  if(estadoInicialFreioDeMao == true)
  {
    estadoFreioDeMao = (valorFreioDeMao > 3000) ? "desligado" : "ligado";
    if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
    {
      Serial.printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
      SerialBT.println("Freio de Mão: " + estadoFreioDeMao);
      estadoAnteriorFreioDeMao = estadoFreioDeMao;
    }
  }
  else
  {
    estadoFreioDeMao = (valorFreioDeMao > 3000) ? "ligado" : "desligado";
    if(estadoAnteriorFreioDeMao != estadoFreioDeMao)
    {
      Serial.printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
      SerialBT.println("Freio de Mão: " + estadoFreioDeMao);
      estadoAnteriorFreioDeMao = estadoFreioDeMao;
    }
  }

  if(estadoInicialEmbreagem == true)
  {
    estadoEmbreagem = (valorEmbreagem > 3000) ? "livre" : "pressionada";
    if(estadoEmbreagem != estadoAnteriorEmbreagem)
    {
      Serial.printf("Embreagem: %s\n", estadoEmbreagem.c_str());
      SerialBT.println("Embreagem: " + estadoEmbreagem);
      estadoAnteriorEmbreagem = estadoEmbreagem;
    }
  }
  else
  {
    estadoEmbreagem = (valorEmbreagem > 3000) ? "pressionada" : "livre";
    if(estadoEmbreagem != estadoAnteriorEmbreagem)
    {
      Serial.printf("Embreagem: %s\n", estadoEmbreagem.c_str());
      SerialBT.println("Embreagem: " + estadoEmbreagem);
      estadoAnteriorEmbreagem = estadoEmbreagem;
    }
  }

  if(estadoInicialSetaEsquerda == true)
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 3000) ? "desligada" : "ligada";
    if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
    {
      Serial.printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
      SerialBT.println("Seta Esquerda: " + estadoSetaEsquerda);
      estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
    }
  }
  else
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 3000) ? "ligada" : "desligada";
    if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
    {
      Serial.printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
      SerialBT.println("Seta Esquerda: " + estadoSetaEsquerda);
      estadoAnteriorSetaEsquerda = estadoSetaEsquerda;
    }
  }

  if(estadoInicialSetaDireita == true)
  {
    estadoSetaDireita = (valorSetaDireita > 3000) ? "desligada" : "ligada";
    if(estadoAnteriorSetaDireita != estadoSetaDireita)
    {
      Serial.printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
      SerialBT.println("Seta Direita: " + estadoSetaDireita);
      estadoAnteriorSetaDireita = estadoSetaDireita;
    }
  }
  else
  {
    estadoSetaDireita = (valorSetaDireita > 3000) ? "ligada" : "desligada";
    if(estadoAnteriorSetaDireita != estadoSetaDireita)
    {
      Serial.printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
      SerialBT.println("Seta Direita: " + estadoSetaDireita);
      estadoAnteriorSetaDireita = estadoSetaDireita;
    }
  }

  if(estadoInicialPortaAberta == true)
  {
    estadoPortaAberta = (valorPortaAberta > 3000) ? "fechada" : "aberta";
    if(estadoAnteriorPortaAberta != estadoPortaAberta)
    {
      Serial.printf("Porta: %s\n", estadoPortaAberta.c_str());
      SerialBT.println("Porta: " + estadoPortaAberta);
      estadoAnteriorPortaAberta = estadoPortaAberta;
    }
  }
  else
  {
    estadoPortaAberta = (valorPortaAberta > 3000) ? "aberta" : "fechada";
    if(estadoAnteriorPortaAberta != estadoPortaAberta)
    {
      Serial.printf("Porta: %s\n", estadoPortaAberta.c_str());
      SerialBT.println("Porta: " + estadoPortaAberta);
      estadoAnteriorPortaAberta = estadoPortaAberta;
    }
  }
  
  if (estadoInicialFreio == true)
  {
    estadoFreio = (valorFreio > 3000) ? "desligado" : "ligado";
    if(estadoAnteriorFreio != estadoFreio)
    {
      Serial.printf("Freio: %s\n", estadoFreio.c_str());
      SerialBT.println("Freio: " + estadoFreio);
      estadoAnteriorFreio = estadoFreio;
    }
  }
  else
  {
    estadoFreio = (valorFreio > 3000) ? "ligado" : "desligado";
    if(estadoAnteriorFreio != estadoFreio)
    {
      Serial.printf("Freio: %s\n", estadoFreio.c_str());
      SerialBT.println("Freio: " + estadoFreio);
      estadoAnteriorFreio = estadoFreio;
    }
  }

  delay(3000);
}