#include <OneWire.h>
#include <DallasTemperature.h>

// === Pin Sensor ===
#define pinDS18B20 4     // Data DS18B20 di IO4
#define pinTDS     36    // Analog input untuk sensor TDS
#define pinTRIG    12    // Trigger Ultrasonic
#define pinECHO    14    // Echo Ultrasonic

// === DS18B20 Setup ===
OneWire oneWire(pinDS18B20);
DallasTemperature ds18b20(&oneWire);

// === Variabel Global ===
float temperatureC;
float tdsValue;
float distanceCM;

// === Parameter Kalibrasi TDS ===
#define VREF 3.3          // Tegangan referensi ADC ESP32
#define SCOUNT 30         // Jumlah sample untuk filter

int analogBuffer[SCOUNT];
int analogBufferIndex = 0;

// === Fungsi Baca Ultrasonic ===
float readDistanceCM() {
  digitalWrite(pinTRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTRIG, LOW);

  long duration = pulseIn(pinECHO, HIGH, 40000); // timeout 40ms
  float distance = duration * 0.034 / 2; // ke cm
  return distance;
}

// === Fungsi Baca TDS ===
float readTDS(float temperature) {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(pinTDS);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }

  // hitung rata-rata
  long avgValue = 0;
  for (int i = 0; i < SCOUNT; i++)
    avgValue += analogBuffer[i];
  float averageVoltage = (avgValue / (float)SCOUNT) * (VREF / 4095.0);

  // kompensasi suhu (koefisien 2% per °C)
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensatedVoltage = averageVoltage / compensationCoefficient;

  // konversi ke ppm (rumus empiris)
  float tdsValue = (133.42 * compensatedVoltage * compensatedVoltage * compensatedVoltage
                   - 255.86 * compensatedVoltage * compensatedVoltage
                   + 857.39 * compensatedVoltage) * 0.5;

  return tdsValue;
}

void setup() {
  Serial.begin(115200);
  ds18b20.begin();
  pinMode(pinTRIG, OUTPUT);
  pinMode(pinECHO, INPUT);
  pinMode(pinTDS, INPUT);
}

void loop() {
  // Baca suhu
  ds18b20.requestTemperatures();
  temperatureC = ds18b20.getTempCByIndex(0);

  // Baca jarak
  distanceCM = readDistanceCM();

  // Baca TDS
  tdsValue = readTDS(temperatureC);

  // Tampilkan hasil
  Serial.println("=== Pembacaan Sensor ===");
  Serial.print("Suhu (°C): "); Serial.println(temperatureC);
  Serial.print("TDS (ppm): "); Serial.println(tdsValue);
  Serial.print("Jarak (cm): "); Serial.println(distanceCM);
  Serial.println("=========================");
  Serial.println();

  delay(1000);
}
