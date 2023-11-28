#include "Arduino.h"
#include "heltec.h"

const int pinSetaEsquerda = 34;
const int pinSetaDireita = 35;
const int pinCintoSeguranca = 36;
const int pinFreioDeMao = 37;
const int pinEmbreagem = 32;

String estadoSetaEsquerda = "desligada";
String estadoSetaDireita = "desligada";
String estadoCintoSeguranca = "desligado";
String estadoFreioDeMao = "desligado";
int estadoEmbreagem = -1; // Inicializado com um valor que não é possível para garantir a primeira impressão

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
  Heltec.begin(true, false, true);
  pinMode(pinSetaEsquerda, INPUT);
  pinMode(pinSetaDireita, INPUT);
  pinMode(pinCintoSeguranca, INPUT);
  pinMode(pinFreioDeMao, INPUT);
  pinMode(pinEmbreagem, INPUT);
  analogSetAttenuation(ADC_11db);
  Heltec.display->setContrast(255);
  limparTela();
}

void loop() {
  // Leitura do estado dos sensores
  int valorSetaEsquerda = analogReadMilliVolts(pinSetaEsquerda);
  int valorSetaDireita = analogReadMilliVolts(pinSetaDireita);
  int valorCintoSeguranca = analogReadMilliVolts(pinCintoSeguranca);
  int valorFreioDeMao = analogReadMilliVolts(pinFreioDeMao);
  int valorEmbreagem = analogReadMilliVolts(pinEmbreagem);

  // Verifica se houve mudança no estado da seta esquerda
  if ((valorSetaEsquerda > 1000 && estadoSetaEsquerda == "desligada") ||
      (valorSetaEsquerda <= 1000 && estadoSetaEsquerda == "ligada")) {
    estadoSetaEsquerda = (valorSetaEsquerda > 1000) ? "ligada" : "desligada";
    Serial.printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
  }

  // Verifica se houve mudança no estado da seta direita
  if ((valorSetaDireita > 1000 && estadoSetaDireita == "desligada") ||
      (valorSetaDireita <= 1000 && estadoSetaDireita == "ligada")) {
    estadoSetaDireita = (valorSetaDireita > 1000) ? "ligada" : "desligada";
    Serial.printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
  }

  // Verifica se houve mudança no estado do cinto de segurança
  if ((valorCintoSeguranca > 1000 && estadoCintoSeguranca == "desligado") ||
      (valorCintoSeguranca <= 1000 && estadoCintoSeguranca == "ligado")) {
    estadoCintoSeguranca = (valorCintoSeguranca > 1000) ? "ligado" : "desligado";
    Serial.printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
  }

  // Verifica se houve mudança no estado do freio de mão
  if ((valorFreioDeMao > 1000 && estadoFreioDeMao == "desligado") ||
      (valorFreioDeMao <= 1000 && estadoFreioDeMao == "ligado")) {
    estadoFreioDeMao = (valorFreioDeMao > 1000) ? "ligado" : "desligado";
    Serial.printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
  }

  // Verifica se houve mudança no estado da embreagem
  if (valorEmbreagem != estadoEmbreagem) {
    estadoEmbreagem = valorEmbreagem;
    Serial.printf("Embreagem: %d\n", estadoEmbreagem);
  }

  // Desenha os elementos na tela
  limparTela();

  if (estadoSetaEsquerda == "ligada")
    desenharSetaEsquerda(10, 35);

  if (estadoSetaDireita == "ligada")
    desenharSetaDireita(100, 35);

  if (estadoCintoSeguranca == "ligado")
    desenharCintoSeguranca(50, 20);

  if (estadoFreioDeMao == "ligado")
    desenharFreioDeMao(50, 10);

  desenharEmbreagem(20, 52, map(estadoEmbreagem, 0, 3300, 0, 100));

  Heltec.display->display();

  delay(500); // Aguarda meio segundo antes de ler novamente
}