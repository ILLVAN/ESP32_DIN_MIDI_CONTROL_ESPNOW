/*
ESP 3: small MIDI I/O BOX AT TIMELINE, delay / loop control

*/

#include <Arduino.h>
#include <MIDI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ezButton.h>
#include <protocol2copy.h> // defines ESPNOW message struct

#define DEBOUNCE_TIME 50 // the debounce time in millisecond, increase this time if it still chatters

#define LED 2
#define BUTTON_1 13
#define BUTTON_2 27
#define BUTTON_3 26
#define BLUE 32
#define GREEN 33
#define RED 25

#define BOARD_ID 3

ezButton button1(BUTTON_1); 
ezButton button2(BUTTON_2); 
ezButton button3(BUTTON_3); 

hw_timer_t *MIDI_watchr = NULL;

struct_message nowMessageSend;
struct_message nowMessageRecv;
esp_now_peer_info_t peerInfo;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

const byte midi_clock = 0xF8;
const byte midi_start = 0xFA;
const byte midi_continue = 0xFB;
const byte midi_stop = 0xFC;

volatile uint8_t delayTapDiv = 3;
volatile byte midiType;
volatile byte incomingByte;
volatile uint8_t beatCount = 0;
volatile uint8_t sendBeatCount = 0;
volatile uint8_t barCount = 1;

volatile uint8_t source;
volatile uint8_t loopVolume;

volatile boolean butflag_1 = false;
volatile boolean butflag_2 = false;
volatile boolean butflag_3 = false;
volatile boolean butState_1 = false;
volatile boolean midiClockToggle = true;
volatile boolean stopClock = true;


volatile uint8_t loopReqState = 0;
volatile uint8_t loopState = 0; // 0: idle 1: recording / 2: playing
volatile uint16_t timelineControlState; 
volatile uint16_t timelineCycleState[2]; 
volatile uint16_t alive; 
volatile boolean aliveFlag;

volatile int i,j;                        

