//// @file evse_espnow_master.cpp
//// @Author: Gopikrishna S
//// @date 07/11/2024
//// @brief ESP-NOW Master Code
//// @All Rights Reserved EVRE

#include "evse_espnow_master.h"
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
// enum PhaseType {phase_R, phase_Y, phase_B};
enum ChargingStatus
{
  not_charging,
  charging,
  power
};
enum DeviceIdentity
{
  DEVICE1 = 1,
  DEVICE2,
  DEVICE3,
  DEVICE4,
  DEVICE5,
  DEVICE6,
  DEVICE7,
  DEVICE8,
  DEVICE9,
  DEVICE10
};
// enum ActionType {PHASE_ASSIGN, PHASE_CHANGE};

uint8_t slave1_mac[6];
uint8_t slave2_mac[6];
uint8_t slave3_mac[6];
uint8_t slave4_mac[6];
uint8_t slave5_mac[6];
uint8_t slave6_mac[6];
uint8_t slave7_mac[6];
uint8_t slave8_mac[6];
uint8_t slave9_mac[6];
uint8_t slave10_mac[6];

uint8_t device_id[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

esp_now_peer_info_t slave;

//websocket
extern WebSocketsClient webSocket;
extern String jsonString;
extern StaticJsonDocument<200> jsonDoc;

// Variable to store if sending data was successful
uint8_t sent_status = 11;
uint8_t data_sent = 0;

volatile uint8_t device1_phase = 0;
volatile uint8_t device2_phase = 0;
volatile uint8_t device3_phase = 0;
volatile uint8_t device4_phase = 0;
volatile uint8_t device5_phase = 0;
volatile uint8_t device6_phase = 0;
volatile uint8_t device7_phase = 0;
volatile uint8_t device8_phase = 0;
volatile uint8_t device9_phase = 0;
volatile uint8_t device10_phase = 0;

volatile uint8_t device_phaseR0 = 0;
volatile uint8_t device_phaseY0 = 0;
volatile uint8_t device_phaseB0 = 0;

volatile uint8_t device_phaseR1 = 0;
volatile uint8_t device_phaseY1 = 0;
volatile uint8_t device_phaseB1 = 0;

volatile uint8_t phase_type = 0;

volatile uint8_t phase_R_cnt = 0;
volatile uint8_t phase_Y_cnt = 0;
volatile uint8_t phase_B_cnt = 0;

uint8_t test_success = 0;

struct SENDDATA send_data;

// Structure RECIEVEDDATA getting data from another node
#pragma pack(1)
struct RECIEVEDDATA
{
  char header1[2];
  uint8_t deviceID;
  uint8_t phase;
  float power;
  uint8_t status;
  uint16_t checksum;
  char footer[2];
};
RECIEVEDDATA recieved_data;

/**
 * @brief      Print MAC address
 *
 * @param[in]  mac_addr  The MAC address
 *
 * @details    This function prints the MAC address in the format "xx:xx:xx:xx:xx:xx"
 */
void printMAC(const uint8_t *mac_addr)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  memset(&send_data, '\0', sizeof(send_data));
  Serial.print("Last Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail");
  test_success = test_success + 1;
  printMAC(mac_addr);
  Serial.println();

  if (status != 0)
  {
    // pairingStatus = NOT_PAIRED;
  }
  if (data_sent == 1)
  {
    sent_status = status;
    data_sent = 0;
  }
}

/**
 * @brief      Deletes the specified peer from the ESP-NOW network.
 *
 * @details    This function attempts to remove a peer with the address stored
 *             in `slave.peer_addr` from the ESP-NOW network. It prints the
 *             status of the delete operation to the Serial monitor.
 *             Possible status messages include:
 *             - Success: The peer was successfully deleted.
 *             - ESPNOW Not Init: ESP-NOW is not initialized.
 *             - Invalid Argument: An invalid argument was provided.
 *             - Peer not found: The peer address does not exist in the network.
 *             - Not sure what happened: An unexpected error occurred.
 */
