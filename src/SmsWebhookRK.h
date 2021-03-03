#ifndef __SMSWEBHOOKRK_H
#define __SMSWEBHOOKRK_H

// Github: https://github.com/rickkas7/SmsWebhookRK
// License: MIT

#include "Particle.h"

#include <deque>

/**
 * @brief Class for setting parameters for a SMS message.
 * 
 * You typically use it like:
 * 
 * ```
 * SmsMessage mesg;
 * mesg.withRecipient("+12125551212")
 *     .withMessage(String::format("Message %d!", ++counter));
 * SmsWebhook::instance().queueSms(mesg);
 * ```
 */
class SmsMessage {
public:
    /**
     * @brief Default constructor - doesn't really do anything
     */
    SmsMessage() {};

    /**
     * @brief Default destructor - doesn't really do anything
     */
    virtual ~SmsMessage() {};

    /**
     * @brief Sets the recipient phone number
     * 
     * @param phoneNum Recipient phone number in + country code format, so for example, in the United States
     * it begins with `+1`. The rest of the phone number should be be just digits, no punctuation.
     * 
     * @returns *this so you can chain withXXX() calls, fluent-style
     * 
     * This method makes a copy of phoneNum string.
     * 
     * Instead of specifying the recipient in each SmsMessage object, you can use withRecipientCallback().
     * Providing a function to get the recipient is handy if you're storing the recipient in EEPROM or
     * a file on the file system.
     */
    SmsMessage &withRecipient(const char *phoneNum) { this->recipient = phoneNum; return *this; };

    /**
     * @brief Gets the previously set phone number
     */
    const char *getRecipient() const { return recipient; };

    /**
     * @brief Returns true if the recipient is a non-empty recipient string
     */
    bool hasRecipient() const { return recipient.length() != 0; };

    /**
     * @brief Sets the SMS message text
     * 
     * @param message The SMS message text, limited to 140 characters.
     * 
     * @returns *this so you can chain withXXX() calls, fluent-style
     * 
     * This method makes a copy of message string. If you don't set the message string, the Twilio API
     * won't let you send an empty SMS, so an error will show in the integration log.
     */
    SmsMessage &withMessage(const char *message) { this->message = message; return *this; };

    /**
     * @brief Gets the previously set message text
     */
    const char *getMessage() const { return message; };
    
protected:
    /**
     * @brief Recipient phone number (+ country code format)
     */
    String recipient;

    /**
     * @brief Message text to send
     */
    String message;
};

/**
 * @brief Class for the library
 * 
 * It's a singleton, so you always access it using `SmsWebhook::instance()`. You never allocate one
 * as a global variable, stack variable, or with `new`.
 * 
 * You must call `SmsWebhook::instance().setup()` from your global application `setup()`. 
 * 
 * You must call `SmsWebhook::instance().loop()` from your global application `loop()`. Failure to do
 * so will cause no messages to ever be sent.
 * 
 * To queue a message to be sent, use `SmsWebhook::instance().queueSms(mesg)`. If the device is connected
 * to the Particle cloud, it will go out almost immediately. If an error occurs, it will retry later.
 */
class SmsWebhook {
public:
    /**
     * @brief Get the singleton instance of this class
     */
    static SmsWebhook &instance();

    /**
     * @brief You must call setup() from global application setup()!
     * 
     * It's normally done like:
     * 
     * ```
     * SmsWebhook::instance().setup();
     * ```
     * 
     * Failure to call setup will prevent the library from doing anything.
     */
    void setup();

    /**
     * @brief You must call loop() from global application loop()!
     * 
     * It's normally done like:
     * 
     * ```
     * SmsWebhook::instance().loop();
     * ```
     * 
     * Failure to call loop will prevent the library from sending messages.
     */
    void loop();

    /**
     * @brief Queue a SmsMessage to send
     * 
     * @param smsMessage Information about the message to be sent, typically containing a recipient
     * phone number and the message content.
     * 
     * The smsMessage object is copied by this call. It's safe to make this call from other threads.
     * It cannot be made at ISR time as it does memory allocation.
     */
    void queueSms(SmsMessage smsMessage);

    /**
     * @brief Sets the event name to use. This must match the webhook. Default is "SendSmsEvent".
     * 
     * @param eventName the event name to use
     * 
     * Event naming rules: 1 - 64 ASCII characters; only use letters, numbers, underscores, dashes and slashes. 
     * Spaces and special characters should not be used). Also note that the event name is a prefix, so any 
     * event name beginning with that string will trigger the webhook.
     */
    SmsWebhook &withEventName(const char *eventName) { this->eventName = eventName; return *this; };

    /**
     * @brief Get the previously set event name (or the default, if it hasn't been set yet)
     * 
     * @return A c-string (null terminated)
     */
    const char *getEventName() const { return eventName; };

    /**
     * @brief Sets a function to call to get the recipient if the SmsMessage recipient field is black
     * 
     * @param recipientCallback the function to call to find the recipient
     * 
     * @return *this, so you can chain this function fluent-style.
     * 
     * The recipientCallback has this prototype:
     * 
     *   bool recipientCallback(String &phone);
     * 
     * The callback returns true if the recipient is know, or false if not. The if false is returned, then
     * an attempt will be made again after the timeout.
     * 
     * The phone number must begin with "+" and the country code, so, for example in the US: +15558675310 .
     */
    SmsWebhook &withRecipientCallback(std::function<bool(String&phone)> recipientCallback) { this->recipientCallback = recipientCallback; return *this; };

