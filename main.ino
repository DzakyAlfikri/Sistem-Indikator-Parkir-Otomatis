#include <Wire.h>                  // Memasukkan library Wire untuk komunikasi I2C
#include <LiquidCrystal_I2C.h>     // Memasukkan library untuk mengontrol layar LCD I2C
#include <Servo.h>                 // Memasukkan library untuk mengendalikan motor servo

LiquidCrystal_I2C lcd(0x20, 16, 2); // Membuat objek 'lcd' dengan alamat I2C 0x20, ukuran 16 kolom x 2 baris

// SERVO
Servo palangMasuk;                 // Mendaftarkan variabel objek servo untuk palang gerbang masuk
Servo palangKeluar;                // Mendaftarkan variabel objek servo untuk palang gerbang keluar

// ULTRASONIK
const int trigIn = 4;              // Menetapkan pin digital 4 sebagai pin Trigger sensor masuk
const int echoIn = 5;              // Menetapkan pin digital 5 sebagai pin Echo sensor masuk
const int trigOut = 6;             // Menetapkan pin digital 6 sebagai pin Trigger sensor keluar
const int echoOut = 7;             // Menetapkan pin digital 7 sebagai pin Echo sensor keluar

// LED
const int ledHijau = 8;            // Menetapkan pin digital 8 untuk lampu LED Hijau (Tersedia)
const int ledMerah = 11;           // Menetapkan pin digital 11 untuk lampu LED Merah (Penuh/Darurat)

// LAMPU OTOMATIS PWM
const int lampu = 3;               // Menetapkan pin PWM 3 untuk lampu penerangan parkir otomatis

// LDR
const int ldrPin = A0;             // Menetapkan pin Analog A0 untuk membaca sensor cahaya (LDR)

// BUZZER
const int buzzer = 12;             // Menetapkan pin digital 12 untuk mengontrol bunyi buzzer/sirine

// TOMBOL
const int tombolDarurat = 2;       // Menetapkan pin 2 (Mendukung Interrupt) untuk tombol mode darurat
const int tombolMasuk = A1;        // Menetapkan pin Analog A1 sebagai input tombol buka/tutup palang masuk manual
const int tombolKeluar = A2;       // Menetapkan pin Analog A2 sebagai input tombol buka/tutup palang keluar manual

// SLOT
int slotTersedia = 5;              // Mendeklarasikan variabel jumlah slot parkir yang kosong saat ini (awal: 5)
int kapasitasMaks = 5;             // Mendeklarasikan batas kapasitas maksimal parkiran yaitu 5 mobil

// MODE DARURAT
volatile bool modeDarurat = false; // Variabel penanda status darurat (volatile karena diakses oleh fungsi Interrupt)
bool sebelumnyaDarurat = false;    // Variabel untuk melacak apakah sistem baru saja keluar dari mode darurat

// TOGGLE MANUAL
bool statusPalangMasuk = false;    // Variabel penyimpan status palang masuk saat digerakkan manual (terbuka/tertutup)
bool statusPalangKeluar = false;   // Variabel penyimpan status palang keluar saat digerakkan manual (terbuka/tertutup)

// BLINK LED
unsigned long lastBlink = 0;       // Variabel penyimpan waktu (milidetik) terakhir kali LED Merah berkedip
bool blinkState = false;           // Variabel status nyala/mati untuk membuat efek kedip pada LED

// SIREN BUZZER
unsigned long lastSiren = 0;       // Variabel penyimpan waktu (milidetik) terakhir kali nada sirine berubah
int sirenFreq = 800;               // Frekuensi awal nada sirine (800 Hz)
bool sirenUp = true;               // Penanda apakah nada sirine sedang naik (melengking) atau turun

// SENSOR FLAG
bool terdeteksiMasuk = false;      // Penanda apakah saat ini ada mobil yang sedang berada di gerbang masuk
bool terdeteksiKeluar = false;     // Penanda apakah saat ini ada mobil yang sedang berada di gerbang keluar

