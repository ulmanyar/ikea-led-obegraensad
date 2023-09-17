#include <SPI.h>
#include <cstdint>
#include "new_screen.h"



void NewScreen::clear() {
    memset(buffer_, 0, ROWS * COLS);
}

void NewScreen::setup() {
    // Configure SPI
    // Note that MISO and SS is not used and assigned to unused pins
    SPI.pins(PIN_CLOCK, 5, PIN_DATA, 15);  // SCLK, MISO, MOSI, SS
    SPI.begin();
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));

    timer1_attachInterrupt(&renderOnTimer);
    timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
    timer1_write(100);
    
    // Setup the brightnessMap
    delay(1000);
    uint8_t unusedBits = 27 - __builtin_clz(NewScreen::grayLevels);
    for (uint8_t idx = 0; idx < NewScreen::grayLevels; idx++) {
        uint8_t bitReversedIdx = idx;
        // Reverse the byte: 10010110 -> 01101001
        // First, swap sequence of 4: 01234567 -> 45670123
        bitReversedIdx = (bitReversedIdx & 0xF0) >> 4 | (bitReversedIdx & 0x0F) << 4;
        // Then swap sequence of 2: 45670123 -> 67452301
        bitReversedIdx = (bitReversedIdx & 0xCC) >> 2 | (bitReversedIdx & 0x33) << 2;
        // Then swap sequence of 1: 67452301 -> 76543210
        bitReversedIdx = (bitReversedIdx & 0xAA) >> 1 | (bitReversedIdx & 0x55) << 1;
        // We only want to reverse the first log2(grayLevels) bits, since they
        // represent the number of steps that will be taken by the brightness
        // counter to reach 256 in grayLevels steps
        bitReversedIdx >>= unusedBits;
        // Now, assign the values in a lookup table
        brightnessMap_[idx] = bitReversedIdx * (256 / grayLevels);
        Serial.println();
        Serial.print(brightnessMap_[idx]);
    }

}

void NewScreen::setBuffer(const uint8_t* buffer) {
    memcpy(buffer_, buffer, ROWS * COLS);
}

const uint8_t* NewScreen::getBuffer() const {
    return buffer_;
}

uint8_t NewScreen::getBufferIndex(int index) { // From Screen_
  return buffer_[index];
}

void NewScreen::renderOnTimer() {
    Screen.render();
}

void NewScreen::render() {
    const uint8_t* buffer = getBuffer();
    static uint8_t counter = 0;

    static uint8_t bufferByteMap[ROWS * COLS / 8] = {0};
    memset(bufferByteMap, 0, ROWS * COLS / 8);
    
    // Skip brightness counter for now
    for (uint16_t pixel = 0; pixel < ROWS * COLS; pixel++) {
        //bufferByteMap[pixel >> 3] |= (buffer[pixel] > counter ? 1 : 0) << pixel % 8;
        uint8_t brightnessThreshold = brightnessMap_[counter % grayLevels];
        bufferByteMap[pixel >> 3] |= (buffer[pixelMap_[pixel]] > brightnessThreshold ? 0x80 : 0) >> pixel % 8;
    }

    // Increment counter
    counter++;

    // Write to shift registers using SPI
    digitalWrite(PIN_LATCH, LOW);
    SPI.writeBytes(bufferByteMap, sizeof(bufferByteMap));
    digitalWrite(PIN_LATCH, HIGH);
    // Wait another 100 ticks until next render cycle
    timer1_write(100);
}

std::vector<int> NewScreen::readBytes(std::vector<int> bytes) { // From Screen_
    std::vector<int> bits;
    int k = 0;

    for (uint i = 0; i < bytes.size(); i++) {
        for (int j = 8 - 1; j >= 0; j--) {
            int b = (bytes[i] >> j) & 1;
            bits.push_back(b);
            k++;
        }
    }

    return bits;
}

void NewScreen::setPixelAtIndex(uint8_t index, uint8_t value, uint8_t brightness) { // From Screen_
    if (index >= 0 && index < COLS * ROWS) {
        buffer_[index] = value * brightness;
    }
}

void NewScreen::setPixel(uint8_t x, uint8_t y, uint8_t value, uint8_t brightness) { // From Screen_
    if (x >= 0 && y >= 0 && x < 16 && y < 16) {
        buffer_[y * 16 + x] = value * brightness;
    }
}


void NewScreen::drawLine(int x1, int y1, int x2, int y2, int ledStatus, uint8_t brightness) { // From Screen_
    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;
    int dy = abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;
    int error = (dx > dy ? dx : -dy) / 2;

    while (x1 != x2 || y1 != y2) {
        setPixel(x1, y1, ledStatus, brightness);

        int error2 = error;
        if (error2 > -dx) {
            error -= dy;
            x1 += sx;
            setPixel(x1, y1, ledStatus, brightness);
        }

        else if (error2 < dy) {
            error += dx;
            y1 += sy;
            setPixel(x1, y1, ledStatus, brightness);
        }
    };
};

void NewScreen::drawRectangle(int x, int y, int width, int height, bool fill, int ledStatus, uint8_t brightness) { // From Screen_
    if (!fill) {
        drawLine(x, y, x + width, y, ledStatus, brightness);
        drawLine(x, y + 1, x, y + height - 1, ledStatus, brightness);
        drawLine(x + width, y + 1, x + width, y + height - 1, ledStatus, brightness);
        drawLine(x, y + height - 1, x + width, y + height - 1, ledStatus, brightness);
    }
    else {
        for (int i = x; i < x + width; i++) {
            drawLine(i, y, i, y + height - 1, ledStatus, brightness);
        }
    }
};

void NewScreen::drawCharacter(int x, int y, std::vector<int> bits, int bitCount, uint8_t brightness) { // From Screen_
    for (uint i = 0; i < bits.size(); i += bitCount) {
        for (int j = 0; j < bitCount; j++) {
            setPixel(x + j, (y + (i / bitCount)), bits[i + j], brightness);
        }
    }
}

void NewScreen::drawNumbers(int x, int y, std::vector<int> numbers, uint8_t brightness) { // From Screen_
    for (uint i = 0; i < numbers.size(); i++) {
        drawCharacter(x + (i * 5), y, readBytes(smallNumbers[numbers.at(i)]), 4, brightness);
    }
}

void NewScreen::drawBigNumbers(int x, int y, std::vector<int> numbers, uint8_t brightness) { // From Screen_
    for (uint i = 0; i < numbers.size(); i++) {
        drawCharacter(x + (i * 8), y, readBytes(bigNumbers[numbers.at(i)]), 8, brightness);
    }
}

void NewScreen::drawWeather(int x, int y, int weather, uint8_t brightness) { // From Screen_
    drawCharacter(x, y, readBytes(weatherIcons[weather]), 16, brightness);
}

//void NewScreen::setBrightness(uint8_t brightness) {
//    brightness = brightness;
//    //analogWrite(PIN_ENABLE, 255 - brightness);
//    digitalWrite(PIN_ENABLE, LOW);
//}
//
//uint8_t NewScreen::getCurrentBrightness() const {
//    return brightness;
//}

// Getter for singleton class
NewScreen& NewScreen::getInstance() {
    static NewScreen instance;
    return instance;
}

NewScreen& Screen = Screen.getInstance();

