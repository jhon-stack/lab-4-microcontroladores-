import serial
import json
import paho.mqtt.client as mqtt

# Nos conectamos al puerto
ser = serial.Serial('/dev/ttyACM2', 115200, timeout=1)
# Nos conectamos al broker
client = mqtt.Client()
client.username_pw_set("Piax7kunFzRnvLV6vrBY")
client.connect("iot.eie.ucr.ac.cr", 1883)

# Topic
topic = "v1/devices/me/telemetry" 

# Publicamos por MQTT
while True:
    try:
        data = ser.readline().strip().decode('utf-8')
        x, y, z, battery = data.split('\t')
        
        # Convertimos el valor de la batería a entero
        battery = int(battery)
        
        # Creamos un diccionario con los valores de X, Y, Z y la batería
        json_data = {"x": float(x), "y": float(y), "z": float(z), "battery": battery}
        
        # Si la batería es menor a 7, creamos un nuevo diccionario con un mensaje personalizado
        if battery < 7:
            warning_data = {"warning": "Batería baja, por favor cargar el dispositivo"}
            json_data.update(warning_data)
            
        # Convertimos el diccionario a formato JSON y lo publicamos por MQTT
        client.publish(topic, json.dumps(json_data))
        print(json_data)
    except:
        pass
