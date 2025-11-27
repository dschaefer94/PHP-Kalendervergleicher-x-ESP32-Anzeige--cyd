#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

const char* ssid = ""; //WLAN Name einfügen
const char* password = ""; //WLAN Passwort einfügen

TFT_eSPI tft = TFT_eSPI();

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
    HTTPClient http;
    http.begin("/unterschiede.json"); // IP anpassen
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();

      StaticJsonDocument<8192> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
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
        generiereSlides();
      }
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  ladeEintraege();
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

