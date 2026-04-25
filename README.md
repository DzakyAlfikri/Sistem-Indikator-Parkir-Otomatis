# Sistem Indikator Parkir Otomatis

**Sistem Indikator Parkir Otomatis** adalah sebuah sistem manajemen parkir yang dirancang untuk mengotomatisasi proses masuk dan keluar kendaraan dari area parkir. Sistem ini menggunakan Arduino sebagai mikrokontroler utama dengan berbagai sensor dan aktuator untuk mendeteksi dan mengontrol arus kendaraan.

### Fitur Utama:
- Deteksi kendaraan otomatis menggunakan sensor ultrasonik
- Pembuka palang otomatis saat kendaraan terdeteksi
- Tampilan informasi slot parkir pada LCD
- Indikator LED merah (penuh) dan hijau (tersedia)
- Sistem alarm darurat dengan sirine buzzer
- Lampu otomatis berdasarkan sensor cahaya (LDR)
- Kontrol manual menggunakan tombol push button
- Mode darurat untuk evakuasi cepat

---

## Spesifikasi Hardware

### Komponen Utama:

| Komponen | Jumlah | Keterangan |
|----------|--------|-----------|
| Arduino Uno | 1 | Mikrokontroler utama |
| Sensor Ultrasonik HC-SR04 | 2 | Masuk & Keluar |
| Motor Servo SG90 | 2 | Palang masuk & keluar |
| LCD 16x2 (I2C) | 1 | Tampilan informasi |
| LED Merah (5mm) | 1 | Indikator parkir penuh |
| LED Hijau (5mm) | 1 | Indikator slot tersedia |
| Buzzer 5V | 1 | Alarm mode darurat |
| Sensor LDR | 1 | Deteksi cahaya |
| Push Button | 3 | Tombol masuk, keluar, darurat |

### Koneksi Pin Arduino:

```
Pin Sensor Ultrasonik:
  - Masuk:  Trig = Pin 4, Echo = Pin 5
  - Keluar: Trig = Pin 6, Echo = Pin 7

Pin Servo:
  - Palang Masuk  = Pin 9
  - Palang Keluar = Pin 10

Pin LED & Buzzer:
  - LED Hijau = Pin 8
  - LED Merah = Pin 11
  - Buzzer    = Pin 12

Pin Sensor & Button:
  - LDR         = A0 (Analog)
  - Tombol Masuk   = A1 (Analog)
  - Tombol Keluar  = A2 (Analog)
  - Tombol Darurat = Pin 2 (Interrupt)

Pin Lampu Otomatis:
  - Lampu = Pin 3 (PWM)
```

---

## Library yang Digunakan

### 1. **Wire.h**
```cpp
#include <Wire.h>
```
**Fungsi:** Komunikasi protokol I2C antara Arduino dan perangkat I2C seperti LCD  
**Kegunaan:** Memungkinkan Arduino berkomunikasi dengan LCD I2C melalui 2 jalur (SDA dan SCL)  
**Manfaat:** Menghemat pin digital, komunikasi yang andal untuk jarak jauh

---

### 2. **LiquidCrystal_I2C.h**
```cpp
#include <LiquidCrystal_I2C.h>
```
**Fungsi:** Mengontrol LCD 16x2 dengan modul I2C  
**Kegunaan:** 
- Menampilkan jumlah slot parkir tersedia
- Menampilkan status parkir (penuh/tersedia/darurat)
- Interface informasi kepada pengguna

**Fitur yang digunakan:**
- `lcd.init()` - Inisialisasi LCD
- `lcd.backlight()` - Menyalakan backlight
- `lcd.clear()` - Membersihkan display
- `lcd.setCursor(kolom, baris)` - Mengatur posisi kursor
- `lcd.print()` - Menampilkan teks/angka

---

### 3. **Servo.h**
```cpp
#include <Servo.h>
```
**Fungsi:** Mengontrol motor servo sebagai palang otomatis  
**Kegunaan:**
- Membuka palang saat kendaraan terdeteksi (sudut 90°)
- Menutup palang setelah timeout (sudut 0°)
- Respons cepat dan presisi pergerakan

