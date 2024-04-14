#include "request.hpp"

Request::Request(std::string& method, std::string& path, std::string http_version) {
  set_header(method, path, http_version);
}

void Request::set_header(std::string& method, std::string& path, std::string http_version) {
  m_method = method;
  m_path = path;
  m_http_version = http_version;
}

std::string Request::get_method() {
  return m_method;
}

std::string Request::get_path() {
  return m_path;
}
