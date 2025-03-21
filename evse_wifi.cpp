
/*
* WiFi Events

0  ARDUINO_EVENT_WIFI_READY               < ESP32 WiFi ready
1  ARDUINO_EVENT_WIFI_SCAN_DONE                < ESP32 finish scanning AP
2  ARDUINO_EVENT_WIFI_STA_START                < ESP32 station start
3  ARDUINO_EVENT_WIFI_STA_STOP                 < ESP32 station stop
4  ARDUINO_EVENT_WIFI_STA_CONNECTED            < ESP32 station connected to AP
5  ARDUINO_EVENT_WIFI_STA_DISCONNECTED         < ESP32 station disconnected from AP
6  ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
7  ARDUINO_EVENT_WIFI_STA_GOT_IP               < ESP32 station got IP from connected AP
8  ARDUINO_EVENT_WIFI_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
9  ARDUINO_EVENT_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
10 ARDUINO_EVENT_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
11 ARDUINO_EVENT_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
12 ARDUINO_EVENT_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
13 ARDUINO_EVENT_WIFI_AP_START                 < ESP32 soft-AP start
14 ARDUINO_EVENT_WIFI_AP_STOP                  < ESP32 soft-AP stop
15 ARDUINO_EVENT_WIFI_AP_STACONNECTED          < a station connected to ESP32 soft-AP
16 ARDUINO_EVENT_WIFI_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
17 ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
18 ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
19 ARDUINO_EVENT_WIFI_AP_GOT_IP6               < ESP32 ap interface v6IP addr is preferred
19 ARDUINO_EVENT_WIFI_STA_GOT_IP6              < ESP32 station interface v6IP addr is preferred
20 ARDUINO_EVENT_ETH_START                < ESP32 ethernet start
21 ARDUINO_EVENT_ETH_STOP                 < ESP32 ethernet stop
22 ARDUINO_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
23 ARDUINO_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
24 ARDUINO_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
19 ARDUINO_EVENT_ETH_GOT_IP6              < ESP32 ethernet interface v6IP addr is preferred
25 ARDUINO_EVENT_MAX
*/

#include "evse_wifi.h"
#include <string.h>
#include <ArduinoJson.h>

// Set Your WiFi Credentials Here
 String evse_ssid     = "EVRE HQ";      //Replace with Your Router SSID
 String evse_password = "Amplify@5";    //Replace with Your Router Password
// String evse_ssid = " ";     // Replace with Your Router SSID
// String evse_password = " "; // Replace with Your Router Password

// // Set your access point network credentials
// const char* ssid = "7S_9101982";
// const char* password = "123456789";
// IPAddress local_ip(192,168,0,1);
// IPAddress gateway(192,168,0,1);
// IPAddress subnet(255,255,255,0);
// const int   channel        = 14;                        // WiFi Channel number between 1 and 13
// const bool  hide_SSID      = true;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
// const int   max_connection = 2;

uint8_t disconnection_err_count = 0;
uint8_t isWifiConnected = 0;
uint8_t reasonLost = 0;
int32_t rssi = 0;

uint8_t wifi_Connect = 0;
int chan;

WiFiEventId_t eventID;
bool wifi_deinit_flag = false;

bool websocketDisconnectFlag = false;
bool iswebSocketConncted = false;
WebSocketsClient webSocket;
// String getReq_serverURL = "ws://10.0.0.17:8080/ws/user";
// String getReq_serverURL = "10.0.0.17";
String getReq_serverURL = "192.168.57.32";
int websocketServerPort = 8080;
String alpid = "";
String picUploadStatus = "";
char *alp = NULL;
char *temp = NULL;
TaskHandle_t websocket_task_handle;
char internetStatusBuf[30] = "";
// Create a JSON object
// Serialize JSON to a string
String jsonString = "";
StaticJsonDocument<64> jsonDoc;

