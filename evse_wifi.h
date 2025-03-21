#ifndef EVSE_WIFI_H
#define EVSE_WIFI_H

#include <WiFi.h>
#include "esp_wifi.h"
// #include "WebSockets.h"
#include <WebSocketsClient.h>
// #include "variants.h"

void startWebsocketTask();
void websocketTask(void *pvParameters);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void suspendWebsocketTask();
void resumeWebsocketTask();
void connectToWebsocket(void);
void wifi_init(void);
void wifi_deinit(void);
void wifi_connection_retry(void);

extern uint8_t isWifiConnected;
extern int32_t rssi;
extern int chan;
extern bool wifi_deinit_flag;

extern TaskHandle_t websocket_task_handle;
// extern const char* evse_ssid ;
// extern const char* evse_password;
#endif