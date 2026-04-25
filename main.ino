#include <Wire.h>                  // memanggil library komunikasi I2C
#include <LiquidCrystal_I2C.h>     // memanggil library LCD I2C
#include <Servo.h>                 // memanggil library servo motor

LiquidCrystal_I2C lcd(0x20, 16, 2); // membuat objek LCD alamat 0x20 ukuran 16 kolom 2 baris

Servo palangMasuk;                 // membuat objek servo untuk palang masuk
Servo palangKeluar;                // membuat objek servo untuk palang keluar

const int trigIn = 4;              // pin trigger ultrasonik masuk
const int echoIn = 5;              // pin echo ultrasonik masuk
const int trigOut = 6;             // pin trigger ultrasonik keluar
const int echoOut = 7;             // pin echo ultrasonik keluar

const int ledHijau = 8;            // LED hijau indikator slot tersedia
const int ledMerah = 11;           // LED merah indikator parkir penuh

const int lampu = 3;               // pin lampu otomatis (PWM)

const int ldrPin = A0;             // pin analog sensor cahaya LDR

const int buzzer = 12;             // pin buzzer alarm

const int tombolDarurat = 2;       // tombol mode darurat (interrupt)
const int tombolMasuk = A1;        // tombol manual palang masuk
const int tombolKeluar = A2;       // tombol manual palang keluar

int slotTersedia = 5;              // jumlah slot parkir tersedia
int kapasitasMaks = 5;             // kapasitas maksimum parkir

volatile bool modeDarurat = false; // status mode darurat (dipakai interrupt)
bool sebelumnyaDarurat = false;    // menyimpan kondisi sebelumnya

bool statusPalangMasuk = false;    // status manual palang masuk
bool statusPalangKeluar = false;   // status manual palang keluar

unsigned long lastBlink = 0;       // waktu terakhir LED berkedip
bool blinkState = false;           // status LED berkedip

unsigned long lastSiren = 0;       // waktu terakhir perubahan sirine
int sirenFreq = 800;               // frekuensi awal sirine
bool sirenUp = true;               // arah frekuensi naik/turun

bool terdeteksiMasuk = false;      // flag mobil masuk terdeteksi
bool terdeteksiKeluar = false;     // flag mobil keluar terdeteksi

unsigned long waktuBukaMasuk = 0;  // waktu palang masuk dibuka
unsigned long waktuBukaKeluar = 0; // waktu palang keluar dibuka
const int waktuTahan = 3000;       // lama palang terbuka 3 detik

void darurat(){                    // fungsi interrupt tombol darurat
  modeDarurat = !modeDarurat;     // toggle mode darurat ON/OFF
}

void setup(){                     // fungsi setup dijalankan sekali

  Serial.begin(9600);             // memulai komunikasi serial
  Serial.println("=== SISTEM PARKIR OTOMATIS ==="); // tampil judul

  lcd.init();                     // inisialisasi LCD
  lcd.backlight();                // menyalakan lampu LCD

  palangMasuk.attach(9);          // servo masuk pada pin 9
  palangKeluar.attach(10);        // servo keluar pada pin 10

  palangMasuk.write(0);           // posisi awal palang masuk tertutup
  palangKeluar.write(0);          // posisi awal palang keluar tertutup

  pinMode(trigIn, OUTPUT);        // trig masuk sebagai output
  pinMode(echoIn, INPUT);         // echo masuk sebagai input
  pinMode(trigOut, OUTPUT);       // trig keluar sebagai output
  pinMode(echoOut, INPUT);        // echo keluar sebagai input

  pinMode(ledHijau, OUTPUT);      // LED hijau sebagai output
  pinMode(ledMerah, OUTPUT);      // LED merah sebagai output

  pinMode(lampu, OUTPUT);         // lampu sebagai output PWM
  pinMode(buzzer, OUTPUT);        // buzzer sebagai output

  pinMode(tombolDarurat, INPUT_PULLUP); // tombol darurat pullup
  pinMode(tombolMasuk, INPUT_PULLUP);   // tombol masuk pullup
  pinMode(tombolKeluar, INPUT_PULLUP);  // tombol keluar pullup

  attachInterrupt(digitalPinToInterrupt(2), darurat, FALLING); // interrupt tombol darurat

  tampilkanStatus();              // tampilkan status awal LCD
}

void tampilkanStatus(){           // fungsi menampilkan status slot

  lcd.clear();                    // bersihkan LCD
  lcd.setCursor(0,0);             // posisi baris 1 kolom 1
  lcd.print("Slot: ");            // tampilkan tulisan Slot
  lcd.print(slotTersedia);        // tampilkan jumlah slot

  lcd.setCursor(0,1);             // pindah ke baris kedua

  if(slotTersedia <= 0){          // jika slot habis
    lcd.print("PARKIR PENUH");    // tampilkan parkir penuh
  } else {                        // jika slot masih ada
    lcd.print("SILAHKAN MASUK");  // tampilkan silahkan masuk
  }
}

long bacaJarak(int trig, int echo){ // fungsi membaca jarak ultrasonik

  digitalWrite(trig, LOW);        // pastikan trig LOW
  delayMicroseconds(2);           // delay 2 mikrodetik

  digitalWrite(trig, HIGH);       // kirim gelombang ultrasonik
  delayMicroseconds(10);          // selama 10 mikrodetik

  digitalWrite(trig, LOW);        // matikan trig

  return pulseIn(echo, HIGH) * 0.034 / 2; // hitung jarak dalam cm
}

