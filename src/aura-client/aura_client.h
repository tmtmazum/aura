#pragma once

#include "aura-core/build.h"
#include "aura_client_native.h"
#include <utility>
#include <system_error>

#include <windows.h>

namespace aura
{

struct aura_client;

inline auto to_aura_string8(std::string const& s) noexcept
{
  return aura_string8{(char*)s.data(), static_cast<int>(s.size())};
}

struct aura_client
{
  friend std::pair<std::error_code, aura_client> make_aura_client();

  aura_client() = default;
public:
  constexpr static size_t max_payload_length = 1024;

  std::pair<std::error_code, std::string> request(std::string const& method, std::string const& parameters)
  {
    AURA_ASSERT(m_client_id);
    std::string payload_s(max_payload_length, '\0');

    auto payload = to_aura_string8(payload_s);
    auto meth = to_aura_string8(method);
    auto param = to_aura_string8(parameters);

    http_result res{};
    if (m_call_fn(m_client_id, &meth, &param, &res, &payload))
    {
      return { {}, payload_s };
    }
    return {{}, {}};
  }

  void swap(aura_client& other)
  {
    std::swap(m_module, other.m_module);
    std::swap(m_client_id, other.m_client_id);
    std::swap(m_destroy_fn, other.m_destroy_fn);
    std::swap(m_call_fn, other.m_call_fn);
  }

  aura_client(aura_client&& other)
  {
    swap(other);
  }

  aura_client& operator=(aura_client&& other)
  {
    swap(other);
    return *this;
  }

  aura_client(aura_client const& other) = delete;

  aura_client& operator=(aura_client const& other) = delete;

  ~aura_client()
  {
    if (m_client_id && m_destroy_fn)
    {
      m_destroy_fn(&m_client_id);
    }

    if (m_module)
    {
      ::FreeLibrary(m_module);
    }
  }

private:
  HMODULE m_module{};
  int m_client_id{};
  decltype(aura_destroy_client)*  m_destroy_fn{};
  decltype(aura_call)*  m_call_fn{};
};

inline std::pair<std::error_code, aura_client> make_aura_client()
{
  auto const mod = ::LoadLibraryW(L"aura_client.dll");
  if (!mod)
  {
    auto const error = make_error_code(std::errc::not_supported);
    AURA_ERROR(error, L"Couldn't find aura_client.dll");
    return {error, aura_client{}};
  }
  AURA_LOG(L"Loaded 'aura_client.lib' successfully");
  auto const rest_fn = (decltype(::aura_make_client)*)::GetProcAddress(mod, "aura_make_client");
  if (!rest_fn)
  {
    auto const error = make_error_code(std::errc::function_not_supported);
    AURA_ERROR(error, L"Couldn't find 'rest' function in aura_client");
    return {error, aura_client{}};
  }

  aura_client client{};
  client.m_module = mod;

  if (!::aura_make_client(&client.m_client_id))
  {
    auto const error = make_error_code(std::errc::function_not_supported);
    AURA_ERROR(error, L"Couldn't find 'rest' function in aura_client");
    return {error, aura_client{}};
  }

  client.m_destroy_fn = (decltype(::aura_destroy_client)*)::GetProcAddress(mod, "aura_destroy_client");
  client.m_call_fn = (decltype(::aura_call)*)::GetProcAddress(mod, "aura_call");
  return {std::error_code{}, std::move(client)};
}

}
