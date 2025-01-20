#include "ESP_Mesh_Utility.h"

/* 
  initialize static members
*/
painlessMesh *ESP_Mesh_util::meshPointer = nullptr;
JsonDocument ESP_Mesh_util::jsonDocRx{};
JsonDocument ESP_Mesh_util::jsonDocTx{};
bool ESP_Mesh_util::globalBeacon = false;
uint32_t ESP_Mesh_util::beaconTimer = 0;