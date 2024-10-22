// Code by http://cmosorchestra.com GPL-3.0

#include <SPI.h>
#include <MCP48xx.h>

// Create MCP4822 objects with Chip Select pins on D10 and D9
MCP4822 dac1(10);  // First MCP4822
MCP4822 dac2(9);   // Second MCP4822

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

// Pre-calculated compensating voltages for all notes (in mV)
float compensatingVoltages[] = {
  47.49, 50.40, 53.41, 56.57, 59.93, 63.52, 67.27, 71.27, 75.48, 80.00,  // A1 = 55Hz
  84.70, 89.57, 94.70, 100.15, 105.83, 111.70, 117.93, 124.41, 131.14, 138.18, 
  145.48, 160.00, 169.13, 179.05, 189.89, 201.68, 213.86, 226.56, 239.73, 
  253.57, 268.16, 283.63, 300.10, 320.00, 339.94, 360.82, 382.73, 405.78, 
  430.13, 455.84, 483.04, 511.73, 541.94, 573.81, 607.41, 640.00, 677.18,  // A4 = 440Hz
  715.96, 756.37, 798.40, 842.23, 887.91, 935.55, 985.24, 1037.05, 1091.09, 
  1157.82, 1226.61, 1297.45, 1370.53, 1446.05   // A5 = 880Hz
};

// Chord intervals for each root: root, minor third, perfect fifth, ninth
int chordIntervals[] = { 0, 3, 7, 12 };  // Semi-tone offsets from each root note

// Roots for the two sets of chords (F2, G#2, C3, G3 and F3, G#3, C4, G4)
int chordRoots[][4] = {
  { 17, 20, 24, 31 },  // F2, G#2, C3, G3
  { 29, 32, 36, 43 }   // F3, G#3, C4, G4
};

// Timing variables
unsigned long previousMillis = 0;
const unsigned long updateInterval = 1000;
int chordSetIndex = 0;  // Start with the first set (F2, G#2, C3, G3)
int chordIndex = 0;     // Index within the chord set

unsigned long nextToggleMicros1 = 0;  // For precise square wave generation for Oscillator 1
unsigned long nextToggleMicros2 = 0;  // For precise square wave generation for Oscillator 2
unsigned long nextToggleMicros3 = 0;  // For precise square wave generation for Oscillator 3
unsigned long nextToggleMicros4 = 0;  // For precise square wave generation for Oscillator 4
long period1 = 0;  // Period for Oscillator 1
long period2 = 0;  // Period for Oscillator 2
long period3 = 0;  // Period for Oscillator 3
long period4 = 0;  // Period for Oscillator 4

// Automatic reset counter
int resetCounter = 0;
const int resetAfterCycles = 4;  // Reset after 4 cycles

void setup() {
  SPI.begin();
  dac1.init();
  dac2.init();

  pinMode(2, OUTPUT);  // Output pin for square wave of Oscillator 1
  pinMode(3, OUTPUT);  // Output pin for square wave of Oscillator 2
  pinMode(4, OUTPUT);  // Output pin for square wave of Oscillator 3
  pinMode(5, OUTPUT);  // Output pin for square wave of Oscillator 4
  pinMode(7, INPUT);   // D7 for pause/resume control
}

void loop() {
  // Check if D7 is HIGH (pause mode)
  bool isPaused = digitalRead(7) == HIGH;

  unsigned long currentMicros = micros();
  
  // If not paused, handle note cycling
  if (!isPaused) {
    unsigned long currentMillis = millis();

    // --- DAC Voltage Update ---
    if (currentMillis - previousMillis >= updateInterval) {
      previousMillis = currentMillis;

      // Get the base note of the current root set (F2, G#2, C3, G3 or F3, G#3, C4, G4)
      int rootNote = chordRoots[chordSetIndex][chordIndex];

      // Calculate each note of the chord based on the intervals
      int note1 = rootNote + chordIntervals[0];  // Root
      int note2 = rootNote + chordIntervals[1];  // Minor third
      int note3 = rootNote + chordIntervals[2];  // Perfect fifth
      int note4 = rootNote + chordIntervals[3];  // Ninth

      // Set the DAC voltages for each oscillator
      dac1.setVoltageA(compensatingVoltages[note1]);
      dac1.setVoltageB(compensatingVoltages[note2]);
      dac2.setVoltageA(compensatingVoltages[note3]);
      dac2.setVoltageB(compensatingVoltages[note4]);

      dac1.turnOnChannelA();
      dac1.turnOnChannelB();
      dac2.turnOnChannelA();
      dac2.turnOnChannelB();
      dac1.updateDAC();
      dac2.updateDAC();

      // Calculate the period in microseconds for each frequency
      period1 = 1000000 / noteFrequencies[note1];
      period2 = 1000000 / noteFrequencies[note2];
      period3 = 1000000 / noteFrequencies[note3];
      period4 = 1000000 / noteFrequencies[note4];

      // Set the next toggle times for square wave generation
      nextToggleMicros1 = currentMicros + (period1 / 2);
      nextToggleMicros2 = currentMicros + (period2 / 2);
      nextToggleMicros3 = currentMicros + (period3 / 2);
      nextToggleMicros4 = currentMicros + (period4 / 2);

      // Move to the next root note in the current set (F2 -> G#2 -> C3 -> G3 or F3 -> G#3 -> C4 -> G4)
      chordIndex = (chordIndex + 1) % 4;  // Cycle within the chord set

      // Switch between the two chord sets (F2, G#2, C3, G3) and (F3, G#3, C4, G4)
      if (chordIndex == 0) {
        chordSetIndex = (chordSetIndex + 1) % 2;  // Toggle between the two sets
        resetCounter++;  // Count cycles
      }

      // Automatic reset after a certain number of cycles
      if (resetCounter >= resetAfterCycles) {
        nextToggleMicros1 = currentMicros;
        nextToggleMicros2 = currentMicros;
        nextToggleMicros3 = currentMicros;
        nextToggleMicros4 = currentMicros;
        resetCounter = 0;  // Reset the counter after synchronization
      }
    }
  }

  // --- Square Wave Generation for Oscillator 1 ---
  if (currentMicros >= nextToggleMicros1) {
    digitalWrite(2, !digitalRead(2));  // Toggle the square wave pin for Oscillator 1
    nextToggleMicros1 += (period1 / 2);  // Schedule next toggle for Oscillator 1
  }

  // --- Square Wave Generation for Oscillator 2 ---
  if (currentMicros >= nextToggleMicros2) {
    digitalWrite(3, !digitalRead(3));  // Toggle the square wave pin for Oscillator 2
    nextToggleMicros2 += (period2 / 2);  // Schedule next toggle for Oscillator 2
  }

  // --- Square Wave Generation for Oscillator 3 ---
  if (currentMicros >= nextToggleMicros3) {
    digitalWrite(4, !digitalRead(4));  // Toggle the square wave pin for Oscillator 3
    nextToggleMicros3 += (period3 / 2);  // Schedule next toggle for Oscillator 3
  }

  // --- Square Wave Generation for Oscillator 4 ---
  if (currentMicros >= nextToggleMicros4) {
    digitalWrite(5, !digitalRead(5));  // Toggle the square wave pin for Oscillator 4
    nextToggleMicros4 += (period4 / 2);  // Schedule next toggle for Oscillator 4
  }
}