void loop(){                      // loop utama program

  if(modeDarurat){                // jika mode darurat aktif

    if(!sebelumnyaDarurat){       // jika baru masuk mode darurat
      lcd.clear();                // bersihkan LCD
      sebelumnyaDarurat = true;   // tandai sudah darurat
    }

    palangMasuk.write(90);        // buka palang masuk
    palangKeluar.write(90);       // buka palang keluar

    lcd.setCursor(0,0);           // posisi baris pertama
    lcd.print("MODE DARURAT   "); // tampilkan mode darurat

    lcd.setCursor(0,1);           // baris kedua
    lcd.print("EVAKUASI KELUAR");// tampilkan evakuasi

    if(millis() - lastBlink > 300){ // jika waktu kedip tercapai
      lastBlink = millis();         // reset timer kedip
      blinkState = !blinkState;     // toggle LED
    }

    digitalWrite(ledMerah, blinkState); // LED merah berkedip
    digitalWrite(ledHijau, LOW);        // LED hijau mati

    if(millis() - lastSiren > 80){ // jika waktu sirine tercapai
      lastSiren = millis();        // reset timer sirine

      if(sirenUp){                 // jika frekuensi naik
        sirenFreq += 30;           // tambah frekuensi
        if(sirenFreq >= 2000)      // jika maksimum
          sirenUp = false;         // ubah arah turun
      } else {                     // jika frekuensi turun
        sirenFreq -= 30;           // kurangi frekuensi
        if(sirenFreq <= 600)       // jika minimum
          sirenUp = true;          // ubah arah naik
      }

      tone(buzzer, sirenFreq);     // bunyikan buzzer
    }

    Serial.println("!!! MODE DARURAT AKTIF !!!"); // info serial
    return;                         // keluar loop
  }

  if(sebelumnyaDarurat && !modeDarurat){ // jika darurat dimatikan

    sebelumnyaDarurat = false;     // reset flag darurat
    slotTersedia = kapasitasMaks;  // reset slot parkir

    palangMasuk.write(0);          // tutup palang masuk
    palangKeluar.write(0);         // tutup palang keluar

    noTone(buzzer);                // matikan buzzer
    lcd.clear();                   // bersihkan LCD

    Serial.println("MODE DARURAT DIMATIKAN"); // info serial
    tampilkanStatus();             // tampilkan status
  }

  if(slotTersedia <= 0){           // jika slot habis
    digitalWrite(ledMerah, HIGH);  // LED merah nyala
    digitalWrite(ledHijau, LOW);   // LED hijau mati
  } else {                         // jika slot tersedia
    digitalWrite(ledMerah, LOW);   // LED merah mati
    digitalWrite(ledHijau, HIGH);  // LED hijau nyala
  }

  if(digitalRead(tombolMasuk) == LOW){ // jika tombol masuk ditekan
    delay(200);                         // debounce tombol
    statusPalangMasuk = !statusPalangMasuk; // toggle status
    palangMasuk.write(statusPalangMasuk ? 90 : 0); // buka/tutup
  }

  if(digitalRead(tombolKeluar) == LOW){ // jika tombol keluar ditekan
    delay(200);                         // debounce
    statusPalangKeluar = !statusPalangKeluar; // toggle status
    palangKeluar.write(statusPalangKeluar ? 90 : 0); // buka/tutup
  }

  int nilaiLDR = analogRead(ldrPin); // baca nilai LDR
  analogWrite(lampu, (nilaiLDR < 300) ? 255 : 0); // lampu otomatis

  long jarakMasuk = bacaJarak(trigIn, echoIn); // baca jarak masuk

  if(jarakMasuk < 10 && !terdeteksiMasuk && slotTersedia > 0){ // mobil masuk
    terdeteksiMasuk = true;         // tandai terdeteksi
    waktuBukaMasuk = millis();      // simpan waktu buka
    palangMasuk.write(90);          // buka palang
    slotTersedia--;                 // kurangi slot
    tampilkanStatus();              // update LCD
  }

  if(terdeteksiMasuk && millis() - waktuBukaMasuk > waktuTahan){ // waktu habis
    terdeteksiMasuk = false;        // reset deteksi
    palangMasuk.write(0);           // tutup palang
  }

  long jarakKeluar = bacaJarak(trigOut, echoOut); // baca jarak keluar

  if(jarakKeluar < 10 && !terdeteksiKeluar && slotTersedia < kapasitasMaks){ // mobil keluar
    terdeteksiKeluar = true;        // tandai keluar
    waktuBukaKeluar = millis();     // simpan waktu
    palangKeluar.write(90);         // buka palang
    slotTersedia++;                 // tambah slot
    tampilkanStatus();              // update LCD
  }

  if(terdeteksiKeluar && millis() - waktuBukaKeluar > waktuTahan){ // waktu habis
    terdeteksiKeluar = false;       // reset deteksi
    palangKeluar.write(0);          // tutup palang
  }

  if (Serial.available() > 0) {     // jika ada input serial

    char cmd = Serial.read();       // baca karakter

    if (cmd == '+') slotTersedia++; // tambah slot
    if (cmd == '-') slotTersedia--; // kurangi slot
    if (cmd == 'r') slotTersedia = kapasitasMaks; // reset slot

    tampilkanStatus();              // update LCD
  }

  delay(50);                        // delay loop
}
