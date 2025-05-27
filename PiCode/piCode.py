import threading
import requests
import time
import speech_recognition as sr
import paho.mqtt.client as Mqtt
from paho import mqtt
from datetime import datetime
from secret import *

last_checked_hour = None
stop_event = threading.Event()
def dataextract():
    extracted = False
    if(datetime.now().hour == 0 and not extracted ):
        respond = requests.get(url)
        data = respond.json()
        global rain_chance_list
        rain_chance_list = data["hourly"]["precipitation_probability"]
        global sunrise
        sunrise = datetime.fromisoformat(data["daily"]["sunrise"][0]).hour
        global sunset
        sunset = datetime.fromisoformat(data["daily"]["sunset"][0]).hour
        extracted = True
    elif(datetime.now().hour > 0 and extracted):
        extracted = False
    time.sleep(3500)

def Recognition():
    r = sr.Recognizer()
    while not stop_event.is_set():
        with sr.Microphone() as source:
            print("listening")
            word = ""
            r.adjust_for_ambient_noise(source)
            audio = r.listen(source)
            try:
                word = r.recognize_google(audio)
                print(word)
            except:
                print("not recognized")

            print(word)
            match(word):
                case "open window":
                    client.publish(topic, "openwindowbycommand", 2, True)
                case "close window":
                    client.publish(topic, "closewindowbycommand",2, True)
                case "set auto":
                    client.publish(topic, "setautomode", 2, True)

def on_connect(client, userdata, flags, rc, properties = None):
    if rc == 0:
        print("connected with result code" + str(rc))
    else:
        print("failed")

client = Mqtt.Client(client_id = "RPi", userdata = None, protocol = Mqtt.MQTTv5)
client.on_connect = on_connect
client.tls_set(tls_version = mqtt.client.ssl.PROTOCOL_TLS)
client.username_pw_set(username, password)
try:
    client.connect(broker, port)
except:
    print("cannot connect to client")
client.loop_start()

speech_thread = threading.Thread(target = Recognition)
speech_thread.start()

dataextraction_thread = threading.Thread(target = dataextract)
dataextraction_thread.start()
try:
    while True:
            now = datetime.now()
            hour = now.hour
            minute = now.minute
            print(hour)
            if (minute > 50 and minute <=59) and hour != last_checked_hour:
                print(f"Running check at {now.strftime('%H:%M')}")
                try:
                    rain_chance = rain_chance_list[hour + 1]
                    last_checked_hour = hour
                    if (rain_chance <= threshold) and ( hour > sunrise and hour < sunset):
                        client.publish(topic, 'openwindow', 2, True)
                    elif (rain_chance > threshold) or (hour > sunset or hour < sunrise):
                        client.publish(topic, 'closewindow', 2, True)
                except:
                    print("can't publish the message")
            time.sleep(300)

except KeyboardInterrupt:
        stop_event.set()
        speech_thread.join()
        dataextraction_thread.join()
        client.loop_stop()
