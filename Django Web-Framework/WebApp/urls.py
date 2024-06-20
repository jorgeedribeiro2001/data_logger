from django.urls import path
from django.contrib.auth import views as auth_views
from . import views
from django.contrib.auth.decorators import login_required
from .views import LogFiles_Functions

urlpatterns = [
    path('', auth_views.LoginView.as_view(), name='index'),
    path('devices_manager/', views.devices_manager, name='devices_manager'), 
    path('add_device/', views.add_device, name='add_device'),
    
    path('login/', auth_views.LoginView.as_view(), name='login'),
    path('logout/', auth_views.LogoutView.as_view(), name='logout'),
    path('register/', views.register, name='register'),
    
    #requests trip data
    path('create-trip/', views.create_trip, name='create-trip'),
    path('end-trip-request/<int:logger_id>/', views.end_trip_request, name='end-trip-request'), 
    path('create-trip-request/', views.create_trip_request, name='create-trip-request'),
    path('select-trip/', views.select_trip, name='select_trip'),
    path('delete-trip/<int:trip_id>/', views.delete_trip, name='delete_trip'),
    path('monitoring_trip/<int:trip_id>/', views.monitoring_trip, name='monitoring_trip'),
    path('get-impact-data/<int:trip_id>/<int:impact_id>/', views.get_impact_data, name='get-one-impact-data'),
    path('get-gps-data/<int:trip_id>', views.get_gps_data, name='get-gps-data'),
    path('get-psd-data/<int:trip_id>', views.get_psd_data, name='get-psd-data'),
    
    #request for open and view the log file of an id
    path('update_log/<int:logger_id>/', LogFiles_Functions.as_view(), name='update_log'),
    path('log/<int:logger_id>/', LogFiles_Functions.as_view(), name='log_file'),
    
    #Requests for updating the data of a trip
    path('update_bme/<int:logger_id>/', views.bme_data.as_view(), name='update_bme'),
    path('update_impact/<int:logger_id>/', views.impact_data.as_view(), name='update_impact'),
    path('update_vibration/<int:logger_id>/', views.vibration_data.as_view(), name='update_vibration'),
    path('update_gps/<int:logger_id>/', views.gps_data.as_view(), name='update_gps'),
    path('update_battery/<int:logger_id>/', views.battery_level.as_view(), name='update_battery'),
    
    #Send settings to the device
    path('send_config_file/<int:logger_id>/', views.send_configuration, name='send_config_file'),
]