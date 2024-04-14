#pragma once
#include <string>

class Request {
private:
  std::string m_method, m_path, m_http_version;

public:
  Request() = default;
  Request(std::string& method, std::string& path, std::string http_version);
  ~Request() = default;
  void set_header(std::string& method, std::string& path, std::string http_version);
  std::string get_method();
  std::string get_path();
};
