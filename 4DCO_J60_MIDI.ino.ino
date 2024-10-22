// Code by http://cmosorchestra.com GPL-3.0

#include <MIDI.h>
#include <SPI.h>
#include <MCP48xx.h>

// Create MCP4822 objects with Chip Select pins on D10 and D9
MCP4822 dac1(10);  // First MCP4822
MCP4822 dac2(9);   // Second MCP4822

MIDI_CREATE_DEFAULT_INSTANCE();

// Frequencies for all notes from C1 to C6
float noteFrequencies[] = {
  32.70, 34.65, 36.71, 38.89, 41.20, 43.65, 46.25, 49.00, 51.91, 55.00,
  58.27, 61.74, 65.41, 69.30, 73.42, 77.78, 82.41, 87.31, 92.50, 98.00,
  103.83, 110.00, 116.54, 123.47, 130.81, 138.59, 146.83, 155.56, 164.81,
  174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94, 261.63, 277.18,
  293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16,
  493.88, 523.25, 554.37, 587.33, 622.25, 659.26, 698.46, 739.99, 783.99,
  830.61, 880.00, 932.33, 987.77, 1046.50
};

// Compensating voltages for notes (in mV)
float compensatingVoltages[] = {
  47.49, 50.40, 53.41, 56.57, 59.93, 63.52, 67.27, 71.27, 75.48, 80.00,  // A1 = 55Hz
  84.70, 89.57, 94.70, 100.15, 105.83, 111.70, 117.93, 124.41, 131.14, 138.18, 
  145.48, 160.00, 169.13, 179.05, 189.89, 201.68, 213.86, 226.56, 239.73, 
  253.57, 268.16, 283.63, 300.10, 320.00, 339.94, 360.82, 382.73, 405.78, 
  430.13, 455.84, 483.04, 511.73, 541.94, 573.81, 607.41, 640.00, 677.18,  // A4 = 440Hz
  715.96, 756.37, 798.40, 842.23, 887.91, 935.55, 985.24, 1037.05, 1091.09, 
  1157.82, 1226.61, 1297.45, 1370.53, 1446.05   // A5 = 880Hz
};

// Store the current target voltages for each DAC channel
float targetVoltages[4] = {0, 0, 0, 0};

// Timing variables for square wave generation
unsigned long nextToggleMicros1 = 0;  // For Oscillator 1
unsigned long nextToggleMicros2 = 0;  // For Oscillator 2
unsigned long nextToggleMicros3 = 0;  // For Oscillator 3
unsigned long nextToggleMicros4 = 0;  // For Oscillator 4
long period1 = 0, period2 = 0, period3 = 0, period4 = 0;

void setup() {
  SPI.begin();
  dac1.init();
  dac2.init();
  
  pinMode(2, OUTPUT);  // Square wave output for Oscillator 1
  pinMode(3, OUTPUT);  // Square wave output for Oscillator 2
  pinMode(4, OUTPUT);  // Square wave output for Oscillator 3
  pinMode(5, OUTPUT);  // Square wave output for Oscillator 4

  MIDI.begin(MIDI_CHANNEL_OMNI);  // Receive MIDI on all channels
}

void loop() {
  unsigned long currentMicros = micros();

  // --- MIDI Handling ---
  if (MIDI.read()) {
    byte type = MIDI.getType();
    
    if (type == midi::NoteOn || type == midi::NoteOff) {
      int note = MIDI.getData1() - 21;  // Map MIDI note to range (A0 = 21)
      int velocity = (type == midi::NoteOn) ? MIDI.getData2() : 0;

      if (velocity == 0) {
        handleNoteOff(note);  // Note Off
      } else {
        handleNoteOn(note, velocity);  // Note On
      }
    }
  }

  // --- Square Wave Generation for Oscillator 1 ---
  if (currentMicros >= nextToggleMicros1 && targetVoltages[0] > 0) {
    digitalWrite(2, !digitalRead(2));  // Toggle square wave for Oscillator 1
    nextToggleMicros1 += (period1 / 2);  // Schedule next toggle
  }

  // --- Square Wave Generation for Oscillator 2 ---
  if (currentMicros >= nextToggleMicros2 && targetVoltages[1] > 0) {
    digitalWrite(3, !digitalRead(3));  // Toggle square wave for Oscillator 2
    nextToggleMicros2 += (period2 / 2);  // Schedule next toggle
  }

  // --- Square Wave Generation for Oscillator 3 ---
  if (currentMicros >= nextToggleMicros3 && targetVoltages[2] > 0) {
    digitalWrite(4, !digitalRead(4));  // Toggle square wave for Oscillator 3
    nextToggleMicros3 += (period3 / 2);  // Schedule next toggle
  }

  // --- Square Wave Generation for Oscillator 4 ---
  if (currentMicros >= nextToggleMicros4 && targetVoltages[3] > 0) {
    digitalWrite(5, !digitalRead(5));  // Toggle square wave for Oscillator 4
    nextToggleMicros4 += (period4 / 2);  // Schedule next toggle
  }
}

// Handle Note On (Set target voltages and calculate frequencies)
void handleNoteOn(int note, int velocity) {
  if (note < 0 || note > 87) return;  // Only handle valid notes

  for (int i = 0; i < 4; i++) {
    if (targetVoltages[i] == 0) {
      targetVoltages[i] = compensatingVoltages[note];  // Set target voltage for this oscillator
      setVoltage(i, targetVoltages[i]);  // Set the DAC voltage immediately
      calculatePeriod(i, note);  // Calculate the square wave period
      return;
    }
  }
}

// Handle Note Off (Switch voltage back to 0)
void handleNoteOff(int note) {
  for (int i = 0; i < 4; i++) {
    if (targetVoltages[i] == compensatingVoltages[note]) {
      targetVoltages[i] = 0;  // Set voltage to 0
      setVoltage(i, 0);  // Immediately set DAC to 0
      return;
    }
  }
}

// Set voltage for the specific oscillator
void setVoltage(int oscIndex, float voltage) {
  if (oscIndex == 0) {
    dac1.setVoltageA(voltage);
    dac1.turnOnChannelA();
  } else if (oscIndex == 1) {
    dac1.setVoltageB(voltage);
    dac1.turnOnChannelB();
  } else if (oscIndex == 2) {
    dac2.setVoltageA(voltage);
    dac2.turnOnChannelA();
  } else if (oscIndex == 3) {
    dac2.setVoltageB(voltage);
    dac2.turnOnChannelB();
  }

  dac1.updateDAC();
  dac2.updateDAC();
}

// Calculate the square wave period based on the note frequency
void calculatePeriod(int oscIndex, int note) {
  long period = 1000000 / noteFrequencies[note];
  
  if (oscIndex == 0) {
    period1 = period;
    nextToggleMicros1 = micros() + (period1 / 2);
  } else if (oscIndex == 1) {
    period2 = period;
    nextToggleMicros2 = micros() + (period2 / 2);
  } else if (oscIndex == 2) {
    period3 = period;
    nextToggleMicros3 = micros() + (period3 / 2);
  } else if (oscIndex == 3) {
    period4 = period;
    nextToggleMicros4 = micros() + (period4 / 2);
  }
}