void deletePeer(void)
{
  esp_err_t delStatus = esp_now_del_peer(slave.peer_addr);
  Serial.print("Slave Delete Status: ");
  if (delStatus == ESP_OK)
  {
    // Delete success
    Serial.println("Success");
  }
  else if (delStatus == ESP_ERR_ESPNOW_NOT_INIT)
  {
    // How did we get so far!!
    Serial.println("ESPNOW Not Init");
  }
  else if (delStatus == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (delStatus == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Not sure what happened");
  }
}

/**
 * @brief      Attempts to send data to a specified device using ESP-NOW.
 *
 * @param[in]  device_ID  The ID of the device to send data to.
 * @param[in]  mac_addr   The MAC address of the target device.
 *
 * @details    This function first prints the target device information, then checks if the device
 *             is already paired. If not, it attempts to add the device as a peer. It then sends
 *             data to the device using ESP-NOW. The function includes a commented block that,
 *             when enabled, retries sending data multiple times until successful.
 */
void wait_success_send(uint8_t device_ID, uint8_t phase_type, float assign_load, const uint8_t *mac_addr)
{
  uint8_t sen_cnt = 0;
  // memset(&send_data, '\0', sizeof(send_data));
  Serial.println("assign phase - " + String(phase_type));
  Serial.println("assign load - " + String(assign_load));
  strcpy(send_data.header1, "P");
  send_data.deviceID = device_ID;
  send_data.phase = phase_type;
  send_data.load = assign_load;
  send_data.action = PHASE_ASSIGN;
  send_data.checksum = send_data.deviceID + send_data.phase;
  strcpy(send_data.footer, "#");
  Serial.print("Destination Device-" + String(device_ID) + " MAC: ");
  printMAC(mac_addr);
  // Register peer
  // Serial.println("slave setting ch: "  + String(WiFi.channel()));
  // slave.channel = 0;
  // slave.encrypt = false;

  // WiFi.softAPmacAddress();
  // Add peer
  memset(slave.peer_addr, 0, 6);
  memcpy(slave.peer_addr, mac_addr, 6);
  bool exists = esp_now_is_peer_exist(slave.peer_addr);
  if (exists)
  {
    // Slave already paired.
    Serial.println("Already Paired");
    // return true;
  }
  else
  {
    if (esp_now_add_peer(&slave) != ESP_OK)
    {
      Serial.println("Failed to add peer");
      return;
    }
  }

#if 0
  while (sen_cnt++ < 14)
  {
    // Serial.print(F("\r\nRetry count: "));
    // Serial.print(sen_cnt);
    Serial.print(F("* "));
    esp_now_send(slave.peer_addr, (uint8_t *)&send_data, sizeof(send_data));
    delay(50);
    if (sent_status == 0)
    {
      deletePeer();
      sent_status = 11;
      break;
    }
  }
#else
  esp_now_send(slave.peer_addr, (uint8_t *)&send_data, sizeof(send_data));
  delay(50);

  #if DASHBOARD_ENABLE
  //Update to Server page
  jsonDoc["dev"] = String(device_ID);
  switch(phase_type)
  {
    case 0:
      jsonDoc["phase"] = "R";
      break;
    case 1:
      jsonDoc["phase"] = "Y";
      break;
    case 2:
      jsonDoc["phase"] = "B";
      break;
  }
  jsonDoc["status"] = String("start");
  serializeJson(jsonDoc, jsonString);

  // Send JSON data over WebSocket
  webSocket.sendTXT(jsonString);
  Serial.println("Sent JSON: " + jsonString);
  // Clear JSON document
  jsonDoc.clear();
  jsonString = ""; // Optionally clear the string as well
  #endif
#endif
  Serial.println();
}

/**
 * @brief      Sends a phase assignment response to a specified device.
 *
 * @param[in]  device_ID  The ID of the device to send the response to.
 * @param[in]  mac_addr   The MAC address of the target device.
 *
 * @details    This function assigns a phase to the specified device and sends the phase
 *             assignment information using ESP-NOW. It updates the device's phase information
 *             and counts for phases R, Y, and B. The function supports up to 10 devices, and if
 *             the maximum number of devices is exceeded, it prints a message indicating the limit
 *             has been crossed.
 */
void send_Response(uint8_t device_ID, const uint8_t *mac_addr)
{
  switch (device_ID)
  {
  case DEVICE1:
    memset(slave1_mac, 0, 6);
    memcpy(slave1_mac, mac_addr, 6);

    device1_phase = phase_type;

    switch (device1_phase)
    {
    case phase_R:
      // jsonDoc["phase"] = "R";
      ++phase_R_cnt;
      if (phase_R_cnt == 1)
      {
        device_phaseR0 = DEVICE1;
        if ((phase_Y_cnt) && (phase_B_cnt))
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseY0, 6.6);
          load_change(device_phaseB0, 6.6);
          #endif
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE1, device1_phase, 6.6, slave1_mac);
          #endif
        }
        else
        {
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE1, device1_phase, 7.4, slave1_mac);
          #endif
        }  
        Serial.println("\r\nDev1 phase R0\r\n");
      }
      else
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseY0, 6.6);
        load_change(device_phaseB0, 6.6);
        #endif
        if(device_phaseR0)
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR0, 3.3);
          #endif
        }
        else
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR1, 3.3);
          #endif
        }
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE1, device1_phase, 3.3, slave1_mac);
        #endif
        device_phaseR1 = DEVICE1;
        Serial.println("\r\nDev1 phase R1\r\n");
      }
      break;

    case phase_Y:
      // jsonDoc["phase"] = "Y";
      ++phase_Y_cnt;
      if (phase_Y_cnt == 1)
      {
        if ((phase_R_cnt == 1) && (phase_B_cnt))
        {
          if (device_phaseR0)
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR0, 6.6);
            #endif
          }
          else
          { 
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR1, 6.6);
            #endif
          }
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseB0, 6.6);
          #endif
          
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE1, device1_phase, 6.6, slave1_mac);
          #endif
        }
        else
        {
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE1, device1_phase, 7.4, slave1_mac);
          #endif
        }
        device_phaseY0 = DEVICE1;
      }
      else
      {
        device_phaseY1 = DEVICE1;
      }
      break;

    case phase_B:
      // jsonDoc["phase"] = "B";
      ++phase_B_cnt;
      if (phase_B_cnt == 1)
      {
        if ((phase_R_cnt == 1) && (phase_Y_cnt))
        {
          if(device_phaseR0)
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR0, 6.6);
            #endif
          }
          else
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR1, 6.6);
            #endif
          }
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseY0, 6.6);
          #endif
          // load_change(device_phaseR1, 6.6);
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE1, device1_phase, 6.6, slave1_mac);
          #endif
        }
        else
        {
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE1, device1_phase, 7.4, slave1_mac);
          #endif
        }
        device_phaseB0 = DEVICE1;
      }
      else
      {
        device_phaseB1 = DEVICE1;
      }
      break;
    }
    
    break;

  case DEVICE2:
    memset(slave2_mac, 0, 6);
    memcpy(slave2_mac, mac_addr, 6);

    device2_phase = phase_type;

    switch (device2_phase)
    {
    case phase_R:
      // jsonDoc["phase"] = "R";
      ++phase_R_cnt;
      if (phase_R_cnt == 1)
      {
        device_phaseR0 = DEVICE2;
        if ((phase_Y_cnt) && (phase_B_cnt))
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseY0, 6.6);
          load_change(device_phaseB0, 6.6);
          #endif
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE2, device2_phase, 6.6, slave2_mac);
          #endif
        }
        else
        {
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE2, device2_phase, 7.4, slave2_mac);
          #endif
        }
        Serial.println("\r\nDev2 phase R0\r\n");
      }
      else
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseY0, 6.6);
        load_change(device_phaseB0, 6.6);
        #endif
        if(device_phaseR0)
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR0, 3.3);
          #endif
        }
        else
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR1, 3.3);
          #endif
        }
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE2, device2_phase, 3.3, slave2_mac);
        #endif
        device_phaseR1 = DEVICE2;
        Serial.println("\r\nDev2 phase R1\r\n");
      }
      break;

    case phase_Y:
      // jsonDoc["phase"] = "Y";
      ++phase_Y_cnt;
      if (phase_Y_cnt == 1)
      {
        if ((phase_R_cnt == 1) && (phase_B_cnt))
        {
          if(device_phaseR0)
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR0, 6.6);
            #endif
          }
          else{
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR1, 6.6);
            #endif
          }
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseB0, 6.6);
          #endif
          // load_change(device_phaseR1, 6.6);
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE2, device2_phase, 6.6, slave2_mac);
          #endif
        }
        else
        {
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE2, device2_phase, 7.4, slave2_mac);
          #endif
        }
        device_phaseY0 = DEVICE2;
      }
      else
      {
        device_phaseY1 = DEVICE2;
      }
      break;

    case phase_B:
      // jsonDoc["phase"] = "B";
      ++phase_B_cnt;
      if (phase_B_cnt == 1)
      {
        if ((phase_R_cnt == 1) && (phase_Y_cnt))
        {
          if(device_phaseR0)
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR0, 6.6);
            #endif
          }
          else
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseR1, 6.6);
            #endif
          }
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseY0, 6.6);
          #endif
          // load_change(device_phaseR1, 6.6);
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE2, device2_phase, 6.6, slave2_mac);
          #endif
        }
        else
        {
          #if PHASE_ASSIGN_ENABLE
          wait_success_send(DEVICE2, device2_phase, 7.4, slave2_mac);
          #endif
        }
        device_phaseB0 = DEVICE2;
      }
      else
      {
        device_phaseB1 = DEVICE2;
      }
      break;

    default:
      break;
    }

    break;

