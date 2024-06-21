import paho.mqtt.client as mqtt
import time
import os
import subprocess
import threading
import sys


# Set up MQTT client
broker = 'broker.emqx.io'
port = 1883
client_id = 'main_client'

topics = ["impacts", "bme", "gps", "log", "vibration","esp_stats"]

# Declaring Variables
data = []
event = []
timestamp = 0

# Get the path of the script
script_dir = os.path.dirname(os.path.abspath(__file__))  

# Record the time of the last received message for each data_logger_name
last_received_times = {}

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
            return 
        save_to_file(data_logger_name, msg.topic, message)
        last_received_times[data_logger_name] = time.time()
        
        
    except Exception as e:
        print(f"Error processing message: {e}")
        
        
def on_disconnect(client, userdata, rc, *args):
    if rc != 0:
        print("Unexpected disconnection from MQTT Broker!")
        while True:
            try:
                print("Attempting to reconnect...")
                client.reconnect()
                print("Reconnected successfully.")
                print("Restarting script...")
                os.execv(sys.executable, ['script_data_receiver'] + sys.argv)
            except Exception as e:
                print(f"Reconnect failed: {e}")
                time.sleep(5)  # Wait for 5 seconds before retrying


def save_to_file(data_logger_name, topic, message):
    try:
        # Get the directory of the current script
        script_dir = os.path.dirname(os.path.abspath(__file__))  
        relative_path = "temp_data"
        
        # Join the script's directory with the relative path
        path = os.path.join(script_dir, relative_path)
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
        
def check_timeout():
    timeout = 10  # Set your desired timeout in seconds
    while True:
        for data_logger_name, last_received_time in list(last_received_times.items()):
            if time.time() - last_received_time > timeout:
                print(f"No message received from {data_logger_name} for the last minute, calling other program...")
                script_path = os.path.join(script_dir, "script_data_processing.py")
                subprocess.call(["python", script_path, str(data_logger_name)])
                # Remove this data_logger_name from the dictionary
                del last_received_times[data_logger_name]
        time.sleep(1)


# Main Program
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id)

# Assign callbacks
client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message

try:
    client.connect(broker, port, 60)
    for topic in topics:
        client.subscribe(topic)

except Exception as e:
    print(f"Error connecting to broker or subscribing: {e}")
    # Handle the error appropriately, e.g., retry, exit, etc.

# Start the timeout check in a new thread
timeout_thread = threading.Thread(target=check_timeout)
timeout_thread.start()

client.loop_forever()
