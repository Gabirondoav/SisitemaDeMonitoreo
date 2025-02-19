#include <mcp_can.h>
#include <mcp_can_dfs.h>

#define TINY_GSM_MODEM_A6
#include <TinyGsmClient.h>
#include <SPI.h>
#include <SoftwareSerial.h>

const int spiCSPin = 10;
MCP_CAN CAN(spiCSPin);

// Configuración GSM
#define PWR_KEY 8              // Pin conectado al PWR_KEY del módulo A6
SoftwareSerial SerialAT(3, 2); // RX, TX
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);   // Cliente GSM

bool isPowerOn = false;        // Estado del encendido del módulo GSM
unsigned long powerOnStart = 0; // Marca de tiempo para gestionar el encendido

// Variables para la conexión
const char* server = "66.97.34.161";
const int port = 7500;
bool clientConnected = false;  // Estado de la conexión al servidor

void setup() {
    Serial.begin(115200);
    SerialAT.begin(9600);  // Velocidad para el módem A6

    // Configurar el pin PWR_KEY
    pinMode(PWR_KEY, OUTPUT);
    digitalWrite(PWR_KEY, HIGH); // Asegurar que esté en HIGH inicialmente

    Serial.println("Iniciando el programa...");
    
    // Inicializa el bus CAN
    while (CAN_OK != CAN.begin(CAN_250KBPS, MCP_8MHz)) {
        Serial.println("Error al inicializar el bus CAN.");
        delay(1000);
    }
    Serial.println("Bus CAN inicializado correctamente.");
}

void loop() {
    // Encender el módulo GSM usando millis()
    if (!isPowerOn) {
        if (powerOnStart == 0) {
            // Inicia el proceso de encendido
            powerOnStart = millis();
            digitalWrite(PWR_KEY, LOW); // Simula presionar el botón
            Serial.println("Encendiendo el módulo GSM...");
        } else if (millis() - powerOnStart >= 2000) {
            // Han pasado 2 segundos, libera el botón
            digitalWrite(PWR_KEY, HIGH);
            isPowerOn = true;
            Serial.println("Módulo GSM encendido.");
        }
    }

    // Inicialización del módem GSM una vez que esté encendido
    static bool modemInitialized = false;
    if (isPowerOn && !modemInitialized) {
        Serial.println("Inicializando el módem GSM...");
        if (modem.restart()) {
            modemInitialized = true;
            modem.gprsConnect("igprs.claro.com.ar", "", ""); // APN para Claro
            Serial.println("Módem GSM inicializado correctamente.");
        } else {
            Serial.println("Error al inicializar el módem GSM.");
        }
    }

    // Establecer conexión al servidor si no está conectado
    if (modem.isGprsConnected() && !clientConnected) {
        if (client.connect(server, port)) {
            clientConnected = true;
            Serial.println("Conectado al servidor.");
        } else {
            Serial.println("Error al conectar al servidor.");
            client.stop();
            
        }
    }

    // Revisa si hay datos en el bus CAN
    unsigned char len = 0;
    unsigned char buf[6];
    if (CAN_MSGAVAIL == CAN.checkReceive()) {
        CAN.readMsgBuf(&len, buf);
        unsigned long canId = CAN.getCanId();

        // Construcción del mensaje con prefijo según el ID
        String message = "[";
        if (canId == 69) {
            message += "1,";  // Prefijo 1 para ID 69
        } else if (canId == 70) {
            message += "2,";  // Prefijo 2 para ID 70
        } else {
            message += "0,";  // Prefijo 0 para IDs no identificados
        }

        // Agregar los datos CAN al mensaje
        for (int i = 0; i < len; i += 2) {
            message += String(buf[i]) + "." + String(buf[i + 1]);
            if (i < len - 2) {  // Agregar coma solo si no es el último par de datos
                message += ",";
            }
        }
        message += "]";

        Serial.println(message);

        // Enviar datos al servidor si está conectado
        if (clientConnected) {
            client.print(message);
            Serial.println("Datos enviados al servidor.");
        } else {
            Serial.println("No conectado al servidor, reintentando...");
            client.stop();
            modem.restart();
        }
    }

    // Reintentar conexión al servidor si se pierde
    if (!client.connected() && clientConnected) {
        clientConnected = false;
        Serial.println("Desconectado del servidor. Intentando reconectar...");
    }

    delay(5000);
}