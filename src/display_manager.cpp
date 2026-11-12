#include <Arduino.h>
#include <U8g2lib.h>
#include <time.h>
#include <string.h>

#include "display_manager.h"

// Verified OLED wiring: I2C over GPIO 6 (SCL) / GPIO 5 (SDA), address 0x3D.
U8G2_SSD1306_72X40_ER_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, 6, 5);

static constexpr unsigned char FONT_STATUS = 0;
static constexpr unsigned char FONT_CLOCK = 1;

// Baselines chosen so that both clock lines sit vertically centred on the
// 40 pixel tall screen (6x10 digits draw 7 pixels above the baseline).
static constexpr unsigned char STATUS_Y = 20;
static constexpr unsigned char CLOCK_LINE1_Y = 16;
static constexpr unsigned char CLOCK_LINE2_Y = 29;

void DisplayManager::begin() {
    Serial.println("Display: OLED init (I2C 0x3D, SCL=6, SDA=5)");
    display.setI2CAddress(0x3D * 2);
    display.begin();
    display.clearBuffer();
    display.sendBuffer();
    m_drawn = false;
}

void DisplayManager::drawCentered(unsigned char y, const char *text) {
    const u8g2_uint_t width = display.getStrWidth(text);
    const u8g2_uint_t screen = display.getDisplayWidth();
    const u8g2_uint_t x = width < screen ? (screen - width) / 2 : 0;
    display.drawStr(x, y, text);
}

void DisplayManager::render(const char *line1, const char *line2, unsigned char fontId) {
    if (m_drawn && fontId == m_fontId && strcmp(line1, m_line1) == 0 && strcmp(line2, m_line2) == 0) {
        return;
    }

    snprintf(m_line1, sizeof(m_line1), "%s", line1);
    snprintf(m_line2, sizeof(m_line2), "%s", line2);
    m_fontId = fontId;
    m_drawn = true;

    display.clearBuffer();
    display.setFont(fontId == FONT_CLOCK ? u8g2_font_6x10_tf : u8g2_font_5x7_tf);

    if (line2[0] == '\0') {
        drawCentered(STATUS_Y, line1);
    } else {
        drawCentered(CLOCK_LINE1_Y, line1);
        drawCentered(CLOCK_LINE2_Y, line2);
    }
    display.sendBuffer();
}

void DisplayManager::showStatus(const char *status) {
    render(status, "", FONT_STATUS);
}

void DisplayManager::showClock(const struct tm &clock) {
    // Bounded values keep snprintf's output within the fixed buffers below.
    const unsigned day = static_cast<unsigned>(clock.tm_mday) % 100u;
    const unsigned month = static_cast<unsigned>(clock.tm_mon + 1) % 100u;
    const unsigned year = static_cast<unsigned>(clock.tm_year + 1900) % 10000u;
    const unsigned hour = static_cast<unsigned>(clock.tm_hour) % 100u;
    const unsigned minute = static_cast<unsigned>(clock.tm_min) % 100u;
    const unsigned second = static_cast<unsigned>(clock.tm_sec) % 100u;

    char line1[12];
    char line2[11];
    snprintf(line1, sizeof(line1), "%02u.%02u.%04u", day, month, year);
    snprintf(line2, sizeof(line2), "%02u:%02u:%02u", hour, minute, second);
    render(line1, line2, FONT_CLOCK);
}