void IRAM_ATTR onTimer(){
  alive++;
  if (alive > 2000){ 
    alive = 0;
    aliveFlag = false;
  }
  if (MIDI.read() > 0) {
   incomingByte = MIDI.getType(); 
   switch (incomingByte){
      case midi_start:
            stopClock = false;
            aliveFlag = false;
            beatCount = 1;
            barCount = 1;
            break;
      case midi_stop:
            stopClock = true;
            MIDI.sendControlChange(85, 100, 10); // Stop Loop if playing
            digitalWrite(BLUE, LOW); 
            digitalWrite(GREEN, LOW); 
            digitalWrite(RED, LOW);
            beatCount = 1;
            break;
      case midi_clock:
            if (beatCount > 95){
              beatCount = 1;
            }
            else if (!stopClock){
              beatCount++; 
              if (beatCount % 6 == 0){ // Sixteenths
                // MIDI.sendNoteOn(75, 100, 2);
              }
              else if (beatCount % 6 == 2){
                // MIDI.sendNoteOff(75, 0, 2);
              }
              if (beatCount % 12 == 0 && loopReqState + loopState < 1 && !midiClockToggle){ // Eights
                  digitalWrite(RED, HIGH);
                // MIDI.sendNoteOn(63, 100, 5); 
              }
              else if (beatCount % 12 == 4 && loopReqState + loopState < 1){
                if (loopState != 1){
                  digitalWrite(RED, LOW);
                }
                // MIDI.sendNoteOff(63, 0, 5);
              }
              if (beatCount % 24 == 0){ // Quarters
                switch(beatCount){
                  case 24:
                      sendBeatCount = 2; 
                      if (loopState + loopReqState < 1){
                        digitalWrite(BLUE, HIGH); 
                      }
                      else if (loopState == 2 && loopReqState != 4){
                        digitalWrite(BLUE, LOW); 
                      } 
                      if (loopState == 1 || loopReqState > 1){
                        digitalWrite(RED, LOW); 
                      }      
                      digitalWrite(GREEN, LOW);               
                      break;
                  case 48: 
                      sendBeatCount = 3;
                      if ((loopState + loopReqState < 1) || loopReqState == 4){
                        digitalWrite(GREEN, HIGH); 
                      }
                      if ((loopState == 1 && loopReqState < 2) || loopReqState == 4){
                        digitalWrite(RED, HIGH); 
                      }
                      if (loopState == 2 || loopReqState == 4){
                        digitalWrite(BLUE, HIGH); 
                      }
                      break;
                  case 72:
                      sendBeatCount = 4;
                      if (barCount == 4 && loopState == 1){
                        loopReqState = 2;
                      }       
                      if (loopState == 1 || loopReqState > 1){
                        digitalWrite(RED, LOW); 
                      }
                      if (loopState == 2 && loopReqState != 4){
                        digitalWrite(BLUE, LOW); 
                      }   
                      digitalWrite(GREEN, LOW);                
                      break;
                  case 96:
                      sendBeatCount = 1;
                      barCount++;
                      if ((loopState == 1 && loopReqState < 2) || loopReqState == 4){
                        digitalWrite(RED, HIGH); 
                      }
                      if (loopState == 2 || loopReqState == 4){
                        digitalWrite(BLUE, HIGH); 
                      }       
                      if (loopReqState == 4){
                        digitalWrite(GREEN, HIGH); 
                      }                
                      if (barCount == 5){
                        barCount = 1;
                        if (loopReqState == 1 && loopState != 1){ // REC
                          MIDI.sendControlChange(87,100,10);
                          loopState = 1;
                          loopReqState = 0;
                        }
                        else if (loopReqState == 2 && loopState != 2){ // PLAY
                          MIDI.sendControlChange(86, 100, 10);
                          loopState = 2;
                          loopReqState = 0;
                        }
                        else if (loopReqState == 4){ // UNDO
                          loopReqState = 0;
                          MIDI.sendControlChange(89, 100, 10);
                          digitalWrite(BLUE, LOW); 
                          digitalWrite(GREEN, LOW);
                          digitalWrite(RED, LOW);
                        }
                      }
                      if (loopReqState + loopState < 1) {
                        digitalWrite(BLUE, LOW); 
                        digitalWrite(GREEN, LOW);
                        digitalWrite(RED, LOW);
                      }
                      break;
                  default: break;
                }
                timelineControlState = 0;
                if (!stopClock){
                  timelineControlState |= (1 << 1); // clock IN
                  Serial.println(timelineControlState);
                  Serial.println("gotPulse");
                }
                if (midiClockToggle){
                  timelineControlState |= (1 << 2); // clocked
                  Serial.println(timelineControlState);
                  Serial.println("clocked");
                }   
                if (loopState == 2){
                  timelineControlState |= (1 << 3); // PLAYING
                  Serial.println(timelineControlState);
                  Serial.println("PLAYING");
                }      
                if (loopReqState == 2){
                  timelineControlState |= (1 << 4); // PLAY REQ
                  Serial.println(timelineControlState);
                  Serial.println("PLAY REQ");
                }     
                if (loopState == 1){
                  timelineControlState |= (1 << 5); // RECORDING
                  Serial.println(timelineControlState);
                  Serial.println("RECORDING");
                }    
                if (loopReqState == 1){
                  timelineControlState |= (1 << 6); // REC REQ
                  Serial.println(timelineControlState);
                  Serial.println("REC REQ");
                } 
                if (loopReqState == 4){
                  timelineControlState |= (1 << 7); // UNDO REQ
                  Serial.println(timelineControlState);
                  Serial.println("UNDO REQ");
                }                                                         
                nowMessageSend.sourceID = BOARD_ID;
                nowMessageSend.destinationID = 0; // Broadcast
                nowMessageSend.messageType = TIMELINE_CYCLE_STATE + TIMELINE_CONTROL_STATE; 
                nowMessageSend.sendTimelineCycleState[0] = barCount;
                nowMessageSend.sendTimelineCycleState[1] = sendBeatCount;
                nowMessageSend.sendTimelineControlState = timelineControlState;
                esp_now_send(0, (uint8_t *) &nowMessageSend, sizeof(nowMessageSend));
              }
            }; break;
      default: break;
    }
  } 
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // TBD
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) { 
  memcpy(&nowMessageRecv, incomingData, sizeof(nowMessageRecv));
  if (nowMessageRecv.sourceID == 2){
    if (nowMessageRecv.messageType & LOOP_LEVEL){
      if (nowMessageRecv.sendLoopVolume != loopVolume){
        Serial.println("Received new loop volume");
        loopVolume = nowMessageRecv.sendLoopVolume;
        MIDI.sendControlChange(98, loopVolume, 10);
      }

    }
  } 
}