**Fitur yang digunakan:**
- `servo.attach(pin)` - Menghubungkan servo ke pin PWM
- `servo.write(sudut)` - Menggerakkan servo ke sudut tertentu (0-180°)

---

## Deklarasi Variabel

### Inisialisasi Objek

```cpp
LiquidCrystal_I2C lcd(0x20, 16, 2);  // LCD I2C pada alamat 0x20, 16 kolom, 2 baris
Servo palangMasuk;                   // Objek servo untuk palang masuk
Servo palangKeluar;                  // Objek servo untuk palang keluar
```

**Penjelasan:**
- **Alamat 0x20**: Alamat I2C LCD (dapat berbeda, gunakan I2C Scanner untuk cek)
- **16 kolom, 2 baris**: Tipe LCD yang digunakan

---

### Pin Sensor Ultrasonik

```cpp
const int trigIn = 4;              // Pin trigger sensor masuk
const int echoIn = 5;              // Pin echo sensor masuk
const int trigOut = 6;             // Pin trigger sensor keluar
const int echoOut = 7;             // Pin echo sensor keluar
```

**Cara Kerja Sensor Ultrasonik:**
1. Trigger dikirim HIGH selama 10 µs untuk mengirim gelombang suara
2. Sensor menerima pantulan gelombang pada pin echo
3. Durasi echo diukur dan dikonversi menjadi jarak

---

### Pin Indikator LED

```cpp
const int ledHijau = 8;            // LED hijau = slot tersedia (AVAILABLE)
const int ledMerah = 11;           // LED merah = parkir penuh (FULL)
```

**Logika Indikator:**
- ✅ **LED Hijau MENYALA** = Slot masih tersedia, kendaraan boleh masuk
- 🔴 **LED Merah MENYALA** = Parkir penuh, tidak ada slot kosong
- 📟 **Pada Mode Darurat** = LED merah berkedip dengan frekuensi 300ms

---

### Pin Kontroler Lainnya

```cpp
const int lampu = 3;               // Pin lampu otomatis (PWM - Pin 3, 5, 6, 9, 10, 11)
const int ldrPin = A0;             // Pin sensor cahaya (Analog Input)
const int buzzer = 12;             // Pin buzzer alarm
const int tombolDarurat = 2;       // Tombol darurat (INT0 untuk interrupt)
const int tombolMasuk = A1;        // Tombol kontrol manual masuk
const int tombolKeluar = A2;       // Tombol kontrol manual keluar
```

**Penjelasan Khusus:**
- **lampu pada Pin 3**: Menggunakan PWM untuk kontrol brightness (0-255)
- **ldrPin (A0)**: Membaca nilai analog (0-1023) untuk deteksi cahaya
- **tombolDarurat pada Pin 2**: Mendukung external interrupt (FALLING edge triggered)

---

### Variabel Slot Parkir

```cpp
int slotTersedia = 5;              // Slot yang masih kosong (awal = 5)
int kapasitasMaks = 5;             // Kapasitas maksimal parkir
```

**Logika Slot:**
- Saat kendaraan masuk: `slotTersedia--` (berkurang 1)
- Saat kendaraan keluar: `slotTersedia++` (bertambah 1)
- Jika `slotTersedia <= 0`: Parkir penuh ❌
- Jika `slotTersedia > 0`: Slot tersedia ✅
- Reset: `slotTersedia = kapasitasMaks`

---

### Variabel Mode Darurat

```cpp
volatile bool modeDarurat = false;     // Perubahan via interrupt eksternal
bool sebelumnyaDarurat = false;        // Menyimpan kondisi sebelumnya
```

**Keyword `volatile`:** 
- Memberi tahu compiler variabel dapat berubah kapan saja (dari interrupt)
- Compiler tidak akan mengoptimasi pembacaan variabel ini
- Sangat penting untuk variabel yang diubah dari interrupt

**Cara Kerja:**
1. Tekan tombol darurat → interrupt terpicu
2. `modeDarurat = !modeDarurat` (toggle true/false)
3. Sistem masuk mode darurat atau kembali normal

---

### Variabel Status Palang Manual

