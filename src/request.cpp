#include "request.hpp"

Request::Request(std::string& method, std::string& path, std::string http_version) {
  set_request_line(method, path, http_version);
}

void Request::set_request_line(std::vector<std::string>& reqline) {
  m_reqline = reqline;
}

void Request::set_request_line(std::string& method, std::string& path, std::string& http_version) {
  m_method = method;
  m_path = path;
  m_http_version = http_version;
  m_reqline = std::vector<std::string>{method, path, http_version};
}

void Request::append_header(const char* key, const char* val) {
  m_header[key] = val;
}

void Request::set_body(const std::string& body) {
  m_body = body;
}

std::string Request::get_method() {
  return m_method;
}

std::string Request::get_path() {
  return m_path;
}

std::string Request::raw_request() {
  std::string raw{""};
  for(size_t i=0; i<m_reqline.size(); ++i) {
    if(i) raw += " ";
    raw += m_reqline[i];
  }
  raw += "\r\n";
  for (const auto & [ key, value ] : m_header) {
    raw += key + ": " + value + "\r\n";
  }
  raw += "\r\n" + m_body + "\r\n\r\n";
  return raw;
}