void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_READY:
        Serial.println("WiFi interface ready");
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        Serial.println("Completed scan for access points");
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        Serial.println("WiFi clients stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.print(F("LBS Connected to Acess Point: \""));
        Serial.print(evse_ssid);
        Serial.print(F("\" Mac Id: "));
        Serial.println(WiFi.BSSIDstr());
        disconnection_err_count = 0;
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        isWifiConnected = 0;
        Serial.print(F("LBS Disconnected from WiFi access point\r\n"));
        chan = WiFi.channel();

        if (websocketDisconnectFlag == true)
        {
            webSocket.disconnect();
            webSocket.disableHeartbeat();
            suspendWebsocketTask();
            websocketDisconnectFlag = false;
        }

        // Serial.print("Wi-Fi CHANNEL ");
        // Serial.println(WiFi.channel());
        if (reasonLost == WIFI_REASON_NO_AP_FOUND ||
            reasonLost == WIFI_REASON_ASSOC_LEAVE ||
            reasonLost == WIFI_REASON_AUTH_EXPIRE)
        {
            if (disconnection_err_count++ < 5)
            {
                delay(5000);
                esp_wifi_connect();
                break;
            }
            else
            {
                disconnection_err_count = 0;
                Serial.print(F("WIFI retries exceeded..!\r\n"));
                WiFi.disconnect(true);
                delay(1000);
                WiFi.mode(WIFI_STA);
                WiFi.begin(evse_ssid, evse_password);
                delay(6500);
            }
        }
        Serial.print(F("LBS Device FREE HEAP: "));
        Serial.println(ESP.getFreeHeap());
        break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
        Serial.println("Authentication mode of access point has changed");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("Obtained IP address: ");
        Serial.println(WiFi.localIP());
        disconnection_err_count = 0;
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        Serial.println("Lost IP address and IP address is reset to 0");
        delay(500);
        esp_wifi_connect();
        delay(500);
        break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
        Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
        break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
        Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
        Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
        break;
    case ARDUINO_EVENT_WPS_ER_PIN:
        Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
        break;
    case ARDUINO_EVENT_WIFI_AP_START:
        Serial.println("WiFi access point started");
        break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
        Serial.println("WiFi access point  stopped");
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        Serial.println("Client connected");
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        // isWifiConnected = 0;
        Serial.println("Client disconnected");
        break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        Serial.println("Assigned IP address to client");
        break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
        Serial.println("Received probe request");
        break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
        Serial.println("AP IPv6 is preferred");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
        Serial.println("STA IPv6 is preferred");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
        Serial.println("Ethernet IPv6 is preferred");
        break;
    case ARDUINO_EVENT_ETH_START:
        Serial.println("Ethernet started");
        break;
    case ARDUINO_EVENT_ETH_STOP:
        Serial.println("Ethernet stopped");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        Serial.println("Ethernet connected");
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println("Ethernet disconnected");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.println("Obtained IP address");
        break;
    default:
        break;
    }
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    // Serial.println("WiFi connected");
    // Serial.println("IP address: ");
    // Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
    isWifiConnected = 1;
    chan = WiFi.channel();

    if (iswebSocketConncted == false)
    {
        webSocket.disconnect();
        delay(1000);
        connectToWebsocket();
        startWebsocketTask();
        iswebSocketConncted = true;
        websocketDisconnectFlag = true; // ready to disconnect when wifi disconnect once
    }
    else
    {
        webSocket.disconnect();
        delay(1000);
        Serial.println(F("Reconnect to Websocket"));
        connectToWebsocket();
        delay(1000);
        resumeWebsocketTask();
        websocketDisconnectFlag = true; // ready to disconnect when wifi disconnect once
    }
    Serial.print(F("LBS Device FREE HEAP: "));
    Serial.println(ESP.getFreeHeap());
    // Serial.print("Wi-Fi CHANNEL ");
    // Serial.println(WiFi.channel());
    // reConnectAttempt = 0;
}

void wifi_init(void)
{
    // delete old config
    WiFi.disconnect(true);

    delay(1000);

    // Examples of different ways to register wifi events
    WiFi.onEvent(WiFiEvent);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
                           {
                               Serial.print(F("WiFi lost connection. Reason: "));
                               reasonLost = info.wifi_sta_disconnected.reason;
                               Serial.println(reasonLost);
                               // Serial.println(info.wifi_sta_disconnected.reason);
                           },
                           WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    // Remove WiFi event
    Serial.print(F("WiFi Event ID: "));
    Serial.println(eventID);
    // WiFi.removeEvent(eventID);

    WiFi.mode(WIFI_AP_STA);
    // // Setting the ESP as an access point
    // Serial.print("Setting AP (Access Point)…");
    // // Remove the password parameter, if you want the AP (Access Point) to be open
    // // WiFi.softAP(ssid, password);
    // WiFi.softAPConfig(local_ip, gateway, subnet);
    // WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
    // // WiFi.softAP(ssid, password);

    // IPAddress IP = WiFi.softAPIP();
    // Serial.print("AP IP address: ");
    // Serial.println(IP);

    WiFi.begin(evse_ssid, evse_password);

    // Serial.println();
    // Serial.println();
    Serial.println("Wait for WiFi... ");
    while ((WiFi.status() != WL_CONNECTED) && (wifi_Connect++ < 10))
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            wifi_Connect = 0;
            break;
        }
        Serial.print(F("* "));
        delay(1500);
    }

    // Serial.print(F("LBS MAC Address:  "));
    // Serial.println(WiFi.macAddress());
    // chan = WiFi.channel();
    // Serial.print(F("Wi-Fi CHANNEL "));
    // Serial.println(WiFi.channel());

