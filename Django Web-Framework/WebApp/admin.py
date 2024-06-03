from django.contrib import admin
from .models import Logger, Trip, TripConfig

@admin.register(Logger)
class LoggerAdmin(admin.ModelAdmin):
    list_display = ['id', 'user_associated', 'current_trip', 'log']
    list_filter = ['user_associated']

    def log(self, obj):
        # Define your method here
        pass
    log.short_description = 'Log'

@admin.register(Trip)
class TripAdmin(admin.ModelAdmin):
    list_display = ['id', 'logger']
    list_filter = ['logger']
    
@admin.register(TripConfig)
class TripConfigAdmin(admin.ModelAdmin):
    list_display = ['id', 'trip']
    list_filter = ['trip']