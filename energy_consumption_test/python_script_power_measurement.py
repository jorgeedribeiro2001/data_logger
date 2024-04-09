import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import time
import csv
import os

# Print the program's header
print("#######################################")
print("Power Measurement Script V1.0")
print("By: Jorge Ribeiro")
print("#######################################")

# Get a list of available serial ports
ports = list(serial.tools.list_ports.comports())

# Display the available serial ports
print("Available Serial Ports:")
for i, port in enumerate(ports):
    print(f"{i+1}. {port.device}")

# Ask the user to choose a serial port
selection = input("Enter the number of the serial port: ")

# Validate the user's selection
try:
    selection = int(selection)
    if selection < 1 or selection > len(ports):
        raise ValueError
except ValueError:
    print("Invalid selection. Please enter a valid number.")
    exit()

# Get the selected serial port
port = ports[selection-1].device
print("#######################################")

# Configure the serial communication
ser = serial.Serial(port, 115200, timeout=1)
time.sleep(2) # Wait for the serial connection to initialize

# Initialize lists to store data
bus_voltage_data = []
shunt_voltage_data = []
current_data = []
power_data = []
time_data = []

# Function to read and parse data from the serial port
def read_data():
    line = ser.readline().decode('utf-8').strip() # Read a line from the serial port
    parts = line.split(',') # Split the line into parts
    if len(parts) == 5:
        try:
            relative_time, bus_voltage, shunt_voltage, current, power = map(float, parts)
            print(f"Relative Time: {relative_time} ms, Bus Voltage: {bus_voltage} V, Shunt Voltage: {shunt_voltage} mV, Current: {current} mA, Power: {power} mW")
            return relative_time, bus_voltage, shunt_voltage, current, power
        except ValueError:
            pass # Ignore lines that don't match the expected format
    return None, None, None, None, None

# Wait for the first valid data packet
print("Script is ready for receiving data!")
while True:
    relative_time, bus_voltage, shunt_voltage, current, power = read_data()
    if relative_time is not None and bus_voltage is not None and shunt_voltage is not None and current is not None and power is not None:
        print("Recording started.") # Print when recording starts
        break # Exit the loop once valid data is received
    time.sleep(0.1) # Wait for 100ms before the next attempt

# Collect data until no data is coming
while True:
    relative_time, bus_voltage, shunt_voltage, current, power = read_data()
    if relative_time is not None and bus_voltage is not None and shunt_voltage is not None and current is not None and power is not None:
        bus_voltage_data.append(bus_voltage)
        shunt_voltage_data.append(shunt_voltage)
        current_data.append(current)
        power_data.append(power)
        time_data.append(relative_time)
    else:
        print("Recording ended.") # Print when recording ends
        break # Stop collecting data if no data is coming

# Calculate statistics
average_power = sum(power_data) / len(power_data)  # Calculate the average power
average_current = sum(current_data) / len(current_data)
average_voltage = sum(bus_voltage_data) / len(bus_voltage_data)

total_time_s = (time_data[-1] / 1000) # Convert ms to s
total_time_h = total_time_s / 3600 # Convert s to h

total_energy_consumption_mWh = average_power * total_time_h 


print(f"Total Energy Consumption: {total_energy_consumption_mWh:.2f} mWh")
print(f"Average Power: {average_power:.2f} mW")
print(f"Average Current: {average_current:.2f} mA")
print(f"Average Voltage: {average_voltage:.2f} V")

# Save statistics to a text file
script_dir = os.path.dirname(os.path.realpath(__file__)) # Get the directory of the script
stats_filename = os.path.join(script_dir, 'sensor_data_stats.txt')
with open(stats_filename, 'w') as statsfile:
    statsfile.write(f"Total Energy Consumption: {total_energy_consumption_mWh:.5f} mWh\n")
    statsfile.write(f"Average Power: {average_power:.5f} mW\n")
    statsfile.write(f"Average Current: {average_current:.5f} mA\n")
    statsfile.write(f"Average Voltage: {average_voltage:.5f} V\n")