#if 0
    WiFi.softAP("esp32", "123456789", chan, 0, 6);
    // WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("LBS AP IP address: ");
    Serial.println(IP);
#endif
    // chan = WiFi.channel();
    // Serial.print(F("Wi-Fi CHANNEL "));
    // Serial.println(WiFi.channel());
}

void wifi_deinit(void)
{
    WiFi.removeEvent(eventID);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    // esp_wifi_stop();
    // esp_wifi_deinit();
    Serial.print(F("WiFi De-Initialized\r\n"));
    delay(1000);
}

void wifi_connection_retry(void)
{
    delay(5000);
    esp_wifi_connect();
}

/* ****************** WEBSOCKET EVENT HANDLER *************** */

void startWebsocketTask()
{
    xTaskCreatePinnedToCore(
        websocketTask, "Task websocket" // A name just for humans
        ,
        8096 // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
        ,
        NULL // When no parameter is used, simply pass NULL
        ,
        2 // Priority
        ,
        &websocket_task_handle, 1 // Task handle is not used here - simply pass NULL
    );
}
// Function to suspend the websocket task
void suspendWebsocketTask()
{
    vTaskSuspend(websocket_task_handle);
}

// Function to resume the websocket task
void resumeWebsocketTask()
{
    vTaskResume(websocket_task_handle);
}

void websocketTask(void *pvParameters)
{ // This is a task.
    for (;;)
    {
        webSocket.loop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16)
{
    const uint8_t *src = (const uint8_t *)mem;
    // USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
    Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);

    for (uint32_t i = 0; i < len; i++)
    {
        if (i % cols == 0)
        {
            Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
        }
        Serial.printf("%02X ", *src);
        src++;
    }
    Serial.printf("\n");
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        // Serial.printf("[WSc] Disconnected!\n");
        memcpy(internetStatusBuf, "[WSc] Disconnected!", sizeof("[WSc] Disconnected!"));
        Serial.println(internetStatusBuf);
        break;
    case WStype_CONNECTED:
        Serial.printf("[WSc] Connected to url: %s\n", payload);

        // send message to server when Connected
        // webSocket.sendTXT("Connected");
        // delay(50);
        // send ALPR ID to server7
        
        // jsonDoc["con"] = "LBS SERVER";

        // serializeJson(jsonDoc, jsonString);

        // // Send JSON data over WebSocket
        // webSocket.sendTXT(jsonString);
        // Serial.println("Sent JSON: " + jsonString);
        // jsonDoc.clear();
        // jsonString = ""; // Optionally clear the string as well
        // webSocket.sendTXT("LBS SERVER");
        break;
    case WStype_TEXT:
        Serial.printf("\r\nRecieved data from server: %s\r\n", payload);

        alp = (char *)payload;
        temp = strtok(alp, "_");
        // Serial.printf("%s\r\n",temp);
        if (!strncmp("get", temp, 3))
        {
            temp = strtok(NULL, "_");
            // Serial.printf("%s\r\n",temp);
            if (!strncmp("image", temp, 5))
            {
                temp = strtok(NULL, "\n");
                // Serial.printf("%s\r\n",temp);
                alpid = temp;
                // Serial.println(alpid);
            }
            else if (!strncmp("status", temp, 6))
            {
                webSocket.sendTXT(picUploadStatus);
            }
        }
        temp = "";
        break;
    case WStype_BIN:
        Serial.printf("[WSc] get binary length: %u\n", length);
        hexdump(payload, length);

        // send data to server
        // webSocket.sendBIN(payload, length);
        break;
    case WStype_PING:
        // pong will be send automatically
        Serial.print(F("[WSc] get ping\n"));
        break;
    case WStype_PONG:
        // answer to a ping we send
        // Serial.print("[WSc] get pong\n");
        Serial.print(F("[WSc] get pong\n"));
        break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        break;
    }
}

void connectToWebsocket(void)
{
    Serial.print("Connecting to: ");
    // Serial.println(path_m);
    // webSocket_wifi.begin(host_m, port_m, path_m, protocol);
    // server address, port and URL
    // webSocket.begin("www.google.com", 81, "/");
    // webSocket.begin("central.pulseenergy.io", 80, "/ws/OCPP16J/CPIW77GC3A", "ocpp1.6");
    // webSocket.begin("34.100.138.28", 8080, "34.100.138.28", "ws");
    // webSocket.begin("10.0.0.17", 8080, "/user", "ws");
    webSocket.begin(getReq_serverURL, websocketServerPort, "/ws/device", "ws");

    // event handler
    webSocket.onEvent(webSocketEvent);

    // use HTTP Basic Authorization this is optional remove if not needed
    // webSocket.setAuthorization("user", "Password");

    // try ever 5000 again if connection has failed
    webSocket.setReconnectInterval(30000);
    webSocket.enableHeartbeat(25000, 3000, 2);
}