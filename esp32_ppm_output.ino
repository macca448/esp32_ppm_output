// File: esp32_ppm_output.ino
// Ported to now work with the Espressif Arduino core V3.x.x libraries

#include <driver/gpio.h>
#include <esp_rom_gpio.h>

#define PPM_FRAME_LENGTH 22500
#define PPM_PULSE_LENGTH 300
#define PPM_CHANNELS 8
#define OUTPUT_PIN 14
#define DEFAULT_CHANNEL_VALUE 1500

volatile uint16_t channelValue[PPM_CHANNELS] = {
    DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE,
    DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE, DEFAULT_CHANNEL_VALUE};

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

enum ppmState_e {
    PPM_STATE_IDLE,
    PPM_STATE_PULSE,
    PPM_STATE_FILL,
    PPM_STATE_SYNC
};

volatile bool timerFlag = false;

void IRAM_ATTR onPpmTimer() {
    static uint8_t ppmState = PPM_STATE_IDLE;
    static uint8_t ppmChannel = 0;
    static uint8_t ppmOutput = 0;
    static int usedFrameLength = 0;
    int currentChannelValue;

    portENTER_CRITICAL_ISR(&timerMux);

    if (ppmState == PPM_STATE_IDLE) {
        ppmState = PPM_STATE_PULSE;
        ppmChannel = 0;
        usedFrameLength = 0;
        ppmOutput = 0;
    }

    if (ppmState == PPM_STATE_PULSE) {
        ppmOutput = 1;
        usedFrameLength += PPM_PULSE_LENGTH;
        ppmState = PPM_STATE_FILL;
        timerWrite(timer, timerRead(timer) + PPM_PULSE_LENGTH);
    } else if (ppmState == PPM_STATE_FILL) {
        ppmOutput = 0;
        currentChannelValue = channelValue[ppmChannel];
        ppmChannel++;
        ppmState = PPM_STATE_PULSE;

        if (ppmChannel >= PPM_CHANNELS) {
            ppmChannel = 0;
            timerWrite(timer, timerRead(timer) + (PPM_FRAME_LENGTH - usedFrameLength));
            usedFrameLength = 0;
        } else {
            usedFrameLength += currentChannelValue - PPM_PULSE_LENGTH;
            timerWrite(timer, timerRead(timer) + (currentChannelValue - PPM_PULSE_LENGTH));
        }
    }

    gpio_set_level((gpio_num_t)OUTPUT_PIN, ppmOutput);
    portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
    esp_rom_gpio_pad_select_gpio(OUTPUT_PIN);
    gpio_set_direction((gpio_num_t)OUTPUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)OUTPUT_PIN, 0);

    timer = timerBegin(1); // 1 MHz tick (1 µs resolution)
    timerAttachInterrupt(timer, &onPpmTimer);
    timerStart(timer);
}

void loop() {
     /*
    Here you can modify the content of channelValue array and it will be automatically
    picked up by the code and outputted as PPM stream. For example:
    */
    
    channelValue[0] = 1750;
    channelValue[1] = 1350;
    channelValue[2] = 1050;
    channelValue[3] = 1920;

    delay(100);
}
