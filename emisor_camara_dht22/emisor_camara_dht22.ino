//Este codigo es para el emisor de la camara 1, anda bien y envia los datos con ID, temp1

#include <DHT.h>
#include <DHT_U.h>
#include <mcp_can.h>
#include <mcp_can_dfs.h>
#include <SPI.h>

// Definimos el pin digital donde se conecta el sensor
#define DHTPIN1 2
#define DHTPIN2 3
#define DHTPIN3 4

// Dependiendo del tipo de sensor
#define DHTTYPE DHT11
#define DHTTYPE1 DHT22 

// Inicializamos el sensor DHT11
DHT dht1(DHTPIN1, DHTTYPE1);
DHT dht2(DHTPIN2, DHTTYPE1);
DHT dht3(DHTPIN3, DHTTYPE1);

const int spiCSPin = 10;
unsigned long lastSentTime = 0; // Tiempo de la última transmisión
const unsigned long sendInterval = 30000; // Intervalo de envío: 30 segundos

MCP_CAN CAN(spiCSPin);

void setup() {
    Serial.begin(115200);
    // Comenzamos el sensor DHT
    dht1.begin();
    dht2.begin();
    dht3.begin();

    while (CAN_OK != CAN.begin(CAN_250KBPS, MCP_8MHz)) {
        Serial.println("CAN BUS init Failed");
        delay(100);
    }
    Serial.println("CAN BUS Shield Init OK!");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastSentTime >= sendInterval) {
        lastSentTime = currentMillis;

        // Leer temperaturas
        float t1 = dht1.readTemperature();
        float t2 = dht2.readTemperature();
        float t3 = dht3.readTemperature();

        Serial.print("Temperatura 1: "); Serial.println(t1);
        Serial.print("Temperatura 2: "); Serial.println(t2);
        Serial.print("Temperatura 3: "); Serial.println(t3);

        //if (isnan(t1) || isnan(t2) || isnan(t3)) {
        //    Serial.println("Error obteniendo los datos de uno o más sensores DHT.");
        //    return;
        //}

        // Separar parte entera y decimal de cada temperatura
        unsigned char t1parteEntera = (unsigned char)t1;
        unsigned char t1parteDecimal = (unsigned char)((t1 - t1parteEntera) * 10);
        unsigned char t2parteEntera = (unsigned char)t2;
        unsigned char t2parteDecimal = (unsigned char)((t2 - t2parteEntera) * 10);
        unsigned char t3parteEntera = (unsigned char)t3;
        unsigned char t3parteDecimal = (unsigned char)((t3 - t3parteEntera) * 10);

        // Preparar datos para enviar
        unsigned char stmp[6] = {t1parteEntera, t1parteDecimal, t2parteEntera, t2parteDecimal, t3parteEntera, t3parteDecimal};

        Serial.println("Enviando mensaje CAN...");
        if (CAN.sendMsgBuf(0x45, 0, 6, stmp) == CAN_OK) {
            Serial.println("Mensaje enviado correctamente");
        } else {
            Serial.println("Error al enviar el mensaje");
        }
    }

    // Recibir datos CAN
    unsigned char len = 0;
    unsigned char buf[6];
    if (CAN_MSGAVAIL == CAN.checkReceive()) {
        CAN.readMsgBuf(&len, buf);
        unsigned long canId = CAN.getCanId();
        Serial.print("ID: ");
        Serial.print(canId);
        Serial.print(" Data: ");
        for (int i = 0; i < len; i++) {
            Serial.print(buf[i]);
            Serial.print("\t");
        }
        Serial.println();
    }
}
