#include "Arduino.h"
#include "heltec.h"
#include <ESPAsyncWebServer.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Configuração do módulo GPS
//SoftwareSerial Serial2(16, 17);  // RX, TX

AsyncWebServer server(80);
// The TinyGPS++ object
TinyGPSPlus gps;

const char* ssid = "WEB-GPS";
const char* password = "esp32password";
const int pinSetaEsquerda = 34;
const int pinSetaDireita = 35;
const int pinCintoSeguranca = 36;
const int pinFreioDeMao = 37;
const int pinEmbreagem = 38;
const int pinPortaAberta = 39;
const int pinFreio = 32;
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
  estadoInicialCinto = (leituraCinto > 1600);
  printf("Cinto de Segurança: %s\n", estadoInicialCinto ? "ligado" : "desligado");

  // Calibração do sensor de freio de mão
  int leituraFreioMao = analogReadMilliVolts(pinFreioDeMao);
  estadoInicialFreioDeMao = (leituraFreioMao > 1600);
  printf("Freio de Mão: %s\n", estadoInicialFreioDeMao ? "ligado" : "desligado");

  // Calibração do sensor de seta esquerda
  int leituraSetaEsquerda = analogReadMilliVolts(pinSetaEsquerda);
  estadoInicialSetaEsquerda = (leituraSetaEsquerda > 1600);
  printf("Seta Esquerda: %s\n", estadoInicialSetaEsquerda ? "ligada" : "desligada");
//
  // Calibração do sensor de seta direita
  int leituraSetaDireita = analogReadMilliVolts(pinSetaDireita);
  estadoInicialSetaDireita = (leituraSetaDireita > 1600);
  printf("Seta Direita: %s\n", estadoInicialSetaDireita ? "ligada" : "desligada");

  // Calibração do sensor de porta aberta
  int leituraPortaAberta = analogReadMilliVolts(pinPortaAberta);
  estadoInicialPortaAberta = (leituraPortaAberta > 1600);
  printf("Porta: %s\n", estadoInicialPortaAberta ? "aberta" : "fechada");

  // Calibração do sensor de freio
  int leituraFreio = analogReadMilliVolts(pinFreio);
  estadoInicialFreio = (leituraFreio > 1600);
  printf("Freio: %s\n", estadoInicialFreio ? "ligado" : "desligado");

  Serial.println("Calibração concluída.\n\n\n");
}

void desenharSetaDireita(int x, int y)
{
  int tamanhoSeta = 13; // Ajuste o tamanho da seta conforme necessário
  // Desenha o corpo da seta apontando para a direita
  Heltec.display->drawLine(x, y - 5, x + tamanhoSeta, y - 5);
  Heltec.display->drawLine(x, y + 5, x + tamanhoSeta, y + 5);
  Heltec.display->drawLine(x + tamanhoSeta, y + 5, x + tamanhoSeta, y + 10);
  Heltec.display->drawLine(x + tamanhoSeta, y - 5, x + tamanhoSeta, y - 10);
  //ponta
  Heltec.display->drawLine(x + tamanhoSeta, y + 10, x + tamanhoSeta + 10, y);
  Heltec.display->drawLine(x + tamanhoSeta, y - 10, x + tamanhoSeta + 10, y);
}

void desenharSetaEsquerda(int x, int y)
{
  int tamanhoSeta = 13; // Ajuste o tamanho da seta conforme necessário
  // Desenha o corpo da seta apontando para a esquerda
  Heltec.display->drawLine(x, y - 5, x + tamanhoSeta, y - 5);
  Heltec.display->drawLine(x, y + 5, x + tamanhoSeta, y + 5);
  Heltec.display->drawLine(x, y + 5, x, y + 10);
  Heltec.display->drawLine(x, y - 5, x, y - 10);
  //ponta
  Heltec.display->drawLine(x, y + 10, x - 10, y);
  Heltec.display->drawLine(x, y - 10, x - 10, y);
}

void desenharCintoSeguranca(int x, int y)
{
  //usar o heltec text pra escrever cinto
  Heltec.display->drawString(x, y, "CINTO");
}

void desenharFreioDeMao(int x, int y)
{
  //usar o heltec text pra escrever cinto
  Heltec.display->drawString(x, y, "FREIO MÃO");
}

void desenharPorta(int x, int y)
{
  //usar o heltec text pra escrever cinto
  Heltec.display->drawString(x, y, "PORTA");
}

void desenharFreio(int x, int y)
{
  //usar o heltec text pra escrever cinto
  Heltec.display->drawString(x, y, "FREIO");
}

void desenharEmbreagem(int x, int y, int nivel)
{
  // Ajusta o nível para garantir que esteja dentro do intervalo [0, 100]
  nivel = constrain(nivel, 0, 100);

  // Desenha a barra de progresso para representar o nível da embreagem
  Heltec.display->drawProgressBar(x, y, 80, 7, nivel);
}

