/*
 * DisplayManager - OLED (SSD1306, 72x40, I2C) rendering.
 *
 * The screen is redrawn only when the text that should be shown actually
 * changed, which in practice means roughly one redraw per second while the
 * clock is running and no redraws at all for a static status message.
 */
#pragma once

#include <time.h>

class DisplayManager {
public:
    void begin();
    void showStatus(const char *status);
    void showClock(const struct tm &clock);

private:
    void drawCentered(unsigned char y, const char *text);
    void render(const char *line1, const char *line2, unsigned char fontId);

    unsigned char m_fontId = 0;
    bool m_drawn = false;
    char m_line1[12] = {};
    char m_line2[11] = {};
};