```cpp
bool statusPalangMasuk = false;    // false = tertutup, true = terbuka
bool statusPalangKeluar = false;   // false = tertutup, true = terbuka
```

**Penggunaan:**
```cpp
// Jika tombol masuk ditekan:
statusPalangMasuk = !statusPalangMasuk;
palangMasuk.write(statusPalangMasuk ? 90 : 0);
// Jika true (terbuka)  → servo = 90°
// Jika false (tertutup) → servo = 0°
```

---

### Variabel LED Berkedip (Mode Darurat)

```cpp
unsigned long lastBlink = 0;       // Waktu terakhir LED berubah
bool blinkState = false;           // false = mati, true = menyala
```

**Timer-based Blinking Logic:**
```cpp
if(millis() - lastBlink > 300) {   // Setiap 300ms
    lastBlink = millis();          // Reset timer
    blinkState = !blinkState;      // Toggle state
}
digitalWrite(ledMerah, blinkState);
```

---

### Variabel Sirine Buzzer

```cpp
unsigned long lastSiren = 0;       // Waktu terakhir frekuensi berubah
int sirenFreq = 800;               // Frekuensi awal buzzer (Hz)
bool sirenUp = true;               // Arah: true = naik, false = turun
```

**Cara Kerja Sirine:**
1. Frekuensi mulai dari 800 Hz
2. Setiap 80ms, frekuensi bertambah 30 Hz (jika naik)
3. Mencapai maksimal 2000 Hz → berubah arah turun
4. Turun hingga 600 Hz → berubah arah naik
5. Menghasilkan efek sirine naik-turun

**Rentang Frekuensi:**
```
600 Hz ← → 800 Hz (awal) ← → 2000 Hz
[Rendah] [Sedang] [Tinggi]
```

---

### Variabel Deteksi Kendaraan

```cpp
bool terdeteksiMasuk = false;      // Flag kendaraan masuk sedang terbuka palangnya
bool terdeteksiKeluar = false;     // Flag kendaraan keluar sedang terbuka palangnya
```

**Fungsi:**
Mencegah pembacaan sensor berulang kali untuk kendaraan yang sama
- Saat terdeteksi: Set flag `true`
- Saat palang menutup: Set flag `false`
- Jika flag `true` dan kendaraan masih di depan sensor: Abaikan deteksi

**Tanpa flag ini**, palang akan membuka berkali-kali untuk satu kendaraan!

---

### Variabel Timer Palang Otomatis

```cpp
unsigned long waktuBukaMasuk = 0;  // Timestamp saat palang masuk dibuka
unsigned long waktuBukaKeluar = 0; // Timestamp saat palang keluar dibuka
const int waktuTahan = 3000;       // Lama palang terbuka = 3000ms = 3 detik
```

**Logika Timer:**
```cpp
// Deteksi kendaraan masuk
if(jarakMasuk < 10 && !terdeteksiMasuk && slotTersedia > 0) {
    waktuBukaMasuk = millis();     // Catat waktu pembukaan
    palangMasuk.write(90);         // Buka palang
}

// Penutupan otomatis setelah timeout
if(terdeteksiMasuk && millis() - waktuBukaMasuk > 3000) {
    palangMasuk.write(0);          // Tutup palang
}
```

---

## Penjelasan Fungsi

### 1. Fungsi Interrupt Darurat

```cpp
void darurat() {
    modeDarurat = !modeDarurat;    // Toggle mode darurat
}
```

**Penjelasan:**
- Dipanggil saat tombol darurat ditekan (interrupt eksternal Pin 2)
- Toggle: `false` → `true` (masuk darurat) atau `true` → `false` (keluar darurat)
- Eksekusi sangat cepat, tidak mengganggu loop utama

**Keuntungan Interrupt:**
- Respons instan saat tombol ditekan
- Tidak perlu menunggu loop utama selesai
- Ideal untuk kasus emergency

---

### 2. Fungsi Setup

```cpp
void setup() {
    // Dijalankan SEKALI saat Arduino dihidupkan
    // Inisialisasi semua hardware
}
```

**Tahap Inisialisasi:**

