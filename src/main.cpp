#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <Ticker.h>

#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <RotaryEncoder.h>

#include "AsyncJson.h"
#include "ArduinoJson.h"

ADC_MODE(ADC_VCC);

#define MOTOR_1 D1
#define MOTOR_2 D2
#define ENCODER_1 D7
#define ENCODER_2 D5

#define POS_UP 0
#define POS_DOWN -196

AsyncWebServer server(80);
DNSServer dns;
Ticker ticker;
Ticker ticker2;
RotaryEncoder *encoder = nullptr;

int direction = 0;
auto position = 0;

void stop() {
    digitalWrite(MOTOR_1, LOW);
    digitalWrite(MOTOR_2, LOW);
    direction = 0;
}

IRAM_ATTR void checkPosition()
{
  encoder->tick(); // just call tick() to check the state.
  auto pos = encoder->getPosition();
  if ((pos == POS_UP && direction == 1 )|| (pos == POS_DOWN && direction == -1)) {
    stop();
  }
}

void setup() {
  encoder = new RotaryEncoder(ENCODER_1, ENCODER_2, RotaryEncoder::LatchMode::TWO03);
  encoder->setPosition(0);
  // register interrupt routine
  attachInterrupt(digitalPinToInterrupt(ENCODER_1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_2), checkPosition, CHANGE);

  Serial.begin(115200);
  pinMode(MOTOR_1, OUTPUT);
  pinMode(MOTOR_2, OUTPUT);
  digitalWrite(MOTOR_1, LOW);
  digitalWrite(MOTOR_2, LOW);

  WiFi.persistent(true);
  AsyncWiFiManager wifiManager(&server,&dns);
  wifiManager.autoConnect("ESP_BLINDS");
  Serial.println("Connected");
  Serial.println(WiFi.localIP());

  server.on("/up", HTTP_GET, [&](AsyncWebServerRequest *request){
    digitalWrite(MOTOR_1, LOW);
    digitalWrite(MOTOR_2, HIGH);
    direction = 1;
    request->send(200);
  });
  server.on("/down", HTTP_GET, [&](AsyncWebServerRequest *request){
    digitalWrite(MOTOR_1, HIGH);
    digitalWrite(MOTOR_2, LOW);
    direction = -1;
    request->send(200);
  });
  server.on("/stop", HTTP_GET, [&](AsyncWebServerRequest *request){
    stop();
    request->send(200);
  });
  server.on("/pos", HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["pos"] = encoder->getPosition();
    serializeJson(jsonDoc, *response);
    request->send(response);
  });

  server.on("/full_up", HTTP_GET, [&](AsyncWebServerRequest *request){
    digitalWrite(MOTOR_1, LOW);
    digitalWrite(MOTOR_2, HIGH);
    direction = 1;
    request->send(200);
  });
  server.on("/full_down", HTTP_GET, [&](AsyncWebServerRequest *request){
    digitalWrite(MOTOR_1, HIGH);
    digitalWrite(MOTOR_2, LOW);
    direction = -1;
    request->send(200);
  });

  server.on("/status", HTTP_GET, [&](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["battery"] = ESP.getVcc();
    serializeJson(jsonDoc, *response);
    request->send(response);
  });
  

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  server.begin();
}

void loop() {
  ArduinoOTA.handle();
  delay(1);
}