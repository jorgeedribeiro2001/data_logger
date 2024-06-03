import json
import sys
import time
import paho.mqtt.client as mqtt

client_id = 'config_client' # Client ID for the MQTT client

def on_connect(client, userdata, flags, rc, *args):
    print(f"Connected with result code {rc}")

def on_publish(client, userdata, mid, rc, *args):
    print(f"Message published with result code {rc}")

def main(settings1_json, settings2_json, logger_id):
    
    # Parse the settings JSON string
    settings1 = json.loads(settings1_json)
    settings2 = json.loads(settings2_json)
    
    # Create an MQTT client
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id)

    # Assign the callbacks
    client.on_connect = on_connect
    client.on_publish = on_publish

    # Connect to the MQTT broker
    client.connect(settings1['mqtt_address'], int(settings1['mqtt_port']), 60)

    # Start the MQTT loop
    client.loop_start()

    # Publish the settings every 5 seconds until 2 minutes have passed
    for i in range(10):
        client.publish("config1", settings1_json)
        time.sleep(5)
        client.publish("config2", settings2_json)
        time.sleep(10)

    # Stop the MQTT loop
    client.loop_stop()

if __name__ == "__main__":
    # Get the command line arguments
    settings1_json = sys.argv[1]
    settings2_json = sys.argv[2]
    logger_id = sys.argv[3]
    
    print(f"Logger ID: {logger_id}")
    print(f"Settings1 JSON: {settings1_json}")
    print(f"Settings2 JSON: {settings2_json}")
    
    main(settings1_json, settings2_json, logger_id)