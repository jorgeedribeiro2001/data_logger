from django.shortcuts import render, redirect, get_object_or_404
from django.shortcuts import redirect
from django.contrib.auth import logout as LOGOUT
from .forms import RegistrationForm
from .models import Trip  # Import the Trip model
from .models import Logger  # Import the Logger model
from .models import TripConfig  # Import the TripConfig model
from django.http import HttpResponse, JsonResponse
from django.views import View
import os
from django.conf import settings
from django.contrib.auth.decorators import login_required
from django.core.exceptions import ObjectDoesNotExist
from django.views.decorators.csrf import csrf_exempt
from django.utils.decorators import method_decorator
import json
import csv
from datetime import datetime
from django.utils import timezone
from django.core.serializers.json import DjangoJSONEncoder
import pandas as pd
import subprocess
from django.core.files import File
from io import BytesIO
from django.contrib import messages

###############################################################################
# Functions for managing the users

def index(request):
    return render(request, 'index.html')

def register(request):
    if request.method == 'POST':
        form = RegistrationForm(request.POST)
        if form.is_valid():
            form.save()
            return redirect('login')
    else:
        form = RegistrationForm()
    return render(request, 'register.html', {'form': form})

def logout_view(request):
    LOGOUT(request)
    return redirect('index')

###############################################################################
# Functions for managing the devices

@login_required(login_url='/login')
def devices_manager(request):
    loggers = Logger.objects.filter(user_associated=request.user)
    return render(request, 'devices_manager.html', {'loggers': loggers})

@csrf_exempt
def add_device(request):
    if request.method == 'POST':
        device_name = request.POST.get('name') # Get the name of the device from the POST request
        device_id = request.POST.get('device_id') # Get the id of the device from the POST request
        

        current_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        device_log_path = os.path.join(current_dir, f"devices_logs/device_{device_id}")  # Get the log path of the device from the POST request

        os.makedirs(device_log_path, exist_ok=True)  # Create the directory if it does not exist

        full_path = os.path.join(device_log_path, 'log.txt')

        with open(full_path, 'wb') as f:
            file = File(f)
        
        
        # Create a new Logger instance
        logger = Logger.objects.create(
            user_associated=request.user,
            id=device_id,
            name=device_name,
            device_log_path=device_log_path,
            last_communication_timestamp=None,
            last_gps_timestamp=None,
            last_gps_location_latitude=None,
            last_gps_location_longitude=None,
            battery_level=None,
            current_trip=None,
        )
        
        messages.success(request, 'Device added successfully!')
        return redirect('devices_manager')
    return render(request, 'add_device.html')

@csrf_exempt
def send_configuration(request, logger_id):
    print(logger_id)
    
    # Get the logger and verifies if exist
    try:
        logger = Logger.objects.get(id=logger_id)
    except ObjectDoesNotExist:
        return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)

    # Get the current trip for the logger
    trip = logger.current_trip
    if trip is None:
        return JsonResponse({'status': 'failure', 'error': 'No ongoing trip found for this logger'}, status=404)
    
    #Get the start date of the trip
    start_date = trip.start_date.timestamp() if trip.start_date is not None else None

    # Get the TripConfig for the trip
    try:
        tripconfig = TripConfig.objects.get(trip=trip)
    except TripConfig.DoesNotExist:
        return JsonResponse({'status': 'failure', 'error': 'No configuration found for this trip'}, status=404)

    settings1 = {
        'device_id': f"device_{logger.id}",
        'mqtt_address': tripconfig.mqtt_address,
        'mqtt_port': tripconfig.mqtt_port,
        #'start_date': start_date,
        'sleep_mode': tripconfig.sleep_mode,
        'sleep_interval': tripconfig.sleep_interval,
        'movement_detection_threshold': tripconfig.movement_detection_threshold,
        'bme_mode': tripconfig.bme_mode,
        'bme_sampling_interval': tripconfig.bme_sampling_interval,
        'bme_sampling_duration': tripconfig.bme_sampling_duration,
        'gps_mode': tripconfig.gps_mode,
        'gps_interval_timer': tripconfig.gps_interval_timer,
    }
        
    settings2 = {
        'device_id': f"device_{logger.id}",
        'max_time_try_location': tripconfig.max_time_try_location,
        'accelerometer_scale': tripconfig.accelerometer_scale,
        'sample_rate': tripconfig.sample_rate,
        'impact_mode': tripconfig.impact_mode,
        'impact_threshold': tripconfig.impact_threshold,
        'vibration_mode': tripconfig.vibration_mode,
        'n_vibration_per_sample': tripconfig.n_vibration_per_sample,
        'vibration_threshold': tripconfig.vibration_threshold,
        'vibration_sampling_interval': tripconfig.vibration_sampling_interval,
        'mqtt_mode': tripconfig.mqtt_mode,
        'mqtt_interval': tripconfig.mqtt_interval,
        'logs_mode': tripconfig.logs_mode,
        'esp_stats_mode': tripconfig.esp_stats_mode,
        'esp_stats_interval_timer': tripconfig.esp_stats_interval_timer,
    }
    
    script_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    script_path = os.path.join(script_dir, "script_remote_configuration.py")
    subprocess.run(['python', script_path, json.dumps(settings1),json.dumps(settings2), str(logger.id)])
    
    
    return JsonResponse(settings)
    
