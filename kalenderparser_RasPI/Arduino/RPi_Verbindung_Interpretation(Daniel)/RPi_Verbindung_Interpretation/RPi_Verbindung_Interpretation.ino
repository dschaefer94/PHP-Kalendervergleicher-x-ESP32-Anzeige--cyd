#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

const char* ssid = "bib-FHDW-PSK"; //WLAN Name einfügen
const char* password = "Ezb527+#+Z"; //WLAN Passwort einfügen

TFT_eSPI tft = TFT_eSPI();

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

struct Eintrag {
  String type;
  String summary;
  String alt_summary;
  String start;
};

#define MAX_EINTRAEGE 50
Eintrag eintraege[MAX_EINTRAEGE];
int anzahlEintraege = 0;

struct Slide {
  String type;
  int startIndex;
  int count;
};

#define MAX_SLIDES 10
Slide slides[MAX_SLIDES];
int anzahlSlides = 0;

const int entryHeight = 80;

int currentSlide = 0;

unsigned long lastLoadTime = 0;
unsigned long lastSlideTime = 0;
const int slideDuration = 5000;

void printDateTime(const char* datetime) {
  if (strlen(datetime) < 15) {
    tft.print("Unbekannt");
    return;
  }
  char year[5], month[3], day[3], hour[3], minute[3], second[3];

  strncpy(year, datetime, 4);
  year[4] = '\0';
  strncpy(month, datetime + 4, 2);
  month[2] = '\0';
  strncpy(day, datetime + 6, 2);
  day[2] = '\0';
  strncpy(hour, datetime + 9, 2);
  hour[2] = '\0';
  strncpy(minute, datetime + 11, 2);
  minute[2] = '\0';
  strncpy(second, datetime + 13, 2);
  second[2] = '\0';

  tft.printf("%s.%s.%s %s:%s:%s", day, month, year, hour, minute, second);
}

void zeichneEintrag(int index, int y) {
  if (index < 0 || index >= anzahlEintraege) return;

  Eintrag& e = eintraege[index];
  tft.setTextSize(2);

  uint16_t bgColor;
  if (e.type == "neu") bgColor = TFT_GREEN;
  else if (e.type == "geaendert") bgColor = TFT_YELLOW;
  else if (e.type == "geloescht") bgColor = TFT_RED;
  else bgColor = TFT_BLACK;

  tft.fillRect(0, y, tft.width(), entryHeight, bgColor);
  tft.setTextColor(TFT_BLACK, bgColor);
  tft.setCursor(0, y);

  if (e.type == "geaendert") {
    tft.println("Geändert:");

    // alt_summary mit Durchstrich
    int xStart = tft.getCursorX();
    int yStart = tft.getCursorY();
    tft.print(e.alt_summary);

    int textWidth = tft.textWidth(e.alt_summary);
    int lineY = yStart + 8; // Linie etwas unter dem Text
    tft.drawLine(xStart, lineY, xStart + textWidth, lineY, TFT_BLACK);

    tft.println();
    tft.printf("Neu: %s\n", e.summary.c_str());
  } else {
    tft.println(e.summary);
  }
  printDateTime(e.start.c_str());
}

void generiereSlides() {
  anzahlSlides = 0;
  if (anzahlEintraege == 0) return;

  String aktuellerTyp = eintraege[0].type;
  int startIndex = 0;
  int count = 1;

  for (int i = 1; i < anzahlEintraege; i++) {
    if (eintraege[i].type == aktuellerTyp) {
      count++;
    } else {
      if (anzahlSlides < MAX_SLIDES) {
        slides[anzahlSlides].type = aktuellerTyp;
        slides[anzahlSlides].startIndex = startIndex;
        slides[anzahlSlides].count = count;
        anzahlSlides++;
      }
      aktuellerTyp = eintraege[i].type;
      startIndex = i;
      count = 1;
    }
  }
  if (anzahlSlides < MAX_SLIDES) {
    slides[anzahlSlides].type = aktuellerTyp;
    slides[anzahlSlides].startIndex = startIndex;
    slides[anzahlSlides].count = count;
    anzahlSlides++;
  }
}

void ladeEintraege() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WLAN verbunden, versuche JSON zu laden...");

    HTTPClient http;
    http.begin("http://172.18.75.38/unterschiede.json"); // IP anpassen
    int httpCode = http.GET();

    Serial.print("HTTP-Code: ");
    Serial.println(httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("JSON empfangen:");
      Serial.println(payload);

      StaticJsonDocument<8192> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        Serial.println("JSON erfolgreich geparst.");
        JsonObject root = doc.as<JsonObject>();
        anzahlEintraege = 0;
        for (JsonPair kv : root) {
          if (anzahlEintraege >= MAX_EINTRAEGE) break;
          JsonObject obj = kv.value().as<JsonObject>();
          eintraege[anzahlEintraege].type = obj["type"] | "";
          eintraege[anzahlEintraege].summary = obj["summary"] | obj["neu_summary"] | "";
          eintraege[anzahlEintraege].alt_summary = obj["alt_summary"] | "";
          eintraege[anzahlEintraege].start = obj["start"] | obj["alt_start"] | "";
          anzahlEintraege++;
        }
        Serial.print("Einträge geladen: ");
        Serial.println(anzahlEintraege);
        generiereSlides();
      } else {
        Serial.print("Fehler beim Parsen der JSON: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.println("Fehler beim Abrufen der JSON-Datei.");
    }
    http.end();
  } else {
    Serial.println("WLAN nicht verbunden.");
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  Serial.println("Test im Setup");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("."); // zeigt Verbindungsfortschritt
  }

  Serial.println("\nVerbunden mit WLAN!");
  Serial.print("ESP32 IP-Adresse: ");
  Serial.println(WiFi.localIP()); // ← zeigt die IP im Serial Monitor

  ladeEintraege();

  
tft.setTextSize(2);
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.setCursor(0, 0);
tft.println("Testanzeige");

}

void loop() {
  unsigned long now = millis();

  static unsigned long lastLoadTime = 0;
  if (now - lastLoadTime > 10000) {
    ladeEintraege();
    lastLoadTime = now;
  }

  static unsigned long lastSlideTime = 0;
  if (now - lastSlideTime > slideDuration) {
    lastSlideTime = now;
    tft.fillScreen(TFT_BLACK);

    if (anzahlSlides > 0) {
      Slide& slide = slides[currentSlide];
      for (int i = 0; i < slide.count; i++) {
        zeichneEintrag(slide.startIndex + i, i * entryHeight);
      }

      currentSlide++;
      if (currentSlide >= anzahlSlides) currentSlide = 0;
    }
  }
}

