# Generated by Django 5.0.3 on 2024-04-10 12:22

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('WebApp', '0005_remove_logger_config_file_remove_trip_log_and_more'),
    ]

    operations = [
        migrations.AddField(
            model_name='logger',
            name='name',
            field=models.CharField(blank=True, max_length=200, null=True),
        ),
    ]