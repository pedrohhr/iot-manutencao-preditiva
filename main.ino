#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include "model.h" 

// --- SEUS PINOS ---
#define MEU_SDA 4
#define MEU_SCL 6

// --- CREDENCIAIS ---
const char* ssid = "NOME_WIFI";
const char* password = "SENHA_WIFI";
const char* mqtt_server = "bc554d357e854126b1a75b93c13b11b5.s1.eu.hivemq.cloud"; 
const int   mqtt_port   = 8883;            
const char* mqtt_user   = "psrocha"; 
const char* mqtt_pass   = "SENHA_MQTT"; 
const char* topic_data  = "iot/banca/sensor_data";

// Hardware
// ATENÇÃO: Se sua placa for ESP32-S3, LED_BUILTIN pode variar. 
// Se não acender, tente trocar LED_BUILTIN por 2, 10 ou 13.
#define LED_PIN LED_BUILTIN    
#define BUTTON_PIN 0  

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
WiFiClientSecure espClient; 
PubSubClient client(espClient);

// Variáveis
float offset_desgaste = 0.0;
unsigned long lastMsg = 0;
bool aht_ok = false;
bool bmp_ok = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("\n--- INICIANDO SISTEMA ---");

  // 1. Configura I2C
  Wire.begin(MEU_SDA, MEU_SCL);

  // 2. Inicializa BMP
  if (bmp.begin(0x77) || bmp.begin(0x76)) {
    Serial.println("✅ BMP280 OK");
    bmp_ok = true;
    // Configuração para resposta rápida
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                    Adafruit_BMP280::SAMPLING_X2,     
                    Adafruit_BMP280::SAMPLING_X16,    
                    Adafruit_BMP280::FILTER_OFF,      
                    Adafruit_BMP280::STANDBY_MS_1);
  } else {
    Serial.println("⚠️ BMP Falhou (Usando Simulação)");
  }

  // 3. Inicializa AHT
  if (aht.begin()) {
    Serial.println("✅ AHT20 OK");
    aht_ok = true;
  } else {
    Serial.println("⚠️ AHT Falhou (Usando Simulação)");
  }

  // 4. Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Wi-Fi OK!");
  
  // 5. MQTT Seguro
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(512); 
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32_Banca_" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) { 
      Serial.println("MQTT Conectado!");
    } else {
      Serial.print("Falha MQTT rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 1000) { 
    lastMsg = now;

    float t_base, h_final, p_final;

    // 1. Leitura dos Sensores (Base)
    if (aht_ok) {
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      t_base = temp.temperature;
      h_final = humidity.relative_humidity;
    } else {
      t_base = 25.0 + ((rand() % 100) / 50.0); 
      h_final = 50.0 + ((rand() % 100) / 10.0);
    }

    if (bmp_ok) {
      p_final = bmp.readPressure() / 100.0F;
    } else {
      p_final = 1013.0 + ((rand() % 20) / 10.0); 
    }

    // 2. Aplica Simulação do Botão (Offset)
    if (digitalRead(BUTTON_PIN) == LOW) {
      offset_desgaste += 2.0; // Sobe 2 graus por segundo pressionado
      Serial.println("🔥 Botão: Aumentando temperatura...");
    }
    
    // Temperatura Final (Real + Simulação)
    float t_total = t_base + offset_desgaste;

    // --- LÓGICA DO LED (O QUE VOCÊ PEDIU) ---
    // Regra explícita: Se maior que 45.0, LIGA. Senão, DESLIGA.
    int status_critico = 0;
    
    if (t_total > 45.0) {
      digitalWrite(LED_PIN, HIGH); // Acende
      status_critico = 1;
      Serial.print("🔴 ALERTA! Temp acima de 45C: ");
    } else {
      digitalWrite(LED_PIN, LOW);  // Apaga
      status_critico = 0;
      Serial.print("🟢 Normal: ");
    }
    Serial.println(t_total);

    // AI Edge (Mantemos para enviar o status 'smart' para a nuvem)
    // Passamos a temperatura simulada para a IA concordar com o LED
    int16_t input_features[2] = { (int16_t)t_total, (int16_t)h_final };
    // Opcional: Se quiser que a IA decida, use 'predicao'. 
    // Mas para garantir o 45C, usaremos a variável 'status_critico' no JSON.
    int predicao_ia = edge_model_predict(input_features, 2); 

    // --- CRIAÇÃO DO JSON ---
    JsonDocument doc; 
    doc["temp"] = t_total;     // Envia temperatura já somada
    doc["umid"] = h_final;
    doc["pres"] = p_final; 
    
    // Aqui garantimos que o Dashboard mostre CRÍTICO se passar de 45
    // Usamos 'status_critico' (lógica > 45) em vez da IA antiga para alinhar tudo
    doc["alert_edge"] = status_critico; 

    char buffer[512]; 
    serializeJson(doc, buffer);
    
    if (client.publish(topic_data, buffer)) {
        Serial.print("JSON Enviado: ");
        Serial.println(buffer); 
    } else {
        Serial.println("Erro no envio MQTT");
    }
  }
}