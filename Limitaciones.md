# Limitaciones del Proyecto — SmartJoy IoT

Análisis detallado de las limitaciones técnicas identificadas durante el desarrollo y pruebas del sistema.

---

## 1. ESP32 WROOM-32 no soporta USB HID nativo

**Descripción:** El ESP32 WROOM-32 utiliza su puerto USB exclusivamente para programación y comunicación Serial. No puede emular dispositivos HID (Human Interface Device) como mouse o teclado, funcionalidad que sí está disponible en las variantes ESP32-S2 y ESP32-S3.

**Impacto:** El sistema requiere un script Python corriendo permanentemente en la PC para traducir los mensajes MQTT en acciones de mouse y teclado. Esto significa que el joystick no funciona de forma autónoma — siempre necesita una PC con el script activo.

**Workaround aplicado:** Script Python con `pynput` suscrito al broker MQTT.

---

## 2. Dependencia de red Wi-Fi externa

**Descripción:** El ESP32 solo soporta redes WPA2-Personal. Las redes universitarias o corporativas que usan WPA2-Enterprise (autenticación por usuario y contraseña de dominio) son incompatibles con el ESP32.

**Impacto:** En la Universidad de La Sabana, la red `Estudiante-Usabana` usa autenticación Microsoft con dominio `@unisabana.edu.co`, lo que impide la conexión del ESP32. El sistema solo funciona con hotspot móvil o redes domésticas WPA2-Personal.

**Impacto adicional:** La PC y el ESP32 deben estar en la misma red para que el healthcheck `/status` sea accesible desde el navegador.

---

## 3. Latencia variable según condiciones de red

**Descripción:** El sistema publica mensajes cada 50ms (20 veces por segundo), pero la latencia real depende de la calidad de la conexión Wi-Fi, la carga del broker HiveMQ y la distancia al servidor (HiveMQ Cloud eu1 está en Europa).

**Impacto medido:** En condiciones normales la latencia es de 80–150ms. En momentos de congestión puede superar 300ms, lo que hace el control del mouse perceptiblemente lento o con saltos. Para uso en videojuegos competitivos esto es inaceptable.

**Workaround parcial:** Reducir el intervalo de publicación a 20ms mejora la respuesta, pero aumenta el consumo de datos y la carga sobre el broker.

---

## 4. Sin reconexión automática robusta del script Python

**Descripción:** Si el broker MQTT se desconecta momentáneamente (corte de internet, reinicio del router), el script Python en la PC lanza una excepción y se detiene. El ESP32 sí tiene lógica de reconexión automática, pero Python no.

**Impacto:** Cualquier interrupción de red requiere reiniciar manualmente el script Python. Durante una sesión de juego esto interrumpe completamente el control.

---

## 5. Calibración manual requerida al cambiar joysticks

**Descripción:** Los valores de centro calibrado (`CENTER1_X`, `CENTER1_Y`, etc.) están hardcodeados en el firmware basados en los joysticks físicos específicos del proyecto. Si se reemplaza alguno de los KY-023 por una unidad diferente, el centro real puede variar y causar detección de teclas fantasma.

**Impacto:** El firmware no es plug-and-play con cualquier KY-023 — requiere recalibrar y recompilar. La calibración automática al arrancar mitiga esto parcialmente pero los valores hardcodeados en el código siguen siendo un punto frágil.

---

## 6. NTP no funciona en redes con puerto 123 bloqueado

**Descripción:** La sincronización de tiempo usa el protocolo NTP en el puerto UDP 123. Algunas redes (especialmente universitarias y corporativas) bloquean este puerto por políticas de seguridad.

**Impacto:** Los timestamps en los mensajes MQTT muestran `"sin-hora"` en lugar de la hora real. Esto afecta la trazabilidad de los datos pero no el funcionamiento del control en sí.

**Workaround aplicado:** El sistema continúa funcionando después de 20 intentos fallidos de NTP, simplemente sin timestamps válidos.

---

## 7. Un solo botón disponible en JOY2

