#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>

// Inisialisasi LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Komponen
const int trigPin = 7;
const int echoPin = 6;
const int ledMerah = 9; 
const int buzzer = 10;

// Batas Jarak (dalam cm)
const int batasKosong = 50;
const int jarakJauh = 30;
const int jarakAman = 10;
const long tarifPerDetik = 50;

// Shared Variables menggunakan 'volatile' agar sinkron antar task tanpa Semaphore
volatile long globalJarak = 100;
String lcdBaris1 = "   SLOT KOSONG  ";
String lcdBaris2 = " Menunggu...    ";

// Handle untuk Task
void TaskBacaSensor(void *pvParameters);
void TaskLogikaParkir(void *pvParameters);
void TaskTampilanLCD(void *pvParameters);

void setup() {
  // Inisialisasi Pin
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledMerah, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Slot Parkir ");
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
    
    long durasi = pulseIn(echoPin, HIGH, 30000); 
    long jarakCm = durasi * 0.034 / 2;
    
    if (jarakCm > 0) {
      globalJarak = jarakCm; 
    } else {
      globalJarak = 999; 
    }

    vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
  }
}

void TaskLogikaParkir(void *pvParameters) {
  (void) pvParameters;

  bool mobilParkir = false;
  bool modeTampilTotal = false;
  TickType_t waktuMulaiStop = 0;
  TickType_t waktuMulaiParkir = 0;
  TickType_t waktuMulaiKeluar = 0;
  unsigned long durasiParkirDetik = 0;
  long totalTarifAkhir = 0;
  unsigned long durasiAkhir = 0;

  for (;;) {
    long jarak = globalJarak; 
    bool skipBaseDelay = false;

    if (modeTampilTotal) {
      // Menampilkan total biaya selama 5 detik secara NON-BLOCKING
      lcdBaris1 = "Total Waktu:" + formatWaktu(durasiAkhir);
      lcdBaris2 = "Total: Rp" + String(totalTarifAkhir);
      
      // Mengatur bunyi buzzer 1 detik pertama saat keluar
      if ((xTaskGetTickCount() - waktuMulaiKeluar) < (1000 / portTICK_PERIOD_MS)) {
        digitalWrite(buzzer, HIGH);
      } else {
        digitalWrite(buzzer, LOW);
      }

      // Setelah 5 detik selesai, kembalikan ke standby
      if ((xTaskGetTickCount() - waktuMulaiKeluar) >= (5000 / portTICK_PERIOD_MS)) {
        modeTampilTotal = false;
        mobilParkir = false;
        waktuMulaiStop = 0;
      }
    }
    else if (!mobilParkir) {
      if (jarak > batasKosong) {
        lcdBaris1 = "   SLOT KOSONG  ";
        lcdBaris2 = " Menunggu...    ";
        digitalWrite(ledMerah, LOW);
        digitalWrite(buzzer, LOW);
        waktuMulaiStop = 0;
      }
      else if (jarak > jarakJauh && jarak <= batasKosong) {
        lcdBaris1 = "MUNDUR...";
        lcdBaris2 = "Jarak: " + String(jarak) + "cm";
        digitalWrite(ledMerah, LOW);
        digitalWrite(buzzer, LOW);
        waktuMulaiStop = 0;
      } 
      else if (jarak > jarakAman && jarak <= jarakJauh) {
        lcdBaris1 = "PELAN-PELAN";
        lcdBaris2 = "Jarak: " + String(jarak) + "cm";
        digitalWrite(ledMerah, LOW);
        
        digitalWrite(buzzer, HIGH);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        digitalWrite(buzzer, LOW);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        
        waktuMulaiStop = 0;
        skipBaseDelay = true; 
      } 
      else if (jarak <= jarakAman) {
        lcdBaris1 = "!! STOP !!";
        lcdBaris2 = "Jarak: " + String(jarak) + "cm";
        digitalWrite(ledMerah, HIGH);

        digitalWrite(buzzer, HIGH);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        digitalWrite(buzzer, LOW);
        vTaskDelay(50 / portTICK_PERIOD_MS);

        if (waktuMulaiStop == 0) {
          waktuMulaiStop = xTaskGetTickCount();
        } else if ((xTaskGetTickCount() - waktuMulaiStop) >= (3000 / portTICK_PERIOD_MS)) {
          mobilParkir = true;
          waktuMulaiParkir = xTaskGetTickCount();
          digitalWrite(buzzer, LOW);
        }
        skipBaseDelay = true;
      }
    }
    else {
      digitalWrite(ledMerah, HIGH);
      digitalWrite(buzzer, LOW);

      durasiParkirDetik = (xTaskGetTickCount() - waktuMulaiParkir) * portTICK_PERIOD_MS / 1000;
      long totalTarif = durasiParkirDetik * tarifPerDetik;

      lcdBaris1 = "TERISI | T:" + formatWaktu(durasiParkirDetik);
      lcdBaris2 = "Biaya: Rp " + String(totalTarif);

      if (jarak > (jarakAman + 10)) { 
        // Kunci data akhir ke variabel sementara
        durasiAkhir = durasiParkirDetik;
        totalTarifAkhir = totalTarif;
        
        // Aktifkan mode transisi non-blocking
        waktuMulaiKeluar = xTaskGetTickCount();
        modeTampilTotal = true; 
      }
    }
    
    if (!skipBaseDelay) {
      vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
  }
}

void TaskTampilanLCD(void *pvParameters) {
  (void) pvParameters;
  String lastBaris1 = "", lastBaris2 = "";

  for (;;) {
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

String formatWaktu(long totalDetik) {
  long menit = totalDetik / 60;
  long detik = totalDetik % 60;
  String strMenit = (menit < 10) ? "0" + String(menit) : String(menit);
  String strDetik = (detik < 10) ? "0" + String(detik) : String(detik);
  return strMenit + ":" + strDetik;
}