#ifndef __SMSWEBHOOKRK_H
#define __SMSWEBHOOKRK_H

#include "Particle.h"

#include <deque>

class SmsMessage {
public:
    SmsMessage() {};
    virtual ~SmsMessage() {};

    SmsMessage &withRecipient(const char *phoneNum) { this->recipient = phoneNum; return *this; };

    const char *getRecipient() const { return recipient; };

    bool hasRecipient() const { return recipient.length() != 0; };

    SmsMessage &withMessage(const char *message) { this->message = message; return *this; };

    const char *getMessage() const { return message; };
    
    SmsMessage &withExpirationMs(unsigned long milliseconds) { this->expirationMs = milliseconds; return *this; };
    SmsMessage &withExpiration(std::chrono::milliseconds chronoLiteral) { this->expirationMs = chronoLiteral.count(); return *this; };

    unsigned long getExpirationMs() const { return expirationMs; };

    void updateQueueMs() { queueMs = millis(); };

    bool isExpired() const;

protected:
    String recipient;
    String message;
    unsigned long expirationMs = 0;
    unsigned long queueMs = 0;
};

class SmsWebhook {
public:
    static SmsWebhook &instance();

    void setup();

    void loop();

    void queueSms(SmsMessage smsMessage);

    SmsWebhook &withEventName(const char *eventName) { this->eventName = eventName; return *this; };

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

protected:
    SmsWebhook();
    virtual ~SmsWebhook();

    /**
     * @brief This class is not copyable
     */
    SmsWebhook(const SmsWebhook&) = delete;

    /**
     * @brief This class is not copyable
     */
    SmsWebhook& operator=(const SmsWebhook&) = delete;

    void stateWaitForMessage();
    void stateWaitPublish();
    void stateWaitRetry();

    String eventName = "SendSmsEvent";
    std::function<bool(String&)> recipientCallback = 0;

    os_mutex_t sendQueueMutex = 0;
    std::deque<SmsMessage> sendQueue;
    particle::Future<bool> publishFuture;
    unsigned long stateTime = 0;
    unsigned long retryTimeMs = 0;

    unsigned long retryNoRecipientMs = 15000;
    unsigned long retryPublishFailMs = 15000;
    unsigned long publishRateLimitMs = 1010;

    static const size_t JSON_BUF_SIZE = 256;
    char *jsonBuf = 0;

    /**
     * @brief State handler for the main state machine
     */
    std::function<void(SmsWebhook &)> stateHandler = 0;

    static SmsWebhook *_instance;
};

#endif /* __SMSWEBHOOKRK_H */
