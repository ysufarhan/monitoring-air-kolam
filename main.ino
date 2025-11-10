// === Monitoring Kolam ESP32 dengan Arduino IoT Cloud ===
// Sensor: TDS, DS18B20, JSN-SR04M
// Cloud Variables: suhu (°C), ketinggian (cm), oksigen (ppm)
// Dibuat oleh ChatGPT (GPT-5) untuk Farhan – 2025

#include <OneWire.h>
#include <DallasTemperature.h>
#include "thingProperties.h"   // File dari Arduino IoT Cloud

// === Konfigurasi Pin ===
#define TDS_PIN 36           // ADC pin ESP32
#define ONE_WIRE_BUS 4       // DS18B20 data pin
#define TRIG_PIN 12          // JSN-SR04M trigger
#define ECHO_PIN 14          // JSN-SR04M echo

// === Inisialisasi Objek ===
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// === Variabel Konstanta ===
const float VREF = 3.3;          // Tegangan referensi ADC ESP32
const int SCOUNT = 30;           // Jumlah sample rata-rata
const float tankHeight = 100.0;  // Tinggi total kolam (cm)

// === Buffer untuk perataan nilai TDS ===
int analogBuffer[SCOUNT];
int analogBufferIndex = 0;

// === Fungsi Ultrasonic ===
float readDistanceCM() {
  long duration;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  float distance = duration * 0.0343 / 2; // cm
  return distance;
}

// === Fungsi TDS (dengan kompensasi suhu) ===
float getTDS(float temperature) {
  int avgADC = 0;
  for (int i = 0; i < SCOUNT; i++) {
    analogBuffer[i] = analogRead(TDS_PIN);
    delay(10);
  }
  for (int i = 0; i < SCOUNT; i++) {
    avgADC += analogBuffer[i];
  }
  avgADC /= SCOUNT;

  float voltage = avgADC * (VREF / 4095.0);
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
  float compensationVoltage = voltage / compensationCoefficient;
  float tdsValue = (133.42 * pow(compensationVoltage, 3)
                   - 255.86 * pow(compensationVoltage, 2)
                   + 857.39 * compensationVoltage) * 0.5;
  return tdsValue;
}

// === SETUP ===
void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  sensors.begin();

  // Inisialisasi koneksi IoT Cloud
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  Serial.println("\n=== Sistem Monitoring Kolam Terhubung ke Arduino IoT Cloud ===");
  Serial.println("Sensor: TDS, DS18B20, JSN-SR04M");
  Serial.println("===============================================================");
}

// === LOOP ===
void loop() {
  ArduinoCloud.update();  // Kirim/update data ke Cloud

  // 1️⃣ Baca suhu
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // 2️⃣ Baca jarak & hitung ketinggian air
  float distance = readDistanceCM();
  float waterLevel = tankHeight - distance;
  if (waterLevel < 0) waterLevel = 0;
  if (waterLevel > tankHeight) waterLevel = tankHeight;

  // 3️⃣ Baca TDS
  float tdsValue = getTDS(temperature);

  // 4️⃣ Kirim ke variabel Cloud
  suhu = temperature;
  ketinggian = (int)waterLevel;
  oksigen = tdsValue;

  // 5️⃣ Tampilkan ke Serial Monitor
  Serial.println("\n=== Data Kolam ===");
  Serial.printf("Suhu Air       : %.2f °C\n", suhu);
  Serial.printf("TDS (Oksigen)  : %.2f ppm\n", oksigen);
  Serial.printf("Jarak Permukaan: %.2f cm\n", distance);
  Serial.printf("Ketinggian Air : %d cm\n", ketinggian);

  delay(1000); // jeda 5 detik antar update
}

// === Fungsi Callback (tidak digunakan tapi wajib ada) ===
void onSuhuChange() {}
void onKetinggianChange() {}
void onOksigenChange() {}
