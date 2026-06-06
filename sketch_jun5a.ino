#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>

#define PIN_TRIG   7
#define PIN_ECHO   6
#define PIN_LED    9    // PWM
#define PIN_BUZZER 10   // PWM

QueueHandle_t queueJarak;

void taskBacaJarak(void *pvParameters);
void taskKontrolOutput(void *pvParameters);

void setup() {
  Serial.begin(9600);
  Serial.println("=========================================");
  Serial.println("--- SISTEM PARKIR PINTAR (ANTI-NOISE) ---");
  Serial.println("=========================================");

  pinMode(PIN_TRIG,   OUTPUT);
  pinMode(PIN_ECHO,   INPUT);
  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  queueJarak = xQueueCreate(1, sizeof(float));
  
  xTaskCreate(taskBacaJarak, "BacaJarak", 200, NULL, 1, NULL);
  xTaskCreate(taskKontrolOutput, "kontrolOutput", 250, NULL, 1, NULL);
}

void taskBacaJarak(void *pvParameters) {
  (void) pvParameters;
  static float jarakTerakhir = 999.0;

  for(;;){
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long durasi = pulseIn(PIN_ECHO, HIGH, 30000);
    float jarakHasil;

    if (durasi == 0){
      if (jarakTerakhir <= 10.0) {
        jarakHasil = 1.0; 
      } else {
        jarakHasil = -1; 
      }
    } else {
      jarakHasil = durasi * 0.0343 / 2.0;
      jarakTerakhir = jarakHasil;
    }

    xQueueSend(queueJarak, &jarakHasil, portMAX_DELAY);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void taskKontrolOutput(void *pvParameters) {
  (void) pvParameters;

  float jarak = 0.0;
  unsigned long previousMillis = 0;
  bool buzzerState = false;

  unsigned long waktuMulaiParkir = 0;
  unsigned long waktuTerakumulasi = 0;
  unsigned long waktuMulaiKosong = 0;
  unsigned long waktuKonfirmasiMasuk = 0;
  
  bool statusParkir = false;         
  bool sedangKosong = false;         
  bool sedangKonfirmasi = false;     

  for(;;){
    if (xQueueReceive(queueJarak, &jarak, portMAX_DELAY) == pdPASS){
      unsigned long sekarang = millis();
      int ledVal = 0, freq = 0;
      unsigned long interval = 0;
      const char* statusSistem = "";

      // --- LOGIKA HISTERESIS (ANTI ERROR / FLUKTUASI) ---
      // Jika sudah berstatus PARKIR, batas jarak aman dinaikkan ke 8.0 cm sebagai toleransi
      float batasMaksParkir = statusParkir ? 8.0 : 5.0; 

      // 1. PENGATURAN HARDWARE BERDASARKAN BATAS TOLERANSI
      if (jarak < 0 || jarak > 50) { 
        ledVal = 0;       
        freq = 0;         
        interval = 0;
      }
      else if (jarak <= batasMaksParkir) { // Menggunakan batas toleransi baru
        ledVal = 255;     // LED Merah diam
        freq = 0;         // Buzzer mati
        interval = 0;
      }
      else if (jarak <= 15) { ledVal = 255; freq = 2500; interval = 80;   } 
      else if (jarak <= 30) { ledVal = 180; freq = 2000; interval = 200;  } 
      else if (jarak <= 50) { ledVal = 100; freq = 1500; interval = 500;  } 

      // 2. LOGIKA STATE MACHINE STOPWATCH PARKIR
      if (!statusParkir) {
        // --- KONDISI A: SYSTEM READY ---
        statusSistem = "READY / KOSONG";
        
        if (jarak > 0 && jarak <= 5.0) { // Tetap harus <= 5cm untuk memicu parkir awal
          if (!sedangKonfirmasi) {
            waktuKonfirmasiMasuk = sekarang;
            sedangKonfirmasi = true;
          } else if (sekarang - waktuKonfirmasiMasuk >= 2000) { 
            statusParkir = true;
            sedangKonfirmasi = false;
            waktuMulaiParkir = sekarang;
            waktuTerakumulasi = 0;
          }
          statusSistem = "Konfirmasi Mobil...";
        } else {
          sedangKonfirmasi = false;
        }
      } 
      else {
        // --- KONDISI B: MOBIL SUDAH PARKIR ---
        // Di sini toleransi diuji. Selama jarak di bawah 8cm, stopwatch aman!
        if (jarak > 0 && jarak <= batasMaksParkir) {
          waktuTerakumulasi += (sekarang - waktuMulaiParkir);
          waktuMulaiParkir = sekarang;
          sedangKosong = false; 
          statusSistem = "PARKED (TIMING RUNNING)";
        } 
        else {
          // Mobil benar-benar keluar melampaui 8cm -> PAUSE
          waktuMulaiParkir = sekarang; 
          statusSistem = "MOVING OUT (TIMING PAUSED)";

          // Hitung mundur 3 detik untuk reset jika benar-benar kosong (>50cm)
          if (jarak < 0 || jarak > 50) {
            if (!sedangKosong) {
              waktuMulaiKosong = sekarang;
              sedangKosong = true;
            } else if (sekarang - waktuMulaiKosong >= 3000) {
              statusParkir = false;
              sedangKosong = false;
              waktuTerakumulasi = 0;
            }
          } else {
            sedangKosong = false; 
          }
        }
      }

      // 3. EKSEKUSI LED & BUZZER
      analogWrite(PIN_LED, ledVal); 

      if (freq > 0) {
        if (interval == 0) {
          tone(PIN_BUZZER, freq);
        } else {
          if (sekarang - previousMillis >= interval) {
            previousMillis = sekarang;
            buzzerState = !buzzerState;
            if (buzzerState) { tone(PIN_BUZZER, freq); }
            else { noTone(PIN_BUZZER); }
          }
        }
      } else {
        noTone(PIN_BUZZER);
        buzzerState = false;
      }

      // 4. KONVERSI WAKTU
      unsigned long totalDetik = waktuTerakumulasi / 1000;
      unsigned long menit = totalDetik / 60;
      unsigned long detik = totalDetik % 60;

      // 5. TAMPILAN SERIAL MONITOR
      Serial.print("Jarak: ");
      if (jarak < 0) Serial.print("No Object");
      else { Serial.print(jarak, 1); Serial.print(" cm"); }

      Serial.print(" | Status: ");
      Serial.print(statusSistem);

      if (statusParkir || sedangKonfirmasi) {
        Serial.print(" | Waktu: ");
        if(menit < 10) Serial.print("0");
        Serial.print(menit);
        Serial.print("m ");
        if(detik < 10) Serial.print("0");
        Serial.print(detik);
        Serial.println("s");
      } else {
        Serial.println("");
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void loop() {}