case DEVICE3:
  memset(slave3_mac, 0, 6);
  memcpy(slave3_mac, mac_addr, 6);

  device3_phase= phase_type;

  switch (device3_phase)
  {
  case phase_R:
    // jsonDoc["phase"] = "R";
    ++phase_R_cnt;
    if (phase_R_cnt == 1)
    {
      device_phaseR0 = DEVICE3;
      if ((phase_Y_cnt) && (phase_B_cnt))
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseY0, 6.6);
        load_change(device_phaseB0, 6.6);
        #endif
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE3, device3_phase, 6.6, slave3_mac);
        #endif
      }
      else
      {
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE3, device3_phase, 7.4, slave3_mac);
        #endif
      }
      Serial.println("\r\nDev3 phase R0\r\n");
    }
    else
    {
      #if LOAD_ASSIGN_ENABLE
      load_change(device_phaseY0, 6.6);
      load_change(device_phaseB0, 6.6);
      #endif
      if(device_phaseR0)
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseR0, 3.3);
        #endif
      }
      else
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseR1, 3.3);
        #endif
      }
      #if PHASE_ASSIGN_ENABLE
      wait_success_send(DEVICE3, device3_phase, 3.3, slave3_mac);
      #endif
      device_phaseR1 = DEVICE3;
      Serial.println("\r\nDev3 phase R1\r\n");
    }
    break;

  case phase_Y:
    // jsonDoc["phase"] = "Y";
    ++phase_Y_cnt;
    if (phase_Y_cnt == 1)
    {
      if ((phase_R_cnt == 1) && (phase_B_cnt))
      {
        if(device_phaseR0)
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR0, 6.6);
          #endif
        }
        else{
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR1, 6.6);
          #endif
        }
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseB0, 6.6);
        #endif
        // load_change(device_phaseR1, 6.6);
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE3, device3_phase, 6.6, slave3_mac);
        #endif
      }
      else
      {
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE3, device3_phase, 7.4, slave3_mac);
        #endif
      }
      device_phaseY0 = DEVICE3;
    }
    else
    {
      device_phaseY1 = DEVICE3;
    }
    break;

  case phase_B:
    // jsonDoc["phase"] = "B";
    ++phase_B_cnt;
    if (phase_B_cnt == 1)
    {
      if ((phase_R_cnt == 1) && (phase_Y_cnt))
      {
        if(device_phaseR0)
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR0, 6.6);
          #endif
        }
        else{
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR1, 6.6);
          #endif
        }
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseY0, 6.6);
        #endif
        // load_change(device_phaseR1, 6.6);
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE3, device3_phase, 6.6, slave3_mac);
        #endif
      }
      else
      {
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE3, device3_phase, 7.4, slave3_mac);
        #endif
      }
      device_phaseB0 = DEVICE3;
    }
    else
    {
      device_phaseB1 = DEVICE3;
    }
    break;

  default:
    break;
  }
  
  break;

