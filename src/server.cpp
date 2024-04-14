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
#include <vector>
#include <unordered_map>
#include "request.hpp"

const unsigned int MAX_BUFF_LENGTH = 4096;

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
  str.erase();
  return result;
}

void request_handler(int client_fd) {
  while(true) {
    char* tmp_buffer = (char*)malloc(sizeof(char) * MAX_BUFF_LENGTH);
    Request request;
    size_t recv_len;
    do {
      int recv_len = recv(client_fd, tmp_buffer, MAX_BUFF_LENGTH, 0);
      if(recv_len == -1) {
        std::cerr << "error while receiving data\n";
        break;
      }
      std::string request_str{tmp_buffer};
      std::string response;

      std::string delimiter = "\r\n";
      std::string space = " ";
      size_t pos = 0;
      
      std::string token;
      std::vector<std::string> splitted = split_str(request_str, delimiter);
      std::vector<std::string> reqline = split_str(splitted[0], space);
      
      request.set_header(reqline[0], reqline[1], reqline[2]);

      if(request.get_method() == "GET") {
        if(request.get_path() == "/") {
          response = "HTTP/1.1 200 OK\r\n\r\n";
        }
        else {
          response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
      }

      for(auto a: reqline) {
        std::cout << a << " ";
      }
      for(auto line: splitted) {
        std::cout << line << std::endl;
      }
      send(client_fd, response.c_str(), response.size(), 0);
      break;
    } while (recv_len);

    break;
  }
}

int main(int argc, char **argv) {
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

    request_handler(client_fd);
    break;
  }
  
  close(server_fd);

  return 0;
}
