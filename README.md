# Thesis: Earth-Satellite communication system for CubeSats

This repo contains the sketches used to test and implement the modules 
and software principles proposed for the communication system of the 
CubeSat built by the Aether team of KU Leuven.

By Vincent Surkijn & Thimo Van Elst

## Sketches overview:
	FinalFramesCubeSat_CC1101 --> Loop architecture, data flow 
example
	FinalErrorCubeSat_CC1101 --> Loop architecture, error 
detection and handling
	FinalFramesGroundStation_CC1101 --> Loop architecture, 
forwards serial input over 433 MHz link
	CC1101FramesTest --> FreeRTOS, frame decoding and 
reading
	CC1101FramesTestTx --> FreeRTOS, frame transmitting
	FreeRTOS_AntennaTestTX --> FreeRTOS, transmitting and 
blinking test