**Descripción:** El KY-023 tiene un solo botón (SW, eje Z). Actualmente está mapeado a la tecla Enter. Para videojuegos que requieren múltiples botones (saltar, disparar, recargar, agacharse) el sistema es insuficiente.

**Impacto:** El joystick no puede reemplazar un gamepad completo ni un teclado completo — solo cubre WASD + un botón adicional, lo que limita su uso a géneros específicos de videojuegos (juegos de estrategia con movimiento básico, algunos RPGs).

---

## 8. Sensibilidad del mouse no ajustable en tiempo real

**Descripción:** La velocidad máxima del mouse (`velMax = 10`) y la zona muerta (`DEADZONE = 150`) están definidas como constantes en el firmware. No existe forma de ajustarlas sin recompilar y recargar el sketch.

**Impacto:** Usuarios con diferentes preferencias de sensibilidad no pueden ajustar el control sin acceso al código fuente y al Arduino IDE. En juegos donde se necesita alternar entre movimientos lentos (apuntar) y rápidos (girar), un valor fijo es una limitación notable.

---

## 9. Broker MQTT en la nube — dependencia de internet

**Descripción:** El sistema usa HiveMQ Cloud como broker, lo que requiere conexión a internet activa tanto en el ESP32 como en la PC. No existe un modo offline o local.

**Impacto:** Sin internet el sistema no funciona en absoluto. Una implementación con broker local (Mosquitto en la misma PC) eliminaría esta dependencia y reduciría la latencia significativamente, pero no se implementó por simplicidad de configuración.

---

## 10. Plan gratuito de HiveMQ con límites de uso

**Descripción:** El plan Serverless gratuito de HiveMQ Cloud tiene límites de mensajes por mes y conexiones simultáneas. Publicando a 20 mensajes por segundo, en una sesión de 1 hora se generan ~72,000 mensajes.

**Impacto:** Para uso intensivo o demostraciones largas se puede alcanzar el límite del plan gratuito, lo que cortaría la conexión hasta el siguiente mes o requeriría actualizar a un plan de pago.

---

## 11. Seguridad básica en las credenciales

**Descripción:** Las credenciales de Wi-Fi y del broker MQTT están escritas directamente en el código fuente del firmware como strings en texto plano. Cualquier persona con acceso al repositorio o al binario compilado podría extraerlas.

**Impacto:** Para un proyecto académico esto es aceptable, pero en un producto real representaría un riesgo de seguridad significativo. Las credenciales deberían almacenarse en memoria NVS del ESP32 o cargarse desde un archivo de configuración cifrado.

---

## 12. Ausencia de confirmación de comandos (QoS 0)

**Descripción:** Los mensajes se publican con QoS 0 (fire and forget), lo que significa que no hay confirmación de que el broker recibió el mensaje ni de que Python lo procesó.

**Impacto:** En condiciones de red inestable, algunos comandos pueden perderse sin que el sistema lo detecte. Para el caso de uso (control de mouse) esto se manifiesta como movimientos faltantes o teclas que no responden momentáneamente.

---

## Resumen

| # | Limitación | Severidad | Tiene workaround |
|---|---|---|---|
| 1 | Sin USB HID nativo | Alta | Script Python |
| 2 | Incompatible con WPA2-Enterprise | Alta | Hotspot móvil |
| 3 | Latencia variable | Media | Parcial |
| 4 | Sin reconexión automática en Python | Media | ❌ |
| 5 | Calibración hardcodeada | Media | Parcial |
| 6 | NTP bloqueado en algunas redes | Baja | Continúa sin timestamps |
| 7 | Un solo botón disponible | Media | ❌ |
| 8 | Sensibilidad no ajustable en tiempo real | Baja | ❌ |
| 9 | Requiere internet (broker en la nube) | Alta | ❌ |
| 10 | Límites del plan gratuito HiveMQ | Baja | Parcial |
| 11 | Credenciales en texto plano | Media | ❌ |
| 12 | QoS 0 sin confirmación | Baja | ❌ |
