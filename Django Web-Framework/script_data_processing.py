import os
import requests
import json
import sys

def list_devices(folder_path):
    
    # Check if the folder exists
    if not os.path.exists(folder_path):
        print(f"Error: Folder '{folder_path}' not found.")
        return []
    if not os.path.isdir(folder_path):
        print(f"Error: '{folder_path}' is not a directory.")
        return []

    try:
        # List all subfolders
        subfolders = [f.path for f in os.scandir(folder_path) if f.is_dir()]

        # Save the names of the subfolders in a list
        subfolder_names = [os.path.basename(subfolder) for subfolder in subfolders]

        return subfolder_names
    except Exception as e:
        print(f"An error occurred: {e}")
        return []
    
def read_file(device_name, file):
    file_path = os.path.join(temp_path, device_name, file)
    try:
        with open(file_path, 'r') as file:
            data = file.read()
        return data
    except FileNotFoundError:
        return f''
    except Exception as e:
        print(f"An error occurred: {e}")
        return f''
    
def post_request(device_name, topic, data):
    
     # Defining the relative URL to send the POST request
    device_number = device_name.split('_')[1]
    relative_url = f"/update_{topic}/{device_number}/"
    url = base_url + relative_url

    # Define the headers for the POST request
    headers = {
        'Content-Type': 'application/json',
    }

    # Make the POST request
    response = requests.post(url, headers=headers, json={'data': data})

    # Print the response from the server and check if the response text contains "success"
    print(response.text)
    if "success" in response.text.lower():
        return True
    else:
        return False
    
def convert_bme_csv_to_json(data):
    # Split the data into lines
    lines = data.strip().split('\n')

    # Initialize an empty list to store dictionaries
    json_data = []

    # Iterate over each line and convert it into a dictionary
    for line in lines:
        try:
            timestamp, temperature, humidity, pressure = line.split(',')
            entry = {
            "timestamp": int(timestamp),
            "temperature": float(temperature),
            "humidity": float(humidity),
            "pressure": float(pressure)
        }
            json_data.append(entry)
            
        except Exception as e:
            continue
        
    # Convert the list of dictionaries into a JSON string
    json_string = json.dumps(json_data, indent=2)

    # Print the JSON string
    return json_string

def convert_impacts_csv_to_json(data):
    lines = data.split('\n')
    output = []

    current_item = {"timestamp": None, "index": [], "accelx": [], "accely": [], "accelz": []}

    for line in lines:
        parts = line.split(',')
        try:
            if parts[1] == '0':
                if current_item["timestamp"] is not None:
                    output.append(current_item)
                    current_item = {"timestamp": None, "index": [], "accelx": [], "accely": [], "accelz": []}
                current_item["timestamp"] = int(parts[0])
            current_item["index"].append(int(parts[1]))
            current_item["accelx"].append(float(parts[2]))
            current_item["accely"].append(float(parts[3]))
            current_item["accelz"].append(float(parts[4]))
        except Exception as e:
            continue

    # Append the last item
    output.append(current_item)

    # Convert to JSON and print
    json_string = json.dumps(output, indent=2)
    return json_string

def convert_vibration_csv_to_json(data):
    # Split the data into lines
    lines = data.strip().split('\n')

    # Initialize an empty dictionary to store data series
    data_series = {}

    # Iterate over each line
    for line in lines:
        try:
            timestamp, frequency, magnitude = line.split(',')
            timestamp = int(timestamp)
            frequency = float(frequency)
            magnitude = float(magnitude)

            # If the timestamp is not in the data series, add it
            if timestamp not in data_series:
                data_series[timestamp] = {"frequency": [], "magnitude": []}

            # Append the frequency bin and magnitude to the data series
            data_series[timestamp]["frequency"].append(frequency)
            data_series[timestamp]["magnitude"].append(magnitude)
        except Exception as e:
            continue

    # Convert the data series into a list of dictionaries
    json_data = [{"timestamp": timestamp, **values} for timestamp, values in data_series.items()]

    # Convert the list of dictionaries into a JSON string
    json_string = json.dumps(json_data, indent=2)

    return json_string