unsigned long waktuBukaMasuk = 0;  // Variabel untuk mencatat kapan (waktu milidetik) palang masuk mulai dibuka
unsigned long waktuBukaKeluar = 0; // Variabel untuk mencatat kapan (waktu milidetik) palang keluar mulai dibuka

const int waktuTahan = 3000;       // Durasi waktu palang terbuka sebelum tertutup otomatis (3000 ms / 3 detik)

// INTERRUPT DARURAT
void darurat(){                    // Fungsi khusus (Interrupt Service Routine) yang dipanggil saat tombol darurat ditekan
  modeDarurat = !modeDarurat;      // Membalikkan status mode darurat (jika False jadi True, jika True jadi False)
}

void setup(){                      // Fungsi inisialisasi yang dijalankan sekali saat Arduino baru dinyalakan

  Serial.begin(9600);              // Membuka jalur komunikasi serial ke laptop dengan kecepatan 9600 bps
  Serial.println("=== SISTEM PARKIR OTOMATIS ==="); // Menampilkan teks judul sistem di Serial Monitor
  Serial.println("System Ready..."); // Menampilkan teks status bahwa sistem sudah siap
  Serial.println("------------------------------"); // Menampilkan garis pembatas

  lcd.init();                      // Menginisialisasi persiapan menyalakan modul LCD
  lcd.backlight();                 // Menyalakan lampu latar (backlight) pada LCD agar tulisan terlihat

  palangMasuk.attach(9);           // Menghubungkan kontrol servo palang masuk ke pin digital 9
  palangKeluar.attach(10);         // Menghubungkan kontrol servo palang keluar ke pin digital 10

  palangMasuk.write(0);            // Menempatkan posisi awal servo masuk di sudut 0 derajat (palang tertutup)
  palangKeluar.write(0);           // Menempatkan posisi awal servo keluar di sudut 0 derajat (palang tertutup)

  pinMode(trigIn, OUTPUT);         // Mengatur pin trigger sensor ultrasonik masuk sebagai Output
  pinMode(echoIn, INPUT);          // Mengatur pin echo sensor ultrasonik masuk sebagai Input
  pinMode(trigOut, OUTPUT);        // Mengatur pin trigger sensor ultrasonik keluar sebagai Output
  pinMode(echoOut, INPUT);         // Mengatur pin echo sensor ultrasonik keluar sebagai Input

  pinMode(ledHijau, OUTPUT);       // Mengatur pin LED Hijau sebagai Output
  pinMode(ledMerah, OUTPUT);       // Mengatur pin LED Merah sebagai Output

  pinMode(lampu, OUTPUT);          // Mengatur pin Lampu parkir otomatis sebagai Output
  pinMode(buzzer, OUTPUT);         // Mengatur pin Buzzer sebagai Output

  pinMode(tombolDarurat, INPUT_PULLUP); // Mengatur pin tombol darurat sebagai Input dengan resistor Pull-Up internal
  pinMode(tombolMasuk, INPUT_PULLUP);   // Mengatur pin tombol manual masuk sebagai Input dengan Pull-Up internal
  pinMode(tombolKeluar, INPUT_PULLUP);  // Mengatur pin tombol manual keluar sebagai Input dengan Pull-Up internal

  // Menghubungkan fungsi 'darurat' ke interupsi pin 2 (Interrupt 0) yang terpicu saat tombol ditekan (FALLING)
  attachInterrupt(digitalPinToInterrupt(2), darurat, FALLING); 

  tampilkanStatus();               // Memanggil fungsi tampilkanStatus() untuk mencetak teks awal ke LCD
}

