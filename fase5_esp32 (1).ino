// SmartJoy — Fase 5 Final: MQTT TLS + NTP + Healthcheck HTTP
// Todo en un solo sketch listo para cargar

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <time.h>

// ── Wi-Fi ────────────────────────────────────────────────────────────
const char* ssid     = "REDMI15C";
const char* password = "juansami2";

// ── HiveMQ Cloud ─────────────────────────────────────────────────────
const char* mqtt_server   = "902c9c7bb1a74af18a94675dab007c83.s1.eu.hivemq.cloud";
const char* mqtt_username = "smartjoy";
const char* mqtt_password = "Msd9Qd3JZ@5XNXY";
const int   mqtt_port     = 8883;

// ── Pines joysticks ──────────────────────────────────────────────────
#define JOY1_VRX 34
#define JOY1_VRY 35
#define JOY2_VRX 32
#define JOY2_VRY 33
#define JOY2_SW  25

// ── Centros calibrados ───────────────────────────────────────────────
#define CENTER1_X 1810
#define CENTER1_Y 1859
#define CENTER2_X 1811
#define CENTER2_Y 1810
#define DEADZONE  150

// ── Tópicos MQTT ─────────────────────────────────────────────────────
#define TOPIC_MOUSE  "smartjoy/mouse"
#define TOPIC_KEYS   "smartjoy/keys"
#define TOPIC_STATUS "smartjoy/status"

// ── Certificado ISRG Root X1 ─────────────────────────────────────────
static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

WiFiClientSecure espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

unsigned long lastMsg  = 0;
unsigned long startTime = 0;
bool mqttOK = false;

// ── NTP ──────────────────────────────────────────────────────────────
String obtenerHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "sin-hora";
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(buf);
}

// ── Healthcheck /status ───────────────────────────────────────────────
void handleStatus() {
  unsigned long uptime = (millis() - startTime) / 1000;
  String json = "{";
  json += "\"status\":\"ok\",";
  json += "\"uptime_s\":" + String(uptime) + ",";
  json += "\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "connected" : "disconnected") + "\",";
  json += "\"mqtt\":\"" + String(mqttOK ? "connected" : "disconnected") + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"time\":\"" + obtenerHora() + "\"";
  json += "}";
  server.send(200, "application/json", json);
  Serial.println("[HTTP] GET /status respondido");
}

void handleNotFound() {
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

// ── Wi-Fi ─────────────────────────────────────────────────────────────
void setup_wifi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  Serial.printf("Conectando a %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.printf("\nWi-Fi OK — IP: %s\n", WiFi.localIP().toString().c_str());
}

// ── MQTT ──────────────────────────────────────────────────────────────
void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando a HiveMQ...");
    String clientId = "smartjoy-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println(" conectado!");
      mqttOK = true;
      mqtt.publish(TOPIC_STATUS, "{\"status\":\"online\"}");
    } else {
      mqttOK = false;
      Serial.printf(" fallo rc=%d, reintentando en 5s\n", mqtt.state());
      delay(5000);
    }
  }
}

int joystickVelocidad(int valor, int centro, int deadzone = DEADZONE, int velMax = 10) {
  int delta = valor - centro;
  if (abs(delta) < deadzone) return 0;
  float vel = (float)(abs(delta) - deadzone) / (2048 - deadzone) * velMax;
  return (int)vel * (delta > 0 ? 1 : -1);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(JOY2_SW, INPUT_PULLUP);

  setup_wifi();
  delay(2000);

  // NTP — Colombia UTC-5
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.google.com");
  Serial.print("Sincronizando NTP");
  struct tm timeinfo;
  int intentosNTP = 0;
  while (!getLocalTime(&timeinfo) && intentosNTP < 20) {
    Serial.print("."); delay(500); intentosNTP++;
  }
  Serial.printf("\nHora: %s\n", obtenerHora().c_str());

  // Servidor HTTP
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.printf("Healthcheck en http://%s/status\n", WiFi.localIP().toString().c_str());
  Serial.printf("IP real: %s\n", WiFi.localIP().toString().c_str());

  // MQTT
  espClient.setCACert(root_ca);
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setBufferSize(512);
  conectarMQTT();

  startTime = millis();
  Serial.println("SmartJoy Fase 5 listo!");
}

void loop() {
  server.handleClient();

  if (!mqtt.connected()) {
    mqttOK = false;
    conectarMQTT();
  }
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastMsg > 50) {
    lastMsg = now;

    int j1x = analogRead(JOY1_VRX);
    int j1y = analogRead(JOY1_VRY);
    int j2x = analogRead(JOY2_VRX);
    int j2y = analogRead(JOY2_VRY);
    bool btn = !digitalRead(JOY2_SW);

    int vel_x = joystickVelocidad(j1x, CENTER1_X);
    int vel_y = joystickVelocidad(j1y, CENTER1_Y);

    int dx2 = j2x - CENTER2_X;
    int dy2 = j2y - CENTER2_Y;
    const char* tecla = "none";
    if      (dy2 < -DEADZONE) tecla = "w";
    else if (dy2 >  DEADZONE) tecla = "s";
    else if (dx2 < -DEADZONE) tecla = "a";
    else if (dx2 >  DEADZONE) tecla = "d";
    if (btn)                   tecla = "enter";

    char msgMouse[80];
    snprintf(msgMouse, sizeof(msgMouse),
             "{\"vx\":%d,\"vy\":%d,\"ts\":\"%s\"}", vel_x, vel_y, obtenerHora().c_str());
    mqtt.publish(TOPIC_MOUSE, msgMouse);

    char msgKeys[80];
    snprintf(msgKeys, sizeof(msgKeys),
             "{\"key\":\"%s\",\"btn\":%d,\"ts\":\"%s\"}", tecla, btn ? 1 : 0, obtenerHora().c_str());
    mqtt.publish(TOPIC_KEYS, msgKeys);
  }
}
