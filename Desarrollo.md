# Proceso de Desarrollo — SmartJoy IoT

Documentación del proceso de desarrollo por fases, incluyendo decisiones técnicas, obstáculos encontrados y soluciones implementadas.

---

## Fase 1 — Lectura y calibración de sensores KY-023

### Objetivo
Verificar que los dos joysticks KY-023 leen correctamente los ejes X/Y y el botón antes de agregar cualquier componente de red.

### Qué implica
- Conexión física de dos KY-023 al ESP32 WROOM-32
- Lectura analógica de 4 ejes (GPIO 34, 35, 32, 33) y un botón digital (GPIO 25)
- Calibración automática del punto central de cada joystick al arrancar
- Aplicación de zona muerta para evitar detección de teclas fantasma

### Requisitos
- ESP32 WROOM-32
- 2× KY-023 conectados a 3.3V (no 5V)
- Arduino IDE con soporte ESP32
- Librería: ninguna extra, solo `analogRead()` y `digitalRead()` nativos

### Problema encontrado: centro no estándar
El ESP32 asume que el centro del joystick es 2048 (mitad del rango de 12 bits), pero los KY-023 físicos tienen un centro real aproximado de 1800 debido a tolerancias de fabricación. Esto causaba que siempre se detectara la tecla "A" aunque el joystick estuviera quieto.

**Solución:** calibración automática al arrancar — el ESP32 toma 30 muestras de cada eje durante 3 segundos con el joystick en reposo y calcula el centro real. Desde ese momento todas las detecciones son relativas a ese centro.

### Criterio de logro
✅ Los valores de delta X e Y se mantienen cerca de 0 cuando el joystick está quieto, y cambian correctamente al moverlo en cualquier dirección. La tecla detectada cambia entre W/A/S/D al mover el joystick #2.

---

## Fase 2 — Control de mouse y teclado desde Python

### Objetivo
Verificar que Python puede mover el mouse y presionar teclas en la PC de forma programática, sin involucrar el ESP32 todavía.

### Qué implica
- Uso de la librería `pynput` para controlar el mouse y el teclado
- Prueba con valores hardcodeados (sin hardware real aún)
- Verificación de que teclas especiales como Enter funcionan correctamente

### Requisitos
- Python 3.x instalado en Windows
- Librería: `pip install pynput`

### Problema encontrado: teclas especiales
La librería `pynput` en Windows no acepta el string `"enter"` directamente como tecla. Intentar `keyboard.press("enter")` lanza una excepción. Las teclas especiales requieren el enum `Key.enter` del módulo `pynput.keyboard`.

**Solución:** se implementó un diccionario de mapeo de strings a objetos `Key`:
```python
TECLAS_ESPECIALES = {
    "enter": Key.enter,
    "space": Key.space,
    "esc":   Key.esc,
}
```

### Criterio de logro
✅ El mouse se mueve en un cuadrado de forma autónoma y las letras W, A, S, D aparecen en el Bloc de Notas al ejecutar el script de prueba.

---

## Fase 3 — Comunicación MQTT con TLS entre ESP32 y Python

### Objetivo
Conectar el ESP32 y el script Python a través de HiveMQ Cloud usando MQTT con cifrado TLS, de modo que los movimientos del joystick lleguen a Python en tiempo real.

### Qué implica
- Cuenta en HiveMQ Cloud (plan Serverless gratuito)
- ESP32 publica en tópicos `smartjoy/mouse` y `smartjoy/keys` cada 50ms
- Python se suscribe a esos tópicos y controla el mouse/teclado en respuesta
- Comunicación cifrada por TLS en puerto 8883

### Requisitos
- Librería Arduino: `PubSubClient` de Nick O'Leary
- Librería Python: `pip install paho-mqtt`
- Red Wi-Fi con acceso a internet (hotspot móvil recomendado)
- Credenciales de HiveMQ Cloud

### Por qué se usó Python en lugar de HID nativo

Originalmente el proyecto buscaba que el ESP32 emulara un dispositivo HID (mouse + teclado USB) directamente, como si fuera un gamepad. Esto es técnicamente posible pero **solo en ESP32-S2 y ESP32-S3**. El ESP32 WROOM-32 disponible en el proyecto no tiene soporte nativo de USB HID — su puerto USB es exclusivamente para programación y comunicación Serial.

Se evaluaron alternativas:
- **Arduino Leonardo/Pro Micro** como intermediario HID — descartado por no tener el hardware disponible
- **Cambio a ESP32-S2/S3** — descartado por restricciones de tiempo y presupuesto

La solución adoptada fue un **script Python corriendo en la PC** que se suscribe al broker MQTT y usa `pynput` para traducir los mensajes del joystick en movimientos reales de mouse y pulsaciones de teclado. Esto cumple el objetivo del proyecto y todos los requisitos del curso.

### Problema encontrado: red universitaria bloqueada
La red Wi-Fi de la universidad (`Estudiante-Usabana`) usa autenticación WPA2-Enterprise con cuentas de Microsoft, lo que impide la conexión del ESP32 (que solo soporta WPA2-Personal). Además, dicha red bloquea el puerto 8883 (MQTT/TLS).

**Solución:** uso de hotspot móvil personal como red de datos para el ESP32 y la PC durante las pruebas.

