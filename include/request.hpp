#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class Request {
private:
  std::string m_method, m_path, m_http_version;
  std::vector<std::string> m_reqline;
  std::unordered_map<std::string, std::string> m_header;
  std::string m_body;

public:
  Request() = default;
  Request(std::string& method, std::string& path, std::string http_version);
  ~Request() = default;
  void set_request_line(std::vector<std::string>& reqline);
  void set_request_line(std::string& method, std::string& path, std::string& http_version);
  void append_header(const char* key, const char* val);
  void set_body(const std::string& body);
  std::string get_method();
  std::string get_path();
  std::string raw_request();
};
