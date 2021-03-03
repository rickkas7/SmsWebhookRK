#include "SmsWebhookRK.h"

SerialLogHandler logHandler;
SYSTEM_MODE(SEMI_AUTOMATIC);

void buttonHandler(system_event_t event, int data);

bool buttonPressed = false;


void setup() {
    SmsWebhook::instance().setup();

    System.on(button_final_click, buttonHandler);
    Particle.connect();
}

void loop() {
    SmsWebhook::instance().loop();

    if (buttonPressed) {
        static unsigned int counter = 0;

        buttonPressed = false;

        SmsMessage msg;
        msg.withMessage(String::format("Message %d!", ++counter));
        Log.info("queued %s", msg.getMessage());

        SmsWebhook::instance().queueSms(msg);
    }
}

void buttonHandler(system_event_t event, int data) {
    int clicks = system_button_clicks(data);
    if (clicks == 1) {
        buttonPressed = true;
    }
}
