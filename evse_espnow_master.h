/// @file evse_espnow_master.h
/// @Author: Gopikrishna S
/// @date 07/11/2024
/// @brief ESP-NOW Master Code
/// @All Rights Reserved EVRE

#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define LOAD_ASSIGN_ENABLE       (1)
#define PHASE_ASSIGN_ENABLE      (1)
#define PHASE_CHANGE_ENABLE      (1)
#define DASHBOARD_ENABLE         (1)

// Channel to be used by the ESP-NOW protocol
#define ESPNOW_WIFI_CHANNEL 2

enum PhaseType {phase_R, phase_Y, phase_B};
enum ActionType {PHASE_ASSIGN, PHASE_CHANGE, LOAD_CHANGE};

// Must match the receiver structure
// Structure ALPRDATA to send alpr data
#pragma pack(1)
struct SENDDATA {
  char header1[2] = "P";
  uint8_t deviceID;
  uint8_t phase;
  uint8_t action;
  float load;
  uint16_t checksum;
  char footer[2] = "#";
};
// extern struct SENDDATA send_data;
void initESPNow(void);
// void send_Response(uint8_t device_ID,const uint8_t *mac_addr);
void wait_success_send(uint8_t device_ID, uint8_t phase_type, float assign_load, const uint8_t *mac_addr);
void send_phaseR_change(uint8_t device_Phase);
void send_load_change_packet(uint8_t device_id, float load, const uint8_t *mac_addr);
void load_change(uint8_t device_id, float load);

extern volatile uint8_t phase_type;
extern volatile uint8_t phase_R_cnt;
extern volatile uint8_t phase_Y_cnt;
extern volatile uint8_t phase_B_cnt;

extern volatile uint8_t device_phaseR0;
extern volatile uint8_t device_phaseY0;
extern volatile uint8_t device_phaseB0;

extern volatile uint8_t device_phaseR1;
extern volatile uint8_t device_phaseY1;
extern volatile uint8_t device_phaseB1;