###############################################################################
# Functions for managing the trips
@login_required(login_url='/login')
def create_trip(request):
    loggers_available = Logger.objects.filter(current_trip__isnull=True)
    
    return render(request, 'create_trip.html', {'loggers_available': loggers_available})

@login_required(login_url='/login')
def select_trip(request):
    # Get all trips from the database
    trips = Trip.objects.all().values('id', 'name')  # Only get the id and name fields

    if request.method == 'POST':
        selected_trip_id = request.POST.get('trip')
        request.session['selected_trip_id'] = selected_trip_id
        return redirect('monitoring_trips')

    return render(request, 'select_trip_page.html', {'trips': trips})

@login_required(login_url='/login')
def monitoring_trip(request, trip_id):
    # Get the Trip instance with the trip_id
    # If no such Trip exists, this will raise a 404 error
    trip = get_object_or_404(Trip, id=trip_id)

    # Initialize an empty list for the timestamps
    bme_timestamps = []
    temperature = []
    humidity = []
    pressure = []
    latitude = []
    longitude = []
    gps_timestamps = []
    
    # Get the last_communication_timestamp
    logger = trip.logger
    last_communication_timestamp = logger.last_communication_timestamp
    device_battery_level = logger.battery_level


    # Check if the file exists
    if os.path.exists(trip.bme_data.path):
        # If the file exists, read the data
        with open(trip.bme_data.path, 'r') as file:
            try:
                data = json.load(file)
                # Convert the epoch timestamps to datetime objects and extract them from the data
                bme_timestamps = [datetime.fromtimestamp(item['timestamp']) for item in data if item.get('timestamp') is not None]
                temperature = [item['temperature'] for item in data if item.get('temperature') is not None]
                humidity = [item['humidity'] for item in data if item.get('humidity') is not None]
                pressure = [item['pressure'] for item in data if item.get('pressure') is not None]
            except json.JSONDecodeError:
                # If the file does not contain valid JSON, leave timestamps as an empty list
                pass
    
    timestamps = []
    impacts_id = []
    max_magnitudes = []
    # Check if the file exists
    if os.path.exists(trip.impact_data.path):
        # If the file exists, read the data
        with open(trip.impact_data.path, 'r') as file:
            try:
                data = json.load(file)
                # Extract the timestamps from the data             
                timestamps = [item['timestamp'] for item in data if item.get('timestamp') is not None]
                timestamps = [datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S') for ts in timestamps]
                
                #Extract the impacts_ids from the data
                impacts_id= [item['impact_id'] for item in data if item.get('impact_id') is not None]
                
                # Extract the second item of accelx, accely, accelz and calculates the magnitude
                for item in data:
                    accelx_second = item.get('accelx')[1] if item.get('accelx') and len(item.get('accelx')) > 1 else None
                    accely_second = item.get('accely')[1] if item.get('accely') and len(item.get('accely')) > 1 else None
                    accelz_second = item.get('accelz')[1] if item.get('accelz') and len(item.get('accelz')) > 1 else None
                    max_magnitudes.append(round((accelx_second**2 + accely_second**2 + accelz_second**2)**0.5, 2))
                 
                print(impacts_id)
                print(timestamps)   
                print(max_magnitudes)
                
            except json.JSONDecodeError:
                # If the file does not contain valid JSON, leave timestamps as an empty list
                pass
    
    if os.path.exists(trip.gps_data.path):
        # If the file exists, read the data
        with open(trip.gps_data.path, 'r') as file:
            try:
                data = json.load(file)
                # Extract the timestamps from the data             
                gps_timestamps = [item['timestamp'] for item in data if item.get('timestamp') is not None]
                gps_timestamps = [datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S') for ts in gps_timestamps]
                #gps_timestamps = [timestamp.isoformat() for timestamp in gps_timestamps]
                latitude = [item['latitude'] for item in data if item.get('latitude') is not None]
                longitude = [item['longitude'] for item in data if item.get('longitude') is not None] 
                
            except json.JSONDecodeError:
                # If the file does not contain valid JSON, leave timestamps as an empty list
                pass
    
    
    # Serialize the lists to JSON
    temperature_json = json.dumps(temperature, cls=DjangoJSONEncoder)
    humidity_json = json.dumps(humidity, cls=DjangoJSONEncoder)
    pressure_json = json.dumps(pressure, cls=DjangoJSONEncoder)
    latitude_json = json.dumps([lat if lat is not None else 0 for lat in latitude], cls=DjangoJSONEncoder)
    longitude_json = json.dumps([lon if lon is not None else 0 for lon in longitude], cls=DjangoJSONEncoder)
    gps_timestamps_json = json.dumps([ts if ts is not None else 0 for ts in gps_timestamps], cls=DjangoJSONEncoder)

    return render(request, 'monitoring_page.html', {
        'trip': trip, 
        'last_communication_timestamp': last_communication_timestamp,
        'device_battery_level': device_battery_level,
        'impacts_id': impacts_id,
        'impacts_timestamps': timestamps,
        'max_magnitudes': max_magnitudes,
        'bme_timestamps': bme_timestamps, 
        'temperature': temperature_json, 
        'humidity': humidity_json, 
        'pressure': pressure_json,
        'gps_timestamps': gps_timestamps_json,
        'latitude': latitude_json,
        'longitude': longitude_json
    })

@csrf_exempt 
def end_trip_request(request, logger_id):
    # Get the logger and verifies if exist
    try:
        logger = Logger.objects.get(id=logger_id)
    except ObjectDoesNotExist:
        return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)
    
    # Get the current trip for the logger
    trip = logger.current_trip
    if trip is None:
        return JsonResponse({'status': 'failure', 'error': 'No ongoing trip found for this logger'}, status=404)
    
    # Set the end_date of the trip to the current time
    trip.end_date = timezone.now()
    trip.save()
    
    # Set the current_trip of the logger to None
    logger.current_trip = None
    logger.save()
    
    return JsonResponse({'status': 'success'})
###############################################################################
# Function to create a trip
@csrf_exempt
def create_trip_request(request):
    if request.method == 'POST':
        data = json.loads(request.body)
        
        # Create an empty file for each field
        bme_data_file = File(BytesIO(), name='bme_data.txt')
        impact_data_file = File(BytesIO(), name='impact_data.txt')
        vibration_data_file = File(BytesIO(), name='vibration_data.txt')
        psd_data_file = File(BytesIO(), name='psd_data.txt')
        gps_data_file = File(BytesIO(), name='gps_data.txt')
        
        trip = Trip.objects.create(
            name=data['trip_name'],
            logger=Logger.objects.get(name=data['device_name']),
            start_date=datetime.fromtimestamp(data['start_date']),
            end_date=None,
            bme_data=bme_data_file,
            impact_data=impact_data_file,
            vibration_data=vibration_data_file,
            psd_data=psd_data_file,
            gps_data=gps_data_file,
        )

        TripConfig.objects.create(
            trip=trip,
            mqtt_address=data['mqtt_address'],
            mqtt_port=data['mqtt_port'],
            sleep_mode=data['sleep_mode'],
            sleep_interval=data['sleep_interval'],
            movement_detection_threshold=data['movement_detection_threshold'],
            bme_mode=data['bme_mode'],
            bme_sampling_interval=data['bme_sampling_interval'],
            bme_sampling_duration=data['bme_sampling_duration'],
            gps_mode=data['gps_mode'],
            gps_interval_timer=data['gps_sampling_interval'],
            max_time_try_location=data['gps_max_time'],
            accelerometer_scale=data['accelerometer_scale'],
            sample_rate=data['data_rate'],
            impact_mode=data['impact_mode'],
            impact_threshold=data['impact_threshold'],
            vibration_mode=data['vibration_mode'],
            vibration_sampling_interval=data['vibration_sampling_interval'],
            vibration_threshold=data['vibration_threshold'],
            mqtt_mode=data['mqtt_mode'],
            mqtt_interval=data['mqtt_interval'],
            logs_mode=data['logs_mode'],
            esp_stats_mode=data['esp_stats_mode'],
            esp_stats_interval_timer=data['esp_stats_sampling_interval'],
        )
        
        # Get the logger and verifies if exist
        try:
            logger = Logger.objects.get(name=data['device_name'])
        except ObjectDoesNotExist:
            return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)
        
        # Set the current_trip of the logger to the created trip
        logger.current_trip = trip
        logger.save()
        

        return JsonResponse({'status': 'success', 'trip_id': trip.id})
    else:
        return JsonResponse({'status': 'failure', 'error': 'Invalid request method'}, status=405)

