import serial
import json
import paho.mqtt.client as mqtt

# Nos conectamos al puerto
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
# Nos conectamos al broker
client = mqtt.Client()
client.username_pw_set("9bEe52qIhlgwMAYFKkCy")
client.connect("iot.eie.ucr.ac.cr", 1883)

# Topic
topic = "v1/devices/me/telemetry" 

# Publicamos por MQTT
while True:
    try:
        data = ser.readline().strip().decode('utf-8')
        #tranformamos en json lo que se lee en 
        json_data = json.dumps({"serial_data": data})
        print(json_data)
        client.publish(topic, json_data)
    except:
        pass
