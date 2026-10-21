#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

// --- CẤU HÌNH TÊN WI-FI MẠCH SẼ PHÁT RA ---
const char* ap_ssid     = "He_Thong_Bao_Chay";
// Tên Wi-Fi bạn thích đặt là gì cũng được
const char* ap_password = "12345678";
// Mật khẩu (phải từ 8 ký tự trở lên)

// --- ĐỊNH NGHĨA CHÂN KẾT NỐI MẠCH ---
#define DHTPIN       6
#define FLAME_PIN    4
#define MQ2_PIN      2
#define BUZZER_PIN   5
#define LED_PIN      7

#define DHTTYPE      DHT11 

const float TEMP_THRESHOLD = 50.0;
const int SMOKE_THRESHOLD  = 1300;  

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

float temp = 0.0, humid = 0.0;
int smokeVal = 0;
bool isFlame = false;
String sysStatus = "Hệ thống sẵn sàng";

// --- GIAO DIỆN WEB HTML (Giữ nguyên giao diện AJAX mượt mà) ---
const char HTML_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hệ Thống Giám Sát Phòng Cháy IoT</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f4f6f9; margin: 0; padding: 20px; text-align: center; color: #333; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); }
        h1 { color: #2c3e50; margin-bottom: 5px; text-transform: uppercase; font-size: 24px; }
        .status-banner { padding: 15px; font-size: 22px; font-weight: bold; border-radius: 8px; margin: 20px 0; color: white; transition: 0.3s; }
        .safe { background-color: #2ecc71; box-shadow: 0 4px 10px rgba(46, 204, 113, 0.4); }
        .danger { background-color: #e74c3c; animation: blink 1s infinite; box-shadow: 0 4px 10px rgba(231, 76, 60, 0.4); }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 20px; margin-top: 25px; }
        .card { background: #fff; padding: 20px; border-radius: 10px; border: 1px solid #e1e8ed; box-shadow: 0 2px 5px rgba(0,0,0,0.03); }
        .card h3 { margin: 0 0 10px 0; color: #7f8c8d; font-size: 16px; }
        .card .value { font-size: 28px; font-weight: bold; color: #2c3e50; }
        .card .unit { font-size: 16px; color: #95a5a6; }
        @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0.7; } 100% { opacity: 1; } }
    </style>
</head>
<body>
    <div class="container">
        <h1>Hệ Thống Giám Sát & Báo Cháy Tự Động</h1>
        <p style="color: #7f8c8d; margin: 0;">Chế độ: Wi-Fi Access Point trực tiếp</p>
        
        <div id="sys_banner" class="status-banner safe">Đang tải dữ liệu...</div>
        
        <div class="grid">
            <div class="card">
                <h3>Nhiệt Độ</h3>
                <div class="value"><span id="temp">--</span><span class="unit"> °C</span></div>
            </div>
            <div class="card">
                <h3>Độ Ẩm</h3>
                <div class="value"><span id="humid">--</span><span class="unit"> %</span></div>
            </div>
            <div class="card">
                <h3>Nồng Độ Khói</h3>
                <div class="value"><span id="smoke">--</span><span class="unit"> Raw</span></div>
            </div>
            <div class="card">
                <h3>Cảm Biến Lửa</h3>
                <div class="value" id="flame" style="font-size: 20px; margin-top:10px;">--</div>
            </div>
        </div>
    </div>

    <script>
        setInterval(function() {
            fetch('/data')
            .then(response => response.json())
            .then(data => {
                document.getElementById('temp').innerText = data.temperature;
                document.getElementById('humid').innerText = data.humidity;
                document.getElementById('smoke').innerText = data.smoke;
                
                if(data.flame === "PHÁT HIỆN LỬA") {
                    document.getElementById('flame').innerText = "🚨 CÓ LỬA!";
                    document.getElementById('flame').style.color = "#e74c3c";
                } else {
                    document.getElementById('flame').innerText = "✅ Bình thường";
                    document.getElementById('flame').style.color = "#2ecc71";
                }

                let banner = document.getElementById('sys_banner');
                banner.innerText = data.status;
                if(data.is_alarm) {
                    banner.className = "status-banner danger";
                } else {
                    banner.className = "status-banner safe";
                }
            })
            .catch(err => console.error("Lỗi lấy dữ liệu:", err));
        }, 1000);
    </script>
</body>
</html>
)=====";

void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void handleData() {
  bool isAlarmActive = (isFlame || smokeVal > SMOKE_THRESHOLD || temp > TEMP_THRESHOLD);
  
  String json = "{";
  json += "\"temperature\":" + String(isnan(temp) ? 0 : temp, 1) + ",";
  json += "\"humidity\":" + String(isnan(humid) ? 0 : humid, 1) + ",";
  json += "\"smoke\":" + String(smokeVal) + ",";
  json += "\"flame\":\"" + String(isFlame ? "PHÁT HIỆN LỬA" : "AN TOÀN") + "\",";
  json += "\"status\":\"" + sysStatus + "\",";
  json += "\"is_alarm\":" + String(isAlarmActive ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json; charset=utf-8", json);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  dht.begin();
  pinMode(FLAME_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n-------------------------------------------");
  Serial.println("ĐANG CẤU HÌNH MẠCH PHÁT WI-FI (ACCESS POINT)...");
  
  // 💡 BẢN CẬP NHẬT CHO ESP32-C3 MINI: Xử lý delay ngăn xếp mạng và hạ công suất
  WiFi.mode(WIFI_AP);
  delay(100); // Thêm thời gian cho ngăn xếp Wi-Fi sẵn sàng
  
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // Giới hạn công suất phát để chống sụt áp nguồn
  
  WiFi.softAP(ap_ssid, ap_password);
  delay(500); // Đợi module vô tuyến (RF) bật lên ổn định

  Serial.println("🎉 WI-FI ĐÃ PHÁT THÀNH CÔNG!");
  Serial.print("📡 Tên Wi-Fi (SSID): "); Serial.println(ap_ssid);
  Serial.print("🔑 Mật khẩu: "); Serial.println(ap_password);
  
  // Địa chỉ IP mặc định ở chế độ AP của ESP32 luôn là 192.168.4.1
  Serial.print("🌐 Địa chỉ Web điều khiển: http://");
  Serial.println(WiFi.softAPIP()); 

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long lastMeasure = 0;
  if (millis() - lastMeasure > 1000) { 
    lastMeasure = millis();

    temp = dht.readTemperature();
    humid = dht.readHumidity();
    smokeVal = analogRead(MQ2_PIN);
    isFlame = (digitalRead(FLAME_PIN) == LOW); 

    if (isFlame || smokeVal > SMOKE_THRESHOLD || temp > TEMP_THRESHOLD) {
      digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      sysStatus = "🚨 NGUY HIỂM: PHÁT HIỆN TÍN HIỆU HOẢ HOẠN!";
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      sysStatus = "✅ Hệ thống đang hoạt động an toàn";
    }
  }
}