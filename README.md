Berikut adalah pembaruan isi dokumen README yang disesuaikan secara presisi dengan spesifikasi teknis terbaru, implementasi *source code* final, kondisi riil pengujian, serta target pengembangan sistem kamu:

---

# Sistem Monitoring Parkir Pintar Berbasis FreeRTOS

Sistem monitoring parkir pintar menggunakan arsitektur multitasking kernel FreeRTOS untuk mendeteksi rintangan saat proses parkir secara *real-time*, mengalkulasi durasi parkir kendaraan, serta memberikan indikasi audio/visual yang responsif melalui modul output.

---

## Detail Kelompok

**Program Studi:** Teknik Komputer, Universitas Jenderal Soedirman (UNSOED)

**Anggota Kelompok:**

* H1H024035 HURIYATUN NUR ANAJMI
* H1H024036 KHOIRUL ROSYID GUNAWAN
* H1H024037 THUFAIL LABIB ASSHIDQI
* H1H024038 IBNU ABBAS (Ketua)
* H1H024039 ANDYKA ZEFANYA BRAMANTYO
* H1H024040 ARIFIN BUDI KUSUMA

---

## Deskripsi Proyek

Proyek ini adalah prototipe sistem parkir pintar berbasis *real-time operating system* (RTOS). Sistem dirancang untuk mengoptimalkan keamanan proses parkir melalui deteksi jarak rintangan dinamis (*anti-noise filtering*), validasi posisi kendaraan (*debouncing time*), serta pencatatan otomatis durasi parkir berbasis *state machine* untuk penentuan tarif.

### Komponen Utama:

1. Mikrokontroler Kompatibel AVR (Arduino Uno / Nano)
2. Sensor Ultrasonik HC-SR04 (Input Data Jarak)
3. LED Indikator Dinamis (Aktuator PWM Tingkat Kecerahan)
4. Piezo Buzzer Frekuensi Nada (Aktuator Audio Peringatan)

---

### Cara Kerja

Sistem membagi beban kerja secara simultan ke dalam dua tugas utama (`taskBacaJarak` dan `taskKontrolOutput`) yang saling berkomunikasi menggunakan media *FreeRTOS Queue*. Ketika kendaraan mendekati area parkir, sensor ultrasonik mendeteksi jarak objek secara *real-time*. Sinyal output berupa kedipan LED PWM dan bunyi bip buzzer akan meningkat frekuensinya secara otomatis seiring semakin dekatnya kendaraan ke batas kritis aman.

Ketika kendaraan telah mencapai titik pas ($\le 5\text{ cm}$) dan bertahan secara konsisten selama lebih dari 2 detik (*debouncing time*), sistem mengubah status menjadi `PARKED` dan mengaktifkan fungsionalitas *stopwatch* untuk menghitung durasi parkir di memori mikro. Sistem ini juga mengimplementasikan logika *hysteresis* (batas toleransi dinamis hingga 8 cm) untuk mengantisipasi noise, fluktuasi bacaan sensor, ataupun getaran mesin kendaraan di tempat agar perhitungan waktu tetap berjalan akurat.

---

### Status Proyek (Ketercapaian: ~70%)

* **Fungsionalitas Utama (Selesai):** Seluruh arsitektur kode program (FreeRTOS kernel, sinkronisasi *queue*, *state machine*, dan driver periferal output non-blocking) sudah berjalan 100% valid pada perangkat keras melalui pengujian Serial Monitor.
* **Pengembangan Berikutnya (Sisa 30%):** Perapian desain tata letak kabel fisik (*wiring layout*) ke dalam wadah/*enclosure*, serta integrasi modul tambahan berupa LCD I2C (pada pin SDA/SCL) untuk memproyeksikan informasi akumulasi tarif parkir secara langsung tanpa komputer.