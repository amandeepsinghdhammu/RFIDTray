#!/usr/bin/env python
import sys
import RPi.GPIO as GPIO
from lib_nrf24 import NRF24
import time
import spidev
from time import sleep
import signal

GPIO.setmode(GPIO.BCM)

#pipes = [[0xe7, 0xe7, 0xe7, 0xe7, 0xe7], [0xc2, 0xc2, 0xc2, 0xc2, 0xc2]]
#pipes = [[0xE7, 0xE7, 0xE7, 0xE7, 0xE7], [0xF0, 0xF0, 0xF0, 0xF0, 0xE1]]
pipes = [[0xE8, 0xE8, 0xF0, 0xF0, 0xE1], [0xF0, 0xF0, 0xF0, 0xF0, 0xE1]]

radio = NRF24(GPIO, spidev.SpiDev())
radio.begin(0, 17)
radio.setPayloadSize(32)
radio.setChannel(0x76)

radio.setDataRate(NRF24.BR_1MBPS)
radio.setPALevel(NRF24.PA_MIN)

radio.setAutoAck(True)
radio.enableDynamicPayloads()
radio.enableAckPayload()

radio.openReadingPipe(1, pipes[1])
#radio.printDetails()
radio.stopListening()
radio.startListening()

runProgram = True;

try:
    while runProgram:
        while not radio.available(0):
            time.sleep(1 / 100)
        receivedMessage = []
        radio.read(receivedMessage, radio.getDynamicPayloadSize())

        string = ""
        data = []
        for n in receivedMessage:
            # Decode into standard unicode set
            if (n >= 32 and n <= 126):
                string += chr(n)
        #print(string)        
        radio.stopListening()
        radio.startListening()

        if(string != ""):
          data = string.split('-')

        if(len(data) > 0):
            print(string)
            sys.stdout.flush()
        
        #time.sleep(1)

except KeyboardInterrupt:
    GPIO.cleanup()

# Capture SIGINT for cleanup when the script is aborte
def end_read(signal,frame):
    GPIO.cleanup()

# Hook the SIGINT
signal.signal(signal.SIGINT, end_read)