case DEVICE4:
  memset(slave4_mac, 0, 6);
  memcpy(slave4_mac, mac_addr, 6);

  device4_phase = phase_type;

  switch (device4_phase)
  {
  case phase_R:
    // jsonDoc["phase"] = "R";
    ++phase_R_cnt;
    if (phase_R_cnt == 1)
    {
      device_phaseR0 = DEVICE4;
      if ((phase_Y_cnt) && (phase_B_cnt))
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseY0, 6.6);
        load_change(device_phaseB0, 6.6);
        #endif
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE4, device4_phase, 6.6, slave4_mac);
        #endif
      }
      else
      {
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE4, device4_phase, 7.4, slave4_mac);
        #endif
      }
      Serial.println("\r\nDev4 phase R0\r\n");
    }
    else
    {
      #if LOAD_ASSIGN_ENABLE
      load_change(device_phaseY0, 6.6);
      load_change(device_phaseB0, 6.6);
      #endif
      if(device_phaseR0)
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseR0, 3.3);
        #endif
      }
      else
      {
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseR1, 3.3);
        #endif
      }
      #if PHASE_ASSIGN_ENABLE
      wait_success_send(DEVICE4, device4_phase, 3.3, slave4_mac);
      #endif
      device_phaseR1 = DEVICE4;
      Serial.println("\r\nDev4 phase R1\r\n");
    }
    break;

  case phase_Y:
    // jsonDoc["phase"] = "Y";
    ++phase_Y_cnt;
    if (phase_Y_cnt == 1)
    {
      if ((phase_R_cnt == 1) && (phase_B_cnt))
      {
        if(device_phaseR0)
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR0, 6.6);
          #endif
        }
        else
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR1, 6.6);
          #endif
        }
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseB0, 6.6);
        #endif
        // load_change(device_phaseR1, 6.6);
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE4, device4_phase, 6.6, slave4_mac);
        #endif
      }
      else
      {
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE4, device4_phase, 7.4, slave4_mac);
        #endif
      }
      device_phaseY0 = DEVICE4;
    }
    else
    {
      device_phaseY1 = DEVICE4;
    }
    break;

  case phase_B:
    // jsonDoc["phase"] = "B";
    ++phase_B_cnt;
    if (phase_B_cnt == 1)
    {
      if ((phase_R_cnt == 1) && (phase_Y_cnt))
      {
        if(device_phaseR0)
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR0, 6.6);
          #endif
        }
        else
        {
          #if LOAD_ASSIGN_ENABLE
          load_change(device_phaseR1, 6.6);
          #endif
        }
        #if LOAD_ASSIGN_ENABLE
        load_change(device_phaseY0, 6.6);
        #endif
        // load_change(device_phaseR1, 6.6);
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE4, device4_phase, 6.6, slave4_mac);
        #endif
      }
      else
      {
        #if PHASE_ASSIGN_ENABLE
        wait_success_send(DEVICE4, device4_phase, 7.4, slave4_mac);
        #endif
      }
      device_phaseB0 = DEVICE4;
    }
    else
    {
      device_phaseB1 = DEVICE4;
    }
    break;

  default:
    break;
  }

  break;

default:
  Serial.println("Max Connected Device Limit Cross..!");
}
}


/**
 * @brief      Function to count connected devices
 *
 * @param[in]  device_ID  The device ID
 * @param[in]  mac_addr   The mac address
 *
 * @details    This function will count the connected devices and store the device ID in the array.
 *             If the device ID already exists it will not count again.
 *
 * @return     void
 */
