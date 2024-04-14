#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <shared_mutex>
#include <unordered_map>
#include "request.hpp"

const unsigned int MAX_BUFF_LENGTH = 4096;

std::shared_ptr<std::unordered_map<std::string, std::string>> ctx;
std::shared_mutex ctx_mutex;

std::vector<std::string> split_str(std::string& str, std::string& delimiter) {
  size_t pos = 0, curr_line = 0;
  std::string token;

  std::vector<std::string> result;
  while ((pos = str.find(delimiter)) != std::string::npos) {
      token = str.substr(0, pos);
      result.push_back(token);
      str.erase(0, pos + delimiter.length());
  }
  result.push_back(str);

  return result;
}

std::unordered_map<std::string, std::string> parse_header(std::vector<std::string>& unparsed_request) {
  std::unordered_map<std::string, std::string> kv;

  while(unparsed_request[0] != "") {
    std::string kv_delim = ": ";
    std::vector<std::string> key_val = split_str(unparsed_request[0], kv_delim);
    unparsed_request.erase(unparsed_request.cbegin());
    kv[key_val[0]] = key_val[1];
  }

  unparsed_request.erase(unparsed_request.cbegin());
  return kv;
}

std::string get_404_response() {
  Request response_builder;
  std::vector<std::string> tmp_reqline{"HTTP/1.1", "404", "Not Found"};
  response_builder.set_request_line(tmp_reqline);

  return response_builder.raw_request();
}

std::string get_200_response(const std::string& content_type="", size_t content_length=0, const std::string& body="") {
  Request response_builder;
  std::vector<std::string> tmp_reqline{"HTTP/1.1", "200", "OK"};
  response_builder.set_request_line(tmp_reqline);

  if(content_type != "") {
    response_builder.append_header("Content-Type", content_type.c_str());
  }
  if(content_length != 0) {
    response_builder.append_header("Content-Length", std::to_string(content_length).c_str());
  }
  if(body != "") {
    response_builder.set_body(body);
  }

  return response_builder.raw_request();
}

std::string get_201_response() {
  Request response_builder;
  std::vector<std::string> tmp_reqline{"HTTP/1.1", "201", "Created"};
  response_builder.set_request_line(tmp_reqline);

  return response_builder.raw_request();
}


void request_handler(int client_fd) {
  char* tmp_buffer = (char*)malloc(sizeof(char) * MAX_BUFF_LENGTH);
  Request request;

  if(recv(client_fd, tmp_buffer, MAX_BUFF_LENGTH, 0) > 0) {
    std::string request_str{tmp_buffer};
    std::string response;

    std::string delimiter = "\r\n";
    std::string space = " ";

    std::vector<std::string> splitted_request = split_str(request_str, delimiter);
    std::vector<std::string> reqline = split_str(splitted_request[0], space);
    splitted_request.erase(splitted_request.cbegin());

    request.set_request_line(reqline[0], reqline[1], reqline[2]);
    if(request.get_method() == "GET") {
      if(request.get_path() == "/") {
        response = get_200_response();
      }
      else if(strncmp(request.get_path().c_str(), "/echo/", 6) == 0) {
        size_t body_idx = request.get_path().find("/echo/") + 6;
        std::string body = request.get_path().substr(body_idx);
        response = get_200_response("text/plain", body.length(), body);
      }
      else if(strncmp(request.get_path().c_str(), "/user-agent", 11) == 0) {
        std::unordered_map<std::string, std::string> kv = parse_header(splitted_request);
        std::string user_agent = kv["User-Agent"];

        response = get_200_response("text/plain", user_agent.length(), user_agent);
      }
      else if(strncmp(request.get_path().c_str(), "/files/", 7) == 0) {
        size_t filename_idx = request.get_path().find("/files/") + 7;
        std::string filename = request.get_path().substr(filename_idx);
        std::shared_lock<std::shared_mutex> read_lock(ctx_mutex);
        std::string dirname = (*ctx)["directory"];
        if(dirname == "") {
          response = get_404_response();
        }
        std::ifstream infile((*ctx)["directory"] + "/" + filename);
        if(dirname != "" and infile.fail()) {
          response = get_404_response();
        }
        else {
          int content_length = 0;
          infile.seekg (0, infile.end);
          size_t length = infile.tellg();
          infile.seekg (0, infile.beg);

          std::string str_body((std::istreambuf_iterator<char>(infile)),
            (std::istreambuf_iterator<char>()));
          response = get_200_response("application/octet-stream", str_body.length(), str_body);
        }
        infile.close();
      }
      else {
        response = get_404_response();
      }
    }
    else if(request.get_method() == "POST") {
      if(strncmp(request.get_path().c_str(), "/files/", 7) == 0) {
        size_t filename_idx = request.get_path().find("/files/") + 7;
        std::string filename = request.get_path().substr(filename_idx);

        std::shared_lock<std::shared_mutex> read_lock(ctx_mutex);
        std::string dirname = (*ctx)["directory"];
        std::ofstream outfile((*ctx)["directory"] + "/" + filename);
        if(outfile.fail()) {
          response = get_404_response();
        }
        else {
          std::unordered_map<std::string, std::string> kv = parse_header(splitted_request);
          std::string body = (splitted_request[0]);
          splitted_request.erase(splitted_request.cbegin());
          outfile << body;
          outfile.close();
          response = get_201_response();
        }
      }
    }
    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd); // only support one time request per connection
  }
}

int main(int argc, char **argv) {
  char* dirname = (char*)malloc(sizeof(dirname)*1024);
  ctx = std::make_shared<std::unordered_map<std::string, std::string>>();
  if(argc > 2) {
    for(int i=0; i<argc; ++i) {
      if(strcmp(argv[i], "--directory") == 0) {
        sscanf(argv[i+1], "%s", dirname);
        (*ctx)["directory"] = dirname;
      }
    }
  }

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }

  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  while(1) {
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if (client_fd == -1) return 1;
    std::cout << "Client connected\n";
    std::thread(request_handler, client_fd).detach();
  }

  close(server_fd);

  return 0;
}
