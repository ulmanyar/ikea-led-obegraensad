#include <SPI.h>
#include <cstdint>
#include "new_screen.h"



// Getter for singleton class
NewScreen& NewScreen::getInstance() {
    static NewScreen instance;
    return instance;
}

void NewScreen::clear() {
    memset(this->buffer_, 0, ROWS * COLS);
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
}

void NewScreen::setBuffer(const uint8_t* buffer) {
    memcpy(this->buffer_, buffer, ROWS * COLS);
}

const uint8_t* NewScreen::getBuffer() const {
    return this->buffer_;
}

void NewScreen::renderOnTimer() {
    new_screen.render();
}

void NewScreen::render() {
    const uint8_t* buffer = this->getBuffer();
    static uint8_t counter = 0;

    static uint8_t bufferByteMap[ROWS * COLS / 8] = {0};
    memset(bufferByteMap, 0, ROWS * COLS / 8);
    
    // Skip brightness counter for now
    for (uint16_t pixel = 0; pixel < ROWS * COLS; pixel++) {
        // TODO: Find a better way to distribute counter ON/OFF to reduce flicker
        // From: ____----, __------
        // To:   _-_-_-_-, _---_---
        // Idea: (buffer[pixel] | 256 / 64 ? 1 : 0), perhaps offset by some bit
        bufferByteMap[pixel >> 3] |= (buffer[pixel] > counter ? 1 : 0) << pixel % 8;
    }

    counter += 256 / 64 + 128;

    // Write to shift registers using SPI
    digitalWrite(PIN_LATCH, LOW);
    SPI.writeBytes(bufferByteMap, sizeof(bufferByteMap));
    digitalWrite(PIN_LATCH, HIGH);
    // Wait another 100 ticks until next render cycle
    timer1_write(100);
}

NewScreen& new_screen = NewScreen::getInstance();
