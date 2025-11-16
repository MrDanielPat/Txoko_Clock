#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>


// Mapeo recomendado para WeAct 4.2" con ESP32
// VCC  -> 3.3V
// GND  -> GND
// SDA  -> GPIO23
// SCL  -> GPIO18
// CS   -> GPIO5
// DC   -> GPIO17
// RST  -> GPIO16
// BUSY -> GPIO4

#define EPD_CS   5
#define EPD_DC   17
#define EPD_RST  16
#define EPD_BUSY 4

GxEPD2_BW<GxEPD2_420_GYE042A87, GxEPD2_420_GYE042A87::HEIGHT> display(
    GxEPD2_420_GYE042A87(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Txoko Clock iniciado, inicializando ePaper");

    display.init(115200);
    display.setRotation(4);  // prueba en horizontal, luego ajustamos si quieres
    display.setTextColor(GxEPD_BLACK);
    display.setFullWindow();

    const char* hora = "12:34";

    display.firstPage();
    do {

        // display.fillScreen(GxEPD_WHITE);

        // // marco
        // display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

        // // texto
        // display.setFont(&FreeMonoBold9pt7b);
        // display.setCursor(20, 80);
        // display.print("Txoko Clock");

        display.fillScreen(GxEPD_WHITE);

        display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);

        display.setFont(&FreeSansBold24pt7b);

        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(hora, 0, 0, &x1, &y1, &w, &h);

        int16_t x = (display.width()  - w) / 2;
        int16_t y = (display.height() + h) / 2;

        display.setCursor(x, y);
        display.print(hora);


    } while (display.nextPage());

    display.hibernate();
}

void loop() {
}