@csrf_exempt
def delete_trip(request, trip_id):
    print(f"trip_id: {trip_id}")  # Debugging line

    # Attempt to retrieve the Trip instance by trip_id
    try:
        trip = Trip.objects.get(id=trip_id)
        print(f"trip: {trip}")  # Debugging line
    except ObjectDoesNotExist:
        return JsonResponse({'status': 'failure', 'error': 'Trip not found'}, status=404)
    
    #if trip.end_date is None:
        #return JsonResponse({'status': 'failure', 'error': 'Trip is still ongoing'}, status=400)
    
    # Delete the TripConfig instance associated with the trip
    try:
        TripConfig.objects.get(trip=trip).delete()
    except ObjectDoesNotExist:
        pass

    # Delete the Trip instance
    trip.delete()

    return JsonResponse({'status': 'success'})
###############################################################################
# Functions for getting the data

def get_impact_data(request, trip_id, impact_id):

    # Convert the datetime object to an epoch timestamp
    impact_id = int(impact_id)

    data = {
        'index': [],
        'accelx': [],
        'accely': [],
        'accelz': [],
    }

    # Get the Trip instance that corresponds to the provided trip_id
    trip = Trip.objects.get(id=trip_id)

    if os.path.exists(trip.impact_data.path):
        # If the file exists, read the data
        with open(trip.impact_data.path, 'r') as file:
            try:
                impact_data = json.load(file)
                # Filter the data based on the timestamp
                for item in impact_data:
                    if item.get('impact_id') == impact_id:
                        data['index'].append(item.get('index'))
                        data['accelx'].append(item.get('accelx'))
                        data['accely'].append(item.get('accely'))
                        data['accelz'].append(item.get('accelz'))
            except json.JSONDecodeError:
                # If the file does not contain valid JSON, leave data as it is
                pass

    return JsonResponse(data)
    
