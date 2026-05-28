# SmartJoy — Fase 3: Python recibe MQTT y controla mouse + teclado
# Ejecutar con: python fase3_python.py

import json
import paho.mqtt.client as mqtt
from pynput.mouse import Controller as MouseController
from pynput.keyboard import Controller as KeyboardController, Key

# ── Configuración HiveMQ ─────────────────────────────────────────────
MQTT_HOST     = "902c9c7bb1a74af18a94675dab007c83.s1.eu.hivemq.cloud"
MQTT_PORT     = 8883
MQTT_USER     = "smartjoy"
MQTT_PASSWORD = "Msd9Qd3JZ@5XNXY"

TOPIC_MOUSE = "smartjoy/mouse"
TOPIC_KEYS  = "smartjoy/keys"

# ── Controladores HID ────────────────────────────────────────────────
mouse    = MouseController()
keyboard = KeyboardController()

TECLAS_ESPECIALES = {
    "enter": Key.enter,
    "space": Key.space,
    "esc":   Key.esc,
}

tecla_activa = None

def resolver_tecla(nombre):
    return TECLAS_ESPECIALES.get(nombre, nombre)

# VERSION2 de paho requiere 5 argumentos en on_connect
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Conectado a HiveMQ Cloud")
        client.subscribe(TOPIC_MOUSE)
        client.subscribe(TOPIC_KEYS)
        print(f"Suscrito a: {TOPIC_MOUSE}, {TOPIC_KEYS}")
        print("Listo — mueve los joysticks!\n")
    else:
        print(f"Error de conexion: {reason_code}")

def on_message(client, userdata, msg):
    global tecla_activa

    try:
        data = json.loads(msg.payload.decode())
    except Exception:
        return

    # ── Mouse ──
    if msg.topic == TOPIC_MOUSE:
        vx = data.get("vx", 0)
        vy = data.get("vy", 0)
        if vx != 0 or vy != 0:
            mouse.move(vx, vy)

    # ── Teclas ──
    elif msg.topic == TOPIC_KEYS:
        nueva = data.get("key", "none")

        if tecla_activa and tecla_activa != nueva:
            try:
                keyboard.release(resolver_tecla(tecla_activa))
            except Exception:
                pass
            tecla_activa = None

        if nueva != "none" and nueva != tecla_activa:
            try:
                keyboard.press(resolver_tecla(nueva))
                tecla_activa = nueva
                print(f"  Tecla: {nueva.upper()}")
            except Exception as e:
                print(f"  Error tecla '{nueva}': {e}")

        if nueva == "none" and tecla_activa:
            try:
                keyboard.release(resolver_tecla(tecla_activa))
            except Exception:
                pass
            tecla_activa = None

def on_disconnect(client, userdata, flags, reason_code, properties):
    print(f"Desconectado: {reason_code}")

# ── Cliente MQTT ─────────────────────────────────────────────────────
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="smartjoy-python")
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
client.tls_set()
client.on_connect    = on_connect
client.on_message    = on_message
client.on_disconnect = on_disconnect

print("=== SmartJoy Fase 3 — Control por MQTT ===")
print(f"Conectando a {MQTT_HOST}:{MQTT_PORT}...")

client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)

try:
    client.loop_forever()
except KeyboardInterrupt:
    if tecla_activa:
        keyboard.release(resolver_tecla(tecla_activa))
    print("\nScript detenido.")