def convert_gps_csv_to_json(data):
    # Split the data into lines
    lines = data.strip().split('\n')

    # Initialize an empty list to store dictionaries
    json_data = []

    # Iterate over each line and convert it into a dictionary
    for line in lines:
        try:
            timestamp, latitude, longitude, fix, satellites, hdop, altitude, geoidalseparation = line.split(',')
            entry = {
            "timestamp": int(timestamp),
            "latitude": float(latitude),
            "longitude": float(longitude),
            "fix": int(fix),
            "satellites": int(satellites),
            "hdop": float(hdop),
            "altitude": float(altitude),
            "geoidalseparation": float(geoidalseparation)
        }
            json_data.append(entry)
            
        except Exception as e:
            continue
        
    # Convert the list of dictionaries into a JSON string
    json_string = json.dumps(json_data, indent=2)

    # Print the JSON string
    return json_string


# Program START
# Check if an argument was passed
if len(sys.argv) < 2:
    print("Error: No data_logger_name argument provided.")
    sys.exit(1)

# Get the data_logger_name argument
data_logger_name = sys.argv[1]
print(data_logger_name)

# Get the path of the script
script_dir = os.path.dirname(os.path.abspath(__file__))  
relative_path = "temp_data"
temp_path = os.path.join(script_dir, relative_path)

# Define the base URL for Django server
base_url = "http://127.0.0.1:8000/"



print("For device: " + str(data_logger_name))
data = read_file(data_logger_name, 'bme.txt')
data_to_send = convert_bme_csv_to_json(data)
if len(data) > 0:
    if (post_request(data_logger_name, "bme", data_to_send)):
        print("BME data sent successfully")
        try:
            os.remove(os.path.join(temp_path, data_logger_name, 'bme.txt'))
        except Exception as e:
            print(f"Error removing file: {e}")
    else:
        print("Error sending BME data")
else:
    print("No BME data to send")
    
    
    
data = read_file(data_logger_name, 'impacts.txt')
if len(data) > 0:
    data_to_send = convert_impacts_csv_to_json(data)
    if (post_request(data_logger_name, "impact", data_to_send)):
        print("Impact data sent successfully")
        try:
            os.remove(os.path.join(temp_path, data_logger_name, 'impacts.txt'))
        except Exception as e:
            print(f"Error removing file: {e}")
    else:
        print("Error sending Impact data")
else:
    print("No Impact data to send")
    

data = read_file(data_logger_name, 'vibration.txt')
print(data)
if len(data) > 0:
    data_to_send = convert_vibration_csv_to_json(data)
    if (post_request(data_logger_name, "vibration", data_to_send)):
        print("Vibration data sent successfully")
        try:
            os.remove(os.path.join(temp_path, data_logger_name, 'vibration.txt'))
        except Exception as e:
            print(f"Error removing file: {e}")
    else:
        print("Error sending Vibration data")
else:
    print("No vibration data to send")

   
    
        
data = read_file(data_logger_name, 'gps.txt')
if len(data) > 0:
    data_to_send = convert_gps_csv_to_json(data)
    if (post_request(data_logger_name, "gps", data_to_send)):
        print("GPS data sent successfully")
        try:
            os.remove(os.path.join(temp_path, data_logger_name, 'gps.txt'))
        except Exception as e:
            print(f"Error removing file: {e}")
    else:
        print("Error sending GPS data")
else:
    print("No GPS data to send")
    
    
    
data = read_file(data_logger_name, 'log.txt')
if len(data) > 0:
    if (post_request(data_logger_name, "log", data)):
        print("Log data sent successfully")
        try:
            os.remove(os.path.join(temp_path, data_logger_name, 'log.txt'))
        except Exception as e:
            print(f"Error removing file: {e}")
    else:
        print("Error sending Log data")
else:
    print("No Log data to send")
    
    
    
print("Device data processing completed\n\n")

   