void tampilkanStatus(){            // Deklarasi fungsi buatan untuk memperbarui tulisan di layar LCD

  lcd.clear();                     // Menghapus seluruh tulisan yang ada di layar LCD saat ini
  lcd.setCursor(0,0);              // Memindahkan kursor LCD ke kolom 0 (paling kiri) dan baris 0 (atas)
  lcd.print("Slot: ");             // Menampilkan kata "Slot: "
  lcd.print(slotTersedia);         // Menampilkan nilai angka dari variabel slotTersedia di sebelah tulisan "Slot: "

  if(slotTersedia <= 0){           // Memeriksa apakah jumlah slot tersisa kurang dari atau sama dengan nol (0)
    lcd.setCursor(0,1);            // Memindahkan kursor LCD ke kolom 0 (paling kiri) dan baris 1 (bawah)
    lcd.print("PARKIR PENUH");     // Menampilkan peringatan "PARKIR PENUH" jika kondisi di atas benar
  } else {                         // Jika kondisi slot masih tersedia (lebih dari 0)
    lcd.setCursor(0,1);            // Memindahkan kursor LCD ke kolom 0 (paling kiri) dan baris 1 (bawah)
    lcd.print("SILAHKAN MASUK");   // Menampilkan pesan selamat datang "SILAHKAN MASUK"
  }
}

long bacaJarak(int trig, int echo){ // Fungsi buatan untuk menghitung jarak pada sensor ultrasonik tertentu

  digitalWrite(trig, LOW);         // Memastikan pin trigger dalam keadaan mati (LOW) terlebih dahulu
  delayMicroseconds(2);            // Menunggu selama 2 mikrodetik agar sinyal stabil

  digitalWrite(trig, HIGH);        // Menyalakan pin trigger (HIGH) untuk mengirimkan gelombang suara ultrasonik
  delayMicroseconds(10);           // Menahan tembakan gelombang suara selama 10 mikrodetik

  digitalWrite(trig, LOW);         // Mematikan kembali pin trigger untuk menghentikan pancaran gelombang

  // Membaca durasi pantulan suara pada pin echo, lalu mengkonversinya menjadi jarak dalam satuan cm
  return pulseIn(echo, HIGH) * 0.034 / 2; 
}

