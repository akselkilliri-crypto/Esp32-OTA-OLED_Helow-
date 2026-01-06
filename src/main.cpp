#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === НАСТРОЙКИ WI-FI (ОБЯЗАТЕЛЬНО ИЗМЕНИТЬ!) ===
const char* ssid = "MyWiFi";        // ← ЗАМЕНИТЕ НА ВАШУ СЕТЬ!
const char* password = "12345678";  // ← ЗАМЕНИТЕ НА ВАШ ПАРОЛЬ!
// ==============================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32 Hello + OTA ===");

  // Инициализация OLED
  Wire.begin(21, 22); // SDA, SCL
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED не найден!");
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 20);
    display.print("Sedr");
    display.display();
  }

  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Подключение к Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nУспешно! IP: " + WiFi.localIP().toString());

  // OTA-сервер
  server.on("/", HTTP_GET, []() {
    String html = "<h2>ESP32 OTA</h2>";
    html += "<p>Текущая прошивка: Hello + OLED</p>";
    html += "<form method='POST' enctype='multipart/form-data' action='/update'>";
    html += "<input type='file' name='firmware' accept='.bin' required>";
    html += "<input type='submit' value='Прошить'>";
    html += "</form>";
    server.send(200, "text/html", html);
  });

  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", Update.hasError() ? "Ошибка" : "OK! Перезагрузка...");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START) Update.begin();
    else if(upload.status == UPLOAD_FILE_WRITE) Update.write(upload.buf, upload.currentSize);
    else if(upload.status == UPLOAD_FILE_END) Update.end(true);
  });

  server.begin();
}

void loop() {
  server.handleClient();
  delay(1); // Важно для стабильности OTA

}
