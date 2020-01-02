#include <aura-core/build.h>
#include "aura_client_native.h"
#include <cpp-httplib/httplib.h>
#include <unordered_map>

std::unordered_map<int, httplib::Client> clients;

__declspec(dllexport) bool __stdcall aura_make_client(int *client_id)
{
  static int i = 1;
  *client_id = i++;

  clients.emplace(*client_id, httplib::Client{"localhost", 1234});
  AURA_LOG(L"Made client with id %d, Total clients = %zu", *client_id, clients.size());
  return true;
}

__declspec(dllexport) bool __stdcall aura_destroy_client(int *client_id)
{
  AURA_LOG(L"Destroying client with id %d, Total clients = %zu", *client_id, clients.size());
  clients.clear();
  return true;
}

__declspec(dllexport) bool __stdcall aura_call(
  int client_id,
  aura_string8* method,
  aura_string8* parameters,
  http_result* result_out,
  aura_string8* payload_out
  )
{
  auto const client_it = clients.find(client_id);
  if (client_it == clients.end())
  {
    AURA_LOG(L"No client with id %d found. Total items = %zu", client_id, clients.size());
    return false;
  }

  AURA_LOG(L"sending request method:'%hs' parameters:'%hs'", std::string{method->data}.c_str(),
    std::string{parameters->data}.c_str());
  auto const response = client_it->second.Get(method->data);
  if (!response)
  {
    AURA_LOG("request failed..");
    return false;
  }

  if (result_out)
  {
    AURA_LOG("found status '%d'", response->status);
    result_out->status = response->status;
  }
  if (payload_out)
  {
    AURA_LOG("got back payload '%hs'", response->body.c_str());
    strcpy_s(payload_out->data, payload_out->size, response->body.c_str());
  }
  return true;
}
