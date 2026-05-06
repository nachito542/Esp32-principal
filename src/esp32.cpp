#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_PWMServoDriver.h>

// ================= CONFIG =================
const char* ssid = "Arac-rescue";
const char* password = "password123";

const char* server_commands = "http://192.168.4.4:5000/commands";

// UDP
WiFiUDP udp;
unsigned int localPort = 5000;
char incomingPacket[255];

// PCA9685
#define PCA_SDA 21
#define PCA_SCL 22
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

int servo_angles[16];

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n🕷️ ARAC-RESCUE INICIANDO");

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n🌐 WiFi conectado");
  Serial.println(WiFi.localIP());

  // UDP
  udp.begin(localPort);
  Serial.printf("🟢 UDP puerto %d listo\n", localPort);

  // PCA9685
  Wire.begin(PCA_SDA, PCA_SCL);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);

  standCenter();
}

// ================= LOOP =================
void loop() {

  // 🔥 PRIORIDAD 1 → UDP (tiempo real)
  if (checkUDP()) {
    executeMovement();
  }
  else {
    // 🔵 PRIORIDAD 2 → HTTP (fallback)
    String commands = getCommands();
    if (commands.length() > 0) {
      parseCommands(commands);
      executeMovement();
    }
  }

  delay(100);
}

// ================= UDP =================
bool checkUDP() {
  int packetSize = udp.parsePacket();

  if (packetSize) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) incomingPacket[len] = 0;

    Serial.print("📩 UDP: ");
    Serial.println(incomingPacket);

    parseCommands(String(incomingPacket));

    // Respuesta opcional
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print("OK");
    udp.endPacket();

    return true;
  }

  return false;
}

// ================= HTTP =================
String getCommands() {
  HTTPClient http;
  http.begin(server_commands);
  int code = http.GET();

  String payload = "";
  if (code == 200) {
    payload = http.getString();
    Serial.print("🌐 HTTP: ");
    Serial.println(payload);
  }

  http.end();
  return payload;
}

// ================= PARSER =================
void parseCommands(String cmd_str) {
  int ch, ang;

  char buffer[150];
  cmd_str.toCharArray(buffer, 150);

  char* token = strtok(buffer, ",");

  while (token != NULL) {
    if (sscanf(token, "ch%d:%d", &ch, &ang) == 2) {
      if (ch < 16) {
        servo_angles[ch] = ang;
      }
    }
    token = strtok(NULL, ",");
  }
}

// ================= MOVIMIENTO =================
void executeMovement() {

  int ang0 = servo_angles[0];
  int ang1 = servo_angles[1];

  if (ang0 == 30 && ang1 == 150) {
    Serial.println("🚶 ADELANTE");
    walkForward();
  }
  else if (ang0 == 60 && ang1 == 60) {
    Serial.println("↩️ IZQUIERDA");
    turnLeft();
  }
  else if (ang0 == 120 && ang1 == 120) {
    Serial.println("↪️ DERECHA");
    turnRight();
  }
  else {
    standCenter();
  }
}

// ================= SERVOS =================
void setServo(int channel, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, 150, 600);
  pwm.setPWM(channel, 0, pulse);
  servo_angles[channel] = angle;
}

// ================= ACCIONES =================
void walkForward() {
  Serial.println("Paso adelante");
  // TODO: lógica real
}

void turnLeft() {
  Serial.println("Giro izquierda");
}

void turnRight() {
  Serial.println("Giro derecha");
}

void standCenter() {
  for (int i = 0; i < 16; i++) {
    setServo(i, 90);
  }
}