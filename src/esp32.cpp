#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_PWMServoDriver.h>

// =====================================================
//                     CONFIGURACIÓN
// =====================================================
const char* ssid = "Arac-rescue";
const char* password = "password123";
const char* server_commands = "http://192.168.4.2:5000/commands";

// PCA9685
#define PCA_SDA 21
#define PCA_SCL 22
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

int servo_angles[16];

void setup() {
  Serial.begin(115200);
  Serial.println("\n🕷️ ARAC-RESCUE: INICIANDO SISTEMA DE LOCOMOCIÓN");

  // WIFI CLIENTE
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n🌐 WiFi Conectado");

  // PCA9685
  Wire.begin(PCA_SDA, PCA_SCL);
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50); 

  standCenter(); // Posición inicial
}

void loop() {
  String commands = getCommands();
  if (commands.length() > 0) {
    parseCommands(commands);
    executeMovement(); // Traducir ángulos a pasos
  }
  delay(200); // Ajustado para respuesta más rápida
}

// 🎛️ FUNCIÓN PARA MOVER CUALQUIER SERVO
void setServo(int channel, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, 150, 600);
  pwm.setPWM(channel, 0, pulse);
  servo_angles[channel] = angle;
}

// 🌐 OBTENER COMANDOS DEL SERVIDOR PYTHON
String getCommands() {
  HTTPClient http;
  http.begin(server_commands);
  int code = http.GET();
  String payload = (code == 200) ? http.getString() : "";
  http.end();
  return payload;
}

// 🔍 PARSEAR LOS DATOS DEL SERVIDOR
void parseCommands(String cmd_str) {
  int ch, ang;
  char buffer[150];
  cmd_str.toCharArray(buffer, 150);
  char* token = strtok(buffer, ",");
  while (token != NULL) {
    if (sscanf(token, "ch%d:%d", &ch, &ang) == 2) {
      if (ch < 16) servo_angles[ch] = ang;
    }
    token = strtok(NULL, ",");
  }
}

// 🚶 LÓGICA DE MOVIMIENTO SEGÚN LOS ÁNGULOS RECIBIDOS
void executeMovement() {
  int ang0 = servo_angles[0];
  int ang1 = servo_angles[1];

  if (ang0 == 30 && ang1 == 150) {
    Serial.println("🚶 ADELANTE: Persona centrada");
    walkForward();
  } 
  else if (ang0 == 60 && ang1 == 60) {
    Serial.println("↩️ IZQUIERDA: Girando...");
    turnLeft();
  } 
  else if (ang0 == 120 && ang1 == 120) {
    Serial.println("↪️ DERECHA: Girando...");
    turnRight();
  } 
  else if (ang0 == 90 && ang1 == 90) {
    standCenter();
  }
}

// --- SECUENCIAS DE PASOS (Aquí defines tus 16-18 servos) ---

void walkForward() {
  // Ejemplo: Mover un grupo de patas adelante
  // Aquí debes usar setServo para cada pata de tu diseño
  Serial.println("Paso adelante ejecutado");
}

void turnLeft() {
  Serial.println("Giro izquierda ejecutado");
}

void turnRight() {
  Serial.println("Giro derecha ejecutado");
}

void standCenter() {
  for(int i = 0; i < 16; i++) {
    setServo(i, 90);
  }
}