void device_count(uint8_t device_ID, const uint8_t *mac_addr)
{
  switch (device_ID)
  {
  case DEVICE1:
    if (device_id[0] == 0)
    {
      device_id[0] = 1;
      // jsonDoc["dev"] = String(DEVICE1);
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE2:
    if (device_id[1] == 0)
    {
      device_id[1] = 1;
      // jsonDoc["dev"] = String(DEVICE2);
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE3:
    if (device_id[2] == 0)
    {
      device_id[2] = 1;
      // jsonDoc["dev"] = String(DEVICE3);
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE4:
    if (device_id[3] == 0)
    {
      device_id[3] = 1;
      // jsonDoc["dev"] = String(DEVICE4);
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE5:
    if (device_id[4] == 0)
    {
      device_id[4] = 1;
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE6:
    if (device_id[5] == 0)
    {
      device_id[5] = 1;
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE7:
    if (device_id[6] == 0)
    {
      device_id[6] = 1;
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE8:
    if (device_id[7] == 0)
    {
      device_id[7] = 1;
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE9:
    if (device_id[8] == 0)
    {
      device_id[8] = 1;
      send_Response(device_ID, mac_addr);
    }

    break;
  case DEVICE10:
    if (device_id[9] == 0)
    {
      device_id[9] = 1;
      send_Response(device_ID, mac_addr);
    }

    break;
  default:
    Serial.println("Max Connected Device Limit Cross..!");
  }
}

/**
 * @brief      Deletes a device from the network based on its ID.
 *
 * @param[in]  device_ID  The ID of the device to be deleted.
 *
 * @details    This function checks if a device with the given ID is connected,
 *             and if so, it removes the device by resetting its ID state and
 *             reducing the connected device count. It updates and decrements
 *             the phase counters (R, Y, B) according to the phase assignment
 *             of the respective device. If the device is not found, it prints
 *             a message indicating the limit of connected devices has been
 *             exceeded.
 */
void delete_device(uint8_t device_ID, uint8_t stop_device_phase)
{
  switch (device_ID)
  {
  case DEVICE1:
    if ((device_id[0]) == 1)
    {
      device_id[0] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          --phase_R_cnt;

          if((phase_R_cnt == 1))
          {
            if (device_phaseR1 == DEVICE1)
            {
              if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR0, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR1 = 0;
            }
            if (device_phaseR0 == DEVICE1)
            {
              if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR1, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR0 = 0;
            }
          }
          else if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            load_change(device_phaseY0, 7.4);
            #endif
          }

          Serial.println("Dev1 R dec");
        }

        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE1);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif
        memset(slave1_mac, 0, sizeof(slave1_mac));
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        { 
          --phase_Y_cnt;
          Serial.println("Dev1 Y dec");
          if((phase_B_cnt == 1) && (phase_R_cnt == 2))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_B_cnt == 1) && (phase_R_cnt == 1))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            #endif
          }
          // device_phaseY0 = 0;
          // device_phaseR1 = 0;
        }

        delay(50);
        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE1);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif
        memset(slave1_mac, 0, sizeof(slave1_mac));
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev1 B dec");
          if((phase_Y_cnt == 1) && (phase_R_cnt == 2))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_Y_cnt == 1) && (phase_R_cnt == 1))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 7.4);
            #endif
          }
          // device_phaseB0 = 0;
          // device_phaseR1 = 0;
        }
        
        delay(50);
        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE1);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif
        memset(slave1_mac, 0, sizeof(slave1_mac));
        break;
      }
    }
    break;
  case DEVICE2:
    if ((device_id[1]) == 1)
    {
      device_id[1] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          --phase_R_cnt;
          if(phase_R_cnt == 1)
          {
            if(device_phaseR0 == DEVICE2)
            {
              if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR1, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR0 = 0;
            }
            if(device_phaseR1 == DEVICE2)
            {
              if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR0, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR1 = 0;
            }
          }
          else if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            load_change(device_phaseY0, 7.4);
            #endif
          }
          
          Serial.println("Dev2 R dec");
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE2);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave2_mac, 0, sizeof(slave2_mac));
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev2 Y dec");
          if((phase_B_cnt == 1) && (phase_R_cnt == 2))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_B_cnt == 1) && (phase_R_cnt == 1))
          {
            if (device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            #endif
          }
          // device_phaseY0 = 0;
          // device_phaseR1 = 0;
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE2);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave2_mac, 0, sizeof(slave2_mac));
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev2 B dec");
          if((phase_Y_cnt == 1) && (phase_R_cnt == 2))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_Y_cnt == 1) && (phase_R_cnt == 1))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 7.4);
            #endif
          }
          // device_phaseB0 = 0;
          // device_phaseR1 = 0;
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE2);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave2_mac, 0, sizeof(slave2_mac));
        break;
      }
    }
    break;
  case DEVICE3:
    if ((device_id[2]) == 1)
    {
      device_id[2] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          --phase_R_cnt;
          if(phase_R_cnt == 1)
          {
            if(device_phaseR0 == DEVICE3)
            {
              if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR1, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR0 = 0;
            }
            if(device_phaseR1 == DEVICE3)
            {
              if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR0, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR1 = 0;
            }
          }
          else if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            load_change(device_phaseY0, 7.4);
            #endif
          }
          Serial.println("Dev3 R dec");
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE3);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave3_mac, 0, sizeof(slave3_mac));
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev3 Y dec");
          if((phase_B_cnt == 1) && (phase_R_cnt == 2))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_B_cnt == 1) && (phase_R_cnt == 1))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            #endif
          }
          // device_phaseY0 = 0;
          // device_phaseR1 = 0;
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE3);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave3_mac, 0, sizeof(slave3_mac));
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev3 B dec");
          if((phase_Y_cnt == 1) && (phase_R_cnt == 2)) 
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_Y_cnt == 1) && (phase_R_cnt == 1))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 7.4);
            #endif
          }
          // device_phaseB0 = 0;
          // device_phaseR1 = 0;
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE3);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave3_mac, 0, sizeof(slave3_mac));
        break;
      }
    }
    break;
  case DEVICE4:
    if ((device_id[3]) == 1)
    {
      device_id[3] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          --phase_R_cnt;
          if(phase_R_cnt == 1)
          {
            if(device_phaseR0 == DEVICE4)
            {
              if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR1, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR0 = 0;
            }
            if(device_phaseR1 == DEVICE4)
            {
              if((phase_B_cnt == 1) && (phase_Y_cnt == 1))
              {
                #if LOAD_ASSIGN_ENABLE
                load_change(device_phaseR0, 6.6);
                load_change(device_phaseB0, 6.6);
                load_change(device_phaseY0, 6.6);
                #endif
              }
              device_phaseR1 = 0;
            }
          }
          else if ((phase_B_cnt == 1) && (phase_Y_cnt == 1))
          {
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            load_change(device_phaseY0, 7.4);
            #endif
          }
          Serial.println("Dev4 R dec");
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE4);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave4_mac, 0, sizeof(slave4_mac));
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev4 Y dec");
          if((phase_B_cnt == 1) && (phase_R_cnt == 2))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_B_cnt == 1) && (phase_R_cnt == 1))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseB0, 7.4);
            #endif
          }
          // device_phaseY0 = 0;
          // device_phaseR1 = 0;
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE4);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave4_mac, 0, sizeof(slave4_mac));
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev4 B dec");
          if((phase_Y_cnt == 1) && (phase_R_cnt == 2)) 
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 6.6);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 6.6);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 6.6);
            #endif
            // load_change(device_phaseR1, 6.6);
          }
          else if((phase_Y_cnt == 1) && (phase_R_cnt == 1))
          {
            if(device_phaseR0)
            {
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR0, 7.4);
              #endif
            }
            else{
              #if LOAD_ASSIGN_ENABLE
              load_change(device_phaseR1, 7.4);
              #endif
            }
            #if LOAD_ASSIGN_ENABLE
            load_change(device_phaseY0, 7.4);
            #endif
          }
          // device_phaseB0 = 0;
          // device_phaseR1 = 0;
        }
        
        delay(50);

        #if DASHBOARD_ENABLE
        jsonDoc["dev"] = String(DEVICE4);
        switch(stop_device_phase)
        {
        case phase_R:
          jsonDoc["phase"] = "R";
          break;
        case phase_Y:
          jsonDoc["phase"] = "Y";
          break;
        case phase_B:
          jsonDoc["phase"] = "B";
          break;
        }
        jsonDoc["status"] = String("stop");
        serializeJson(jsonDoc, jsonString);
        // Send JSON data over WebSocket
        webSocket.sendTXT(jsonString);
        Serial.println("Sent JSON: " + jsonString);
        // Clear JSON document
        jsonDoc.clear();
        jsonString = ""; // Optionally clear the string as well
        #endif

        memset(slave4_mac, 0, sizeof(slave4_mac));
        break;
      }
    }
    break;
  case DEVICE5:
    if ((device_id[4]) == 1)
    {
      device_id[4] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          if (phase_R_cnt == 2)
          {
            device_phaseR1 = 0;
          }
          else
          {
            device_phaseR0 = 0;
          }
          --phase_R_cnt;
          Serial.println("Dev5 R dec");
        }
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev5 Y dec");
        }
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev5 B dec");
        }
        break;
      }
    }
    break;
  case DEVICE6:
    if ((device_id[5]) == 1)
    {
      device_id[5] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          if (phase_R_cnt == 2)
          {
            device_phaseR1 = 0;
          }
          else
          {
            device_phaseR0 = 0;
          }
          --phase_R_cnt;
          Serial.println("Dev6 R dec");
        }
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev6 Y dec");
        }
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev6 B dec");
        }
        break;
      }
    }
    break;
  case DEVICE7:
    if ((device_id[6]) == 1)
    {
      device_id[6] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          if (phase_R_cnt == 2)
          {
            device_phaseR1 = 0;
          }
          else
          {
            device_phaseR0 = 0;
          }
          --phase_R_cnt;
          Serial.println("Dev7 R dec");
        }
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev7 Y dec");
        }
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev7 B dec");
        }
        break;
      }
    }
    break;
  case DEVICE8:
    if ((device_id[7]) == 1)
    {
      device_id[7] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          if (phase_R_cnt == 2)
          {
            device_phaseR1 = 0;
          }
          else
          {
            device_phaseR0 = 0;
          }
          --phase_R_cnt;
          Serial.println("Dev8 R dec");
        }
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev8 Y dec");
        }
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev8 B dec");
        }
        break;
      }
    }
    break;
  case DEVICE9:
    if ((device_id[8]) == 1)
    {
      device_id[8] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          if (phase_R_cnt == 2)
          {
            device_phaseR1 = 0;
          }
          else
          {
            device_phaseR0 = 0;
          }
          --phase_R_cnt;
          Serial.println("Dev9 R dec");
        }
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev9 Y dec");
        }
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev9 B dec");
        }
        break;
      }
    }
    break;
  case DEVICE10:
    if ((device_id[9]) == 1)
    {
      device_id[9] = 0;

      switch (stop_device_phase)
      {
      case phase_R:
        if (phase_R_cnt > 0)
        {
          if (phase_R_cnt == 2)
          {
            device_phaseR1 = 0;
          }
          else
          {
            device_phaseR0 = 0;
          }
          --phase_R_cnt;
          Serial.println("Dev10 R dec");
        }
        break;

      case phase_Y:
        if (phase_Y_cnt > 0)
        {
          --phase_Y_cnt;
          Serial.println("Dev10 Y dec");
        }
        break;

      case phase_B:
        if (phase_B_cnt > 0)
        {
          --phase_B_cnt;
          Serial.println("Dev10 B dec");
        }
        break;
      }
    }
    break;
  default:
    Serial.println("Max Connected Device Limit Cross..!");
  }
}