### Problema encontrado: certificado TLS incorrecto
El primer intento usó el certificado ISRG Root X1 (Let's Encrypt), pero HiveMQ Cloud usa **Amazon Root CA 1** como autoridad certificadora. Esto causaba `rc=-2` (fallo de conexión TLS) de forma consistente.

Adicionalmente, el método `setFingerprint()` de `WiFiClientSecure` fue eliminado en versiones recientes de la librería para ESP32, por lo que no era posible usar verificación por huella digital SHA-256.

**Solución:** uso del certificado Amazon Root CA 1 en formato `PROGMEM` con raw string literal `R"EOF(...)EOF"`. El formato `PROGMEM` es clave — almacena el certificado en Flash en lugar de RAM, resolviendo el problema de memoria del ESP32 WROOM que impedía el handshake TLS completo.

```cpp
static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)EOF";
```

### Problema encontrado: API deprecada en paho-mqtt
La versión reciente de `paho-mqtt` para Python cambió la firma de los callbacks `on_connect` y `on_disconnect` de 4 a 5 argumentos. El código antiguo lanzaba `TypeError: on_connect() takes 4 positional arguments but 5 were given`.

**Solución:** migración a `CallbackAPIVersion.VERSION2` y actualización de firmas:
```python
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="smartjoy-python")

def on_connect(client, userdata, flags, reason_code, properties):
    ...

def on_disconnect(client, userdata, flags, reason_code, properties):
    ...
```

### Criterio de logro
✅ El Serial Monitor muestra "conectado!" al broker HiveMQ. Python muestra "Listo — mueve los joysticks!" y el mouse de la PC responde en tiempo real al mover el JOY1. Las teclas WASD se detectan correctamente al mover el JOY2.

---

## Fase 4 — Calibración y pruebas de campo

### Objetivo
Ajustar la sensibilidad del mouse y la zona muerta para que el control sea usable en un juego real.

### Qué implica
- Ajuste de la función de mapeo joystick → velocidad de mouse
- Verificación de que las teclas se presionan y sueltan sin delay perceptible
- Prueba de uso continuo por al menos 30 segundos

### Parámetros ajustados
| Parámetro | Valor | Efecto |
|---|---|---|
| `DEADZONE` | 150 | Rango de reposo relativo al centro calibrado |
| `velMax` | 10 | Velocidad máxima del mouse en píxeles por tick |
| Frecuencia de publicación | 50ms | 20 mensajes por segundo |

### Criterio de logro
✅ El mouse se mueve suavemente sin saltos. Las teclas WASD responden sin delay visible. No hay teclas fantasma cuando el joystick está en reposo.

---

## Fase 5 — NTP y Healthcheck HTTP

### Objetivo
Agregar sincronización de tiempo real (NTP) y un endpoint HTTP de estado al ESP32, cumpliendo los requisitos obligatorios del curso.

### Qué implica
- Sincronización de hora con servidor NTP al arrancar
- Servidor HTTP embebido en el ESP32 que responde en `/status`
- Timestamps reales incluidos en cada mensaje MQTT publicado

### Requisitos
- Librería: `WebServer.h` (incluida en el core de ESP32)
- Librería: `time.h` (incluida en el core de ESP32)

### Endpoint /status
```
GET http://<IP_DEL_ESP32>/status

Respuesta:
{
  "status": "ok",
  "uptime_s": 120,
  "wifi": "connected",
  "mqtt": "connected",
  "ip": "192.168.x.x",
  "time": "2026-05-27T10:35:00"
}
```

### Problema encontrado: NTP bloqueado en red universitaria
La red universitaria bloquea el puerto 123 (NTP), por lo que el ESP32 no puede sincronizar la hora cuando está conectado a esa red. El sistema espera un máximo de 20 intentos y continúa con `"sin-hora"` si no logra sincronizar.

**Solución:** se usan dos servidores NTP como fallback (`pool.ntp.org` y `time.google.com`). En red móvil funciona correctamente.

### Problema encontrado: IP `0.0.0.0` en el servidor HTTP
Cuando el servidor HTTP se inicializa muy rápido después del Wi-Fi, la IP aún no está asignada y muestra `0.0.0.0`.

**Solución:** se agregó `delay(2000)` entre la conexión Wi-Fi y la inicialización del servidor HTTP para dar tiempo a que el DHCP asigne la IP.

### Criterio de logro
✅ El Serial Monitor muestra la hora sincronizada al arrancar. Abriendo `http://<IP>/status` en el navegador responde con JSON válido. Los mensajes MQTT incluyen el campo `"ts"` con la hora real.

---

## Resumen de decisiones técnicas

| Decisión | Alternativa considerada | Razón del cambio |
|---|---|---|
| Python para HID | ESP32 HID nativo | ESP32 WROOM no soporta USB HID |
| Hotspot móvil | Red universitaria | Red universitaria bloquea WPA2-Personal y puerto 8883 |
| Amazon Root CA 1 | ISRG Root X1 | HiveMQ Cloud usa Amazon como CA |
| PROGMEM para certificado | setCACert() con string normal | El ESP32 WROOM no tiene RAM suficiente para el handshake TLS sin PROGMEM |
| paho-mqtt VERSION2 | VERSION1 | VERSION1 deprecada en versiones recientes |

---

## Librerías utilizadas

### ESP32 (Arduino/C++)
| Librería | Versión | Uso |
|---|---|---|
| `WiFi.h` | Core ESP32 | Conexión Wi-Fi |
| `WiFiClientSecure.h` | Core ESP32 | TLS para MQTT |
| `PubSubClient` | 2.8 | Cliente MQTT |
| `WebServer.h` | Core ESP32 | Endpoint /status |
| `time.h` | Core ESP32 | Sincronización NTP |

### Python (PC)
| Librería | Versión | Uso |
|---|---|---|
| `paho-mqtt` | 2.x | Cliente MQTT |
| `pynput` | 1.x | Control de mouse y teclado |
