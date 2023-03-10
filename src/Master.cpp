#include <M5Atom.h>

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "espNowFloodingMeshLibrary2/EspNowFloodingMesh.h"

struct MeshProbe_struct
{
  char name[15]; // name of the mesh slave
  uint64_t TimeStamp;
  float MPU_Temperature;
};
MeshProbe_struct MeshProbe;

char MasterName[15];

unsigned char secredKey[] = {0xB8, 0xF0, 0xF4, 0xB7, 0x4B, 0x1E, 0xD7, 0x1E, 0x8E, 0x4B, 0x7C, 0x8A, 0x09, 0xE0, 0x5A, 0xF1}; // AES 128bit
unsigned char iv[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

int ESP_NOW_CHANNEL = 11;
int bsid = 0x010101;
const int ttl = 3;

void espNowFloodingMeshRecv(const uint8_t *data, int len, uint32_t replyPrt)
{
  if (len > 0)
  {
    MeshProbe_struct *MeshProbe = (MeshProbe_struct *)data;
    Serial.printf("   >=== received Data from: %s %llu %6.2f\n", MeshProbe->name, MeshProbe->TimeStamp, MeshProbe->MPU_Temperature);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Welcome to ESP-NOW Master"));

  int8_t power;
  // esp_wifi_set_max_tx_power(20);
  esp_wifi_get_max_tx_power(&power);
  Serial.printf("wifi power: %d \n", power);

  uint64_t chipid;
  chipid = ESP.getEfuseMac();
  sprintf(MasterName, "Master%04X", (uint16_t)(chipid >> 32));

  espNowFloodingMesh_RecvCB(espNowFloodingMeshRecv);
  espNowFloodingMesh_secredkey(secredKey);
  espNowFloodingMesh_disableTimeDifferenceCheck();
  espNowFloodingMesh_setAesInitializationVector(iv);

  espNowFloodingMesh_setToMasterRole(true); // Set ttl to 3.

  espNowFloodingMesh_ErrorDebugCB([](int level, const char *str)
                                  {
                            if (level == 0) {
                               Serial.printf("ERROR %s", str);
                            }
                            if (level == 1) {
                               Serial.printf("WRN   %s", str);
                            }
                            if (level == 2) {
                               Serial.printf("INFO  %s", str);
                            } });

  espNowFloodingMesh_begin(ESP_NOW_CHANNEL, bsid, true);

  Serial.println(F("WiFi Settings before setting new channel"));
  WiFi.printDiag(Serial); // shows default channel
  ESP_ERROR_CHECK(esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
  Serial.println(F("WiFi Settings after setting new channel"));
  WiFi.printDiag(Serial); // shows chosen EspNow channel
}

void loop()
{
  espNowFloodingMesh_loop();

  static uint64_t iCount = 0;

  static unsigned long MeshLoopPM = 0;
  unsigned long MeshLoopCM = millis();
  if (MeshLoopCM - MeshLoopPM >= 2000) // sending mesh values every 2 seconds
  {
    strlcpy(MeshProbe.name, MasterName, sizeof(MeshProbe.name));
    MeshProbe.MPU_Temperature = (millis() * 0.001);
    MeshProbe.TimeStamp = iCount++;

    espNowFloodingMesh_send((uint8_t *)&MeshProbe, sizeof(MeshProbe), ttl); // set ttl to 3

    Serial.printf("Send to Slave (%llu) from %s: Temp: %6.2f\n",
                  MeshProbe.TimeStamp,
                  MeshProbe.name,
                  MeshProbe.MPU_Temperature);

    // -------- MeshLoop end --------
    MeshLoopPM = MeshLoopCM;
  }
}