def apply_moving_average_smoothing(df, window_size=5):
    # Ensure the DataFrame is sorted by timestamp
    df = df.sort_values(by='timestamp')
    
    # Apply moving average smoothing to latitude and longitude
    df['latitude_smoothed'] = df['latitude'] #.rolling(window=window_size).mean()
    df['longitude_smoothed'] = df['longitude'] #.rolling(window=window_size).mean()
    
    # Fill NaN values in the smoothed columns with the original values
    df['latitude_smoothed'].fillna(df['latitude'], inplace=True)
    df['longitude_smoothed'].fillna(df['longitude'], inplace=True)
    
    return df

def get_gps_data(request, trip_id):
    data = {
        'timestamp': [],
        'latitude': [],
        'longitude': [],
        'fix': [],
        'satellites': [],
        'hdop': [],
        'altitude': [],
        'geoidalseparation': []
    }

    # Get the Trip instance that corresponds to the provided trip_id
    trip = Trip.objects.get(id=trip_id)

    if os.path.exists(trip.gps_data.path):
        # If the file exists, read the data
        with open(trip.gps_data.path, 'r') as file:
            try:
                gps_data = json.load(file)
                # Filter the data based on the timestamp
                for item in gps_data:
                    data['timestamp'].append(datetime.fromtimestamp(item.get('timestamp')).strftime('%Y-%m-%d %H:%M:%S'))
                    data['latitude'].append(item.get('latitude'))
                    data['longitude'].append(item.get('longitude'))
                    data['fix'].append(item.get('fix'))
                    data['satellites'].append(item.get('satellites'))
                    data['hdop'].append(item.get('hdop'))
                    data['altitude'].append(item.get('altitude'))
                    data['geoidalseparation'].append(item.get('geoidalseparation'))
                    
            except json.JSONDecodeError:
                # If the file does not contain valid JSON, leave data as it is
                pass

    # Convert the data to a DataFrame
    df = pd.DataFrame(data)

    # Apply moving average smoothing
    df = apply_moving_average_smoothing(df)

    # Convert the DataFrame back to a dictionary
    data = df.to_dict(orient='list')

    return JsonResponse(data)

