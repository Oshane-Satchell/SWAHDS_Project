import datetime
import os
import motor.motor_asyncio
from fastapi.middleware.cors import CORSMiddleware
from fastapi import FastAPI, HTTPException, Depends, Form
from pydantic import BaseModel
from motor.motor_asyncio import AsyncIOMotorClient
import os
from dotenv import load_dotenv
from typing import Annotated 
from datetime import datetime

# Load environment variables
load_dotenv()

# MongoDB connection
database_url = os.environ.get('MONGO_URL')
client = motor.motor_asyncio.AsyncIOMotorClient(database_url)
db = client.swahds
COLLECTION_NAME = "user-inputs"
COLLECTION1_NAME = "sensor-data"
settings = db[COLLECTION_NAME]
updates = db[COLLECTION1_NAME]


app = FastAPI()

origins = [
    "http://127.0.0.1:8000"
]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


class Data(BaseModel):
    temperature: float
    humidity: float
    rain_water_level: bool
    nwc_water_level: bool
    zone1_soil_moisture: float
    zone2_soil_moisture: float
    zone3_soil_moisture: float

    moisture1:bool
    moisture2:bool
    moisture3:bool
    central:bool
    created_at: datetime= datetime.now()
 
# Define the data model for user inputs
class UserInput(BaseModel):
    zone1_plant_name: Annotated[str, Form()]
    zone1_watering_schedule: Annotated[str, Form()]
    zone1_watering_intervals: Annotated[int, Form()]
    zone2_plant_name: Annotated[str, Form()]
    zone2_watering_schedule: Annotated[str, Form()]
    zone2_watering_intervals: Annotated[int, Form()]
    zone3_plant_name: Annotated[str, Form()]
    zone3_watering_schedule: Annotated[str, Form()]
    zone3_watering_intervals: Annotated[int, Form()]

    @classmethod
    def as_form(
        cls, 
        zone1_plant_name: str=Form(...),
        zone1_watering_schedule: str=Form(...),
        zone1_watering_intervals: int=Form(...),
        zone2_plant_name: str=Form(...),
        zone2_watering_schedule: str=Form(...),
        zone2_watering_intervals: int=Form(...),
        zone3_plant_name: str=Form(...),
        zone3_watering_schedule: str=Form(...),
        zone3_watering_intervals: int=Form(...),

    ):
        return cls(
            zone1_plant_name=zone1_plant_name,
            zone1_watering_schedule=zone1_watering_schedule,
            zone1_watering_intervals=zone1_watering_intervals,
            zone2_plant_name=zone2_plant_name,
            zone2_watering_schedule=zone2_watering_schedule,
            zone2_watering_intervals=zone2_watering_intervals,
            zone3_plant_name=zone3_plant_name,
            zone3_watering_schedule=zone3_watering_schedule,
            zone3_watering_intervals=zone3_watering_intervals
        )
        

MOISTURE_THRESHOLD_HIGH = 2500
MOISTURE_THRESHOLD_LOW = 1500

@app.post("/log-inputs")
async def log_inputs(user_input: UserInput=Depends(UserInput.as_form)):
    print(user_input)
    obj = await settings.find().sort('_id', -1).limit(1).to_list(1)

    if obj:
        await settings.update_one({"_id": obj[0]["_id"]}, {"$set": user_input.model_dump()})
        new_obj = await settings.find_one({"_id": obj[0]["_id"]})
    else:
        new = await settings.insert_one(user_input.model_dump())
        new_obj = await settings.find_one({"_id": new.inserted_id})
    
    new_obj['_id'] = str(new_obj['_id'])

    if new_obj:
        return {"message": "User input logged successfully"}
    raise HTTPException(status_code=500, detail="Failed to log user input")


@app.post("/sensor-data")
async def sensor_data(data: Data):
    print(data)
    param = await settings.find().sort('_id', -1).limit(1).to_list(1)
    now = datetime.now().time()

    if param:
        time1 = param[0]["zone1_watering_schedule"]
        time2 = param[0]["zone2_watering_schedule"]
        time3 = param[0]["zone2_watering_schedule"]

        zone1 = datetime.strptime(str(time1),"%H:%M:%S.%f")
        zone2 = datetime.strptime(str(time2),"%H:%M:%S.%f")        
        zone3 = datetime.strptime(str(time3),"%H:%M:%S.%f")

        off1 = zone1 + datetime.strptime("00:15:00","%H:%M:%S.%f")
        off2 = zone2 + datetime.strptime("00:15:00","%H:%M:%S.%f")
        off3 = zone3 + datetime.strptime("00:15:00","%H:%M:%S.%f")


        zone1off = datetime.strptime(str(off1),"%H:%M:%S.%f")
        zone2off = datetime.strptime(str(off2),"%H:%M:%S.%f")        
        zone3off = datetime.strptime(str(off3),"%H:%M:%S.%f")

    else:
        zone1 = datetime.strptime(str(now),"%H:%M:%S.%f")
        zone2 = datetime.strptime(str(now),"%H:%M:%S.%f")        
        zone3 = datetime.strptime(str(now),"%H:%M:%S.%f")

        off1 = zone1 + datetime.strptime("00:15:00","%H:%M:%S.%f")
        off2 = zone2 + datetime.strptime("00:15:00","%H:%M:%S.%f")
        off3 = zone3 + datetime.strptime("00:15:00","%H:%M:%S.%f")

        zone1off = datetime.strptime(str(off1),"%H:%M:%S.%f")
        zone2off = datetime.strptime(str(off2),"%H:%M:%S.%f")        
        zone3off = datetime.strptime(str(off3),"%H:%M:%S.%f")


 
    currenttime = datetime.strptime(str(now),"%H:%M:%S.%f")


    data["moisture1"] = (float(data["zone1_soil_moisture"]) >= MOISTURE_THRESHOLD_HIGH and zone1 < currenttime < zone1off )
    data["moisture2"] = (float(data["zone2_soil_moisture"]) >= MOISTURE_THRESHOLD_HIGH and zone2 < currenttime < zone2off )
    data["moisture3"] = (float(data["zone3_soil_moisture"]) >= MOISTURE_THRESHOLD_HIGH and zone3 < currenttime < zone3off )
    data["central"]= (data["moisture1"] or data["moisture2"] or data["moisture3"] )


    new_settings = await updates.insert_one(data.model_dump())
    new_obj = await updates.find_one({"_id": new_settings.inserted_id})
    new_obj['_id'] = str(new_obj['_id'])
   
    if new_obj.inserted_id:
        return {"message": "Sensor data logged successfully"}
    raise HTTPException(status_code=500, detail="Failed to log user input")

@app.get("/get-schedule")
async def get_schedule():
    data = await settings.find().sort([('$natural', -1)]).limit(1).to_list(1)
    if data:
        return data[0]
    raise HTTPException(status_code=404, detail="No schedule found")



@app.get("/get-sensor")
async def get_schedule():
    data = await updates.find().sort([('$natural', -1)]).limit(1).to_list(1)
    if data:
        return data[0]
    raise HTTPException(status_code=404, detail="No schedule found")

