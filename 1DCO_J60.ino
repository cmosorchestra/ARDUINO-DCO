#include <SPI.h>
#include <MCP48xx.h>

// Create MCP4822 object with Chip Select pin on D10
MCP4822 dac(10);

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

// Timing variables
unsigned long previousMillis = 0;
const unsigned long updateInterval = 1000;
int noteIndex = 0;  // Start with C1

unsigned long nextToggleMicros = 0;  // For precise square wave generation
long period = 0;  // Period for the square wave generation

void setup() {
  SPI.begin();
  dac.init();

  pinMode(3, OUTPUT);  // Output pin for square wave
  pinMode(5, INPUT);   // D5 for pause/resume control
}

void loop() {
  // Check if D5 is HIGH (pause mode)
  bool isPaused = digitalRead(5) == HIGH;

  unsigned long currentMicros = micros();
  
  // If not paused, handle note cycling
  if (!isPaused) {
    unsigned long currentMillis = millis();

    // --- DAC Voltage Update ---
    if (currentMillis - previousMillis >= updateInterval) {
      previousMillis = currentMillis;

      // Set the DAC voltage for the current note
      int scaledVoltage = compensatingVoltages[noteIndex];
      dac.setVoltageA(scaledVoltage);
      dac.turnOnChannelA();
      dac.updateDAC();

      // Move to the next note
      noteIndex = (noteIndex + 1) % 61;

      // Calculate the period in microseconds for the next note's frequency
      period = 1000000 / noteFrequencies[noteIndex];
      nextToggleMicros = currentMicros + (period / 2);  // Set next toggle time
    }
  }

  // --- Square Wave Generation (no reset of timing) ---
  // Keep square wave generation precise based on calculated period
  if (currentMicros >= nextToggleMicros) {
    digitalWrite(3, !digitalRead(3));  // Toggle the square wave pin
    nextToggleMicros += (period / 2);  // Schedule next toggle
  }
}
