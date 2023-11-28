#include "Arduino.h"
#include "heltec.h"

const int pinSetaEsquerda = 34;
const int pinSetaDireita = 35;
const int pinCintoSeguranca = 36;
const int pinFreioDeMao = 37;
String estadoSetaEsquerda = "desligada";
String estadoSetaDireita = "desligada";
String estadoCintoSeguranca = "desligado";
String estadoFreioDeMao = "desligado";

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

void limparTela()
{
  Heltec.display->clear();
  Heltec.display->display();
}

void setup()
{
  Heltec.begin(true, false, true);
  pinMode(pinSetaEsquerda, INPUT);
  pinMode(pinSetaDireita, INPUT);
  pinMode(pinCintoSeguranca, INPUT);
  analogSetAttenuation(ADC_11db);
  Heltec.display->setContrast(255);
  limparTela();
}

void loop()
{
  // Leitura do estado dos sensores
  int valorSetaEsquerda = analogReadMilliVolts(pinSetaEsquerda);
  int valorSetaDireita = analogReadMilliVolts(pinSetaDireita);
  int valorCintoSeguranca = analogReadMilliVolts(pinCintoSeguranca);
  int valorFreioDeMao = analogReadMilliVolts(pinFreioDeMao);

  // Verifica se houve mudança no estado da seta esquerda
  if ((valorSetaEsquerda > 350 && estadoSetaEsquerda == "desligada") ||
      (valorSetaEsquerda <= 350 && estadoSetaEsquerda == "ligada"))
  {
    estadoSetaEsquerda = (valorSetaEsquerda > 350) ? "ligada" : "desligada";
    Serial.printf("Seta Esquerda: %s\n", estadoSetaEsquerda.c_str());
  }

  // Verifica se houve mudança no estado da seta direita
  if ((valorSetaDireita > 350 && estadoSetaDireita == "desligada") ||
      (valorSetaDireita <= 350 && estadoSetaDireita == "ligada"))
  {
    estadoSetaDireita = (valorSetaDireita > 350) ? "ligada" : "desligada";
    Serial.printf("Seta Direita: %s\n", estadoSetaDireita.c_str());
  }

  // Verifica se houve mudança no estado do cinto de segurança
  if ((valorCintoSeguranca > 350 && estadoCintoSeguranca == "desligado") ||
      (valorCintoSeguranca <= 350 && estadoCintoSeguranca == "ligado"))
  {
    estadoCintoSeguranca = (valorCintoSeguranca > 350) ? "ligado" : "desligado";
    Serial.printf("Cinto de Segurança: %s\n", estadoCintoSeguranca.c_str());
  }

  // Verifica se houve mudança no estado do freio de mão
  if ((valorFreioDeMao > 350 && estadoFreioDeMao == "desligado") ||
      (valorFreioDeMao <= 350 && estadoFreioDeMao == "ligado"))
  {
    estadoFreioDeMao = (valorFreioDeMao > 350) ? "ligado" : "desligado";
    Serial.printf("Freio de Mão: %s\n", estadoFreioDeMao.c_str());
  }

  // Desenha os elementos na tela
  limparTela();

  if (estadoSetaEsquerda == "ligada")
    desenharSetaEsquerda(10, 35);

  if (estadoSetaDireita == "ligada")
    desenharSetaDireita(100, 35);

  if (estadoCintoSeguranca == "ligado")
    desenharCintoSeguranca(60, 40);

  if (estadoFreioDeMao == "ligado")
    desenharFreioDeMao(60, 20);

  Heltec.display->display();

  delay(500); // Aguarda meio segundo antes de ler novamente
}
