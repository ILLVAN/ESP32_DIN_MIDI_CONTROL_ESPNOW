// ESPNOW MIDI CONTROL Protocol & Adresses
#include <Arduino.h>

typedef struct struct_message { 
    // ROUTING
    uint8_t sourceID;
    uint8_t destinationID; // | 0: Broadcast  | 128: Brain_1   | 64: Display_2   | 32: TimelineControl_3  | 16: SensorPlay_4 

    // CONTROL which changed data group is transmitted by setting corresponding bits
    uint16_t messageType;               // 0: Empty Test 

    // DATA                             // MessageType
    // ESP 3 specific
    uint8_t sendTimelineCycleState[2];  // 32768   | 0: barCount | 1: beatCount
    uint16_t sendTimelineControlState;  // 16384   | 2048-512: TAP DIVISION 0-5 | 256: STOP REQ 
        	                            //         | 128: UNDO REQ | 64: REC REQ  | 32: RECORDING | 16: PLAY REQ | 8: PLAYING | 4: clocked | 2: clockRunning | 1: alive
    uint8_t sendLoopVolume;             // 8192 

    // ESP 1 specific                           
    uint16_t sendBPM;                   // 4096
    uint16_t sendActionCount;           // 2048
    uint16_t sendArrangeLengths [2];    // 1024    | 0: currentLength  | 1: nextLength  
    uint16_t sendArrangeControl;        // 512     | 2048: AR LEAD | 1024: DT LEAD  | 512: randomPattern 
                                        //         | 128: reverse  | 64: sleep  | 32: skip  | 16: setPattern  | 8: setSkipMin  | 4: setSkipMax  | 2: resetAlreadyPlayedRandom | 1: resetAll
    uint8_t sendArrangePatterns[5];     // 256     | 0: currentPattern | 1: nextPattern  | 2:  returnPattern | 3: patternsToPlay  | 4: alreadyPlayedRandom | 5: loadedProjectNo

    // COMMON
    uint8_t sendMIDIbytes[3];           // 128     | 0: midiByte1      | 1: midiByte2    | 2: midibyte3

} struct_message;

// MESSAGE TYPES
const uint16_t TIMELINE_CYCLE_STATE = 32768;        //  10000000 00000000   1<<15 
const uint16_t TIMELINE_CONTROL_STATE = 16384;      //  01000000 00000000   1<<14 
const uint16_t LOOP_LEVEL = 8192;                   //  00100000 00000000   1<<13 
const uint16_t BPM = 4096;                          //  00010000 00000000   1<<12 
const uint16_t ACTION_COUNT = 2048;                 //  00001000 00000000   1<<11 
const uint16_t ARRANGE_LENGTHS = 1024;              //  00000100 00000000   1<<10
const uint16_t ARRANGE_CONTROL = 512;               //  00000010 00000000   1<<9
const uint16_t ARRANGE_PATTERNS = 256;              //  00000001 00000000   1<<8
const uint16_t MIDI_BYTES = 128;                    //  00000000 10000000   1<<7

// MAC ADRESSES - hard coded: placeholders! put your own
const uint8_t MAC_BROAD[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // BROADCAST
const uint8_t MAC_ESP1[6] = {0x11,0x11,0x11,0x11,0x11,0x11};  // ESP1 - Arranger
const uint8_t MAC_ESP2[6] = {0x22,0x22,0x22,0x22,0x22,0x22};  // ESP2 - Display and Loop Volume Control
const uint8_t MAC_ESP3[6] = {0x33,0x33,0x33,0x33,0x33,0x33};  // ESP3 - Timeline Control
const uint8_t MAC_ESP4[6] = {0x44,0x44,0x44,0x44,0x44,0x44};  // ESP4 - Web Server