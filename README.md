# 🏭 Sistema de Manutenção Preditiva IoT com Inteligência Híbrida (Edge & Cloud)

Este projeto apresenta uma solução completa de **Industrial IoT (IIoT)** focada no monitoramento de ativos e predição de falhas. O sistema utiliza uma arquitetura híbrida, combinando **Edge Computing** (processamento na borda com ESP32) e **Cloud Computing** (análise de tendências em Python).

![Status](https://img.shields.io/badge/Status-Concluído-success)
![Tech](https://img.shields.io/badge/Stack-ESP32%20|%20Python%20|%20MQTT-blue)

## 🎯 Objetivos
- Monitorar variáveis críticas (Temperatura, Umidade, Pressão) em tempo real.
- **Edge AI:** Detectar falhas críticas localmente (latência zero) usando Árvores de Decisão embarcadas.
- **Cloud Analytics:** Prever falhas futuras analisando a tendência de aquecimento (Slope/Regressão Linear).
- Notificar operadores via **Telegram** e **Dashboard Web**.

## 🛠️ Arquitetura do Sistema

1.  **Hardware (Edge):**
    - Microcontrolador: **ESP32** (NodeMCU/DevKit).
    - Sensores: **AHT20** (Temp/Umid) e **BMP280** (Pressão).
    - Atuadores: LED de Alerta Local.
2.  **Comunicação:**
    - Protocolo: **MQTT** (Mosquitto/HiveMQ Cloud).
    - Segurança: TLS/SSL.
3.  **Software (Cloud/Dashboard):**
    - **Python 3.x** com Streamlit.
    - Bibliotecas: `scikit-learn` (Machine Learning), `plotly` (Gráficos Interativos), `paho-mqtt`.

## 🚀 Funcionalidades

- [x] **Leitura de Sensores:** Coleta dados via I2C.
- [x] **Simulação de Desgaste:** Botão físico para injetar "drift" nos dados e validar a IA.
- [x] **Inteligência Artificial na Borda:** Modelo `emlearn` rodando dentro do ESP32 para alertas imediatos (>45°C).
- [x] **Manutenção Preditiva:** Cálculo de coeficiente angular (Slope) para detectar aquecimento anormal antes do limite crítico.
- [x] **Alertas Remotos:** Bot do Telegram integrado para avisos de "Manutenção" e "Perigo Crítico".

## 📦 Como Rodar o Projeto

### Pré-requisitos
* Arduino IDE (para o Firmware).
* Python 3.9+ (para o Dashboard).

### 1. Firmware (ESP32)
1. Instale as bibliotecas no Arduino IDE: `Adafruit AHTX0`, `Adafruit BMP280`, `PubSubClient`, `ArduinoJson`.
2. Configure suas credenciais Wi-Fi e MQTT no arquivo `main.ino`.
3. Faça o upload para a placa.

### 2. Dashboard (Python)
1. Clone o repositório.
2. Crie um ambiente virtual e instale as dependências:
   ```bash
   pip install streamlit paho-mqtt pandas plotly scikit-learn requests uuid
