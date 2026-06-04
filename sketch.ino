// --- PIN ---
#define PIN_TRIG    7
#define PIN_ECHO    6
#define PIN_LED     9    // PWM
#define PIN_BUZZER  10   // PWM

// Untuk beep non-blocking tanpa delay()
unsigned long previousMillis = 0;
bool buzzerState = false;

// ======================
void setup() {
  // Inisialisasi Serial Monitor sebagai pengganti LCD
  Serial.begin(9600);

  pinMode(PIN_TRIG,   OUTPUT);
  pinMode(PIN_ECHO,   INPUT);
  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.println("Parking Sensor Siap...");
  delay(1000);
}

// Kirim pulsa TRIG, ukur durasi ECHO, konversi ke cm
float bacaJarak() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long durasi = pulseIn(PIN_ECHO, HIGH, 30000); // timeout 30ms
  if (durasi == 0) return -1; // tidak ada objek

  return durasi * 0.0343 / 2.0; // rumus jarak (cm)
}

// Atur LED dan buzzer sesuai jarak, lalu print ke Serial Monitor
void kontrolOutput(float jarak) {
  unsigned long sekarang = millis();

  int ledVal = 0, freq = 0;
  unsigned long interval = 0;
  String zona = "";

  // Tentukan zona
  if      (jarak < 0)   { zona = "Bebas";    }
  else if (jarak <= 5)  { zona = "< 5cm";    ledVal = 255;} // sensor kurang akurat di jarak ini
  else if (jarak <= 15) { zona = "KRITIS";   ledVal = 255; freq = 2500; interval = 80;   }
  else if (jarak <= 30) { zona = "Dekat";    ledVal = 180; freq = 2000; interval = 200;  }
  else if (jarak <= 50) { zona = "Hati2";    ledVal = 100; freq = 1500; interval = 500;  }
  else                  { zona = "Aman";     }

  // Nyalakan LED sesuai kecerahan zona
  analogWrite(PIN_LED, ledVal); 

  // Buzzer: mati jika zona bebas
  if (interval == 0) {
    noTone(PIN_BUZZER);
    buzzerState = false;
  } else {
    // Beep on/off tanpa delay (non-blocking)
    if (sekarang - previousMillis >= interval) {
      previousMillis = sekarang;
      buzzerState = !buzzerState;
      if (buzzerState) {
        tone(PIN_BUZZER, freq);
      } else {
        noTone(PIN_BUZZER);
      }
    }
  }

  // Tampilkan data ke Serial Monitor (Pengganti LCD)
  Serial.print("Jarak: ");
  if (jarak < 0) {
    Serial.print("Tidak ada objek");
  } else {
    Serial.print(jarak, 1);
    Serial.print(" cm");
  }
  Serial.print(" | Zona: ");
  Serial.println(zona);
}

// ======================
void loop() {
  float jarak = bacaJarak();  // baca sensor
  kontrolOutput(jarak);       // proses output ke LED, Buzzer, dan Komputer
  delay(100);                 // jeda antar baca
}