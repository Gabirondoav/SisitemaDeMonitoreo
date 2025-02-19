# -*- coding: utf-8 -*-
"""
Created on Wed Dec 18 09:32:44 2024

@author: gabir
"""
#Este codigo lo modifiqeu el 6/1/2025, sta funcionando para tres temp y el numero de camara

import socket
import mysql.connector
from mysql.connector import Error
import threading
import requests
import json
import time  # Para el delay

global ultimo_envio_alerta 
ultimo_envio_alerta = 0  # Control del tiempo entre alertas

# Configuración de la base de datos
DB_CONFIG = {
    'host': '',
    'user': '',
    'password': '',
    'database': ''
}


def guardar_en_base_datos(mensaje):
    """Guarda el número de cámara y las tres temperaturas en la base de datos."""
    try:

        valores = mensaje.split("_")

        if len(valores) != 4:
            print("Error: El mensaje no contiene exactamente 4 valores.")
            return None

        camara, t1, t2, t3 = int(valores[0]), float(valores[1]), float(valores[2]), float(valores[3])

        # Conectar a la base de datos
        conexion = mysql.connector.connect(**DB_CONFIG)
        cursor = conexion.cursor()

        query = """
        INSERT INTO log_datos (camara, t1, t2, t3, datos)
        VALUES (%s, %s, %s, %s, %s)
        """
        cursor.execute(query, (camara, t1, t2, t3, mensaje))
        conexion.commit()
        print("Dato guardado en la base de datos.")

        return camara, t1, t2, t3

    except Error as e:
        print("Error al guardar en la base de datos:", e)
        return None
    finally:
        if 'conexion' in locals() and conexion.is_connected():
            cursor.close()
            conexion.close()

def calcular_promedio(t1, t2, t3):
    """Calcula el promedio de las tres temperaturas."""
    promedio = (t1 + t2 + t3) / 3
    print("Promedio de temperaturas: " + str(promedio))
    return promedio

def obtener_numeros_telefonos():
    """Obtiene todos los números de teléfono habilitados desde la tabla 'contactos'."""
    try:
        conexion = mysql.connector.connect(**DB_CONFIG)
        cursor = conexion.cursor()

        # Consulta para obtener los números de teléfono habilitados
        query = "SELECT numero FROM contactos WHERE Habilitados = 1"
        cursor.execute(query)
        resultados = cursor.fetchall()

        if resultados:
            return [resultado[0] for resultado in resultados]  # Retorna una lista de números
        else:
            print("No se encontraron números habilitados en la tabla 'contactos'.")
            return []

    except Error as e:
        print("Error al obtener los números de teléfono:", e)
        return []
    finally:
        if 'conexion' in locals() and conexion.is_connected():
            cursor.close()
            conexion.close()

def enviar_whatsapp(mensaje, numero_telefono):
    """Envía un mensaje a través de WhatsApp usando la API."""
    url = "https://ws.php"
    payload = {
        "user": "",  # Reemplaza con tu usuario
        "pass": "",  # Reemplaza con tu contraseña
        "numerotelefono": numero_telefono,
        "mensaje": mensaje
    }
    
    headers = {
        "Content-Type": "application/x-www-form-urlencoded"
    }
    
    try:
        # Realizar la solicitud POST
        response = requests.post(url, data=payload, headers=headers)
        
        if response.status_code == 200:
            print("Mensaje enviado correctamente a " + str(numero_telefono) + ": " + str(response.text))
        else:
            print("Error al enviar mensaje a " + str(numero_telefono) + ": " + str(response.status_code) + ", " + str(response.text))
    except Exception as e:
        print("Error al hacer la solicitud de WhatsApp para " + str(numero_telefono) + ": " + str(e))
                    

def enviar_alerta_a_numeros_habilitados(mensaje):
    """Envía el mensaje a todos los números habilitados con un delay entre cada envío."""
    numeros = obtener_numeros_telefonos()
    if not numeros:
        print("No hay números habilitados para enviar mensajes.")
        return

    for numero in numeros:
        enviar_whatsapp(mensaje, numero)
        time.sleep(10)  # Delay de 2 segundos entre mensajes

