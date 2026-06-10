#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>

// Inisialisasi LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Komponen
const int trigPin = 2;
const int echoPin = 3;
const int ledHijau = 4;
const int ledKuning = 5;
const int ledMerah = 6;
const int buzzer = 7;

// Batas Jarak (dalam cm)
const int batasKosong = 50;
const int jarakJauh = 30;
const int jarakAman = 10;
const long tarifPerDetik = 50;

// Shared Variables menggunakan 'volatile' agar sinkron antar task tanpa Semaphore
volatile long globalJarak = 100;
String lcdBaris1 = "  SLOT KOSONG  ";
String lcdBaris2 = " Menunggu...   ";

// Handle untuk Task
void TaskBacaSensor(void *pvParameters);
void TaskLogikaParkir(void *pvParameters);
void TaskTampilanLCD(void *pvParameters);

void setup() {
  // Inisialisasi Pin
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledHijau, OUTPUT);
  pinMode(ledKuning, OUTPUT);
  pinMode(ledMerah, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(Slot Parkir);
  lcd.setCursor(0, 1);
  lcd.print("   RTOS Solo   ");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  lcd.clear();

  // Pembuatan Task RTOS
  xTaskCreate(TaskBacaSensor, "BacaSensor", 128, NULL, 3, NULL);
  xTaskCreate(TaskLogikaParkir, "LogikaParkir", 256, NULL, 2, NULL);
  xTaskCreate(TaskTampilanLCD, "TampilanLCD", 192, NULL, 1, NULL);
}

void loop() {
  // Kosong
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBacaSensor(void *pvParameters) {
  (void) pvParameters;
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Timeout 30ms agar tidak mengunci Task lain jika sensor error
    long durasi = pulseIn(echoPin, HIGH, 30000); 
    long jarakCm = durasi * 0.034 / 2;
    
    if (jarakCm > 0) {
      globalJarak = jarakCm; // Ditulis langsung ke variabel volatile
    } else {
      globalJarak = 999; 
    }

    vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
  }
}

void TaskLogikaParkir(void *pvParameters) {
  (void) pvParameters;

  bool mobilParkir = false;
  unsigned long waktuMulaiStop = 0;
  unsigned long waktuMulaiParkir = 0;
  unsigned long durasiParkirDetik = 0;

  for (;;) {
    // Membaca nilai globalJarak yang diupdate oleh TaskBacaSensor
    long jarak = globalJarak; 

    if (!mobilParkir) {
      if (jarak > batasKosong) {
        lcdBaris1 = "  SLOT KOSONG  ";
        lcdBaris2 = " Menunggu...   ";
        setKondisiKomponen(HIGH, LOW, LOW, false, 0);
        waktuMulaiStop = 0;
      }
      else if (jarak > jarakJauh && jarak <= batasKosong) {
        lcdBaris1 = "MUNDUR...";
        lcdBaris2 = "Jarak: " + String(jarak) + "cm";
        setKondisiKomponen(HIGH, LOW, LOW, false, 0);
        waktuMulaiStop = 0;
      } 
      else if (jarak > jarakAman && jarak <= jarakJauh) {
        lcdBaris1 = "PELAN-PELAN";
        lcdBaris2 = "Jarak: " + String(jarak) + "cm";
        setKondisiKomponen(LOW, HIGH, LOW, true, 500);
        waktuMulaiStop = 0;
      } 
      else if (jarak <= jarakAman) {
        lcdBaris1 = "!! STOP !!";
        lcdBaris2 = "Jarak: " + String(jarak) + "cm";
        setKondisiKomponen(LOW, LOW, HIGH, true, 100);

        if (waktuMulaiStop == 0) {
          waktuMulaiStop = millis();
        } else if (millis() - waktuMulaiStop >= 3000) {
          mobilParkir = true;
          waktuMulaiParkir = millis();
          digitalWrite(buzzer, LOW);
        }
      }
    }
    else {
      setKondisiKomponen(LOW, LOW, HIGH, false, 0);

      durasiParkirDetik = (millis() - waktuMulaiParkir) / 1000;
      long totalTarif = durasiParkirDetik * tarifPerDetik;

      lcdBaris1 = "TERISI | T:" + formatWaktu(durasiParkirDetik);
      lcdBaris2 = "Biaya: Rp " + String(totalTarif);

      if (jarak > (jarakAman + 10)) { 
        lcdBaris1 = "Total Waktu:" + formatWaktu(durasiParkirDetik);
        lcdBaris2 = "Total: Rp" + String(totalTarif);
        
        digitalWrite(buzzer, HIGH);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        digitalWrite(buzzer, LOW);
        vTaskDelay(4000 / portTICK_PERIOD_MS); 

        mobilParkir = false;
        waktuMulaiStop = 0;
      }
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS); 
  }
}

void TaskTampilanLCD(void *pvParameters) {
  (void) pvParameters;
  String lastBaris1 = "", lastBaris2 = "";

  for (;;) {
    // Langsung membaca String global yang diperbarui TaskLogikaParkir
    if (lcdBaris1 != lastBaris1) {
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print(lcdBaris1);
      lastBaris1 = lcdBaris1;
    }
    if (lcdBaris2 != lastBaris2) {
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(lcdBaris2);
      lastBaris2 = lcdBaris2;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS); 
  }
}

/*--------------------------------------------------*/
/*---------------- Helper Functions ----------------*/
/*--------------------------------------------------*/

void setKondisiKomponen(int h, int k, int m, bool bz, int jeda) {
  digitalWrite(ledHijau, h);
  digitalWrite(ledKuning, k);
  digitalWrite(ledMerah, m);
  
  if (bz) {
    if ((millis() / jeda) % 2 == 0) {
      digitalWrite(buzzer, HIGH);
    } else {
      digitalWrite(buzzer, LOW);
    }
  } else {
    digitalWrite(buzzer, LOW);
  }
}

String formatWaktu(long totalDetik) {
  long menit = totalDetik / 60;
  long detik = totalDetik % 60;
  String strMenit = (menit < 10) ? "0" + String(menit) : String(menit);
  String strDetik = (detik < 10) ? "0" + String(detik) : String(detik);
  return strMenit + ":" + strDetik;
}