void loop(){                       // Fungsi utama loop() yang akan berjalan berulang-ulang tanpa henti

  // =====================
  // MODE DARURAT
  // =====================
  if(modeDarurat){                 // Memeriksa jika variabel modeDarurat bernilai True (tombol interrupt ditekan)

    if(!sebelumnyaDarurat){        // Memeriksa apakah siklus ini adalah momen pertama kali masuk mode darurat
      lcd.clear();                 // Menghapus layar LCD sebagai persiapan pesan peringatan
      sebelumnyaDarurat = true;    // Mengubah penanda bahwa sistem sudah sepenuhnya berada di dalam siklus darurat
    }

    palangMasuk.write(90);         // Membuka gerbang masuk lebar-lebar (90 derajat) untuk evakuasi
    palangKeluar.write(90);        // Membuka gerbang keluar lebar-lebar (90 derajat) untuk evakuasi

    lcd.setCursor(0,0);            // Mengatur kursor LCD ke baris atas
    lcd.print("MODE DARURAT   ");  // Menampilkan peringatan teks MODE DARURAT
    lcd.setCursor(0,1);            // Mengatur kursor LCD ke baris bawah
    lcd.print("EVAKUASI KELUAR");  // Menampilkan perintah evakuasi

    if(millis() - lastBlink > 300){ // Mengecek apakah waktu berlalu sudah lebih dari 300 ms sejak kedipan terakhir
      lastBlink = millis();        // Mereset catatan waktu kedipan terakhir dengan waktu saat ini
      blinkState = !blinkState;    // Membalikkan status kedipan LED (jika mati jadi nyala, sebaliknya)
    }

    digitalWrite(ledMerah, blinkState); // Mengirim sinyal berkedip (HIGH/LOW bergantian) ke LED Merah
    digitalWrite(ledHijau, LOW);   // Memastikan LED Hijau dalam keadaan mati sepenuhnya

    if(millis() - lastSiren > 80){ // Mengecek apakah waktu berlalu sudah 80 ms sejak perubahan nada sirine terakhir
      lastSiren = millis();        // Mereset catatan waktu perubahan sirine dengan waktu saat ini

      if(sirenUp){                 // Jika status sirine sedang menaik (nada melengking ke atas)
        sirenFreq += 30;           // Tambahkan frekuensi nada sebesar 30 Hz
        if(sirenFreq >= 2000) sirenUp = false; // Jika frekuensi sudah mencapai batas atas (2000Hz), ubah arah menjadi turun
      } else {                     // Jika status sirine sedang menurun (nada merendah)
        sirenFreq -= 30;           // Kurangi frekuensi nada sebesar 30 Hz
        if(sirenFreq <= 600) sirenUp = true;  // Jika frekuensi menyentuh batas bawah (600Hz), ubah arah menjadi naik lagi
      }

      tone(buzzer, sirenFreq);     // Membunyikan buzzer dengan frekuensi nada yang baru saja dihitung
    }

    Serial.println("!!! MODE DARURAT AKTIF !!!"); // Mengirim notifikasi bahaya ke Serial Monitor laptop secara terus-menerus

    return;                        // Menghentikan sisa kode di bawahnya dan kembali ke awal loop() selama mode darurat aktif
  }

  // Jika kode sampai di sini, berarti mode darurat TIDAK aktif (false)
  if(sebelumnyaDarurat && !modeDarurat){ // Memeriksa jika kondisi darurat *baru saja* dimatikan oleh pengguna

    sebelumnyaDarurat = false;     // Mengubah penanda bahwa sistem sudah tidak lagi dalam status pemulihan darurat

    slotTersedia = kapasitasMaks;  // Mengembalikan jumlah slot parkir seperti keadaan kosong (kapasitas penuh)

    palangMasuk.write(0);          // Menutup gerbang masuk kembali (kembali ke 0 derajat)
    palangKeluar.write(0);         // Menutup gerbang keluar kembali (kembali ke 0 derajat)

    noTone(buzzer);                // Mematikan suara sirine dari buzzer secara total

    lcd.clear();                   // Membersihkan pesan evakuasi dari layar LCD

    Serial.println("MODE DARURAT DIMATIKAN"); // Mengirim log pemberitahuan ke Serial Monitor bahwa darurat usai
    Serial.println("SISTEM KEMBALI NORMAL");  // Mengirim log pemberitahuan ke Serial Monitor
    Serial.print("Slot reset: ");             // Mengirim tulisan informasi ke Serial Monitor
    Serial.println(slotTersedia);             // Mencetak angka kapasitas slot terbaru ke Serial Monitor

    tampilkanStatus();             // Memanggil ulang fungsi untuk menampilkan status parkiran reguler di LCD
  }

  if(slotTersedia <= 0){           // Memeriksa kondisi apakah tempat parkir saat ini sudah penuh (0 slot)
    digitalWrite(ledMerah, HIGH);  // Menyalakan lampu indikator LED Merah
    digitalWrite(ledHijau, LOW);   // Mematikan lampu indikator LED Hijau
  } else {                         // Memeriksa kondisi apabila masih ada tempat parkir (slot > 0)
    digitalWrite(ledMerah, LOW);   // Mematikan lampu indikator LED Merah
    digitalWrite(ledHijau, HIGH);  // Menyalakan lampu indikator LED Hijau sebagai tanda boleh masuk
  }

  if(digitalRead(tombolMasuk) == LOW){ // Memeriksa apakah tombol palang masuk manual sedang ditekan (LOW karena Pull-Up)
    delay(200);                    // Menunda sedikit waktu (debouncing) untuk mencegah satu tekanan terbaca berulang kali

    statusPalangMasuk = !statusPalangMasuk; // Membalik nilai status gerbang masuk (dari tertutup ke terbuka, atau sebaliknya)
    if(statusPalangMasuk){         // Jika status sekarang berubah menjadi ingin dibuka
      palangMasuk.write(90);       // Menggerakkan servo palang masuk ke posisi terbuka (90 derajat)
      Serial.println("Palang Masuk Dibuka Manual"); // Memberi log aktivitas manual di Serial Monitor
    }
    else{                          // Jika status sekarang berubah menjadi ingin ditutup
      palangMasuk.write(0);        // Menggerakkan servo palang masuk ke posisi tertutup (0 derajat)
      Serial.println("Palang Masuk Ditutup Manual"); // Memberi log aktivitas manual di Serial Monitor
    }
  }

  if(digitalRead(tombolKeluar) == LOW){ // Memeriksa apakah tombol palang keluar manual sedang ditekan
    delay(200);                    // Menunda sedikit waktu (debouncing) mencegah pembacaan ganda

    statusPalangKeluar = !statusPalangKeluar; // Membalik nilai status gerbang keluar manual
    if(statusPalangKeluar){        // Jika gerbang keluar dikehendaki untuk terbuka
      palangKeluar.write(90);      // Menggerakkan servo palang keluar ke sudut 90 derajat
      Serial.println("Palang Keluar Dibuka Manual"); // Mencetak laporan tindakan ke Serial Monitor
    }
    else{                          // Jika gerbang keluar dikehendaki untuk ditutup
      palangKeluar.write(0);       // Menggerakkan servo palang keluar ke sudut 0 derajat
      Serial.println("Palang Keluar Ditutup Manual"); // Mencetak laporan tindakan ke Serial Monitor
    }
  }

  int nilaiLDR = analogRead(ldrPin); // Membaca tegangan analog dari sensor cahaya LDR dan merubahnya ke angka (0-1023)
  // Menyalakan lampu dengan kecerahan penuh (255) jika gelap (nilai < 300), atau matikan (0) jika terang
  analogWrite(lampu, (nilaiLDR < 300) ? 255 : 0); 

  long jarakMasuk = bacaJarak(trigIn, echoIn); // Memanggil fungsi untuk membaca jarak mobil di depan gerbang masuk

  if(jarakMasuk < 10 && !terdeteksiMasuk && slotTersedia > 0){ // Jika jarak <10cm, mobil baru tiba, dan slot belum penuh

    terdeteksiMasuk = true;        // Menandai bahwa ada mobil yang sedang diproses di gerbang masuk
    waktuBukaMasuk = millis();     // Mencatat waktu milidetik saat palang mulai dibuka

    palangMasuk.write(90);         // Menggerakkan servo masuk ke 90 derajat (membuka palang)
    slotTersedia--;                // Mengurangi kuota slot yang tersedia sebanyak 1 mobil

    Serial.println("Mobil Masuk"); // Memberitahukan PC bahwa ada mobil yang masuk
    Serial.print("Slot tersisa: ");// Mencetak kalimat sisa slot
    Serial.println(slotTersedia);  // Mencetak angka sisa slot setelah dikurangi

    tampilkanStatus();             // Memperbarui layar LCD agar menampilkan jumlah slot yang baru
  }

  // Jika mobil sudah terdeteksi dan waktu buka palang sudah melebihi 'waktuTahan' (3 detik)
  if(terdeteksiMasuk && millis() - waktuBukaMasuk > waktuTahan){ 
    terdeteksiMasuk = false;       // Mereset status sensor masuk agar siap menerima mobil berikutnya
    palangMasuk.write(0);          // Mengembalikan posisi servo ke 0 derajat (menutup palang secara otomatis)
  }

  long jarakKeluar = bacaJarak(trigOut, echoOut); // Memanggil fungsi untuk membaca jarak mobil di depan gerbang keluar

  if(jarakKeluar < 10 && !terdeteksiKeluar && slotTersedia < kapasitasMaks){ // Jika mobil <10cm, baru terdeteksi keluar, & slot tidak lebih dari maksimal

    terdeteksiKeluar = true;       // Menandai bahwa ada mobil yang sedang keluar dari area parkir
    waktuBukaKeluar = millis();    // Mencatat waktu milidetik saat palang keluar dibuka

    palangKeluar.write(90);        // Menggerakkan servo keluar ke 90 derajat (membuka jalan)
    slotTersedia++;                // Menambahkan 1 kuota kosong kembali ke sistem parkir

    Serial.println("Mobil Keluar");// Memberitahukan PC bahwa ada mobil yang baru keluar
    Serial.print("Slot tersisa: ");// Mencetak teks informasi status slot
    Serial.println(slotTersedia);  // Mencetak sisa slot yang telah ditambahkan

    tampilkanStatus();             // Memperbarui layar LCD untuk mencetak angka slot yang baru bertambah
  }

  // Jika ada mobil yang sedang keluar dan batas waktu buka palang (3 detik) sudah terlewati
  if(terdeteksiKeluar && millis() - waktuBukaKeluar > waktuTahan){
    terdeteksiKeluar = false;      // Mereset penanda bahwa proses keluar mobil sudah selesai
    palangKeluar.write(0);         // Menggerakkan servo keluar ke 0 derajat untuk menutup jalan kembali
  }

  // =========================
  // UPDATE SLOT MANUAL VIA SERIAL
  // =========================
  if (Serial.available() > 0) {    // Memeriksa apakah ada data karakter yang dikirimkan oleh user dari laptop/PC
  char cmd = Serial.read();        // Membaca satu karakter pertama yang dikirim dan menyimpannya di variabel 'cmd'

  if (cmd == '+') {                // Jika karakter yang diketik pengguna adalah simbol plus (+)
    if (slotTersedia < kapasitasMaks) { // Mengecek apakah slot yang ada masih di bawah batas maksimal (5)
      slotTersedia++;              // Menambah jumlah slot tersedia secara manual sebanyak 1
      Serial.println("Slot ditambah manual (+1)"); // Memberikan respon log ke layar komputer
    } else {                       // Jika slot sudah menyentuh batas maksimum
      Serial.println("Slot sudah maksimal");       // Memberi peringatan ke PC bahwa slot tidak bisa ditambah lagi
    }
  }

  if (cmd == '-') {                // Jika karakter yang diketik pengguna adalah simbol minus (-)
    if (slotTersedia > 0) {        // Mengecek apakah slot yang tersedia masih lebih dari 0
      slotTersedia--;              // Mengurangi jumlah slot secara manual sebanyak 1
      Serial.println("Slot dikurangi manual (-1)");// Memberikan respon log ke layar komputer
    } else {                       // Jika slot sudah habis (0)
      Serial.println("Slot sudah 0");              // Memberi peringatan ke PC bahwa angka tidak bisa bernilai negatif
    }
  }

  if (cmd == 'r') {                // Jika karakter yang diketik pengguna adalah huruf 'r' (untuk reset)
    slotTersedia = kapasitasMaks;  // Mengembalikan nilai slot yang tersedia ke batas maksimal (5)
    Serial.println("Slot direset ke maksimal");    // Mengirim pemberitahuan ke Serial Monitor bahwa reset berhasil
  }

  Serial.print("Slot saat ini: "); // Mencetak teks awal laporan kondisi terbaru
  Serial.println(slotTersedia);    // Mencetak angka pasti slot setelah diubah oleh pengguna via Serial Monitor

  tampilkanStatus();               // Memperbarui informasi pada fisik layar LCD sesuai perintah dari Serial
  }

  delay(50);                       // Memberikan jeda singkat selama 50 milidetik pada akhir loop untuk menstabilkan kinerja mikrokontroler
}
