# SmartJoy — Fase 2: Prueba de control de mouse y teclado con pynput
# Ejecutar en la PC con: python fase2_test_hid.py
# NO necesita el ESP32 todavía — valores hardcodeados para probar que funciona

from pynput.mouse import Controller as MouseController
from pynput.keyboard import Controller as KeyboardController, Key
import time

mouse    = MouseController()
keyboard = KeyboardController()

print("=== SmartJoy Fase 2 — Prueba HID en PC ===")
print("El script va a mover el mouse y presionar teclas.")
print("Tienes 3 segundos para poner el cursor en un lugar seguro...\n")
time.sleep(3)

# ── Test 1: Mover el mouse ──────────────────────────────────────────
print("[Test 1] Moviendo el mouse en un cuadrado...")

for _ in range(30):   # derecha
    mouse.move(5, 0)
    time.sleep(0.03)

for _ in range(30):   # abajo
    mouse.move(0, 5)
    time.sleep(0.03)

for _ in range(30):   # izquierda
    mouse.move(-5, 0)
    time.sleep(0.03)

for _ in range(30):   # arriba
    mouse.move(0, -5)
    time.sleep(0.03)

print("[Test 1] Listo — el mouse deberia haber dibujado un cuadrado\n")
time.sleep(1)

# ── Test 2: Presionar teclas WASD ───────────────────────────────────
print("[Test 2] Presionando W, A, S, D con 0.5s cada una...")
print("         Abre el Bloc de Notas para verlas aparecer\n")
time.sleep(3)  # tiempo para abrir el Bloc de Notas

for tecla in ['w', 'a', 's', 'd']:
    print(f"  Presionando: {tecla.upper()}")
    keyboard.press(tecla)
    time.sleep(0.3)
    keyboard.release(tecla)
    time.sleep(0.5)

print("\n[Test 2] Listo\n")
time.sleep(0.5)

# ── Test 3: Simular joystick → movimiento de mouse ──────────────────
print("[Test 3] Simulando joystick analogico (valores del ADC)...")
print("         El mouse se va a mover suavemente\n")
time.sleep(1)

def joystick_a_velocidad(valor_adc, centro, deadzone=150, velocidad_max=8):
    """
    Convierte un valor ADC (0-4095) a velocidad de mouse (-velocidad_max a +velocidad_max).
    Aplica zona muerta relativa al centro calibrado.
    """
    delta = valor_adc - centro
    if abs(delta) < deadzone:
        return 0
    # Mapear el delta al rango de velocidad
    rango = 2048 - deadzone
    vel = (abs(delta) - deadzone) / rango * velocidad_max
    return int(vel) * (1 if delta > 0 else -1)

# Simular 3 segundos de joystick moviéndose a la derecha y abajo
centro_x = 1800  # reemplazar con tu valor calibrado real
centro_y = 1800

print("  Simulando: joystick hacia arriba-derecha")
for i in range(60):
    # Simular joystick empujado: X=2600 (derecha), Y=900 (arriba)
    adc_x = 2600
    adc_y = 900
    vel_x = joystick_a_velocidad(adc_x, centro_x)
    vel_y = joystick_a_velocidad(adc_y, centro_y)
    mouse.move(vel_x, vel_y)
    time.sleep(0.05)

print("\n=== Todos los tests completados ===")
print("Si el mouse se movio y las teclas aparecieron en el Bloc de Notas,")
print("la Fase 2 esta completa. Avanza a la Fase 3 (MQTT).")
