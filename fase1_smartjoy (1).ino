// SmartJoy — Fase 1 v2: Lectura KY-023 con calibración automática de centro
// ESP32 WROOM-32

#define JOY1_VRX 34
#define JOY1_VRY 35
#define JOY2_VRX 32
#define JOY2_VRY 33
#define JOY2_SW  25

#define DEADZONE 150      // zona muerta relativa al centro calibrado
#define SAMPLES  30       // muestras para calcular el centro real

// Centros calibrados (se calculan al arrancar)
int center1_x, center1_y;
int center2_x, center2_y;

// Lee N muestras y devuelve el promedio (para calibrar)
int promediar(int pin) {
  long suma = 0;
  for (int i = 0; i < SAMPLES; i++) {
    suma += analogRead(pin);
    delay(10);
  }
  return suma / SAMPLES;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(JOY2_SW, INPUT_PULLUP);

  Serial.println("=== SmartJoy Fase 1 v2 — Calibración automática ===");
  Serial.println("NO toques los joysticks durante 3 segundos...");
  delay(2000);

  // Calibrar centros reales
  center1_x = promediar(JOY1_VRX);
  center1_y = promediar(JOY1_VRY);
  center2_x = promediar(JOY2_VRX);
  center2_y = promediar(JOY2_VRY);

  Serial.printf("Centro JOY1 calibrado → X: %d | Y: %d\n", center1_x, center1_y);
  Serial.printf("Centro JOY2 calibrado → X: %d | Y: %d\n", center2_x, center2_y);
  Serial.println("Zona muerta: ±150 del centro");
  Serial.println("====================================================");
  delay(500);
}

void loop() {
  int joy1_x = analogRead(JOY1_VRX);
  int joy1_y = analogRead(JOY1_VRY);
  int joy2_x = analogRead(JOY2_VRX);
  int joy2_y = analogRead(JOY2_VRY);
  bool btn   = !digitalRead(JOY2_SW);

  // Desplazamiento relativo al centro calibrado
  int dx2 = joy2_x - center2_x;
  int dy2 = joy2_y - center2_y;

  // Detectar tecla según desplazamiento relativo
  String tecla = "ninguna";
  if      (dy2 < -DEADZONE) tecla = "W (arriba)";
  else if (dy2 >  DEADZONE) tecla = "S (abajo)";
  else if (dx2 < -DEADZONE) tecla = "A (izquierda)";
  else if (dx2 >  DEADZONE) tecla = "D (derecha)";
  if (btn)                   tecla = "ENTER (boton)";

  Serial.println("------------------------------------------");
  Serial.printf("[JOY1 - Mouse]  X: %4d  |  Y: %4d  (delta X: %+d, Y: %+d)\n",
                joy1_x, joy1_y,
                joy1_x - center1_x, joy1_y - center1_y);
  Serial.printf("[JOY2 - Teclas] X: %4d  |  Y: %4d  (delta X: %+d, Y: %+d)\n",
                joy2_x, joy2_y, dx2, dy2);
  Serial.printf("[Boton: %s] [Tecla: %s]\n",
                btn ? "PRESIONADO" : "suelto", tecla.c_str());

  delay(200);
}
