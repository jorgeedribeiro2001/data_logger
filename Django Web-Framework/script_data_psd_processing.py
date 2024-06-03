import os
import sys
import json
import numpy as np

print("Data PSD Processing Script Started")

# Expects the following arguments:
if len(sys.argv) < 3:
    print("Error: Not enough arguments provided.")
    sys.exit(1)

# 1. psd_data_path
psd_data_path = sys.argv[1]
print(psd_data_path)

# 2. sample_rate
sample_rate = int(sys.argv[2])

vibration_data_path = sys.argv[3]
print(vibration_data_path)

import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

# Opening and reading the JSON data from a file
with open(vibration_data_path, 'r') as file:
    json_data = file.read()

# Parse the JSON data
data = json.loads(json_data)


def convert_json_to_fftlist(data):
    # List to hold all FFT segments' magnitudes
    fft_segments = []

    for entry in data:
        # Create a dictionary with frequencies as keys and magnitudes as values
        freq_to_magnitude = {fb: mag for fb, mag in zip(entry['frequency'], entry['magnitude'])}
        
        # Initialize an empty list for the new magnitudes
        new_magnitudes = []

        # Calculate the maximum frequency based on the Nyquist frequency
        max_frequency = int(sample_rate / 2)
        current_frequency = 0
        
        for i in range(1, max_frequency):
            mag = freq_to_magnitude.get(i, 0)
            new_magnitudes.append(mag ** 2)
        
        fft_segments.append(new_magnitudes)
        
    return fft_segments
        
    

# Get the modified FFT data
fft_lists = convert_json_to_fftlist(data)

# Calculate average PSD
psd_average = np.mean(fft_lists, axis=0)

# Generate frequency bins for plotting
frequency = np.arange(1, int(sample_rate / 2))

# Calculate the moving average
window_size = 5  # Change this to adjust the smoothness
moving_avg = np.convolve(psd_average, np.ones(window_size)/window_size, mode='valid')

# Correct the x-axis data for moving average to align with the y data
adjusted_frequency = frequency[(window_size//2):-(window_size//2)]

# Custom formatter for y-axis
def custom_formatter(x, pos):
    return f'{x:.4f}'

# Create a dictionary to hold the data
data = {
    "frequency": frequency.tolist(),
    "frequency_adjusted": adjusted_frequency.tolist(),
    "psd_value": psd_average.tolist(),
    "psd_value_avg": moving_avg.tolist()
}

# Write the data to a JSON file
with open(psd_data_path, 'w') as f:
    json.dump(data, f, indent=2)

print("Data PSD Processing Script end")