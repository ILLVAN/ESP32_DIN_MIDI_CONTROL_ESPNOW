# ESP32_DIN_MIDI_CONTROL_ESPNOW_ESP3

Using ESP32 boards to build a multinode MIDI controller with each node being responsible for a different task. 

Communication between boards via ESPNOW.

ESP 1 - Arrange <br>
ESP 2 - Display and Inputs <br>
ESP 3 - TimeLine hidden features control: in sync looping, clock on/off etc. <br>
ESP 4 - Web Server <br>

WORK IN PROGRESS
It's public for easily being able to talk about it with others, not because its done.
ESP 3 is working well already, handling MIDI clock and controlling delay and looper functions. 
It can receive controls via ESPNOW. Right now, it only receives looper level from ESP2.
If you have suggestions or questions feel free to contact.
