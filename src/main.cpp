#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

#include "secrets.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

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
const char *TZ_INFO = "CET-1CEST,M3.5.0/2,M10.5.0/3";

// Pines WeAct 4.2 con ESP32
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> display(
    GxEPD2_420_GYE042A87(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// estado
int ultimoMinuto = -1;

float obtenerPrecioBitcoin()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("No hay WiFi para obtener BTC");
    return -1;
  }

  HTTPClient http;
  http.begin("https://api.coinbase.com/v2/prices/BTC-EUR/spot");

  int httpCode = http.GET();
  if (httpCode != 200)
  {
    Serial.print("Error HTTP BTC: ");
    Serial.println(httpCode);
    http.end();
    return -1;
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    Serial.print("Error parseando JSON BTC: ");
    Serial.println(error.c_str());
    return -1;
  }

  const char *amountStr = doc["data"]["amount"]; // viene como texto
  if (!amountStr)
  {
    Serial.println("Campo amount no encontrado en JSON BTC");
    return -1;
  }

  float precio = atof(amountStr);
  Serial.print("Precio BTC (EUR): ");
  Serial.println(precio);

  return precio;
}

void conectarWifi()
{
  Serial.print("Conectando a ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 40)
  {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi conectado IP ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("No se pudo conectar a WiFi");
  }
}

void configurarHoraNTP()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Sin WiFi no se configura NTP");
    return;
  }

  Serial.println("Configurando NTP");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", TZ_INFO, 1);
  tzset();

  struct tm timeinfo;
  int reintentos = 0;
  while (!getLocalTime(&timeinfo) && reintentos < 20)
  {
    Serial.println("Esperando hora NTP");
    delay(500);
    reintentos++;
  }

  if (!getLocalTime(&timeinfo))
  {
    Serial.println("No se pudo obtener hora NTP");
    return;
  }

  Serial.println("Hora NTP configurada");
}

bool obtenerHora(struct tm &timeinfo)
{
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Error al leer hora local");
    return false;
  }
  return true;
}

void dibujarHora(String h)
{
  display.setRotation(4);
  display.setTextColor(GxEPD_BLACK);

  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

    display.setFont(&FreeSansBold24pt7b);
    display.setTextSize(3);

    int16_t x1, y1;
    uint16_t w, htxt;
    display.getTextBounds(h.c_str(), 0, 0, &x1, &y1, &w, &htxt);

    int16_t x = (display.width() - w) / 2;
    int16_t y = (display.height() + htxt) / 2;

    display.setCursor(x, y);
    display.print(h);

  } while (display.nextPage());

  Serial.print("Hora dibujada completa ");
  Serial.println(h);
}

void actualizarHoraParcial(String h)
{
  display.setRotation(4);
  display.setTextColor(GxEPD_BLACK);

  display.setFont(&FreeSansBold24pt7b);
  display.setTextSize(3);
  int16_t x1, y1;
  uint16_t w, htxt;
  display.getTextBounds(h.c_str(), 0, 0, &x1, &y1, &w, &htxt);

  int16_t x = (display.width() - w) / 2;
  int16_t y = (display.height() + htxt) / 2;

  int16_t margen = 10;
  int16_t px = x - margen;
  int16_t py = y - htxt - margen;
  uint16_t pw = w + margen * 2;
  uint16_t ph = htxt + margen * 2;

  if (px < 0)
    px = 0;
  if (py < 0)
    py = 0;
  if (px + pw > display.width())
    pw = display.width() - px;
  if (py + ph > display.height())
    ph = display.height() - py;

  display.setPartialWindow(px, py, pw, ph);

  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(h);
  } while (display.nextPage());

  Serial.print("Hora actualizada parcial ");
  Serial.println(h);
}

void dibujarPrecioBitcoin(float precio)
{
  display.setRotation(4);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();

  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

      // Tamaño específico solo para la pantalla BTC
    display.setFont(&FreeSansBold18pt7b);
    display.setTextSize(1);

    String linea1 = "Precio BTC";

    char buffer[16];
    if (precio < 0)
    {
      snprintf(buffer, sizeof(buffer), "--");
    }
    else
    {
      snprintf(buffer, sizeof(buffer), "%.0f EUR", precio);
    }

    // Línea 2: precio (más grande)
    display.setFont(&FreeSansBold24pt7b);
    display.setTextSize(1);

    String linea2 = String(buffer);

    int16_t x1, y1;
    uint16_t w1, h1;
    display.getTextBounds(linea1.c_str(), 0, 0, &x1, &y1, &w1, &h1);

    int16_t x2, y2;
    uint16_t w2, h2;
    display.getTextBounds(linea2.c_str(), 0, 0, &x2, &y2, &w2, &h2);

    int16_t centroX = display.width() / 2;
    int16_t centroY = display.height() / 2;

    int16_t xLinea1 = centroX - w1 / 2;
    int16_t yLinea1 = centroY - h2;

    int16_t xLinea2 = centroX - w2 / 2;
    int16_t yLinea2 = centroY + h2 / 2;

    display.setCursor(xLinea1, yLinea1);
    display.print(linea1);

    display.setCursor(xLinea2, yLinea2);
    display.print(linea2);

  } while (display.nextPage());

  Serial.print("Pantalla BTC dibujada. Precio ");
  Serial.println(precio);
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Txoko Clock NTP reloj ePaper");

  display.init(115200);

  conectarWifi();
  configurarHoraNTP();

  struct tm t;
  if (obtenerHora(t))
  {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", t.tm_hour, t.tm_min);
    String horaStr = String(buffer);
    dibujarHora(horaStr);
    ultimoMinuto = t.tm_min;
  }
}

void loop()
{
  // estado de alternancia entre HORA y BTC
  static bool modoBTC = false;           // false = mostrando hora, true = mostrando BTC
  static unsigned long ultimoCambio = 0; // instante del último cambio de modo
  static bool precioActualizado = false; // para evitar actualizar demasiadas veces

  unsigned long ahora = millis();

  // alternar modo cada 15 minutos (900.000 ms)   Poner esto 900000
  // 1 min = 60.000 ms                            Poner esto 60000
  // 5 min = 300.000 ms                           Poner esto 300000
  // 10 min = 600.000 ms                          600000

  if (ahora - ultimoCambio > 300000)
  {
    ultimoCambio = ahora;
    modoBTC = !modoBTC;
    precioActualizado = false;

    if (!modoBTC)
    {
      // entramos en modo HORA: refresco completo de hora
      struct tm t2;
      if (obtenerHora(t2))
      {
        char buffer2[6];
        snprintf(buffer2, sizeof(buffer2), "%02d:%02d", t2.tm_hour, t2.tm_min);
        String horaStr2 = String(buffer2);
        dibujarHora(horaStr2);
        ultimoMinuto = t2.tm_min;
      }
    }
  }

  // comportamiento según modo
  if (!modoBTC)
  {
    // modo HORA
    struct tm t;
    if (!obtenerHora(t))
    {
      delay(5000);
      return;
    }

    if (t.tm_min != ultimoMinuto)
    {
      ultimoMinuto = t.tm_min;

      char buffer[6];
      snprintf(buffer, sizeof(buffer), "%02d:%02d", t.tm_hour, t.tm_min);
      String horaStr = String(buffer);

      actualizarHoraParcial(horaStr);
    }
  }
  else
  {
    // modo BTC durante 15 min seguidos
    if (!precioActualizado)
    {
      float precio = obtenerPrecioBitcoin();
      dibujarPrecioBitcoin(precio);
      precioActualizado = true;
    }
  }

  delay(1000);
}