/**
 * @brief      Sends a phase change packet for a specified device.
 *
 * @param[in]  device_ID  The ID of the device for which the phase change is to be sent.
 * @param[in]  phase      The new phase to be assigned to the device.
 * @param[in]  mac_addr   The MAC address of the target device.
 *
 * @details    This function constructs a data packet with the new phase information and sends
 *             it to the specified device using ESP-NOW. The packet includes a header, device ID,
 *             phase, action type, checksum, and footer. It prints the destination device
 *             information before sending the packet.
 */
void send_phaseR_change_packet(uint8_t device_ID, uint8_t phase, const uint8_t *mac_addr)
{
  memset(&send_data, '\0', sizeof(send_data));
  Serial.println("send change phase - " + String(phase));
  strcpy(send_data.header1, "P");
  send_data.deviceID = device_ID;
  send_data.phase = phase;
  send_data.load = 6.60f;
  send_data.action = PHASE_CHANGE;
  send_data.checksum = send_data.deviceID + send_data.phase;
  strcpy(send_data.footer, "#");

  Serial.print("Destination Device-" + String(device_ID) + " MAC: ");
  printMAC(mac_addr);
  memset(slave.peer_addr, 0, 6);
  memcpy(slave.peer_addr, mac_addr, 6);
  bool exists = esp_now_is_peer_exist(slave.peer_addr);
  if (exists)
  {
    // Slave already paired.
    Serial.println("Already Paired");
    // return true;
  }
  else
  {
    if (esp_now_add_peer(&slave) != ESP_OK)
    {
      Serial.println("Failed to add peer");
      return;
    }
  }
  esp_now_send(slave.peer_addr, (uint8_t *)&send_data, sizeof(send_data));
  delay(50);
}

/**
 * @brief      Sends a phase R change command to a specified device.
 *
 * @param[in]  device_Phase  The phase of the device for which the command is sent.
 *
 * @details    This function updates the phase information for the specified device and sends a
 *             phase change packet to the device using ESP-NOW. It checks if the device is
 *             registered, decrements the current phase R count, and increments the count for the
 *             new phase. The function handles up to 10 devices.
 */
