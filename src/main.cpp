#include "Arduino.h"
//#include "heltec.h"
#include <ESPAsyncWebServer.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "WiFi.h"

const int iteracoes = 50;

// Configuração do módulo GPS
HardwareSerial gpsSerial(1);
//AsyncWebServer server(80);
// The TinyGPS++ object
TinyGPSPlus gps;
int channel;

const char* ssid = "Telemetria";
const char* password = "esp32pass";
const int localPort = 8080;
const int pinSetaEsquerda = 36; //em teoria tem pulldown
const int pinSetaDireita = 39;  //em teoria tem pulldown
const int pinCintoSeguranca = 34; //em teoria nao tem pulldown
const int pinFreioDeMao = 35; //em teoria nao tem pulldown
//const int pinEmbreagem = 32; //em teoria tem pulldown
const int pinPortaAberta = 33; //em teoria tem pulldown
const int pinFreio = 32; //em teoria tem pulldown
//const int pinAcelerador = 22; //em teoria tem pulldown @MUDAR
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

WiFiClient client;

//Cria uma função de média movel para filtrar ruidos na leitura
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

//Imprime a data no formato dd/mm/yy
String printData()
{
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", gps.date.day(), gps.date.month(), gps.date.year());
    return String(sz);
}

//Imprime a hora no formato hh:mm:ss
String printHora()
{
    char sa[32];
    sprintf(sa, "%02d:%02d:%02d ", gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(sa);
}

//Função que lê a latitude do GPS
String readLatitude() {
  if (gps.location.isValid()) {
    return formatFloat(gps.location.lat(), 6);
  }
  return "Invalid";
}

//Função que lê a longitude do GPS
String readLongitude() {
  if (gps.location.isValid()) {
    return formatFloat(gps.location.lng(), 6);
  }
  return "Invalid";
}

//Função que lê a velocidade do GPS
String readSpeed() {
  if (gps.speed.isValid()) {
    return formatFloat(gps.speed.kmph(), 2);
  }
  return "Invalid";
}

//Função que lê a data do GPS
String readData() {
  if (gps.date.isValid()) {
    return printData();
  }
  return "Invalid";
}

//Função que lê a hora do GPS
String readHora() {
  if (gps.time.isValid()) {
    return printHora();
  }
  return "Invalid"; 
}

//Calibra os sensores antes de iniciar as leituras. Verifica se o valor inicial é ligado ou desligado
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
  // int leituraEmbreagem = mediaMilivolts(pinEmbreagem);
  // estadoInicialEmbreagem = (leituraEmbreagem > 3000);
  // printf("Embreagem: %s\n", estadoInicialEmbreagem ? "pressionada" : "livre");

  Serial.println("Calibração concluída.\n\n\n");
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 23, 22); // RX, TX

  String macAddress;
  uint8_t mac[6];

  WiFi.macAddress(mac);
  for (int i = 0; i < 6; i++)
  {
    macAddress += String(mac[i], HEX);
    if (i < 5)
    {
      macAddress += ":";
    }
  }
  Serial.printf("MAC Address: %s\n", macAddress.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando ao Wi-Fi...");
  }
  // if(client.connect("Telemetria", localPort))
  // {
  //   Serial.println("Conectado ao Wi-Fi");
  //   Serial.print("Endereço IP: ");
  //   Serial.println(WiFi.localIP());
  //   Serial.print("Porta local: ");
  //   Serial.println(localPort);
  // }
  // else
  // {
  //   Serial.println("Falha ao conectar ao Wi-Fi");
  // }

  pinMode(pinSetaEsquerda, INPUT_PULLDOWN);
  pinMode(pinSetaDireita, INPUT_PULLDOWN);
  pinMode(pinCintoSeguranca, INPUT_PULLDOWN);
  pinMode(pinFreioDeMao, INPUT_PULLDOWN);
  // pinMode(pinEmbreagem, INPUT_PULLDOWN);
  pinMode(pinPortaAberta, INPUT_PULLDOWN);
  pinMode(pinFreio, INPUT_PULLDOWN);
  analogSetAttenuation(ADC_11db);

  delay(3000); // Aguarda 1 segundo antes de iniciar a calibração
  calibrarSensores();
}

void loop() {

  // Leitura do estado dos sensores
  int valorSetaEsquerda = mediaMilivolts(pinSetaEsquerda);
  int valorSetaDireita = mediaMilivolts(pinSetaDireita);
  int valorCintoSeguranca = mediaMilivolts(pinCintoSeguranca);
  int valorFreioDeMao = mediaMilivolts(pinFreioDeMao);
  //  int valorEmbreagem = mediaMilivolts(pinEmbreagem);
  int valorPortaAberta = mediaMilivolts(pinPortaAberta);
  int valorFreio = analogRead(pinFreio);
  //int valorAcelerador = mediaMilivolts(pinAcelerador);
  
    if(estadoInicialCinto == true)
    {
      estadoCintoSeguranca = "Cinto: ";
      estadoCintoSeguranca += (valorCintoSeguranca > 3000) ? "desligado" : "ligado";
      if(estadoAnteriorCintoSeguranca != estadoCintoSeguranca)
      {
        Serial.println(estadoCintoSeguranca.c_str());
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
        delay(30);
        estadoAnteriorFreioDeMao = estadoFreioDeMao;
      }
    }

    // if(estadoInicialEmbreagem == true)
    // {
    //   estadoEmbreagem = "Embreagem: ";
    //   estadoEmbreagem += (valorEmbreagem > 3000) ? "livre" : "pressionada";
    //   if(estadoEmbreagem != estadoAnteriorEmbreagem)
    //   {
    //     Serial.println(estadoEmbreagem.c_str());
    //     delay(30);
    //     estadoAnteriorEmbreagem = estadoEmbreagem;
    //   }
    // }
    // else
    // {
    //   estadoEmbreagem = "Embreagem: ";
    //   estadoEmbreagem += (valorEmbreagem > 3000) ? "pressionada" : "livre";
    //   if(estadoEmbreagem != estadoAnteriorEmbreagem)
    //   {
    //     Serial.println(estadoEmbreagem.c_str());
    //     delay(30);
    //     estadoAnteriorEmbreagem = estadoEmbreagem;
    //   }
    // }

    if(estadoInicialSetaEsquerda == true)
    {
      estadoSetaEsquerda = "Seta Esquerda: ";
      estadoSetaEsquerda += (valorSetaEsquerda > 3000) ? "desligada" : "ligada";
      if(estadoAnteriorSetaEsquerda != estadoSetaEsquerda)
      {
        Serial.println(estadoSetaEsquerda.c_str());
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
        delay(30);
        estadoAnteriorFreio = estadoFreio;
      }
    } 
  delay(3000);
}