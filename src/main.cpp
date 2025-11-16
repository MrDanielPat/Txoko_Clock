#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#include "secrets.h"


// Mapeo recomendado para WeAct 4.2" con ESP32
// VCC  -> 3.3V
// GND  -> GND
// SDA  -> GPIO23
// SCL  -> GPIO18
// CS   -> GPIO5
// DC   -> GPIO17
// RST  -> GPIO16
// BUSY -> GPIO4


// Zona horaria España peninsular
const char* TZ_INFO = "CET-1CEST,M3.5.0/2,M10.5.0/3";

// Pines WeAct 4.2 con ESP32
#define EPD_CS   5
#define EPD_DC   17
#define EPD_RST  16
#define EPD_BUSY 4

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> display(
    GxEPD2_420_GYE042A87(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// estado
int ultimoMinuto = -1;

void conectarWifi() {
    Serial.print("Conectando a ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 40) {
        delay(500);
        Serial.print(".");
        intentos++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi conectado IP ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("No se pudo conectar a WiFi");
    }
}

void configurarHoraNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Sin WiFi no se configura NTP");
        return;
    }

    Serial.println("Configurando NTP");

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", TZ_INFO, 1);
    tzset();

    struct tm timeinfo;
    int reintentos = 0;
    while (!getLocalTime(&timeinfo) && reintentos < 20) {
        Serial.println("Esperando hora NTP");
        delay(500);
        reintentos++;
    }

    if (!getLocalTime(&timeinfo)) {
        Serial.println("No se pudo obtener hora NTP");
        return;
    }

    Serial.println("Hora NTP configurada");
}

bool obtenerHora(struct tm& timeinfo) {
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Error al leer hora local");
        return false;
    }
    return true;
}

void dibujarHora(String h) {
    display.setRotation(4);
    display.setTextColor(GxEPD_BLACK);

    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

        display.setFont(&FreeSansBold24pt7b);

        int16_t x1, y1;
        uint16_t w, htxt;
        display.getTextBounds(h.c_str(), 0, 0, &x1, &y1, &w, &htxt);

        int16_t x = (display.width()  - w) / 2;
        int16_t y = (display.height() + htxt) / 2;

        display.setCursor(x, y);
        display.print(h);

    } while (display.nextPage());

    Serial.print("Hora dibujada completa ");
    Serial.println(h);
}

void actualizarHoraParcial(String h) {
    display.setRotation(4);
    display.setTextColor(GxEPD_BLACK);

    display.setFont(&FreeSansBold24pt7b);
    int16_t x1, y1;
    uint16_t w, htxt;
    display.getTextBounds(h.c_str(), 0, 0, &x1, &y1, &w, &htxt);

    int16_t x = (display.width()  - w) / 2;
    int16_t y = (display.height() + htxt) / 2;

    int16_t margen = 10;
    int16_t px = x - margen;
    int16_t py = y - htxt - margen;
    uint16_t pw = w + margen * 2;
    uint16_t ph = htxt + margen * 2;

    if (px < 0) px = 0;
    if (py < 0) py = 0;
    if (px + pw > display.width())  pw  = display.width()  - px;
    if (py + ph > display.height()) ph = display.height() - py;

    display.setPartialWindow(px, py, pw, ph);

    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(x, y);
        display.print(h);
    } while (display.nextPage());

    Serial.print("Hora actualizada parcial ");
    Serial.println(h);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Txoko Clock NTP reloj ePaper");

    display.init(115200);

    conectarWifi();
    configurarHoraNTP();

    struct tm t;
    if (obtenerHora(t)) {
        char buffer[6];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", t.tm_hour, t.tm_min);
        String horaStr = String(buffer);
        dibujarHora(horaStr);
        ultimoMinuto = t.tm_min;
    }
}

void loop() {
    struct tm t;
    if (!obtenerHora(t)) {
        delay(5000);
        return;
    }

    if (t.tm_min != ultimoMinuto) {
        ultimoMinuto = t.tm_min;

        char buffer[6];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", t.tm_hour, t.tm_min);
        String horaStr = String(buffer);

        actualizarHoraParcial(horaStr);
    }

    delay(5000);
}