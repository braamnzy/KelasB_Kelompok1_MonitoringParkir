#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>

#define PIN_TRIG   7
#define PIN_ECHO   6
#define PIN_LED    9    // PWM
#define PIN_BUZZER 10   // PWM

QueueHandle_t queueJarak;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void taskBacaJarak(void *pvParameters);
void taskKontrolOutput(void *pvParameters);

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("System Ready...");

  pinMode(PIN_TRIG,   OUTPUT);
  pinMode(PIN_ECHO,   INPUT);
  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  queueJarak = xQueueCreate(1, sizeof(float));
  xTaskCreate(taskBacaJarak, "BacaJarak", 128, NULL, 1, NULL);
  xTaskCreate(taskKontrolOutput, "kontrolOutput", 128, NULL, 1, NULL);
}

void taskBacaJarak(void *pvParameters) {
  (void) pvParameters;

  for(;;){
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long durasi = pulseIn(PIN_ECHO, HIGH, 30000);
    float jarakHasil;

    if (durasi == 0){
      jarakHasil = -1;
    } else {
      jarakHasil = durasi * 0.0343 / 2.0;
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

  unsigned long waktuMasuk = 0;
  bool statusParkir = false;

  for(;;){
    if (xQueueReceive(queueJarak, &jarak, portMAX_DELAY) == pdPASS){
      unsigned long sekarang = millis();
      int ledVal = 0, freq = 0;
      unsigned long interval = 0;
      String zona = "";
      bool buzzerContinue = false;

      if (jarak < 0 || jarak > 50) { 
        zona = (jarak < 0) ? "Bebas" : "Aman";
        statusParkir = false;
      }
      else { 
        if (!statusParkir) {
          waktuMasuk = sekarang;
          statusParkir = true;
        }

        if      (jarak <= 5)  { zona = "< 5cm";  ledVal = 255; freq = 3000; buzzerContinue = true;} 
        else if (jarak <= 15) { zona = "KRITIS"; ledVal = 255; freq = 2500; interval = 80;   }
        else if (jarak <= 30) { zona = "Dekat";  ledVal = 180; freq = 2000; interval = 200;  }
        else if (jarak <= 50) { zona = "Hati2";  ledVal = 100; freq = 1500; interval = 500;  }
      }

      unsigned long menit = 0;
      unsigned long detik = 0;
      
      if (statusParkir) {
        unsigned long totalDetik = (sekarang - waktuMasuk) / 1000;
        menit = totalDetik / 60;
        detik = totalDetik % 60;
        tarif = ((menit / 30) + 1) * 2000;
      }

      analogWrite(PIN_LED, ledVal); 

      if (buzzerContinue) {
        tone(PIN_BUZZER, freq);
      } else if (interval == 0) {
        noTone(PIN_BUZZER);
        buzzerState = false;
      } else {
        if (sekarang - previousMillis >= interval) {
          previousMillis = sekarang;
          buzzerState = !buzzerState;
          if (buzzerState) {tone(PIN_BUZZER, freq);}
          else {noTone(PIN_BUZZER);}
        }
      }

      lcd.clear();

      if (statusParkir) {
        lcd.setCursor(0, 0);
        lcd.print("Durasi: ");
        if(menit < 10) lcd.print("0");
        lcd.print(menit);
        lcd.print("m ");
        if(detik < 10) lcd.print("0");
        lcd.print(detik);
        lcd.print("s");

        lcd.setCursor(0, 1);
        lcd.print("Tarif : Rp ");
        lcd.print(tarif);
      } 
      else {
        lcd.setCursor(0, 0);
        lcd.print("Slot: Kosong");
        lcd.setCursor(0, 1);
        lcd.print("Tarif: Rp 0");
      }

      Serial.print("Jarak: ");
      Serial.print(jarak);
      if (statusParkir) {
        Serial.print(" | Waktu: "); Serial.print(menit); Serial.print("m "); Serial.print(detik); Serial.print("s");
        Serial.print(" | Tarif: Rp "); Serial.println(tarif);
      } else {
        Serial.println(" | Slot Kosong");
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void loop() {}