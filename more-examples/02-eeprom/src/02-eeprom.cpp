#include "SmsWebhookRK.h"
#include "CloudConfigRK.h"

SerialLogHandler logHandler;
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void buttonHandler(system_event_t event, int data);

bool buttonPressed = false;

// particle call test2 setConfig '{"t":"+12125551212"}'

size_t EEPROM_OFFSET = 0;


void setup() {
    // Initialize cloud-based configuration
    // Store data in emulated EEPROM
    // Get the recipient SMS phone number via function call ("setConfig")
    CloudConfig::instance()
        .withUpdateMethod(new CloudConfigUpdateFunction("setConfig"))
        .withStorageMethod(new CloudConfigStorageEEPROM<128>(EEPROM_OFFSET))
        .setup();

    SmsWebhook::instance().setup();

    SmsWebhook::instance()
        .withEventName("SendSmsEvent")
        .withRecipientCallback([](String &recipient) {
            // This function is called to find out the recipient's SMS phone number
            // If this has not been set yet, the string will be empty and we will
            // try again later.
            recipient = CloudConfig::instance().getString("t");
            return (recipient.length() > 0);
        });

    System.on(button_final_click, buttonHandler);
    Particle.connect();
}

void loop() {
    CloudConfig::instance().loop();
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
