
#include "SmsWebhookRK.h"

static Logger _log("sms");

SmsWebhook *SmsWebhook::_instance;


SmsWebhook &SmsWebhook::instance() {
    if (!_instance) {
        _instance = new SmsWebhook();
    }
    return *_instance;
}

void SmsWebhook::setup() {
    os_mutex_create(&sendQueueMutex);
    stateHandler = &SmsWebhook::stateWaitForMessage;
}

void SmsWebhook::loop() {
    if (stateHandler) {
        stateHandler(*this);
    }
}


void SmsWebhook::queueSms(SmsMessage smsMessage) {
    if (!sendQueueMutex) {
        return;
    }

    os_mutex_lock(sendQueueMutex);
    sendQueue.push_back(smsMessage);
    os_mutex_unlock(sendQueueMutex);
}




SmsWebhook::SmsWebhook() {

}

SmsWebhook::~SmsWebhook() {
    if (sendQueueMutex) {
        os_mutex_destroy(sendQueueMutex);
    }
}


void SmsWebhook::stateWaitForMessage() {

    SmsMessage msg;
    bool hasMsg;

    os_mutex_lock(sendQueueMutex);
    hasMsg = !sendQueue.empty();
    if (hasMsg) {
        msg = sendQueue.front();
    }
    os_mutex_unlock(sendQueueMutex);

    if (!hasMsg || !Particle.connected()) {
        // No message to send OR
        // Not cloud connected, can't send event
        return;
    }

    // Do we need to query for a recipient?
    String recipient;

    if (!msg.hasRecipient()) {
        if (recipientCallback && !recipientCallback(recipient)) {
            // Don't know the recipient yet; try again after timeout
            _log.info("no recipient");
            stateTime = millis();
            retryTimeMs = retryNoRecipientMs;
            stateHandler = &SmsWebhook::stateWaitRetry;
            return;
        }
    }
    else {
        recipient = msg.getRecipient();
    }

    char jsonBuf[JSON_BUF_SIZE];    
    JSONBufferWriter writer(jsonBuf, JSON_BUF_SIZE - 1);

    writer.beginObject();
    writer.name("b").value(msg.getMessage());
    if (recipient.length() > 0) {
        writer.name("t").value(recipient);
    }
    writer.endObject();
    writer.buffer()[std::min(writer.bufferSize(), writer.dataSize())] = 0;
    
    _log.info("publishing %s", jsonBuf);

    // Have a message and are connected
    publishFuture = Particle.publish(eventName, jsonBuf, PRIVATE | WITH_ACK);

    stateTime = millis();
    stateHandler = &SmsWebhook::stateWaitPublish;
}

void SmsWebhook::stateWaitPublish() {
    // When checking the future, the isDone() indicates that the future has been resolved, 
    // basically this means that Particle.publish would have returned.
    if (publishFuture.isDone()) {
        // isSucceeded() is whether the publish succeeded or not, which is basically the
        // boolean return value from Particle.publish.
        if (publishFuture.isSucceeded()) {
            _log.info("successfully published");
            os_mutex_lock(sendQueueMutex);
            sendQueue.pop_front();
            os_mutex_unlock(sendQueueMutex);
            retryTimeMs = publishRateLimitMs;
        }
        else {
            _log.info("failed to publish, will try again");
            retryTimeMs = retryPublishFailMs;
        }
        stateTime = millis();
        stateHandler = &SmsWebhook::stateWaitRetry;
        return;
    }
}


void SmsWebhook::stateWaitRetry() {
    if (millis() - stateTime >= retryTimeMs) {
        stateHandler = &SmsWebhook::stateWaitForMessage;
    }
}