void send_phaseR_change(uint8_t device_Phase)
{
  switch (device_Phase)
  {
  case DEVICE1:
    if (device1_phase != phase_type)
    {
      if ((device_id[0]) == 1)
      {
        --phase_R_cnt;
        Serial.println("Dev1 Change Ph R dec");
        device1_phase = phase_type;
        #if PHASE_CHANGE_ENABLE
        send_phaseR_change_packet(DEVICE1, device1_phase, slave1_mac);
        #endif
        switch (device1_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          Serial.println("Dev1 Change Ph R inc");
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE1;
          }
          else
          {
            device_phaseR1 = DEVICE1;
          }
          break;

        case phase_Y:
          ++phase_Y_cnt;
          Serial.println("Dev1 Change Ph Y inc");
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE1;
            if(device_phaseR1 == DEVICE1)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseY1 = DEVICE1;
          }
          break;

        case phase_B:
          ++phase_B_cnt;
          Serial.println("Dev1 Change Ph B inc");
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE1;
            if(device_phaseR1 == DEVICE1)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseB1 = DEVICE1;
          }
          break;

        default:
          break;
        }
      }
    }

    break;
  case DEVICE2:
    if (device2_phase != phase_type)
    {
      if ((device_id[1]) == 1)
      {
        --phase_R_cnt;
        Serial.println("Dev2 Change Ph R dec");
        device2_phase = phase_type;
        #if PHASE_CHANGE_ENABLE
        send_phaseR_change_packet(DEVICE2, device2_phase, slave2_mac);
        #endif
        switch (device2_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          Serial.println("Dev2 Change Ph R inc");
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE2;
          }
          else
          {
            device_phaseR1 = DEVICE2;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          Serial.println("Dev2 Change Ph Y inc");
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE2;
            if(device_phaseR1 == DEVICE2)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseY1 = DEVICE2;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          Serial.println("Dev2 Change Ph B inc");
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE2;
            if(device_phaseR1 == DEVICE2)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseB1 = DEVICE2;
          }
          break;
        }
      }
    }

    break;
  case DEVICE3:
    if (device3_phase != phase_type)
    {
      if ((device_id[2]) == 1)
      {
        --phase_R_cnt;
        Serial.println("Dev3 Change Ph R dec");
        device3_phase = phase_type;
        #if PHASE_CHANGE_ENABLE
        send_phaseR_change_packet(DEVICE3, device3_phase, slave3_mac);
        #endif
        switch (device3_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          Serial.println("Dev3 Change Ph R inc");
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE3;
          }
          else
          {
            device_phaseR1 = DEVICE3;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          Serial.println("Dev3 Change Ph Y inc");
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE3;
            if(device_phaseR1 == DEVICE3)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseY1 = DEVICE3;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          Serial.println("Dev3 Change Ph B inc");
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE3;
            if(device_phaseR1 == DEVICE3)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseB1 = DEVICE3;
          }
          break;
        }
      }
    }

    break;
  case DEVICE4:
    if (device4_phase != phase_type)
    {
      if ((device_id[3]) == 1)
      {
        --phase_R_cnt;
        Serial.println("Dev4 Change Ph R dec");
        device4_phase = phase_type;
        #if PHASE_CHANGE_ENABLE
        send_phaseR_change_packet(DEVICE4, device4_phase, slave4_mac);
        #endif
        switch (device4_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          Serial.println("Dev4 Change Ph R inc");
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE4;
          }
          else
          {
            device_phaseR1 = DEVICE4;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          Serial.println("Dev4 Change Ph Y inc");
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE4;
            if(device_phaseR1 == DEVICE4)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseY1 = DEVICE4;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          Serial.println("Dev4 Change Ph B inc");
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE4;
            if(device_phaseR1 == DEVICE4)
            {
              device_phaseR1 = 0;
            }
          }
          else
          {
            device_phaseB1 = DEVICE4;
          }
          break;
        }
      }
    }

    break;
  case DEVICE5:
    if (device5_phase != phase_type)
    {

      if ((device_id[4]) == 1)
      {
        --phase_R_cnt;
        Serial.println("Dev5 Change Ph R dec");
        device5_phase = phase_type;
        #if PHASE_CHANGE_ENABLE
        send_phaseR_change_packet(DEVICE5, device5_phase, slave5_mac);
        #endif
        switch (device5_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          Serial.println("Dev5 Change Ph R inc");
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE5;
          }
          else
          {
            device_phaseR1 = DEVICE5;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          Serial.println("Dev5 Change Ph Y inc");
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE5;
          }
          else
          {
            device_phaseY1 = DEVICE5;
          }

          break;
        case phase_B:
          ++phase_B_cnt;
          Serial.println("Dev5 Change Ph B inc");
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE5;
          }
          else
          {
            device_phaseB1 = DEVICE5;
          }
          break;
        }
      }
    }

    break;
  case DEVICE6:
    if (device6_phase != phase_type)
    {
      if ((device_id[5]) == 1)
      {
        device6_phase = phase_type;
        send_phaseR_change_packet(DEVICE6, device6_phase, slave6_mac);
        --phase_R_cnt;
        switch (device6_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE6;
          }
          else
          {
            device_phaseR1 = DEVICE6;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE6;
          }
          else
          {
            device_phaseY1 = DEVICE6;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE6;
          }
          else
          {
            device_phaseB1 = DEVICE6;
          }
          break;
        }
      }
    }

    break;
  case DEVICE7:
    if (device7_phase != phase_type)
    {
      if ((device_id[6]) == 1)
      {
        device7_phase = phase_type;
        send_phaseR_change_packet(DEVICE7, device7_phase, slave7_mac);
        --phase_R_cnt;
        switch (device7_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE7;
          }
          else
          {
            device_phaseR1 = DEVICE7;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE7;
          }
          else
          {
            device_phaseY1 = DEVICE7;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE7;
          }
          else
          {
            device_phaseB1 = DEVICE7;
          }
          break;
        }
      }
    }

    break;
  case DEVICE8:
    if (device8_phase != phase_type)
    {
      if ((device_id[7]) == 1)
      {
        device8_phase = phase_type;
        send_phaseR_change_packet(DEVICE8, device8_phase, slave8_mac);
        --phase_R_cnt;
        switch (device8_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE8;
          }
          else
          {
            device_phaseR1 = DEVICE8;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE8;
          }
          else
          {
            device_phaseY1 = DEVICE8;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE8;
          }
          else
          {
            device_phaseB1 = DEVICE8;
          }
          break;
        }
      }
    }

    break;
  case DEVICE9:
    if (device9_phase != phase_type)
    {
      if ((device_id[8]) == 1)
      {
        device9_phase = phase_type;
        send_phaseR_change_packet(DEVICE9, device9_phase, slave9_mac);
        --phase_R_cnt;
        switch (device9_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE9;
          }
          else
          {
            device_phaseR1 = DEVICE9;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE9;
          }
          else
          {
            device_phaseY1 = DEVICE9;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE9;
          }
          else
          {
            device_phaseB1 = DEVICE9;
          }
          break;
        }
      }
    }

    break;
  case DEVICE10:
    if (device10_phase != phase_type)
    {
      if ((device_id[9]) == 1)
      {
        device10_phase = phase_type;
        send_phaseR_change_packet(DEVICE10, device10_phase, slave10_mac);
        --phase_R_cnt;
        switch (device10_phase)
        {
        case phase_R:
          ++phase_R_cnt;
          if (phase_R_cnt == 1)
          {
            device_phaseR0 = DEVICE10;
          }
          else
          {
            device_phaseR1 = DEVICE10;
          }
          break;
        case phase_Y:
          ++phase_Y_cnt;
          if (phase_Y_cnt == 1)
          {
            device_phaseY0 = DEVICE10;
          }
          else
          {
            device_phaseY1 = DEVICE10;
          }
          break;
        case phase_B:
          ++phase_B_cnt;
          if (phase_B_cnt == 1)
          {
            device_phaseB0 = DEVICE10;
          }
          else
          {
            device_phaseB1 = DEVICE10;
          }
          break;
        }
      }
    }

    break;
  }
}