print(f"Statistics saved as {stats_filename}")

# Plot the data
plt.figure(figsize=(12, 8))

plt.subplot(4, 1, 1)
plt.plot(time_data, bus_voltage_data)
plt.title('Bus Voltage (V)') # Set the title
plt.grid(True, linestyle='-.')
plt.tick_params(labelcolor='black', labelsize='medium', width=1)
plt.xlabel('Relative Time (ms)') # Set the x-axis label
plt.ylabel('Voltage (V)') # Set the x-axis label
plt.text(0.98, 0.93, f"Min: {min(bus_voltage_data):.2f} V\nMax: {max(bus_voltage_data):.2f} V\nMean: {sum(bus_voltage_data) / len(bus_voltage_data):.2f} V", transform=plt.gca().transAxes, ha='right', va='top')

plt.subplot(4, 1, 2)
plt.plot(time_data, shunt_voltage_data)
plt.title('Shunt Voltage (mV)') # Set the title
plt.grid(True, linestyle='-.')
plt.tick_params(labelcolor='black', labelsize='x-small', width=1)
plt.xlabel('Relative Time (ms)') # Set the x-axis label
plt.ylabel('Voltage (mV)') # Set the x-axis label
plt.text(0.98, 0.93, f"Min: {min(shunt_voltage_data):.2f} mV\nMax: {max(shunt_voltage_data):.2f} mV\nMean: {sum(shunt_voltage_data) / len(shunt_voltage_data):.2f} mV", transform=plt.gca().transAxes, ha='right', va='top')

plt.subplot(4, 1, 3)
plt.plot(time_data, current_data)
plt.title('Current Consumption (mA)') # Set the title
plt.grid(True, linestyle='-.')
plt.tick_params(labelcolor='black', labelsize='x-small', width=1)
plt.xlabel('Relative Time (ms)') # Set the x-axis label
plt.ylabel('Current (mA)') # Set the x-axis label
plt.text(0.98, 0.93, f"Min: {min(current_data):.2f} mA\nMax: {max(current_data):.2f} mA\nMean: {sum(current_data) / len(current_data):.2f} mA", transform=plt.gca().transAxes, ha='right', va='top')

plt.subplot(4, 1, 4)
plt.plot(time_data, power_data)
plt.title('Power Consumption (mW)') # Set the title
plt.grid(True, linestyle='-.')
plt.tick_params(labelcolor='black', labelsize='x-small', width=1)
plt.xlabel('Relative Time (ms)') # Set the x-axis label
plt.ylabel('Power (mW)') # Set the x-axis label
plt.text(0.98, 0.93, f"Min: {min(power_data):.2f} mW\nMax: {max(power_data):.2f} mW\nMean: {sum(power_data) / len(power_data):.2f} mW", transform=plt.gca().transAxes, ha='right', va='top')

plt.tight_layout()

# Save the graph as an image
graph_filename = os.path.join(script_dir, 'sensor_data_graph.png')
plt.savefig(graph_filename)
print(f"Graph saved as {graph_filename}")

# Save the data to a CSV file
csv_filename = os.path.join(script_dir, 'sensor_data.csv')
with open(csv_filename, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(['Relative Time (ms)', 'Bus Voltage (V)', 'Shunt Voltage (mV)', 'Current (mA)', 'Power (mW)'])
    for i in range(len(time_data)):
        csv_writer.writerow([f"{time_data[i]:.0f}", f"{bus_voltage_data[i]:.3f}", f"{shunt_voltage_data[i]:.3f}", f"{current_data[i]:.3f}", f"{power_data[i]:.3f}"])

print(f"Data saved as {csv_filename}")

# Close the serial connection
ser.close()

# Wait for a key press to continue
input("Press any key to close...")