def get_psd_data(request, trip_id):
    data = {
        'frequency': [],
        'frequency_adjusted': [],
        'psd_value': [],
        'psd_value_avg': []
    }

    # Attempt to retrieve the Trip instance by trip_id
    try:
        trip = Trip.objects.get(id=trip_id)
    except ObjectDoesNotExist:
        return JsonResponse({'error': 'Trip not found'}, status=404)

    # Check if the PSD data file exists and is accessible
    if os.path.exists(trip.psd_data.path):
        try:
            with open(trip.psd_data.path, 'r') as file:
                psd_data = json.load(file)
                # Extract each list from the JSON and add it to the respective key in the 'data' dictionary
                data['frequency'] = psd_data.get('frequency', [])
                data['frequency_adjusted'] = psd_data.get('frequency_adjusted', [])
                data['psd_value'] = psd_data.get('psd_value', [])
                data['psd_value_avg'] = psd_data.get('psd_value_avg', [])
        except (IOError, json.JSONDecodeError) as e:
            # Log error and return an empty data structure if file can't be read or is not valid JSON
            return JsonResponse({'error': str(e)}, status=500)

    return JsonResponse(data)


###############################################################################
#Functions to Update the data on the files

# Define the view for updating the BME data
@method_decorator(csrf_exempt, name='dispatch')
class bme_data(View):
    def post(self, request, *args, **kwargs):
        logger_id = kwargs['logger_id']   
        
        # Get the logger and verifies if exist
        try:
            logger = Logger.objects.get(id=logger_id)
        except ObjectDoesNotExist:
            return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)
        
        logger.last_communication_timestamp = datetime.now()
        logger.save()

        # Get the current trip for the logger
        trip = logger.current_trip
        if trip is None:
            return JsonResponse({'status': 'failure', 'error': 'No ongoing trip found for this logger'}, status=404)
    
        # Parse the JSON body of the request
        body_unicode = request.body.decode('utf-8')
        body_data = json.loads(body_unicode)
        new_bme_data = body_data.get('data')

        if new_bme_data:
            try:
                if isinstance(new_bme_data, str):
                    new_bme_data = json.loads(new_bme_data)
                # Check if the file exists
                if os.path.exists(trip.bme_data.path):
                    # If the file exists, read the existing data
                    with open(trip.bme_data.path, 'r') as file:
                        try:
                            existing_data = json.load(file)
                        except json.JSONDecodeError:
                            existing_data = []
                else:
                    # If the file doesn't exist, initialize an empty list
                    existing_data = []

                # Append the new data to the existing data
                existing_data.extend(new_bme_data)

                # Write the data back to the file
                with open(trip.bme_data.path, 'w') as file:
                    json.dump(existing_data, file, indent=4)

                return JsonResponse({'status': 'success'})
            except FileNotFoundError:
                return HttpResponse('File not found', status=404)
        else:
            return JsonResponse({'status': 'failure', 'error': 'No data provided'}, status=400)
        