void send_load_change_packet(uint8_t device_id, float load, const uint8_t *mac_addr)
{
  memset(&send_data, '\0', sizeof(send_data));
  Serial.println("send to device - " + String(device_id) + ", load - " + String(load));
  strcpy(send_data.header1, "P");
  send_data.deviceID = device_id;
  send_data.load = load;
  send_data.action = LOAD_CHANGE;
  send_data.checksum = send_data.deviceID + send_data.load;
  strcpy(send_data.footer, "#");
  Serial.println("Destination Device-" + String(device_id) + " MAC: ");
  printMAC(mac_addr);
  memset(slave.peer_addr, 0, 6);
  memcpy(slave.peer_addr, mac_addr, 6);
  esp_now_send(slave.peer_addr, (uint8_t *)&send_data, sizeof(send_data));
  delay(50);
  Serial.println("");
}

void load_change(uint8_t device_ID, float load)
{
  switch (device_ID)
  {
  case DEVICE1:
    if (device_id[0] == 1)
    {
      send_load_change_packet(DEVICE1, load, slave1_mac);
    }
    break;
  case DEVICE2:
    if (device_id[1] == 1)
    {
      send_load_change_packet(DEVICE2, load, slave2_mac);
    }
    break;
  case DEVICE3:
    if (device_id[2] == 1)
    {
      send_load_change_packet(DEVICE3, load, slave3_mac);
    }
    break;
  case DEVICE4:
    if (device_id[3] == 1)
    {
      send_load_change_packet(DEVICE4, load, slave4_mac);
    }
    break;
  case DEVICE5:
    if (device_id[4] == 1)
    {
      send_load_change_packet(DEVICE5, load, slave5_mac);
    }
    break;
  case DEVICE6:
    if (device_id[5] == 1)
    {
      send_load_change_packet(DEVICE6, load, slave6_mac);
    }
    break;
  case DEVICE7:
    if (device_id[6] == 1)
    {
      send_load_change_packet(DEVICE7, load, slave7_mac);
    }
    break;
  case DEVICE8:
    if (device_id[7] == 1)
    {
      send_load_change_packet(DEVICE8, load, slave8_mac);
    }
    break;
  case DEVICE9:
    if (device_id[8] == 1)
    {
      send_load_change_packet(DEVICE9, load, slave9_mac);
    }
    break;
  case DEVICE10:
    if (device_id[9] == 1)
    {
      send_load_change_packet(DEVICE10, load, slave10_mac);
    }
    break;

  default: Serial.println("Device is disconnected");
  break;
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.print("\r\nPacket received from: ");
  printMAC(mac);
  // Serial.print("data size = ");
  // Serial.println(sizeof(incomingData));
  memset(&recieved_data, '\0', sizeof(recieved_data));
  memcpy(&recieved_data, incomingData, sizeof(recieved_data));

  switch (recieved_data.status)
  {
  case not_charging:
    Serial.println("Stop Charging device- " + String(recieved_data.deviceID));
    Serial.println("Phase : " + String(recieved_data.phase));
    // stop_device_phase = recieved_data.phase;
    delete_device(recieved_data.deviceID, recieved_data.phase);
    break;
  case charging:
    Serial.print("Charging to device- ");
    Serial.println(recieved_data.deviceID);
    // counting devices
    device_count(recieved_data.deviceID, mac);
    break;
  case power:
    Serial.println("Power Mesurement from device- " + String(recieved_data.deviceID));
    Serial.println("Power : " + String(recieved_data.power));
    Serial.println("Phase : " + String(recieved_data.phase));
    
    #if DASHBOARD_ENABLE
    jsonDoc["dev"] = String(recieved_data.deviceID);
    switch(recieved_data.phase)
    {
    case phase_R:
      jsonDoc["phase"] = "R";
      break;
    case phase_Y:
      jsonDoc["phase"] = "Y";
      break;
    case phase_B:
      jsonDoc["phase"] = "B";
      break;
    }
    jsonDoc["power"] = String((float)recieved_data.power);
    serializeJson(jsonDoc, jsonString);

    // Send JSON data over WebSocket
    webSocket.sendTXT(jsonString);
    Serial.println("Sent JSON: " + jsonString);
    // Clear JSON document
    jsonDoc.clear();
    jsonString = ""; // Optionally clear the string as well
    #endif

    break;
  }
  // Serial.print("Header : ");
  // Serial.println(recieved_data.header1);

  // Serial.print("Device ID : ");
  // Serial.println(recieved_data.deviceID);

  // Serial.print("Phase : ");
  // Serial.println(recieved_data.phase);

  // Serial.print("Power : ");
  // Serial.println(recieved_data.power);

  // Serial.print("Status : ");
  // Serial.println(recieved_data.status);

  // Serial.print("Checksum : ");
  // Serial.println(recieved_data.checksum);

  // Serial.print("Footer : ");
  // Serial.println(recieved_data.footer);

  // Serial.println();
}

/**
 * @brief      Initializes the ESP-NOW protocol and registers callback functions.
 *
 * @details    This function initializes the ESP-NOW protocol. If initialization fails,
 *             it prints an error message to the Serial monitor and exits. Upon successful
 *             initialization, it registers a callback function to be called when data is
 *             sent and another callback for when data is received. The function also prints
 *             the current working Wi-Fi channel used by ESP-NOW.
 */
void initESPNow(void)
{
  // delete old config
  // WiFi.disconnect(true);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  Serial.print("LBS Server Running channel: ");
  Serial.println(WiFi.channel());
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