void setup() {
  pinMode(LED,OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  button1.setDebounceTime(DEBOUNCE_TIME); 
  button2.setDebounceTime(DEBOUNCE_TIME); 
  button3.setDebounceTime(DEBOUNCE_TIME); 
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  memcpy(peerInfo.peer_addr, MAC_ESP2, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);
  MIDI.begin(MIDI_CHANNEL_OFF);
  MIDI_watchr = timerBegin(0, 80, true); 
  timerAttachInterrupt(MIDI_watchr, &onTimer, true);
  timerAlarmWrite(MIDI_watchr, 1000, true); 
  timerAlarmEnable(MIDI_watchr);
}

void loop() {
  button1.loop();
  button2.loop();
  button3.loop();
  if (button1.isPressed() && butflag_1 == false){
    Serial.println ("Button 1 pressed");
    digitalWrite(GREEN, HIGH);
    if (loopReqState > 0){ 
      digitalWrite(RED, LOW);
      digitalWrite(BLUE, LOW);
      butflag_1 = true;
      loopReqState = 0;
      return;
    }
    digitalWrite(GREEN, HIGH);
    butflag_1 = true;
    if (midiClockToggle){
      MIDI.sendControlChange(63,0,10);
      midiClockToggle = false;
      digitalWrite(LED,LOW);
    }
    else {
      MIDI.sendControlChange(63,1,10);
      midiClockToggle = true;
      digitalWrite(LED,HIGH);
    }        
  }
  else if (button1.isReleased()){
    digitalWrite(GREEN, LOW);
    butflag_1 = false;
  }
  if (butflag_1 && button2.isPressed()){
    butflag_2 = true;
    if(midiClockToggle){
      MIDI.sendControlChange(3, 70, 10);
    } 
    else{
      MIDI.sendControlChange(3, 0, 10);
    }
  }
  if (butflag_1 && button3.isPressed()){
    butflag_3 = true;
    MIDI.sendControlChange(21, delayTapDiv, 10);
    delayTapDiv++;
    if (delayTapDiv == 5){
      delayTapDiv = 0;
    }
  }
  if (button2.isPressed() && butflag_2 == false){
    Serial.println ("Button 2 pressed: REC Req");
    butflag_2 = true;
    loopReqState = 1; // Request Rec
    digitalWrite(RED, HIGH);
    digitalWrite(BLUE, LOW);
    digitalWrite(GREEN, LOW);
  }
  else if (button2.isReleased()){
    if (loopReqState < 3){
      digitalWrite(GREEN, LOW);
    }  
    butflag_2 = false;
  }
  if (button3.isPressed() && butflag_3 == false){
    Serial.println ("Button 3 pressed: PLAY Req");
    butflag_3 = true;
    loopReqState = 2; // Request Play
    digitalWrite(RED, LOW);
    digitalWrite(BLUE, HIGH);
    digitalWrite(GREEN, LOW);
  }
  else if (button3.isReleased()){
    butflag_3 = false;
  }
  if (butflag_2 && button3.isPressed()){
    butflag_3 = true;
    Serial.println ("Button 2&..3 pressed: STOP");
    loopReqState = 0; // Request Stop is now done immediately, no more state 3
    loopState = 0;
    MIDI.sendControlChange(85, 100, 10);
    digitalWrite(BLUE, LOW); 
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
  }
  else if (butflag_3 && button2.isPressed()){
    butflag_2 = true;
    Serial.println ("Button 3&..2 pressed: UNDO Req");
    loopReqState = 4; // Request Undo
    digitalWrite(BLUE, HIGH);
    digitalWrite(GREEN, HIGH);
    digitalWrite(RED, HIGH);
  }
  if (stopClock && alive < 2 && !aliveFlag){
    aliveFlag = true;
    timelineControlState = 1; // alive
    Serial.println(timelineControlState);
    Serial.println("alive");
    if (loopReqState == 2){
      timelineControlState |= (1 << 4); // PLAY REQ
      Serial.println(timelineControlState);
      Serial.println("PLAY REQ");
    }     
    if (loopReqState == 1){
      timelineControlState |= (1 << 6); // REC REQ
      Serial.println(timelineControlState);
      Serial.println("REC REQ");
    } 
    if (loopReqState == 4){
      timelineControlState |= (1 << 7); // UNDO REQ
      Serial.println(timelineControlState);
      Serial.println("UNDO REQ");
    }
    nowMessageSend.sourceID = BOARD_ID;
    nowMessageSend.destinationID = 0; // Broadcast
    nowMessageSend.messageType = TIMELINE_CONTROL_STATE; 
    nowMessageSend.sendTimelineCycleState[0] = 0;
    nowMessageSend.sendTimelineCycleState[1] = 0;
    nowMessageSend.sendTimelineControlState = timelineControlState;
    esp_now_send(0, (uint8_t *) &nowMessageSend, sizeof(nowMessageSend));
  }
}