**A. Komunikasi Serial:**
```cpp
Serial.begin(9600);
Serial.println("=== SISTEM PARKIR OTOMATIS ===");
```
- Membuka komunikasi dengan Serial Monitor
- Kecepatan 9600 bps (baud rate)
- Menampilkan pesan startup

**B. Inisialisasi LCD:**
```cpp
lcd.init();      // Nyalakan LCD
lcd.backlight(); // Nyalakan backlight
```

**C. Konfigurasi Servo:**
```cpp
palangMasuk.attach(9);   // Servo masuk pada PWM pin 9
palangKeluar.attach(10); // Servo keluar pada PWM pin 10

palangMasuk.write(0);    // Posisi awal: tertutup (0°)
palangKeluar.write(0);   // Posisi awal: tertutup (0°)
```

**D. Konfigurasi Pin Mode:**
```cpp
// Sensor Ultrasonik
pinMode(trigIn, OUTPUT);  // Mengirim sinyal
pinMode(echoIn, INPUT);   // Menerima sinyal

// Output Digital
pinMode(ledHijau, OUTPUT);
pinMode(ledMerah, OUTPUT);
pinMode(buzzer, OUTPUT);

// Input Digital (dengan Pull-Up internal)
pinMode(tombolDarurat, INPUT_PULLUP);
pinMode(tombolMasuk, INPUT_PULLUP);
pinMode(tombolKeluar, INPUT_PULLUP);
```

**INPUT_PULLUP:** 
- Pin akan dibaca HIGH saat tidak ditekan
- Saat ditekan: SHORT ke GND → dibaca LOW
- Tidak perlu resistor eksternal

**E. Aktivasi Interrupt:**
```cpp
attachInterrupt(digitalPinToInterrupt(2), darurat, FALLING);
```
- Pin 2 (INT0) → panggil fungsi `darurat()`
- FALLING: Trigger saat sinyal turun dari HIGH ke LOW
- Artinya: trigger saat tombol ditekan

**F. Tampilkan Status Awal:**
```cpp
tampilkanStatus();  // Tampilkan slot di LCD
```

---

### 3. Fungsi tampilkanStatus

```cpp
void tampilkanStatus() {
    lcd.clear();           // Hapus layar
    
    // Baris pertama: jumlah slot
    lcd.setCursor(0, 0);
    lcd.print("Slot: ");
    lcd.print(slotTersedia);
    
    // Baris kedua: status parkir
    lcd.setCursor(0, 1);
    if(slotTersedia <= 0) {
        lcd.print("PARKIR PENUH");
    } else {
        lcd.print("SILAHKAN MASUK");
    }
}
```

**LCD Output Contoh:**

```
Slot: 5
SILAHKAN MASUK

------- (setelah 1 kendaraan masuk) -------

Slot: 4
SILAHKAN MASUK

------- (saat penuh) -------

Slot: 0
PARKIR PENUH
```

---

### 4. Fungsi bacaJarak

```cpp
long bacaJarak(int trig, int echo) {
    // Mengirim sinyal trigger
    digitalWrite(trig, LOW);
    delayMicroseconds(2);      // Pastikan LOW minimal 2µs
    
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);     // Pulse HIGH 10µs
    
    digitalWrite(trig, LOW);
    
    // Membaca waktu pantulan dan hitung jarak
    long pulsa = pulseIn(echo, HIGH);  // Durasi echo (mikrodetik)
    return pulsa * 0.034 / 2;          // Konversi ke cm
}
```

**Penjelasan Rumus:**
```
Kecepatan suara = 340 m/s = 0.034 cm/µs
Jarak = (waktu × kecepatan) / 2
Dibagi 2 karena suara pergi pulang
```

**Contoh Perhitungan:**
- Waktu pulsa = 300 µs
- Jarak = 300 × 0.034 / 2 = 5.1 cm

---

## Alur Program

### Flow Chart Mode Normal

