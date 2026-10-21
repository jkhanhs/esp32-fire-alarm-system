// Cấu hình Blynk 
#define BLYNK_TEMPLATE_ID "TMPL6XtLFu5-x"
#define BLYNK_TEMPLATE_NAME "HỆ THỐNG GIÁM SÁT VÀ BÁO CHÁY TỰ ĐỘNG"
#define BLYNK_AUTH_TOKEN "O-hfsMc_R0-m_OHQ6e8tZ-fMBoIKjrvC"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// --- THÔNG TIN WIFI ---
const char ssid[] = "Avramax";
const char pass[] = "12345678";

#define DHT_PIN       4
#define DHT_TYPE      DHT11 
#define SMOKE_PIN     0   
#define FLAME_PIN     5    
#define BUZZER_PIN    8    
#define LED_PIN       9    

#define TEMP_WARNING_THRESHOLD  50.0  
#define TEMP_FIRE_THRESHOLD     65.0  
#define SMOKE_WARNING_THRESHOLD 1500  
#define SMOKE_FIRE_THRESHOLD    2500  

BlynkTimer timer;

class TempSensor {
  private:
    DHT dht;
    float currentTemp = 0.0;
  public:
    TempSensor(int pin, uint8_t type) : dht(pin, type) {}
    void init() {
      dht.begin();
    }
    float readValue() {
      float t = dht.readTemperature();
      if (!isnan(t)) {
        currentTemp = t;
      }
      return currentTemp;
    }
};

class SmokeSensor {
  private:
    int pin;
  public:
    SmokeSensor(int p) { pin = p; }
    void init() {
      pinMode(pin, INPUT);
    }
    int readValue() {
      long total = 0;
      for(int i = 0; i < 10; i++) {
        total += analogRead(pin);
        delay(1);
      }
      return total / 10;
    }
};

class FlameSensor {
  private:
    int pin;
  public:
    FlameSensor(int p) { pin = p; }
    void init() {
      pinMode(pin, INPUT);
    }
    bool isFlamePresent() {
      return digitalRead(pin) == HIGH; 
    }
};

class AlarmSystem {
  private:
    int buzzerPin;
    int ledPin;
    bool isMuted;
  public:
    AlarmSystem(int b, int l) {
      buzzerPin = b;
      ledPin = l;
      isMuted = false;
    }
    void init() {
      pinMode(buzzerPin, OUTPUT);
      pinMode(ledPin, OUTPUT);
      deactivate();
    }
    void activate() {
      digitalWrite(ledPin, HIGH);
      if (!isMuted) {
        digitalWrite(buzzerPin, HIGH); 
      }
    }
    void deactivate() {
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, LOW);
    }
    void muteBuzzer(bool state) {
      isMuted = state;
      if (isMuted) digitalWrite(buzzerPin, LOW);
    }
};

TempSensor myTempSensor(DHT_PIN, DHT_TYPE);
SmokeSensor mySmokeSensor(SMOKE_PIN);
FlameSensor myFlameSensor(FLAME_PIN);
AlarmSystem myAlarm(BUZZER_PIN, LED_PIN);

void checkLogicAndSendData() {
  float temp = myTempSensor.readValue();
  int smoke = mySmokeSensor.readValue();
  bool hasFlame = myFlameSensor.isFlamePresent();
  String systemState = "AN TOÀN";

  if (hasFlame == true) {
    systemState = "CÓ CHÁY! (LỬA)";
    myAlarm.activate();
  } 
  else if (temp > TEMP_FIRE_THRESHOLD && smoke > SMOKE_FIRE_THRESHOLD) {
    systemState = "CÓ CHÁY! (KHÓI & NHIỆT)";
    myAlarm.activate();
  }
  else if (temp > TEMP_WARNING_THRESHOLD || smoke > SMOKE_WARNING_THRESHOLD) {
    systemState = "CẢNH BÁO";
    myAlarm.deactivate(); 
  }
  else {
    systemState = "AN TOÀN";
    myAlarm.deactivate();
  }

  Serial.print("Nhiệt độ: "); Serial.print(temp);
  Serial.print(" | Khói: "); Serial.print(smoke);
  Serial.print(" | Lửa: "); Serial.print(hasFlame);
  Serial.print(" => "); Serial.println(systemState);

  // Gửi dữ liệu lên Server Blynk
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, smoke);
  Blynk.virtualWrite(V2, hasFlame ? 1 : 0);
  Blynk.virtualWrite(V3, systemState);
}

// Hàm nhận lệnh từ ứng dụng Blynk
BLYNK_WRITE(V4) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    myAlarm.muteBuzzer(true); 
    Serial.println("Đã TẮT còi từ App!");
  } else {
    myAlarm.muteBuzzer(false); 
    Serial.println("Đã BẬT LẠI còi từ App!");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("He thong bat dau khoi dong...");
  
  myTempSensor.init();
  mySmokeSensor.init();
  myFlameSensor.init();
  myAlarm.init();

  // Kết nối WiFi và Blynk Server
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Cài đặt Timer: 1 giây (1000ms) thì chạy hàm đọc cảm biến 1 lần
  timer.setInterval(1000L, checkLogicAndSendData);
}

void loop() {
  Blynk.run();
  timer.run();
}