void limparTela()
{
  Heltec.display->clear();
  Heltec.display->display();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Setting the ESP as an access point
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/latitude", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readLatitude().c_str());
  });
  server.on("/longitude", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readLongitude().c_str());
  });
  server.on("/velocidade", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSpeed().c_str());
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readData().c_str());
  });
  server.on("/hora", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readHora().c_str());
  });
  server.on("/setaEsquerda", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", estadoSetaEsquerda.c_str());
  });
  server.on("/setaDireita", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", estadoSetaDireita.c_str());
  });
  server.on("/cintoSeguranca", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", estadoCintoSeguranca.c_str());
  });
  server.on("/freioDeMao", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", estadoFreioDeMao.c_str());
  });
  server.on("/portaAberta", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", estadoPortaAberta.c_str());
  });
  server.on("/freio", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", estadoFreio.c_str());
  });
  server.on("/embreagem", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(estadoEmbreagem).c_str());
  });
  
  server.begin();

  Heltec.begin(true, false, true);
  pinMode(pinSetaEsquerda, INPUT);
  pinMode(pinSetaDireita, INPUT);
  pinMode(pinCintoSeguranca, INPUT);
  pinMode(pinFreioDeMao, INPUT);
  pinMode(pinEmbreagem, INPUT);
  analogSetAttenuation(ADC_11db);
  Heltec.display->setContrast(255);

  delay(1600); // Aguarda 1 segundo antes de iniciar a calibração
  calibrarSensores();
  limparTela();
}

void loop() {

  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      // Dados do GPS atualizados
    }
  }

  printf("\n");
  printf(readData().c_str());
  printf("\n");
  printf(readHora().c_str());
  printf("\n");
  printf(readSpeed().c_str());
  printf("\n");
  printf(readLatitude().c_str());
  printf("\n");
  printf(readLongitude().c_str());
  printf("\n\n");

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
    estadoCintoSeguranca = (valorCintoSeguranca > 1600) ? "desligado" : "ligado";
    printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
  }
  else
  {
    estadoCintoSeguranca = (valorCintoSeguranca > 1600) ? "ligado" : "desligado";
    printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
  }


  if(estadoInicialFreioDeMao == true)
  {
    estadoFreioDeMao = (valorFreioDeMao > 1600) ? "desligado" : "ligado";
    printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
  }
  else
  {
    estadoFreioDeMao = (valorFreioDeMao > 1600) ? "ligado" : "desligado";
    printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
  }

  if (valorEmbreagem != estadoEmbreagem) {
    estadoEmbreagem = valorEmbreagem;
    Serial.printf("Embreagem: %d\n", estadoEmbreagem);
  }

  if(estadoInicialSetaEsquerda == true)
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 1600) ? "desligada" : "ligada";
    printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
  }
  else
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 1600) ? "ligada" : "desligada";
    printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
  }

  if(estadoInicialSetaDireita == true)
  {
    estadoSetaDireita = (valorSetaDireita > 1600) ? "desligada" : "ligada";
    printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
  }
  else
  {
    estadoSetaDireita = (valorSetaDireita > 1600) ? "ligada" : "desligada";
    printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
  }

  if(estadoInicialPortaAberta == true)
  {
    estadoPortaAberta = (valorPortaAberta > 1600) ? "fechada" : "aberta";
    printf("Porta: %s\n", estadoPortaAberta.c_str());
  }
  else
  {
    estadoPortaAberta = (valorPortaAberta > 1600) ? "aberta" : "fechada";
    printf("Porta: %s\n", estadoPortaAberta.c_str());
  }
  
  if (estadoInicialFreio == true)
  {
    estadoFreio = (valorFreio > 1600) ? "desligado" : "ligado";
    printf("Freio: %s\n", estadoFreio.c_str());
  }
  else
  {
    estadoFreio = (valorFreio > 1600) ? "ligado" : "desligado";
    printf("Freio: %s\n", estadoFreio.c_str());
  }

  // Desenha os elementos na tela
  limparTela();

  if (estadoSetaEsquerda == "ligada")
    desenharSetaEsquerda(10, 30);

  if (estadoSetaDireita == "ligada")
    desenharSetaDireita(100, 30);

  if (estadoCintoSeguranca == "ligado")
    desenharCintoSeguranca(45, 20);

  if (estadoFreioDeMao == "ligado")
    desenharFreioDeMao(36, 0);

  if (estadoPortaAberta == "aberta")
    desenharPorta(45, 30);

  if (estadoFreio == "ligado")
    desenharFreio(45, 10);
    
  //imprimir na tela os dados do GPS
  Heltec.display->drawString(0, 0, readData().c_str());
  Heltec.display->drawString(0, 7, readHora().c_str());
  Heltec.display->drawString(95, 0, readSpeed().c_str());
  Heltec.display->drawString(95, 7, readLatitude().c_str());

  desenharEmbreagem(22, 53, map(estadoEmbreagem, 0, 3300, 0, 100));

  Heltec.display->display();

  delay(2000); // Aguarda meio segundo antes de ler novamente
}