def comparar_promedios(promedio, rangos, camara):
    """Compara el promedio con los rangos establecidos y genera alertas si es necesario."""
    RangoInferior1, RangoInferior2, RangoSuperior1, RangoSuperior2 = rangos

    if promedio < RangoInferior2 or promedio > RangoSuperior2:
        alerta = "ALERTA ROJA: Cámara " + str(camara) + " (" + str(promedio) + " °C) fuera del rango permitido."
        print(alerta)
        enviar_alerta_a_numeros_habilitados(alerta)
    elif promedio < RangoInferior1 or promedio > RangoSuperior1:
        alerta = "ALERTA MEDIA: Cámara " + str(camara) + " (" + str(promedio) + " °C) está cerca de los límites."
        print(alerta)
        enviar_alerta_a_numeros_habilitados(alerta)

def verificar_sensores(t1, t2, t3, camara):
    """Verifica si hay una diferencia anormal entre las temperaturas de los sensores."""
    if abs(t1 - t2) > 4:
        alerta = "POSIBLE FALLA: Cámara " + str(camara) + ", diferencia anormal entre T1 (" + str(t1) + " °C) y T2 (" + str(t2) + " °C)."
        print(alerta)
        enviar_alerta_a_numeros_habilitados(alerta)

    if abs(t2 - t3) > 4:
        alerta = "POSIBLE FALLA: Cámara " + str(camara) + ", diferencia anormal entre T2 (" + str(t2) + " °C) y T3 (" + str(t3) + " °C)."
        print(alerta)
        enviar_alerta_a_numeros_habilitados(alerta)

    if abs(t1 - t3) > 4:
        alerta = "POSIBLE FALLA: Cámara " + str(camara) + ", diferencia anormal entre T1 (" + str(t1) + " °C) y T3 (" + str(t3) + " °C)."
        print(alerta)
        enviar_alerta_a_numeros_habilitados(alerta)
        

def manejar_cliente(datos):
    """Maneja la conexión con el cliente y procesa los mensajes recibidos."""
    global ultimo_envio_alerta

    if datos:
        print("Mensaje recibido: " + datos)

        valores = guardar_en_base_datos(datos)
        if valores:
            camara, t1, t2, t3 = valores
            promedio = calcular_promedio(t1, t2, t3)

            # Obtener los rangos desde la base de datos
            conexion = mysql.connector.connect(**DB_CONFIG)
            cursor = conexion.cursor()
            cursor.execute("SELECT RangoInferior1, RangoInferior2, RangoSuperior1, RangoSuperior2 FROM rangos LIMIT 1")
            rangos = cursor.fetchone()
            conexion.close()

            if rangos:
                tiempo_actual = time.time()
                if tiempo_actual - ultimo_envio_alerta >= 3600:  # 3600 segundos = 1 hora
                    ultimo_envio_alerta = tiempo_actual
                    comparar_promedios(promedio, rangos, camara)
                    verificar_sensores(t1, t2, t3, camara)
                else:
                    print("tiempo insuficiente para enviar otra alerta...")
                
    else:
        print("No se recibieron datos.")

def iniciar_servidor():
    """Inicia el servidor y escucha por conexiones entrantes."""

    localPort   = 7500
    localIP     = ""
    bufferSize  = 1024
    msgFromServer       = "UDP Client"
    bytesToSend         = str.encode(msgFromServer)
    UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
    UDPServerSocket.bind((localIP, localPort))
    print('UDP server up and listening. Puerto: ' + str(localPort))


    try:
        while True:

            bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)
            message = bytesAddressPair[0]
            address = bytesAddressPair[1]
            Data = format(message).split(',')

            if Data[0]=='b\'+RESP:GTDAT':
                
                #["b'+RESP:GTDAT", '6E0902', '868589060900431', '', 'hola_mundo_2832', '', "1918$'"]
                #obtengo el mensaje de la camara
                mensaje = Data[4]
                
                msgFromServer = '+SACK:' + Data[len(Data)-1]
                bytesToSend = str.encode(msgFromServer)
                UDPServerSocket.sendto(bytesToSend, address)

                #manejo el mensaje
                manejar_cliente(mensaje)

            elif Data[0]=='b\'+ACK:GTHBD':
                    Data[len(Data)-1] = Data[len(Data)-1].replace("'", "")
                    msgFromServer = '+SACK:GTHBD,' + Data[1] + "," + Data[len(Data)-1]
                    print(msgFromServer)
                    bytesToSend = str.encode(msgFromServer)
                    UDPServerSocket.sendto(bytesToSend, address)
            else:
                msgFromServer = '+SACK:' + Data[len(Data)-1]
                bytesToSend = str.encode(msgFromServer)
                UDPServerSocket.sendto(bytesToSend, address)

    except KeyboardInterrupt:
        print("\nServidor detenido manualmente.")

# Ejecutar el servidor
if __name__ == "__main__":
    iniciar_servidor()
