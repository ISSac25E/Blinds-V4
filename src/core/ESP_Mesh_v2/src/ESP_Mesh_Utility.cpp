#include "ESP_Mesh_Utility.h"

/* 
  initialize static members
*/
painlessMesh *ESP_Mesh_util::meshPointer = nullptr;
JsonDocument ESP_Mesh_util::jsonDocRx{};
JsonDocument ESP_Mesh_util::jsonDocTx{};