from django.contrib.auth.models import User
from django.db import models
from django.core.validators import MinValueValidator, MaxValueValidator
from datetime import datetime

class Logger(models.Model):
    
    def get_upload_path(instance, filename):
        # Get the last Trip in the database
        last_device = Logger.objects.last()

        # If there are no trips in the database, set the id to 1
        # Otherwise, set the id to the id of the last trip plus one
        id = 1 if last_device is None else last_device.id + 1

        return 'devices_logs/device_{0}/_log'.format(id)
    
    id = models.AutoField(primary_key=True)
    user_associated = models.ForeignKey(User, on_delete=models.CASCADE)
    name = models.CharField(max_length=200,null=True, blank=True)
    current_trip = models.OneToOneField('Trip', on_delete=models.SET_NULL, null=True, blank=True, related_name='associated_logger')
    battery_level = models.FloatField(default=100,null=True, blank=True)
    device_log_path = models.FileField(upload_to=get_upload_path, null=True, blank=True)
    last_communication_timestamp = models.DateTimeField(null=True, blank=True)
    last_gps_timestamp = models.DateTimeField(null=True, blank=True)
    last_gps_location_latitude = models.FloatField(null=True, blank=True)
    last_gps_location_longitude = models.FloatField(null=True, blank=True)
    

    def __str__(self):
        return f"Logger {self.id} - User: {self.user_associated.username}"


class Trip(models.Model):
    
    def get_upload_path(instance, filename):
        # Get the last Trip in the database
        last_trip = Trip.objects.last()

        # If there are no trips in the database, set the id to 1
        # Otherwise, set the id to the id of the last trip plus one
        id = 1 if last_trip is None else last_trip.id + 1

        return 'trip_logs/Trip_{0}/{1}'.format(id, filename)
    
    
    id = models.AutoField(primary_key=True)
    logger = models.ForeignKey(Logger, on_delete=models.CASCADE, related_name='trips')
    config = models.OneToOneField('TripConfig', on_delete=models.CASCADE, related_name='associated_trip',null=True, blank=True)
    name = models.CharField(max_length=200, default='Default Name')
    start_date = models.DateTimeField(default=datetime.fromtimestamp(0))
    end_date = models.DateTimeField(null=True, blank=True)
    bme_data = models.FileField(upload_to=get_upload_path,null=True, blank=True)
    impact_data = models.FileField(upload_to=get_upload_path,null=True, blank=True)
    vibration_data = models.FileField(upload_to=get_upload_path,null=True, blank=True)
    psd_data = models.FileField(upload_to=get_upload_path,null=True, blank=True)
    gps_data = models.FileField(upload_to=get_upload_path,null=True, blank=True)

    def __str__(self):
        return f"Trip {self.id} - Logger: {self.logger.id}"
    


class TripConfig(models.Model):
    trip = models.OneToOneField(Trip, on_delete=models.CASCADE, related_name='associated_config')
    mqtt_address = models.CharField(max_length=255, default='172.187.90.157')
    mqtt_port = models.IntegerField(validators=[MinValueValidator(1), MaxValueValidator(65535)], default=1883)
    sleep_mode = models.BooleanField(default=False)
    sleep_interval = models.IntegerField(default=10)
    movement_detection_threshold = models.FloatField(default=0.3)
    bme_mode = models.BooleanField(default=False)
    bme_sampling_interval = models.IntegerField(default=30)
    bme_sampling_duration = models.IntegerField(default=10)
    gps_mode = models.BooleanField(default=False)
    gps_interval_timer = models.IntegerField(default=2 * 60 * 1000)
    max_time_try_location = models.IntegerField(default=60)
    accelerometer_scale = models.IntegerField(choices=[(2, '2g'), (4, '4g'), (8, '8g'), (16, '16g')], default=8)
    sample_rate = models.IntegerField(default=200)
    impact_mode = models.BooleanField(default=False)
    impact_threshold = models.FloatField(default=4.0)
    vibration_mode = models.BooleanField(default=False)
    n_vibration_per_sample = models.IntegerField(default=512)
    vibration_threshold = models.FloatField(default=0.005)
    vibration_sampling_interval = models.IntegerField(default=20)
    mqtt_mode = models.BooleanField(default=False)
    mqtt_interval = models.IntegerField(default=10)
    logs_mode = models.BooleanField(default=False)
    esp_stats_mode = models.BooleanField(default=False)
    esp_stats_interval_timer = models.IntegerField(default=10)

    def __str__(self):
        return f"Config for Trip {self.trip.id}"
    
    
    

