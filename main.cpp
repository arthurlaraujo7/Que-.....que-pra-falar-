#include <Arduino.h>
#include "internet.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// lcd
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =======================================================================
// PINOS DO LED RGB
// =======================================================================
const int pinLedVer = 4;
const int pinLedAzul = 14;
const int pinLedVerde = 23;

// =======================================================================
// VARIÁVEIS DE CORES RECEBIDAS
// =======================================================================
String corEsp1 = "";
String corEsp2 = "";
String corAtual = "";


// =======================================================================
// FUNÇÃO PARA ATUALIZAR LCD
// =======================================================================
void atualizarLCD()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("");
  lcd.print(corEsp1);
  lcd.print("+");
  lcd.print(corEsp2);

  lcd.setCursor(0, 1);
  lcd.print("=");
  lcd.print(corAtual);
}

// =======================================================================
// FUNÇÕES PARA ACENDER CORES
// =======================================================================
void vermelho()
{
  analogWrite(pinLedVer, 255);
  analogWrite(pinLedAzul, 0);
  analogWrite(pinLedVerde, 0);
  corAtual = "vermelho";
}

void azul()
{
  analogWrite(pinLedVer, 0);
  analogWrite(pinLedAzul, 255);
  analogWrite(pinLedVerde, 0);
  corAtual = "azul";
}

void verde()
{
  analogWrite(pinLedVer, 0);
  analogWrite(pinLedAzul, 0);
  analogWrite(pinLedVerde, 255);
  corAtual = "verde";
}

void roxo()
{
  analogWrite(pinLedVer, 255);
  analogWrite(pinLedAzul, 210);
  analogWrite(pinLedVerde, 0);
  corAtual = "roxo";
}

void ciano()
{
  analogWrite(pinLedVer, 0);
  analogWrite(pinLedAzul, 200);
  analogWrite(pinLedVerde, 200);
  corAtual = "ciano";
}

void laranja()
{
  analogWrite(pinLedVer, 255);
  analogWrite(pinLedAzul, 0);
  analogWrite(pinLedVerde, 25);
  corAtual = "laranja";
}

// =======================================================================
// MQTT
// =======================================================================
const char *mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char *mqtt_client_id = "IOT_grupo7_esp3";

const char *mqtt_topic_sub = "2025_grupo7/projeto_Grupo7/sub";
const char *mqtt_topic_pub = "2025_grupo7/projeto_Grupo7/pub";

WiFiClient espClient;
PubSubClient client(espClient);

// =======================================================================
// PROTÓTIPOS
// =======================================================================
void conectaMqtt();
void retornoMqtt(char *topic, byte *payload, unsigned int length);
void calcularResultante();
void enviaCorMQTT();

// =======================================================================
// SETUP
// =======================================================================
void setup()
{
  Serial.begin(115200);

  pinMode(pinLedVer, OUTPUT);
  pinMode(pinLedAzul, OUTPUT);
  pinMode(pinLedVerde, OUTPUT);

  conectaWiFi();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aguardando...");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(retornoMqtt);
}

// =======================================================================
// LOOP
// =======================================================================
void loop()
{
  checkWiFi();
  client.loop();

  if (!client.connected())
  {
    conectaMqtt();
  }
}

// =======================================================================
// FUNÇÃO MQTT - CONEXÃO
// =======================================================================
void conectaMqtt()
{
  while (!client.connected())
  {
    Serial.println("Conectando ao MQTT...");
    if (client.connect(mqtt_client_id))
    {
      Serial.println("Conectado!");
      client.subscribe(mqtt_topic_sub);
    }
    else
    {
      Serial.print("Falha, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

// =======================================================================
// FUNÇÃO CALCULA RESULTANTE
// =======================================================================
void calcularResultante()
{
  if (corEsp1 == "" || corEsp2 == "")
    return; 
  if (corEsp1 == corEsp2)
  {
    if (corEsp1 == "vermelho")
      vermelho();
    else if (corEsp1 == "azul")
      azul();
    else if (corEsp1 == "verde")
      verde();
  }
  // COMBINAÇÕES
  else if ((corEsp1 == "vermelho" && corEsp2 == "azul") || (corEsp1 == "azul" && corEsp2 == "vermelho"))
  {
    roxo();
  }
  else if ((corEsp1 == "azul" && corEsp2 == "verde") || (corEsp1 == "verde" && corEsp2 == "azul"))
  {
    ciano();
  }
  else if ((corEsp1 == "vermelho" && corEsp2 == "verde") || (corEsp1 == "verde" && corEsp2 == "vermelho"))
  {
    laranja();
  }
  else
  {
    Serial.println("Combinação desconhecida!");
    corAtual = "";
  }

  Serial.print("Cor resultante: ");
  Serial.println(corAtual);

  // envia resultado
  if (corAtual != "")
  { 
    enviaCorMQTT();
  } 
  atualizarLCD();  
  // corEsp1 = "";
  // corEsp2 = "";

}

// =======================================================================
// FUNÇÃO ENVIA COR VIA MQTT
// =======================================================================
void enviaCorMQTT()
{
  JsonDocument doc;
  doc["disp"] = "ESP_3";
  doc["msg"] = corAtual;
  doc["time"] = millis();

  String saida;
  serializeJson(doc, saida);
  client.publish(mqtt_topic_pub, saida.c_str());

  Serial.print("Mensagem enviada ESP3: ");
  Serial.println(saida);
}

// =======================================================================
// CALLBACK MQTT
// =======================================================================
void retornoMqtt(char *topic, byte *payload, unsigned int length)
{
  String mensagemRecebida = "";
  for (int i = 0; i < length; i++)
  {
    mensagemRecebida += (char)payload[i];
  }

  Serial.print("Mensagem recebida: ");
  Serial.println(mensagemRecebida);

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, mensagemRecebida);
  if (erro)
  {
    Serial.println("Erro ao decodificar JSON");
    return;
  }

  String dispositivo = doc["disp"].as<String>();
  String cor_recebida = doc["msg"].as<String>();

  dispositivo.toUpperCase(); 
  if (dispositivo == "ESP_1")
  {
    corEsp1 = cor_recebida;
  }
  else if (dispositivo == "ESP_2")
  {
    corEsp2 = cor_recebida;
  }
  else
  {
    Serial.println("Dispositivo desconhecido, ignorando");
    return;
  }

  atualizarLCD(); 

  Serial.print("ESP1: ");
  Serial.println(corEsp1);
  Serial.print("ESP2: ");
  Serial.println(corEsp2);

  if (corEsp1 != "" && corEsp2 != "")
  {
    calcularResultante();
  }
}