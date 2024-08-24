#include <_types/_uint32_t.h>
enum class GameMsg : uint32_t {
  Server_GetStatus,
  Server_GetPing,

  Client_Accepted,
  Client_AssignID,
  Client_RegisterWithServer,
  Client_UnregisterWithServer,

  Game_AddPlayer,
  Game_RemovePlayer,
  Game_UpdatePlayer,
};
