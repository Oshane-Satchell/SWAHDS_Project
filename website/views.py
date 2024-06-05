from flask import Blueprint, render_template, request, flash, jsonify
from flask_login import login_required, current_user
from . import db
import json

views = Blueprint('views', __name__)

@views.route('/', methods=['GET', 'POST'])
@login_required
def home():
    # Assuming you have a function to fetch system data from ESP32
    system_data = fetch_system_data_from_esp32()

    return render_template("home.html", user=current_user, system_data=system_data)

def fetch_system_data_from_esp32():
    # Your code to fetch system data from ESP32 goes here
    # For example:
    system_data = {
        'temperature': 25.5,
        'humidity': 60,
        'system_state': 'ON',
        'raining': False,
        'soil_moisture_zone_1': 'LOW',
        'soil_moisture_zone_2': 'HIGH',
        'soil_moisture_zone_3': 'HIGH'
    }
    return system_data