# Define the view for updating the impacts data
@method_decorator(csrf_exempt, name='dispatch')
class impact_data(View):
    def post(self, request, *args, **kwargs):
        logger_id = kwargs['logger_id']
        
        # Get the logger and verifies if exist
        try:
            logger = Logger.objects.get(id=logger_id)
        except ObjectDoesNotExist:
            return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)
        
        logger.last_communication_timestamp = datetime.now()
        logger.save()

        # Get the current trip for the logger
        trip = logger.current_trip
        if trip is None:
            return JsonResponse({'status': 'failure', 'error': 'No ongoing trip found for this logger'}, status=404)
    
        # Parse the JSON body of the request
        body_unicode = request.body.decode('utf-8')
        body_data = json.loads(body_unicode)
        new_impact_data = body_data.get('data')

        if new_impact_data:
            try:
                if isinstance(new_impact_data, str):
                    new_impact_data = json.loads(new_impact_data)
                # Check if the file exists
                if os.path.exists(trip.impact_data.path):
                    # If the file exists, read the existing data
                    with open(trip.impact_data.path, 'r') as file:
                        try:
                            existing_data = json.load(file)
                        except json.JSONDecodeError:
                            existing_data = []
                else:
                    # If the file doesn't exist, initialize an empty list
                    existing_data = []

                # Append the new data to the existing data
                existing_data.extend(new_impact_data)
                
                for item in existing_data:
                    if 'impact_id' not in item:
                        item['impact_id'] = existing_data.index(item)
                    print(item['impact_id'])

                # Write the data back to the file
                with open(trip.impact_data.path, 'w') as file:
                    json.dump(existing_data, file, indent=4)

                return JsonResponse({'status': 'success'})
            except FileNotFoundError:
                return HttpResponse('File not found', status=404)
        else:
            return JsonResponse({'status': 'failure', 'error': 'No data provided'}, status=400)
  
# Define the view for updating the vibration data
@method_decorator(csrf_exempt, name='dispatch')
class vibration_data(View):
    def post(self, request, *args, **kwargs):
        logger_id = kwargs['logger_id']   
        
        # Get the logger and verifies if exist
        try:
            logger = Logger.objects.get(id=logger_id)
        except ObjectDoesNotExist:
            return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)
        
        logger.last_communication_timestamp = datetime.now()
        logger.save()

        # Get the current trip for the logger
        trip = logger.current_trip
        # Create a TripConfig instance for the trip
        tripconfig = TripConfig(trip=trip)
        
        if trip is None:
            return JsonResponse({'status': 'failure', 'error': 'No ongoing trip found for this logger'}, status=404)
        
        if tripconfig is None:
            return JsonResponse({'status': 'failure', 'error': 'No configuration found for this trip'}, status=404)
    
        # Parse the JSON body of the request
        body_unicode = request.body.decode('utf-8')
        body_data = json.loads(body_unicode)
        new_vibration_data = body_data.get('data')

        if new_vibration_data:
            try:
                if isinstance(new_vibration_data, str):
                    new_vibration_data = json.loads(new_vibration_data)
                # Check if the file exists
                if os.path.exists(trip.vibration_data.path):
                    # If the file exists, read the existing data
                    with open(trip.vibration_data.path, 'r') as file:
                        try:
                            existing_data = json.load(file)
                        except json.JSONDecodeError:
                            existing_data = []
                else:
                    # If the file doesn't exist, initialize an empty list
                    existing_data = []

                # Append the new data to the existing data
                existing_data.extend(new_vibration_data)

                # Write the data back to the file
                with open(trip.vibration_data.path, 'w') as file:
                    json.dump(existing_data, file, indent=4)  
                
                #Call the script to process the vibration data
                script_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                script_path = os.path.join(script_dir, "script_data_psd_processing.py")
                subprocess.call(["python", script_path, str(trip.psd_data.path), str(tripconfig.sample_rate), str(trip.vibration_data.path)])

                return JsonResponse({'status': 'success'})
            except FileNotFoundError:
                return HttpResponse('File not found', status=404)
        else:
            return JsonResponse({'status': 'failure', 'error': 'No data provided'}, status=400)
              
