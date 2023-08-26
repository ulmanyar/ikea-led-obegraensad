#pragma once

#include <Arduino.h>
#include <cstdint>
#include "constants.h"

class NewScreen {
    public:
        static NewScreen& getInstance();
        // Prohibiting copying
        NewScreen& operator=(const NewScreen&) = delete;
        NewScreen(const NewScreen&) = delete;
        


        void clear();
        void setup();
        void render();
        void setBuffer(const uint8_t* buffer);
        const uint8_t* getBuffer() const;

    private:
        // Make singleton class
        NewScreen() = default;

        // NOTE: Skip global brightness for now, but keep a [0,255] per-LED brightness
        uint8_t buffer_[ROWS * COLS];
        uint8_t pixelMap_[ROWS * COLS] = {
            0 // TBW
        };
        static void renderOnTimer();

};

extern NewScreen& new_screen;