    /**
     * @brief Sets the retry time when no recipient is specified in milliseconds. Default is 15 seconds.
     * 
     * @param milliseconds New value in milliseconds
     * 
     * If the recipient is not specified in the `SmsMessage` object, then the recipient callback is used.
     * If the recipient callback returns false (not known yet), the message is left in the queue. This
     * is common if the recipient SMS is sent from the cloud (Particle function, device notes, or
     * Google sheets). This parameter determines how long to wait before checking again.
     */
    SmsWebhook &withRetryNoRecipientMs(unsigned long milliseconds) { retryNoRecipientMs = milliseconds; };

    /**
     * @brief Get the previously set retry when no recipient is specified value (or the default, if it hasn't been set yet)
     * 
     * @return An unsigned long value of milliseconds.
     */
    unsigned long getRetryNoRecipientMs() const { return retryNoRecipientMs; };

    /**
     * @brief Sets the retry time if the publish fails. Default is 15 seconds.
     * 
     * @param milliseconds New value in milliseconds
     * 
     * If `Particle.publish()` fails, this is how long to wait before retrying. Note that if the webhook
     * fails this is not consulted, but does take care of the situation where you have poor connectivity
     * and the publish is not able to be sent.
     */
    SmsWebhook &withRetryPublishFailMs(unsigned long milliseconds) { retryPublishFailMs = milliseconds; };

    /**
     * @brief Get the previously set retry when publish fails value (or the default, if it hasn't been set yet)
     * 
     * @return An unsigned long value of milliseconds.
     */
    unsigned long getRetryPublishFailMs() const { return retryPublishFailMs; };

    /**
     * @brief Sets publish rate limit. If multiple publishes are required, wait this long between publishes.
     * Default is 1010 milliseconds.
     * 
     * @param milliseconds New value in milliseconds
     * 
     * In addition to the Particle publish rate limit, you could also hit a rate limit at Twilio if you need 
     * to send a lot of SMS messages.
     */
    SmsWebhook &withPublishRateLimitMs(unsigned long milliseconds) { publishRateLimitMs = milliseconds; };

    /**
     * @brief Get the previously set publish rate limit value (or the default, if it hasn't been set yet)
     * 
     * @return An unsigned long value of milliseconds.
     */
    unsigned long getPublishRateLimitMs() const { return publishRateLimitMs; };

protected:
    /**
     * @brief Constructor (protected)
     * 
     * You never construct one of these - use the singleton instance using `SmsWebhook::instance()`.
     */
    SmsWebhook();

    /**
     * @brief Destructor - never used
     * 
     * The singleton cannot be deleted.
     */
    virtual ~SmsWebhook();

    /**
     * @brief This class is not copyable
     */
    SmsWebhook(const SmsWebhook&) = delete;

    /**
     * @brief This class is not copyable
     */
    SmsWebhook& operator=(const SmsWebhook&) = delete;

    /**
     * @brief State handler for waiting for a message and cloud connected
     */
    void stateWaitForMessage();

    /**
     * @brief State handler for waiting for publish to complete
     */
    void stateWaitPublish();

    /**
     * @brief State handler for waiting for to retry
     */
    void stateWaitRetry();

    /**
     * @brief Event name to use. Default is "SendSmsEvent". Use withEventName() to change.
     */
    String eventName = "SendSmsEvent";

    /**
     * @brief Recipient callbac. Use withRecipientCallback() to change. Default: none
     */
    std::function<bool(String&)> recipientCallback = 0;

    /**
     * @brief Mutex to protect sendQueue from access from multiple threads simultaneously
     */
    os_mutex_t sendQueueMutex = 0;

    /**
     * @brief Queue of SmsMessage objects to send
     * 
     * Objects are enqueued using queueSms() and removed from the state handlers called from loop().
     */
    std::deque<SmsMessage> sendQueue;

    /**
     * @brief Future used to monitor the state of `Particle.publish()`.
     * 
     * The publish call uses a future so it does not block loop until completion.
     */
    particle::Future<bool> publishFuture;

    /**
     * @brief millis() value used for various timing purposes. This is always the start time.
     */
    unsigned long stateTime = 0;

    /**
     * @brief How long to wait for retry in stateWaitRetry.
     * 
     * This is compared to stateTime in a way that works across millis() rollover every 49 days.
     */
    unsigned long retryTimeMs = 0;

    /**
     * @brief Retry time when no recipient is specified in milliseconds. Default is 15 seconds.
     */
    unsigned long retryNoRecipientMs = 15000;


    /**
     * @brief Retry time when no if publish fails in milliseconds.
     */
    unsigned long retryPublishFailMs = 15000;

    /**
     * @brief publish rate limit. If multiple publishes are required, wait this long between publishes.
     */
    unsigned long publishRateLimitMs = 1010;

    /**
     * @brief Sets the temporary buffer for the JSON data for the publish
     * 
     * This is allocated on the stack, but is only done from loop() which as a 6K stack. It doesn't
     * make sense for this to be larger than the maximum publish size (622 bytes). Currently the
     * only thing in it is the message and recipient, which probably fit in 160-ish bytes but
     * 256 leaves a little extra room just in case.
     */
    static const size_t JSON_BUF_SIZE = 256;

    /**
     * @brief State handler for the main state machine
     * 
     * The state handler are class method with the prototype:
     * 
     * void stateHandlerMethod();
     */
    std::function<void(SmsWebhook &)> stateHandler = 0;

    /**
     * @brief Singleton instance of this class
     * 
     * Use instance() to obtain this, dereferenced.
     */
    static SmsWebhook *_instance;
};

#endif /* __SMSWEBHOOKRK_H */
