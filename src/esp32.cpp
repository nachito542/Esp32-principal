#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// --- RED ---
const char* ssid = "Arac-rescue";
const char* password = "password123";
const char* server_commands = "http://192.168.4.2:5000/commands";

// --- HARDWARE ---
// Se usa solo una instancia en la dirección 0x40
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

#define SERVOMIN  150 
#define SERVOMAX  600 

// Ángulos de movimiento
const int SUBIR = 60, BAJAR = 90, AVANCE = 120, ATRAS = 60, CENTRO = 90;
int angles[16]; // Para guardar los comandos del servidor

// --- PROTOTIPOS ---
void posicionInicial();
void adelante();
void atras();
void derecha();
void izquierda();
void moverGrupoA(int angFemur, int angCoxa);
void moverGrupoB(int angFemur, int angCoxa);
void moverServo(int canal, int angulo);
void parsearComandos(String payload);
void ejecutarMovimiento();

void setup() {
  Serial.begin(115200);
  
  // Conexión WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\n🌐 Conectado a Arac-rescue");

  // Inicio de I2C en pines 21 y 22
  Wire.begin(21, 22);
  pca.begin();
  pca.setPWMFreq(50);
  
  posicionInicial();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(server_commands);
    http.setTimeout(1500);
    
    int code = http.GET();
    if (code == 200) {
      String payload = http.getString();
      parsearComandos(payload);
      ejecutarMovimiento();
    } 
    http.end();
  }
  delay(100); 
}

// Desarma el texto "ch0:120,ch1:60..." del servidor Flask
void parsearComandos(String payload) {
  int ch, ang;
  char buffer[200];
  payload.toCharArray(buffer, 200);
  char* token = strtok(buffer, ",");
  while (token != NULL) {
    if (sscanf(token, "ch%d:%d", &ch, &ang) == 2) {
      if (ch < 16) angles[ch] = ang;
    }
    token = strtok(NULL, ",");
  }
}

// Lógica de decisión según lo que procesó YOLO
void ejecutarMovimiento() {
  int ejeX = angles[0]; 
  int ejeY = angles[1]; 

  if (ejeX >= 60 && ejeX < 120) {
    izquierda();
  } 
  else if (ejeX >= 120 && ejeX < 150) {
    derecha();
  } 
  else if (ejeY >= 150) {
    adelante();
  } 
  else if (ejeY >= 30 && ejeX < 60) { // Suponiendo que ch1:30 es atrás en tu Python
    atras();
  }
  else {
    posicionInicial(); // Detenerse si no hay comandos claros
  }
}

// --- FUNCIONES DE MOVIMIENTO ---

void moverServo(int canal, int angulo) {
  int pulso = map(angulo, 0, 180, SERVOMIN, SERVOMAX);
  pca.setPWM(canal, 0, pulso);
}

void posicionInicial() {
  for(int i=0; i<16; i++) {
    moverServo(i, CENTRO);
  }
}

void adelante() {
  // --- GRUPO A AVANZA / GRUPO B SOPORTA ---
  // 1. Levanta y adelanta el Grupo A. El Grupo B se queda fijo en el suelo.
  moverGrupoA(SUBIR, AVANCE); 
  moverGrupoB(BAJAR, CENTRO); 
  delay(200);

  // 2. Apoya el Grupo A en su nueva posición (adelantada).
  moverGrupoA(BAJAR, AVANCE); 
  delay(100);

  // --- GRUPO B AVANZA / GRUPO A SOPORTA ---
  // 3. Levanta y adelanta el Grupo B. El Grupo A se queda fijo en el suelo.
  moverGrupoB(SUBIR, AVANCE);
  moverGrupoA(BAJAR, CENTRO); 
  delay(200);

  // 4. Apoya el Grupo B en su nueva posición (adelantada).
  moverGrupoB(BAJAR, AVANCE);
  delay(100);
  
  // OPCIONAL: Reset de coxa para preparar el siguiente ciclo de empuje
  // posicionInicial(); // Descomenta si notas que las patas se abren mucho
}

void atras() {
  // --- GRUPO A RETROCEDE / GRUPO B SOPORTA ---
  moverGrupoA(SUBIR, ATRAS); 
  moverGrupoB(BAJAR, CENTRO); 
  delay(200);

  moverGrupoA(BAJAR, ATRAS); 
  delay(100);

  // --- GRUPO B RETROCEDE / GRUPO A SOPORTA ---
  moverGrupoB(SUBIR, ATRAS);
  moverGrupoA(BAJAR, CENTRO); 
  delay(200);

  moverGrupoB(BAJAR, ATRAS);
  delay(100);
}

void izquierda() {
  // Paso 1: Grupo A levanta y mueve hacia atrás
  moverGrupoA(SUBIR, ATRAS); 
  moverGrupoB(BAJAR, CENTRO); 
  delay(200);
  
  moverGrupoA(BAJAR, ATRAS);
  delay(100);

  // Paso 2: Grupo B levanta y mueve hacia adelante
  moverGrupoB(SUBIR, AVANCE);
  moverGrupoA(BAJAR, CENTRO);
  delay(200);

  moverGrupoB(BAJAR, AVANCE);
  delay(100);
  posicionInicial();
}

void derecha() {
  // Paso 1: Grupo A levanta y mueve a posición de giro
  moverGrupoA(SUBIR, AVANCE); 
  moverGrupoB(BAJAR, CENTRO); 
  delay(200);
  
  moverGrupoA(BAJAR, AVANCE);
  delay(100);

  // Paso 2: Grupo B levanta y mueve en dirección opuesta para pivotar
  moverGrupoB(SUBIR, ATRAS);
  moverGrupoA(BAJAR, CENTRO);
  delay(200);

  moverGrupoB(BAJAR, ATRAS);
  delay(100);
  posicionInicial();
}

// Mapeo de canales para un solo PCA (Patas impares 1,3,5,7)
void moverGrupoA(int angFemur, int angCoxa) {
  moverServo(0, angCoxa);  moverServo(1, angFemur);  // Pata 1
  moverServo(4, angCoxa);  moverServo(5, angFemur);  // Pata 3
  moverServo(8, angCoxa);  moverServo(9, angFemur);  // Pata 5
  moverServo(12, angCoxa); moverServo(13, angFemur); // Pata 7
}

// Mapeo de canales para un solo PCA (Patas pares 2,4,6,8)
void moverGrupoB(int angFemur, int angCoxa) {
  moverServo(2, angCoxa);  moverServo(3, angFemur);  // Pata 2
  moverServo(6, angCoxa);  moverServo(7, angFemur);  // Pata 4
  moverServo(10, angCoxa); moverServo(11, angFemur); // Pata 6
  moverServo(14, angCoxa); moverServo(15, angFemur); // Pata 8
}