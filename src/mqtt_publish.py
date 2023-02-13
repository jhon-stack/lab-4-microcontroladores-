import paho.mqtt.client as mqtt  
import time
import datetime
import json
import serial

def on_log(client, userdata,level,buf):
    print("log: "+buf)
def on_connect(client,userdata,flags,rc):
    if rc == 0:
        print("Conexion exitosa")
    else:
        print("No se puedo conectar")
def on_disconnect(client,userdata,flags,rc):
    print("Disconected result code: "+str(rc))
def on_message(client,userdata,msg):
    topic = msg.topic
    m_decode=str(msg.payload.decode("utf-8","ignore"))
    print("message received", m.decode)

#Informacion de la plataforma de thingsboard
ACCESS_TOKEN        = 'SyVslNMbMQeggxXEsdVt'    #Token of your device
BROKER              = "iot.eie.ucr.ac.cr"  
PORT                = 1883   #data listening port
#Informacion de la extraccion de puertos de manera serial
#Utizamos la misma funcion del lab pasado
#Estas configuraciones se agarran de usart
port = '/dev/ttyACM1'
baudrate_1 = 115200
ser = serial.Serial(port,baudrate_1,timeout=1)

client.on_connected =  on_connected
client.on_disconnect = on_disconnect
client.on_publish = on_publish


