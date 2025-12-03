import streamlit as st
import pandas as pd
import paho.mqtt.client as mqtt
import json
import time
import ssl
import plotly.express as px
from sklearn.linear_model import LinearRegression
import numpy as np
import uuid
import requests # <--- NOVA BIBLIOTECA PARA O TELEGRAM

# --- CONFIGURAÇÕES DE TELEGRAM ---
# Substitua pelos seus dados REAIS que pegou no BotFather
TELEGRAM_TOKEN = "SEU_TOKEN_AQUI" 
TELEGRAM_CHAT_ID = "SEU_ID_AQUI"

def enviar_telegram(mensagem):
    try:
        url = f"https://api.telegram.org/bot{TELEGRAM_TOKEN}/sendMessage"
        data = {"chat_id": TELEGRAM_CHAT_ID, "text": mensagem}
        
        # Pega a resposta do servidor
        response = requests.post(url, data=data)
        
        # Verifica se o Telegram aceitou (Código 200 = OK)
        if response.status_code == 200:
            print(f"📲 Telegram enviado com SUCESSO!")
        else:
            # Se deu erro, mostra o motivo exato
            print(f"❌ ERRO TELEGRAM: {response.text}")
            
    except Exception as e:
        print(f"Erro na conexão com Telegram: {e}")

# --- CONFIGURAÇÕES MQTT ---
BROKER = "bc554d357e854126b1a75b93c13b11b5.s1.eu.hivemq.cloud"
PORT = 8883
MQTT_USER = "psrocha"
MQTT_PASS = "SUA_SENHA_AQUI"
TOPIC = "iot/banca/sensor_data"

st.set_page_config(page_title="Monitoramento Preditivo", layout="wide")
st.title("🏭 Monitoramento de Ativos - Manutenção Preditiva")

# --- BUFFER GLOBAL ---
if 'global_data' not in st.session_state:
    st.session_state['global_data'] = []
    st.session_state['last_alert_time'] = 0 # Para não fazer spam de mensagens

data_buffer = []

# --- CALLBACKS MQTT ---
def on_connect(client, userdata, flags, rc, properties):
    if rc == 0:
        print("✅ CONECTADO AO MQTT!")
    else:
        print(f"❌ Falha: {rc}")

def on_message(client, userdata, msg):
    global data_buffer
    try:
        payload = json.loads(msg.payload.decode())
        payload['time'] = time.time()
        data_buffer.append(payload)
        if len(data_buffer) > 100:
            data_buffer.pop(0)
    except Exception as e:
        print(f"Erro: {e}")

# --- CONEXÃO ---
try:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.tls_set(cert_reqs=ssl.CERT_NONE)
    client.connect(BROKER, PORT, 60)
    client.subscribe(TOPIC)
    client.loop_start()
except Exception as e:
    st.error(f"Erro MQTT: {e}")

# --- CONTAINER PRINCIPAL ---
main_placeholder = st.empty()

while True:
    with main_placeholder.container():
        if len(data_buffer) > 0:
            df = pd.DataFrame(data_buffer)
            last = df.iloc[-1]
            
            current_temp = last.get('temp', 0)
            current_slope = 0.0

            # --- ANÁLISE PREDITIVA (Regressão Linear) ---
            window_size = 5 
            if len(df) > window_size:
                last_n = df.tail(window_size).reset_index()
                X = last_n.index.values.reshape(-1, 1)
                Y = last_n['temp'].values.reshape(-1, 1)
                reg = LinearRegression().fit(X, Y)
                current_slope = reg.coef_[0][0]
                
                # --- SISTEMA DE ALERTAS INTELIGENTE (TELEGRAM) ---
                now = time.time()
                # Só manda mensagem a cada 30 segundos para não travar seu celular
                if now - st.session_state['last_alert_time'] > 30:
                    
                    # 1. ALERTA CRÍTICO (Reativo - O Fogo)
                    if current_temp > 45.0:
                        msg = f"🔥 PERIGO CRÍTICO! Temperatura: {current_temp:.1f}°C. Parada Imediata!"
                        enviar_telegram(msg)
                        st.session_state['last_alert_time'] = now
                        st.toast("📲 Alerta de FOGO enviado!", icon="🔥")
                    
                    # 2. ALERTA PREDITIVO (A Manutenção)
                    # Se temperatura ok, mas subindo muito rápido
                    elif current_slope > 0.05:
                        msg = f"⚠️ MANUTENÇÃO PREDITIVA: Aquecimento anormal detectado (Slope: {current_slope:.3f}). Verifique lubrificação."
                        enviar_telegram(msg)
                        st.session_state['last_alert_time'] = now
                        st.toast("📲 Alerta Preditivo enviado!", icon="⚠️")

                # Visualização na tela
                if current_temp > 45.0:
                    st.error(f"🔥 FOGO! Temp: {current_temp:.1f}°C")
                elif current_slope > 0.05:
                    st.warning(f"⚠️ PREDITIVA: Tendência de Alta (Slope: {current_slope:.3f})")
                else:
                    st.success(f"✅ Sistema Estável (Slope: {current_slope:.3f})")

            # KPIs
            k1, k2, k3, k4, k5 = st.columns(5)
            k1.metric("Temperatura", f"{current_temp:.1f} °C")
            k2.metric("Umidade", f"{last.get('umid',0):.1f} %")
            k3.metric("Pressão", f"{last.get('pres',0):.0f} hPa")
            k4.metric("Predictive Slope", f"{current_slope:.4f}") 
            k5.metric("Edge Status", "CRÍTICO" if last.get('alert_edge',0) == 1 else "Normal")

            # Gráfico
            fig = px.line(df, x='time', y='temp', title="Evolução Térmica e Predição")
            fig.add_hline(y=45, line_dash="dot", line_color="red", annotation_text="Limite Crítico (45°C)")
            st.plotly_chart(fig, width="stretch", key=str(uuid.uuid4()))
        
        else:
            st.info("Aguardando dados da Nuvem...")

    time.sleep(0.5)