#pragma once

extern "C"
{

struct aura_string8
{
  char *data;
  int size;
};

struct http_result
{
  int status;
};

//using rest_t = __declspec(dllexport) void (* __stdcall)(
//    aura_string8*parameters, http_result *result_out,
//    aura_string8 *payload_out);

__declspec(dllexport) bool __stdcall aura_make_client(int *client_id);
__declspec(dllexport) bool __stdcall aura_destroy_client(int *client_id);

__declspec(dllexport) bool __stdcall aura_call(
  int client_id,
  aura_string8* method,
  aura_string8* parameters,
  http_result* result_out,
  aura_string8* payload_out
  );

//__declspec(dllexport) void __stdcall rest(aura_string8 *parameters,
//                                          http_result *result_out,
//                                          aura_string8 *payload_out);

}
