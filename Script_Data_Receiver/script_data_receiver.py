import paho.mqtt.client as mqtt
import time
import logging
import os

# Set up MQTT client
broker = '172.187.90.157'
port = 1883
client_id = 'main_client'

# Declaring Variables
data = []
event = []
timestamp = 0

# MQTT Callbacks
def on_connect(client, userdata, flags, rc, *args):
    if rc == 0:
        print("Connected to MQTT Broker!")
    else:
        print(f"Failed to connect, return code {rc}")
        # Attempt to reconnect
        try:
            client.reconnect()
        except Exception as e:
            print(f"Error reconnecting: {e}")


def on_message(client, userdata, msg):
    try:
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
        message = msg.payload.decode()
        data_logger_name = message.split('%')[1] # Extract the data logger name
        topic_in_message = message.split('%')[2].split(',')[0] # Extract the topic from the message
        if msg.topic == topic_in_message: # Compare the topic from the MQTT broker with the topic in the message
            print(f"Topic confirmed: {msg.topic}")
        else:
            print(f"Topic mismatch: {msg.topic} != {topic_in_message}")
        save_to_file(data_logger_name, msg.topic, message)
    except Exception as e:
        print(f"Error processing message: {e}")

def save_to_file(data_logger_name, topic, message):
    try:
        path = "C:\\Users\\admin.jorge\\Desktop\\Processamento Dados\\Data"
        full_path = os.path.join(path, data_logger_name)
        if not os.path.exists(full_path):
            os.makedirs(full_path)
        # Remove the initial part of the data logger name and the topic
        data = message.split(',', 1)[1]
        # Replace underscores with paragraph breaks
        data = data.replace('_', '\n')
        with open(f'{full_path}/{topic}.txt', 'a') as f:
            f.write(f'{data}\n')
    except Exception as e:
        print(f"Error saving to file: {e}")

# Main Program
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id)

# Assign callbacks
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(broker, port, 60)
    client.subscribe("impacts")
    client.subscribe("bme")
    client.subscribe("gps")
    client.subscribe("log")
except Exception as e:
    print(f"Error connecting to broker or subscribing: {e}")
    # Handle the error appropriately, e.g., retry, exit, etc.

client.loop_forever()
