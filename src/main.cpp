#include "Arduino.h"
//#include "heltec.h"
#include <ESPAsyncWebServer.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>

// Configuração do módulo GPS
HardwareSerial gpsSerial(1);

//AsyncWebServer server(80);
// The TinyGPS++ object
TinyGPSPlus gps;

const char* ssid = "WEB-GPS";
const char* password = "esp32password";
const int pinSetaEsquerda = 36;
const int pinSetaDireita = 39;
const int pinCintoSeguranca = 34;
const int pinFreioDeMao = 35;
const int pinEmbreagem = 32;
const int pinPortaAberta = 33;
const int pinFreio = 25;
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
  estadoInicialCinto = (leituraCinto > 2000);
  printf("Cinto de Segurança: %s\n", estadoInicialCinto ? "ligado" : "desligado");

  // Calibração do sensor de freio de mão
  int leituraFreioMao = analogReadMilliVolts(pinFreioDeMao);
  estadoInicialFreioDeMao = (leituraFreioMao > 2000);
  printf("Freio de Mão: %s\n", estadoInicialFreioDeMao ? "ligado" : "desligado");

  // Calibração do sensor de seta esquerda
  int leituraSetaEsquerda = analogReadMilliVolts(pinSetaEsquerda);
  estadoInicialSetaEsquerda = (leituraSetaEsquerda > 2000);
  printf("Seta Esquerda: %s\n", estadoInicialSetaEsquerda ? "ligada" : "desligada");
//
  // Calibração do sensor de seta direita
  int leituraSetaDireita = analogReadMilliVolts(pinSetaDireita);
  estadoInicialSetaDireita = (leituraSetaDireita > 2000);
  printf("Seta Direita: %s\n", estadoInicialSetaDireita ? "ligada" : "desligada");

  // Calibração do sensor de porta aberta
  int leituraPortaAberta = analogReadMilliVolts(pinPortaAberta);
  estadoInicialPortaAberta = (leituraPortaAberta > 2000);
  printf("Porta: %s\n", estadoInicialPortaAberta ? "aberta" : "fechada");

  // Calibração do sensor de freio
  int leituraFreio = analogReadMilliVolts(pinFreio);
  estadoInicialFreio = (leituraFreio > 2000);
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

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 23, 22); // RX, TX

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
  pinMode(pinSetaEsquerda, INPUT);
  pinMode(pinSetaDireita, INPUT);
  pinMode(pinCintoSeguranca, INPUT);
  pinMode(pinFreioDeMao, INPUT);
  pinMode(pinEmbreagem, INPUT);
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
    estadoCintoSeguranca = (valorCintoSeguranca > 2000) ? "desligado" : "ligado";
    printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
  }
  else
  {
    estadoCintoSeguranca = (valorCintoSeguranca > 2000) ? "ligado" : "desligado";
    printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
  }


  if(estadoInicialFreioDeMao == true)
  {
    estadoFreioDeMao = (valorFreioDeMao > 2000) ? "desligado" : "ligado";
    printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
  }
  else
  {
    estadoFreioDeMao = (valorFreioDeMao > 2000) ? "ligado" : "desligado";
    printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
  }

  if (valorEmbreagem != estadoEmbreagem) {
    estadoEmbreagem = valorEmbreagem;
    Serial.printf("Embreagem: %d\n", estadoEmbreagem);
  }

  if(estadoInicialSetaEsquerda == true)
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 2000) ? "desligada" : "ligada";
    printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
  }
  else
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 2000) ? "ligada" : "desligada";
    printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
  }

  if(estadoInicialSetaDireita == true)
  {
    estadoSetaDireita = (valorSetaDireita > 2000) ? "desligada" : "ligada";
    printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
  }
  else
  {
    estadoSetaDireita = (valorSetaDireita > 2000) ? "ligada" : "desligada";
    printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
  }

  if(estadoInicialPortaAberta == true)
  {
    estadoPortaAberta = (valorPortaAberta > 2000) ? "fechada" : "aberta";
    printf("Porta: %s\n", estadoPortaAberta.c_str());
  }
  else
  {
    estadoPortaAberta = (valorPortaAberta > 2000) ? "aberta" : "fechada";
    printf("Porta: %s\n", estadoPortaAberta.c_str());
  }
  
  if (estadoInicialFreio == true)
  {
    estadoFreio = (valorFreio > 2000) ? "desligado" : "ligado";
    printf("Freio: %s\n", estadoFreio.c_str());
  }
  else
  {
    estadoFreio = (valorFreio > 2000) ? "ligado" : "desligado";
    printf("Freio: %s\n", estadoFreio.c_str());
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