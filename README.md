# SmsWebhookRK

*Library for Particle devices to easily send SMS via a webhook to Twilio*

- [Github repository](https://github.com/rickkas7/SmsWebhookRK)
- [Full browsable API](https://rickkas7.github.io/SmsWebhookRK/index.html)
- License: MIT (can be used in open or closed-source projects, including commercial projects, with no attribution required)
- Library: SmsWebhookRK

## Using the library

- Add the library to your project. Using Particle Workbench, from the Command Palette **Particle: Install Library** and add **SmsWebhookRK**. It's also avaiable in the community libraries in the Web IDE.

- Add the include statement to your main .ino or .cpp source file:

```cpp
#include "SmsWebhookRK.h"
```

- Add a call from setup():

```cpp
void setup() {
    SmsWebhook::instance().setup();
}
```

- Add a call from loop():

```
void loop() {
    SmsWebhook::instance().loop();
}
```

Note that both setup and loop calls are required! You should make the loop call on every loop; if there is nothing to do it returns quickly.

- To send a SMS message you specify the recipient phone number and the message. Note that the phone number must be in +country code format, so in the United States it will always begin with `+1`.

```cpp
SmsMessage mesg;
mesg.withRecipient("+12125551212")
    .withMessage("testing!");
SmsWebhook::instance().queueSms(msg);
```
This queues the SMS message to send. If the device is online and connected to the Particle cloud, it will be sent immediately. Otherwise, it will be queued to send later. 

If an error occurs and the publish fails, the message will be retried later.

If you are only sending SMS to yourself from your own devices, you can leave the recipient phone number out and instead encode it in the webhook, that way you don't need to code your phone number in your application code. 

There are more options available as described in the Examples section, below.


## Setup


### Twilio Setup

This method could easily be updated to work with other SMS providers, however you may need to change authentication methods and the keys where the data is stored.

- [Sign up for a trial Twilio account](https://www.twilio.com/try-twilio) if you don't already have an account set up.

- Follow the instructions to [Buy a number](https://www.twilio.com/docs/sms/quickstart/node). Make sure it has SMS enabled!

![Buy Number](images/buy-number.jpg)

- In the [Twilio Dashboard](https://www.twilio.com/console) (1), note the **Account SID** (2) and **Auth Token** (3). You'll need these to create the webhook.

![Twilio Dashboard](images/dashboard.png)


### Webhook Setup

- Open the [Particle Console](https://console.particle.io) and select the **Integrations** tab.

- Create a new **Webhook** integration.

![Basic configuration](images/webhook-1.png)

- Select an **Event Name**. The default configured in the library is **SendSmsEvent** (case-sensitive!) but you can change it and use the `withEventName()` method to reconfigure the library. They must match and follow event naming rules (1 - 64 ASCII characters; only use letters, numbers, underscores, dashes and slashes. Spaces and special characters should not be used). Also note that the event name is a prefix, so any event name beginning with that string will trigger the webhook.

- Enter the **URL**. Make sure you subtitute your Account SID for `$TWILIO_ACCOUNT_SID`! 

```
https://api.twilio.com/2010-04-01/Accounts/$TWILIO_ACCOUNT_SID/Messages.json
```

- **Request Type: POST** is the default and the correct value.

- **Request Format: Web Form** is the default and the correct value.

- **Device: Any** is the default. You could also restrict the webhook to a specific device so only that device could send SMS messages.

- Open the **Advanced Settings**.

![Advanced Settings](images/webhook-2.png)

- In **Form Fields** select **Custom**.

  - Enter **From** (case-sensitive) and the SMS phone number you want to send from. This must be a Twilio Programmable Messaging phone number, not your actual phone number! Also note that it must be begin with `+` then country code, so it will begin with `+1` in the United States.
  - Enter **To** and `{{{t}}}` (case-sensitive). The triple curly brackets are required. If you are only going to ever send to your own phone, you could enter your phone number here instead of `{{{t}}}` but make sure it begins with a `+`.
  - Enter **Body** and `{{{b}}}` (case-sensitive). 

- **Query Parameters** should remain empty.

- In **HTTP Basic Auth** enter:

  - In **Username** enter your Account SID. Note that your Account SID goes in two places: the URL and the Username!

  - In **Password** enter your account Auth Token.

- In **HTTP Headers** the default **Content-Type** of `application/x-www-form-urlencoded` is correct.

- Save the Webhook.

## Examples

### examples/01-simple

Here's the full code for the simple example. Tapping the MODE (or SETUP) button once will send a SMS.

```cpp
#include "SmsWebhookRK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);
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

        SmsMessage mesg;
        mesg.withRecipient("+12125551212")
            .withMessage(String::format("Message %d!", ++counter));

        Log.info("queued %s", mesg.getMessage());

        SmsWebhook::instance().queueSms(mesg);
    }
}

void buttonHandler(system_event_t event, int data) {
    int clicks = system_button_clicks(data);
    if (clicks == 1) {
        buttonPressed = true;
    }
}

```

Digging into the code:

Include the header file. Note that you must add the library as well, just adding the include is not sufficient.

```
#include "SmsWebhookRK.h"
```

This is so log message can be seen by the USB serial debugging output. The library uses "sms" as the logging category so you can turn off the messages if desired.

```
SerialLogHandler logHandler;
```

System threading enabled is recommended for this library. The code can be used in both `AUTOMATIC` and `SEMI_AUTOMATIC` mode.

```
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);
```

This is a forward declaration, because we use the function in a call to `System.on()` before it's implemented in the .cpp file.

```
void buttonHandler(system_event_t event, int data);
```

The button handler can be called at interrupt time, so it's best to just set a flag and handle it from loop.

```
bool buttonPressed = false;
```

You must call the setup method for the library from global app `setup()`. The library object is a singleton, which you always get using `SmsWebhook::instance()`. You never create the object as a global or allocate it with new.

```
void setup() {
    SmsWebhook::instance().setup();
```

Add the button handler:

```
    System.on(button_final_click, buttonHandler);
```

Since we used `SYSTEM_MODE(SEMI_AUTOMATIC)` it's necessary to call `Particle.connect()` to connect to the cloud. If you use `AUTOMATIC` you don't need this.

```
    Particle.connect();
}
```

You must call `SmsWebhook::instance().loop()` from global application `loop()`!

```
void loop() {
    SmsWebhook::instance().loop();
```

This checks the flag set by the button handler.

```
    if (buttonPressed) {
        static unsigned int counter = 0;

        buttonPressed = false;
```

And this is the part that sends the SMS. 

- Create a `SmsMessage` object on the stack (it's small)
- Set the recipient phone number using `withRecipient()`. Note that the phone number must be in + country code format, so in the United States it will begin with `+1`.
- Set the text of the message using `withMessage()`. In this case, we format a message with a counter that increases each time a message is sent.
- Logs the message to the USB serial debug
- Queues the message to send using `SmsWebhook::instance().queueSms(mesg)`.

```
        SmsMessage mesg;
        mesg.withRecipient("+12125551212")
            .withMessage(String::format("Message %d!", ++counter));

        Log.info("queued %s", mesg.getMessage());

        SmsWebhook::instance().queueSms(mesg);
    }
}
```

When the MODE/SETUP button is tapped once, this handles it and sets a flag to be handled from loop().

```
void buttonHandler(system_event_t event, int data) {
    int clicks = system_button_clicks(data);
    if (clicks == 1) {
        buttonPressed = true;
    }
}
```

### more-examples/02-eeprom

This example uses the [CloudConfigRK](https://github.com/rickkas7/CloudConfigRK/) library to store the recipient phone number in the emulated EEPROM on the device. This allows a per-device phone number, without requiring the phone number be hardcoded in the user firmware. This is also helpful if you are creating a product, so you can have individual customer phone numbers and have one firmware binary and one webhook.

The library has many options, including:

- Storing the data in retained memory, EEPROM, or a file on the Gen 3 flash file system.
- Setting the data using a Particle function (demonstrated here), as well as published message, device notes, or a Google spreadsheet.

The code to hook the two libraries together and set the phone number with a function call (once), store it in EEPROM, then use it for any SMS notification is mostly just this:

```cpp
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

```

To set the phone number make a function call `setConfig` with the data 

```
{"t":"+12125551212"}
```

Remember that the phone number must be in + country code format, so in the United States it will begin with `+1`.

If you want to use the Particle CLI, the command to set the phone number for the device named "test2" is:

```
particle call test2 setConfig '{"t":"+12125551212"}'
```

Tapping the MODE (or SETUP) button sends the SMS to the configured phone number.

## Version History

### 0.0.1 (2021-03-03)

- Initial version