# Define the view for updating the gps data
@method_decorator(csrf_exempt, name='dispatch')
class gps_data(View):
    def post(self, request, *args, **kwargs):
        logger_id = kwargs['logger_id']
        
        # Get the logger and verifies if exist
        try:
            logger = Logger.objects.get(id=logger_id)
        except ObjectDoesNotExist:
            return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)

        logger.last_communication_timestamp = datetime.now()
        logger.save()
        
        # Get the current trip for the logger
        trip = logger.current_trip
        if trip is None:
            return JsonResponse({'status': 'failure', 'error': 'No ongoing trip found for this logger'}, status=404)
    
        # Parse the JSON body of the request
        body_unicode = request.body.decode('utf-8')
        body_data = json.loads(body_unicode)
        new_gps_data = body_data.get('data')

        if new_gps_data:
            try:
                if isinstance(new_gps_data, str):
                    new_gps_data = json.loads(new_gps_data)
                # Check if the file exists
                if os.path.exists(trip.gps_data.path):
                    # If the file exists, read the existing data
                    with open(trip.gps_data.path, 'r') as file:
                        try:
                            existing_data = json.load(file)
                        except json.JSONDecodeError:
                            existing_data = []
                else:
                    # If the file doesn't exist, initialize an empty list
                    existing_data = []

                # Append the new data to the existing data
                existing_data.extend(new_gps_data)
                      
                # Updates the information on the data logger about gps data
                last_timestamp_epoch = existing_data[-1]['timestamp']
                logger.last_gps_timestamp = datetime.fromtimestamp(last_timestamp_epoch)
                logger.last_gps_location_latitude = existing_data[-1]['latitude']
                logger.last_gps_location_longitude = existing_data[-1]['longitude']
                logger.save()

                # Write the data back to the file
                with open(trip.gps_data.path, 'w') as file:
                    json.dump(existing_data, file, indent=4)
                    
            

                return JsonResponse({'status': 'success'})
            except FileNotFoundError:
                return HttpResponse('File not found', status=404)
        else:
            return JsonResponse({'status': 'failure', 'error': 'No data provided'}, status=400)

#Define the view for updating the log data
@method_decorator(csrf_exempt, name='dispatch')
class LogFiles_Functions(View):
    def get(self, request, *args, **kwargs):
        logger_id = kwargs['logger_id']
        logger = Logger.objects.get(id=logger_id)
        try:
            with open(logger.device_log_path.path, 'r') as file:
                data = file.read()
            return HttpResponse(data, content_type='text/plain')
        except FileNotFoundError:
            return HttpResponse('File not found', status=404)
    
    
    def post(self, request, *args, **kwargs):
        logger_id = kwargs['logger_id']
    
        # Get the logger and verifies if exist
        try:
            logger = Logger.objects.get(id=logger_id)
        except ObjectDoesNotExist:
            return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)
        
        logger.last_communication_timestamp = datetime.now()
        logger.save()
    
        # Parse the JSON body of the request
        body_unicode = request.body.decode('utf-8')
        body = json.loads(body_unicode)
        new_log_data = csv.reader(body['data'].splitlines())
    
        if new_log_data:
            try:
                # Check if the file exists
                if os.path.exists(logger.device_log_path.path):
                    # If the file exists, read the existing data
                    with open(logger.device_log_path.path, 'r') as file:
                        try:
                            existing_data = list(csv.reader(file))
                        except csv.Error:
                            existing_data = []
                else:
                    # If the file doesn't exist, initialize an empty list
                    existing_data = []
    
                # Append the new data to the existing data
                existing_data.extend(new_log_data)
    
                # Write the data back to the file
                with open(logger.device_log_path.path, 'w', newline='') as file:
                    writer = csv.writer(file)
                    writer.writerows(existing_data)
    
                return JsonResponse({'status': 'success'})
            except FileNotFoundError:
                return HttpResponse('File not found', status=404)
        else:
            return JsonResponse({'status': 'failure', 'error': 'No data provided'}, status=400)
        
#Define the view for updating the battery level
@method_decorator(csrf_exempt, name='dispatch')
class battery_level(View):
    def post(self, request, *args, **kwargs):
        logger_id = kwargs['logger_id']
        
        # Get the logger and verifies if exist
        try:
            logger = Logger.objects.get(id=logger_id)
        except ObjectDoesNotExist:
            return JsonResponse({'status': 'failure', 'error': 'Logger not found'}, status=404)
        
        # Parse the JSON body of the request
        body_unicode = request.body.decode('utf-8')
        body_data = json.loads(body_unicode)
        battery_level = body_data.get('data')
        new_battery_level = float(battery_level[-1])
        
        if new_battery_level:
            logger.battery_level = int((new_battery_level-3.2)/(4.2-3.2)*100) #Convert the battery level to percentage
            logger.save()
            return JsonResponse({'status': 'success'})
        else:
            return JsonResponse({'status': 'failure', 'error': 'No data provided'}, status=400)