#include <EEPROM.h>
#include <SoftwareSerial.h>
SoftwareSerial bluetooth(2, 3); // RX, TX

const int flexPins[5] = {A0, A1, A2, A3, A4};
const int accelPins[3] = {A5, A6, A7};

const int numSamples = 50;
const int maxGestures = 26; // A–Z

struct Gesture {
  char letter;
  int flexMin[5];
  int flexMax[5];
  int accelMin[3];
  int accelMax[3];
};

Gesture gestures[maxGestures];
int gestureCount = 0;

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
  bluetooth.println("🔷 HC-05 ready to send letters!");
  Serial.println("=== Sign Language Glove ===");
  loadFromEEPROM();
  Serial.print("🧠 Gestures loaded: ");
  Serial.println(gestureCount);
  Serial.println("Type a letter (A–Z) to calibrate, 'run' to detect, or 'clear' to reset EEPROM:");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("run")) {
      Serial.println("🔍 Detecting gestures...");
      detectGestures();
    } else if (input.equalsIgnoreCase("clear")) {
      clearEEPROM();
    } else if (input.length() == 1 && isAlpha(input.charAt(0))) {
      char ch = toupper(input.charAt(0));
      Serial.print("Calibrating for: ");
      Serial.println(ch);
      calibrateGesture(ch);
    } else {
      Serial.println("❌ Invalid input. Type letter A-Z, 'run' or 'clear'");
    }
  }
}

void calibrateGesture(char letter) {
  Gesture g;
  g.letter = letter;

  for (int i = 0; i < 5; i++) {
    int val = analogRead(flexPins[i]);
    g.flexMin[i] = g.flexMax[i] = val;
  }
  for (int i = 0; i < 3; i++) {
    int val = analogRead(accelPins[i]);
    g.accelMin[i] = g.accelMax[i] = val;
  }

  Serial.println("⏳ Hold the gesture... Calibrating...");
  for (int s = 0; s < numSamples; s++) {
    for (int i = 0; i < 5; i++) {
      int val = analogRead(flexPins[i]);
      g.flexMin[i] = min(g.flexMin[i], val);
      g.flexMax[i] = max(g.flexMax[i], val);
    }
    for (int i = 0; i < 3; i++) {
      int val = analogRead(accelPins[i]);
      g.accelMin[i] = min(g.accelMin[i], val);
      g.accelMax[i] = max(g.accelMax[i], val);
    }
    delay(50);
  }

  int index = gestureCount < maxGestures ? gestureCount : letter - 'A';
  gestures[index] = g;
  saveToEEPROM(index);
  if (gestureCount < maxGestures) gestureCount++;

  Serial.println("✅ Gesture saved!");
  Serial.println("Type another letter or 'run'");
}

void detectGestures() {
  while (true) {
    if (Serial.available()) {
      String stop = Serial.readStringUntil('\n');
      if (stop.equalsIgnoreCase("exit")) {
        Serial.println("🔚 Exiting detection mode.");
        return;
      }
    }

    int flex[5], accel[3];

    for (int i = 0; i < 5; i++) flex[i] = analogRead(flexPins[i]);
    for (int i = 0; i < 3; i++) accel[i] = analogRead(accelPins[i]);

    for (int g = 0; g < gestureCount; g++) {
      bool match = true;
      for (int i = 0; i < 5; i++) {
        if (flex[i] < gestures[g].flexMin[i] || flex[i] > gestures[g].flexMax[i]) {
          match = false;
          break;
        }
      }
      for (int i = 0; i < 3 && match; i++) {
        if (accel[i] < gestures[g].accelMin[i] || accel[i] > gestures[g].accelMax[i]) {
          match = false;
          break;
        }
      }

      if (match) {
        Serial.print("✅ Detected: ");
        Serial.println(gestures[g].letter);
        bluetooth.print(gestures[g].letter); 
        delay(1000);
        break;
      }
    }

    delay(200);
  }
}

void saveToEEPROM(int index) {
  int base = index * sizeof(Gesture);
  byte *dataPtr = (byte *)&gestures[index];
  for (unsigned int i = 0; i < sizeof(Gesture); i++) {
    EEPROM.update(base + i, dataPtr[i]);
  }
}

void loadFromEEPROM() {
  gestureCount = 0;
  for (int i = 0; i < maxGestures; i++) {
    Gesture g;
    int base = i * sizeof(Gesture);
    byte *dataPtr = (byte *)&g;
    for (unsigned int j = 0; j < sizeof(Gesture); j++) {
      dataPtr[j] = EEPROM.read(base + j);
    }
    if (isAlpha(g.letter)) {
      gestures[gestureCount++] = g;
    }
  }
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
  gestureCount = 0;
  Serial.println("🚮 EEPROM Cleared. You can recalibrate now.");
}

bool isAlpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}