```
┌─────────────────┐
│   START/SETUP   │
└────────┬────────┘
         │
    ┌────▼─────────────────────────┐
    │ Mode Darurat Aktif?          │
    └────┬──────────────┬──────────┘
         │ YA           │ TIDAK
         │              │
    ┌────▼──────────┐   │
    │ DARURAT MODE  │   │
    │ - Buka Palang │   │
    │ - Sirine      │   │
    └───────────────┘   │
                        │
                   ┌────▼──────────────────────┐
                   │ Cek Sensor Ultrasonik     │
                   │ - Masuk (<10cm)           │
                   │ - Keluar (<10cm)          │
                   └────┬────────────┬─────────┘
                        │ MASUK      │ KELUAR
                        │            │
                    ┌───▼──┐     ┌───▼──┐
                    │Buka  │     │Buka  │
                    │Palang│     │Palang│
                    │Masuk │     │Keluar│
                    │Slot--│     │Slot++│
                    └──────┘     └──────┘
                        │            │
                    ┌───▼────────────▼───┐
                    │ Tampilkan di LCD    │
                    │ Update LED Status   │
                    └────────┬────────────┘
                             │
                    ┌────────▼────────┐
                    │ Delay 50ms      │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │ Loop (repeat)   │
                    └─────────────────┘
```

---

### Skenario Mode Darurat

```
Tombol Darurat Ditekan
        │
        ▼
modeDarurat = !modeDarurat (toggle)
        │
┌───────┴───────┐
│ Masuk Darurat │  (modeDarurat = true)
└───────┬───────┘
        │
    ┌───▼────────────────────┐
    │ 1. Buka palang masuk   │
    │ 2. Buka palang keluar  │
    │ 3. LED merah berkedip  │
    │ 4. Buzzer sirine       │
    │ 5. Tampil "EVAKUASI"   │
    └────────────────────────┘
        │
    (Tombol ditekan lagi)
        │
        ▼
modeDarurat = !modeDarurat (toggle kembali)
        │
┌───────┴──────────┐
│ Keluar Darurat   │  (modeDarurat = false)
└───────┬──────────┘
        │
    ┌───▼────────────────────┐
    │ 1. Tutup palang masuk  │
    │ 2. Tutup palang keluar │
    │ 3. Matikan buzzer      │
    │ 4. Reset slot = 5      │
    │ 5. Kembali normal      │
    └────────────────────────┘
```

---

## Cara Penggunaan

### 1. Persiapan Hardware

1. **Sambungkan Arduino ke PC** melalui USB cable
2. **Verifikasi koneksi pin** sesuai dengan diagram di atas
3. **Pastikan power supply cukup** (minimal 9V 1A untuk servo)
4. **Upload code** ke Arduino

### 2. Testing Awal

**Via Serial Monitor (Baud: 9600):**
```
# Tambah slot: ketik '+' lalu ENTER
# Kurangi slot: ketik '-' lalu ENTER  
# Reset slot: ketik 'r' lalu ENTER
```

### 3. Operasi Normal

**Kendaraan Masuk:**
1. Kendaraan mendekat ke sensor masuk
2. Sensor mendeteksi (<10cm)
3. Palang terbuka otomatis (servo 90°)
4. Slot berkurang 1
5. LCD update
6. Setelah 3 detik palang menutup otomatis

**Kendaraan Keluar:**
1. Kendaraan mendekat ke sensor keluar
2. Sensor mendeteksi (<10cm)
3. Palang terbuka otomatis
4. Slot bertambah 1
5. LCD update
6. Setelah 3 detik palang menutup otomatis

### 4. Mode Darurat

**Aktivasi:**
- Tekan tombol darurat (Pin 2)
- Palang terbuka penuh (90°)
- LED merah berkedip 300ms
- Buzzer berbunyi sirine
- LCD tampilkan "EVAKUASI KELUAR"

**Deaktivasi:**
- Tekan tombol darurat lagi
- Palang tutup (0°)
- Buzzer berhenti
- Slot direset ke maksimal (5)
- Kembali mode normal

### 5. Kontrol Manual

- **Tombol Masuk (A1):** Toggle buka/tutup palang masuk
- **Tombol Keluar (A2):** Toggle buka/tutup palang keluar
- Tekan tombol, palang terbuka
- Tekan lagi, palang tertutup

### 6. Lampu Otomatis

- **Gelap (<300 LDR):** Lampu menyala penuh
- **Terang (≥300 LDR):** Lampu mati
- Otomatis tanpa intervensi pengguna

---
