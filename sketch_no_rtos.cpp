#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Inisialisasi LCD I2C (Alamat default biasanya 0x27 atau 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Komponen
const int trigPin = 7;
const int echoPin = 6;
const int ledHijau = 4;
const int ledKuning = 5;
const int ledMerah = 9;
const int buzzer = 10;

// Batas Jarak (dalam cm)
const int batasKosong = 50;  // Di atas ini: "KOSONG"
const int jarakJauh = 30;    // Antara 30 - 50: "Mundur"
const int jarakSedang = 15;  // Antara 15 - 30: "Pelan"
const int jarakAman = 10;    // Di bawah 15: "STOP"

// Variabel Logika & Waktu
bool mobilParkir = false;
unsigned long waktuMulaiStop = 0;
unsigned long waktuMulaiParkir = 0;
unsigned long durasiParkirDetik = 0;
const long tarifPerDetik = 50; // Contoh tarif: Rp 50 per detik (bisa disesuaikan)

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
  
  // Tampilan Awal
  lcd.setCursor(0, 0);
  lcd.print(" Slot Parkir ");
  lcd.setCursor(0, 1);
  lcd.print("     KOSONG     ");
  delay(2000);
  lcd.clear();
}

void loop() {
  long jarak = hitungJarak();

  // KONDISI 1: Mobil belum parkir menetap (Proses Parkir)
// KONDISI 1: Mobil belum parkir menetap (Proses Parkir)
  if (!mobilParkir) {
    if (jarak > batasKosong) {
      // Tidak ada mobil yang terdeteksi dalam radius 50cm
      tampilanLCD("  SLOT KOSONG  ", " Menunggu...   ");
      setKondisiKomponen(HIGH, LOW, LOW, false, 0); // LED Hijau menyala (tersedia)
      waktuMulaiStop = 0; // Reset timer 3 detik
    }
    else if (jarak > jarakJauh && jarak <= batasKosong) {
      // Jarak 31 - 50: Mobil mulai masuk, arahkan mundur
      tampilanLCD("MUNDUR...", "Jarak: " + String(jarak) + "cm");
      setKondisiKomponen(HIGH, LOW, LOW, false, 0);
      waktuMulaiStop = 0; // Reset timer 3 detik
    } 
    else if (jarak > jarakAman && jarak <= jarakJauh) {
      // Jarak Menengah: Pelan-pelan
      tampilanLCD("PELAN-PELAN", "Jarak: " + String(jarak) + "cm");
      setKondisiKomponen(LOW, HIGH, LOW, true, 500); // Buzzer jeda lambat
      waktuMulaiStop = 0; // Reset timer 3 detik
    } 
    else if (jarak <= jarakAman && jarak > 2) { // Jarak > 2cm untuk menghindari error
      // Jarak Sudah Cukup: STOP!
      tampilanLCD("!! STOP !!", "Jarak: " + String(jarak) + "cm");
      setKondisiKomponen(LOW, LOW, HIGH, true, 100); // Buzzer jeda cepat

      // Mulai hitung apakah mobil diam selama 3 detik
      if (waktuMulaiStop == 0) {
        waktuMulaiStop = millis();
      } else if (millis() - waktuMulaiStop >= 3000) {
        // Sudah 3 detik diam di posisi STOP -> Mobil dianggap sudah parkir & mesin mati
        mobilParkir = true;
        waktuMulaiParkir = millis(); // Mulai Stopwatch Parkir
        lcd.clear();
        digitalWrite(buzzer, LOW); // Matikan buzzer bising
      }
    }
  }
  // KONDISI 2: Mobil Sudah Parkir Menetap (Stopwatch & Hitung Tarif)
  else {
    // LED Merah menyala terus menandakan slot terisi
    digitalWrite(ledHijau, LOW);
    digitalWrite(ledKuning, LOW);
    digitalWrite(ledMerah, HIGH);

    // Hitung durasi parkir berjalan
    durasiParkirDetik = (millis() - waktuMulaiParkir) / 1000;
    long totalTarif = durasiParkirDetik * tarifPerDetik;

    // Tampilkan Stopwatch dan Tarif di LCD
    lcd.setCursor(0, 0);
    lcd.print("TERISI | T:" + formatWaktu(durasiParkirDetik));
    lcd.setCursor(0, 1);
    lcd.print("Biaya: Rp " + String(totalTarif));

    // Cek apakah mobil bergerak pergi (jarak menjauh kembali)
    if (jarak > (jarakAman + 10)) { 
      // Mobil pergi! Tampilkan ringkasan biaya akhir selama 5 detik
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Total Waktu:" + formatWaktu(durasiParkirDetik));
      lcd.setCursor(0, 1);
      lcd.print("Total: Rp" + String(totalTarif));
      
      // Buzzer berbunyi panjang sebagai tanda transaksi selesai
      digitalWrite(buzzer, HIGH);
      delay(1000);
      digitalWrite(buzzer, LOW);
      delay(4000); // Total display 5 detik

      // Reset sistem ke kondisi awal (Kosong)
      mobilParkir = false;
      waktuMulaiStop = 0;
      lcd.clear();
    }
  }
  delay(100); // Delay kecil untuk stabilitas sensor
}

// Fungsi Mengukur Jarak dengan HC-SR04
long hitungJarak() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long durasi = pulseIn(echoPin, HIGH);
  long jarakCm = durasi * 0.034 / 2;
  return jarakCm;
}

// Fungsi Helper untuk Update LCD agar tidak berkedip (flicker)
String lastBaris1 = "", lastBaris2 = "";
void tampilanLCD(String baris1, String baris2) {
  if (baris1 != lastBaris1) {
    lcd.setCursor(0, 0);
    lcd.print("                "); // Clear baris
    lcd.setCursor(0, 0);
    lcd.print(baris1);
    lastBaris1 = baris1;
  }
  if (baris2 != lastBaris2) {
    lcd.setCursor(0, 1);
    lcd.print("                "); // Clear baris
    lcd.setCursor(0, 1);
    lcd.print(baris2);
    lastBaris2 = baris2;
  }
}

// Fungsi Mengatur LED dan Buzzer Berkedip
void setKondisiKomponen(int h, int k, int m, bool bz, int jeda) {
  digitalWrite(ledHijau, h);
  digitalWrite(ledKuning, k);
  digitalWrite(ledMerah, m);
  
  if (bz) {
    // Membuat efek buzzer berkedip sesuai jeda tanpa mengacaukan millis
    if ((millis() / jeda) % 2 == 0) {
      digitalWrite(buzzer, HIGH);
    } else {
      digitalWrite(buzzer, LOW);
    }
  } else {
    digitalWrite(buzzer, LOW);
  }
}

// Fungsi Mengubah Detik menjadi Format Menit:Detik (MM:SS)
String formatWaktu(long totalDetik) {
  long menit = totalDetik / 60;
  long detik = totalDetik % 60;
  String strMenit = (menit < 10) ? "0" + String(menit) : String(menit);
  String strDetik = (detik < 10) ? "0" + String(detik) : String(detik);
  return strMenit + ":" + strDetik;
}