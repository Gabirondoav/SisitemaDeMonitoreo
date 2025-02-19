#include <SoftwareSerial.h>
#include <mcp_can.h>
#include <SPI.h>

// Configuración de SoftwareSerial
SoftwareSerial SerialGps(3, 2); // RX: 3, TX: 2

// Configuración de MCP2515 (CAN)
const int CAN_CS_PIN = 10; // Ajusta según tu configuración
MCP_CAN CAN(CAN_CS_PIN);

int counter = 0;

void setup() {
  // Inicialización de la comunicación serie
  Serial.begin(115200);
  Serial.println("Iniciando!");

  // Inicialización de SerialGps
  SerialGps.begin(9600);

    // Inicializa el bus CAN
    while (CAN_OK != CAN.begin(CAN_250KBPS, MCP_8MHz)) {
        Serial.println("Error al inicializar el bus CAN.");
        delay(1000);
    }
    Serial.println("Bus CAN inicializado correctamente.");
}

void loop() {
  unsigned char len = 0;
  unsigned char buf[6];
  
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    CAN.readMsgBuf(&len, buf);
    unsigned long canId = CAN.getCanId();

    // Construcción del mensaje con prefijo según el ID
    String mensaje = "";
    if (canId == 69) {
      mensaje += "1_"; // Prefijo 1 para ID 69
    } else if (canId == 70) {
      mensaje += "2_"; // Prefijo 2 para ID 70
    } else {
      mensaje += "0_"; // Prefijo 0 para IDs no identificados
    }

    // Agregar los datos CAN al mensaje
    for (int i = 0; i < len; i += 2) {
      mensaje += String(buf[i]) + "." + String(buf[i + 1]);
      if (i < len - 2) { // Agregar guion bajo solo si no es el último par de datos
        mensaje += "_";
      }
    }

    // Enviar el mensaje por UDP
    Serial.print("AT+GTDAT=gv310lau,0,,");
    Serial.print(mensaje);
    Serial.print(",,,,,00FF$");
    Serial.println("\r\n");

    SerialGps.print("AT+GTDAT=gv310lau,0,,");
    SerialGps.print(mensaje);
    SerialGps.print(",,,,,00FF$");
    SerialGps.println("\r\n");


    counter++;
  }

  delay(60000);
}

