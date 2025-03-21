/// @file LBS_Proj.ino
/// @Author: Gopikrishna S
/// @date 09/01/2025
/// @brief ESP-NOW Master Code
/// @All Rights Reserved EVRE

#include "evse_espnow_master.h"
#include "evse_wifi.h"

#define MEM_DEBUG_EN    (0)
#define WATCHDOG_ENABLE (1)

#if WATCHDOG_ENABLE
#include <esp_task_wdt.h>
// 2min WDT
#define WDT_TIMEOUT 120
#endif

extern uint8_t device_id[10] ;
void TaskPhShift(void *pvParameters);

extern uint8_t test_success;
extern uint8_t slave3_mac[6];
extern struct SENDDATA send_data;
void setup()
{
  Serial.begin(115200);
  while (!Serial);

#if MEM_DEBUG_EN
  Serial.print(F("Starting Setup FREE HEAP: "));
  Serial.println(ESP.getFreeHeap());
#endif

#if WATCHDOG_ENABLE
  //  Initializing WatchDogTimer
  Serial.print(F("Configuring WDT . . . \r\n"));
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);              // add current thread to WDT watch
#endif

  // WiFi.disconnect();
  // delay(5);
  // WiFi.mode(WIFI_STA);
  // delay(5);
  wifi_init();
  // Serial.print("Current channel no: ");
  // Serial.println( WiFi.channel() );

  // ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  // // wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
  // ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));
  // ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));

  // WiFi.printDiag(Serial);

  Serial.print("Load Balance System MAC:  ");
  Serial.println(WiFi.macAddress());

  initESPNow();

  uint32_t blink_delay = 1000; // Delay between changing state on LED pin
  xTaskCreate(
      TaskPhShift, "Task Phase Shift" // A name just for humans
      ,
      8096 // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
      ,
      (void *)&blink_delay // Task parameter which can modify the task behavior. This must be passed as pointer to void.
      ,
      2 // Priority
      ,
      NULL // Task handle is not used here - simply pass NULL
  );

#if MEM_DEBUG_EN
  Serial.print(F("After Esp-Now Init, FREE HEAP: "));
  Serial.println(ESP.getFreeHeap());
#endif
}

void loop()
{
#if MEM_DEBUG_EN
  Serial.print(F("Run Time FREE HEAP: "));
  Serial.print(ESP.getFreeHeap());
  Serial.println(" Bytes");
#endif

#if WATCHDOG_ENABLE
  esp_task_wdt_reset();
#endif

  delay(500);
}

void TaskPhShift(void *pvParameters)
{
  int i = 0;
  while (true)
  {
    if (((phase_R_cnt) || (phase_Y_cnt) || (phase_B_cnt)) == 0)
    {
      // Serial.println("R Assigned");
      phase_type = phase_R;
    }
    if((phase_R_cnt <= phase_Y_cnt) && (phase_Y_cnt <= phase_B_cnt))
    {
      phase_type = phase_R;
    }
    if ((phase_R_cnt > phase_Y_cnt) && (phase_Y_cnt <= phase_B_cnt))
    {
      // Serial.println("Y assign");
      phase_type = phase_Y;

    }
    if ((phase_R_cnt >= phase_Y_cnt) && (phase_Y_cnt > phase_B_cnt))
    {
      // Serial.println("B assign");
      phase_type = phase_B;
    }
    if (((phase_R_cnt) && (phase_Y_cnt) && (phase_B_cnt)) == 1)
    {
      // Serial.println("R Assigned");
      phase_type = phase_R;
    }

    if ((phase_R_cnt) == 2)
    {
      send_phaseR_change(device_phaseR1);
    }

#if WATCHDOG_ENABLE
    esp_task_wdt_reset();
#endif
    delay(100);

    if (((i++) % 251) == 0)
    {
      Serial.println("\r\nR: " + String(phase_R_cnt) + " Y: " + String(phase_Y_cnt) + " B: " + String(phase_B_cnt));
      Serial.print(F("Run Time FREE HEAP: "));
      Serial.print(ESP.getFreeHeap());
      Serial.println(" Bytes");
    }
#if 0   /// for test
    if (test_success == 1)
    {
      if (((i) % 997) == 0)
      {
        test_success = 1;
        strcpy(send_data.header1, "P");
        send_data.deviceID = 3;
        send_data.phase = phase_type;
        send_data.action = PHASE_CHANGE;
        send_data.checksum = send_data.deviceID + send_data.phase;
        strcpy(send_data.footer, "#");
        // device3_phase = send_data.phase;
        wait_success_send(slave3_mac);
      }
    }
#endif
  }
  // delay(100);
}
