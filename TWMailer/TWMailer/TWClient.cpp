#include <getopt.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>

// Fork includes
#include <sys/types.h>
#include <sys/wait.h>

// Folder navigation Linux
#include <dirent.h>
#include <errno.h>

// Socket includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Message Class
#include "Message.h"


// User functions
void userInterface();
void SEND();
void SEND(Message message);
void READ();
void LIST();
void DEL();
void QUIT();
void TEST();
// Server functions
void sendToServer(std::string serverCommand);
// Debug funcctions
void testMessageCreation();

// Buffer & Globals
#define BUF 1024
int SOCKET;

// Input format: twmailer-client <ip> <port>
// Test  command ./twmailer-client 127.0.0.1 6543


//############################################################################################
int main(int argc, char *argv[]) {

  // Get port from input argument
  std::string serverPort = argv[optind + 1];
  int port = stoi(serverPort);  

  // Socket variables
  char buffer[BUF];
  struct sockaddr_in address;
  int size;
  std::string command;

  //Debug test
  //testMessageCreation();

  // CREATE A SOCKET
  if ((SOCKET = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("Socket error");
    return EXIT_FAILURE;
  }
  
  // INIT ADDRESS 
  memset(&address, 0, sizeof(address)); // init storage with 0
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (argc < 2){
    inet_aton("127.0.0.1", &address.sin_addr);  // Default IP
  }
  else{
    inet_aton(argv[1], &address.sin_addr);
  }
  
  // CREATE A CONNECTION
  if (connect(SOCKET, (struct sockaddr *)&address, sizeof(address)) == -1){
    perror("Connect error - no server available");
    return EXIT_FAILURE;
  }
  
  // Print Server connection success
  printf("Connection with server (%s) established\n", inet_ntoa(address.sin_addr));
  
  // RECEIVE DATA
  size = recv(SOCKET, buffer, BUF - 1, 0);
  if (size == -1){
    perror("recv error");
  }
  else if (size == 0){
    std::cout << "Server closed remote socket" << std::endl;
  }
  else{
    buffer[size] = '\0';
    std::cout << buffer << std::endl;
  }
  
  // Client-Server communication - Send prepared command to server
  userInterface();

  
  // CLOSES THE DESCRIPTOR
  if (SOCKET != -1)
  {
    if (shutdown(SOCKET, SHUT_RDWR) == -1){
       // invalid in case the server is gone already
       perror("shutdown create_socket"); 
    }
    if (close(SOCKET) == -1){
       perror("close create_socket");
    }
    SOCKET = -1;
  }
  
  return EXIT_SUCCESS;
}
//###########################################################################################



// Take User input
void userInterface(){
  std::string command;
  std::cout << "Welcome to TW-Mailer!" << std::endl;

  if(LOGIN() = "OK\n"){

  }
  
  do{

    std::cout << "Valid commands are SEND, LIST, READ, DEL, QUIT. (and TEST)" << std::endl;
    std::cout << ">> ";
    std::cin >> command;

    // SEND
    if (command == "SEND") {
      SEND();

    }
    // LIST
    else if (command == "LIST") {
      LIST();
    }
    // READ
    else if (command == "READ") {
      READ();
    }
    // DEL
    else if (command == "DEL") {
      DEL();
    }
    // QUIT
    else if (command == "QUIT") {
      QUIT();
    }
    // TEST
    else if (command == "TEST") {
      TEST();
    }
    // Invalid input
    else {
      std::cout << "\nPlease choose a valid input!" << std::endl;
      std::cout << "Valid commands are SEND, LIST, READ, DEL, QUIT." << std::endl;
    }    

  } while (command != "QUIT");
}



// Create a new message and send it to the server
void SEND() {
  Message newMessage;
  std::string sendMessage = newMessage.formatForSending();
  sendMessage = "SEND\n" + sendMessage;
  // Send the message
  sendToServer(sendMessage);
}
// Send prebuilt message
void SEND(Message message) {
  std::string sendMessage = message.formatForSending();
  sendMessage = "SEND\n" + sendMessage;
  // Send the message
  sendToServer(sendMessage);
}



// Display a specific message of a specific user
void READ() { std::cout << "\n"; 
  std::string readMessage = "READ\n";
  std::string username;
  std::string messageID;

  // Get Input
  std::cout << "\nDislpays a specific message of a user." << std::endl;
  std::cout << "\nEnter username: " << std::endl;
  std::cin >> username;
  std::cout << "\nEnter Message id: " << std::endl;
  std::cin >> messageID;

  // Build the READ request
  readMessage = readMessage + username + "\n" + messageID + "\n";
  sendToServer(readMessage);
}



// List all messages of a specific user
void LIST() {
  std::string listMessage = "LIST\n";
  std::string username;

  // Get Input
  std::cout << "\ndislpay a list of messages of a specific user." << std::endl;
  std::cout << "\nEnter username: " << std::endl;
  std::cin >> username;
  
  // Build the LIST request
  listMessage = listMessage + username + "\n";
  sendToServer(listMessage);
}

// Delete a specific message of a specific user
void DEL() {
  std::string delMessage = "DEL\n";
  std::string username;
  std::string messageID;

  // Get Input
  std::cout << "\ndelete a specific message of a specific user" << std::endl;
  std::cout << "\nUsername: " << std::endl;
  std::cin >> username;
  std::cout << "\nMessage Number: " << std::endl;
  std::cin >> messageID;

  // Build the DEL request
  delMessage = delMessage + username + "\n" + messageID + "\n";
  sendToServer(delMessage);
}

/* void LOGIN(){
  std::string loginMessage = "LOGIN\n";
  std::string username;
  std::string password;

  std::cout << "\nEnter your username: " << std::endl;
  std::cin >> username;
  
  std::cout << "\nEnter your password: " << std::endl;
  std::cin >> password;

  // Build the LOGIN request
  delMessage = delMessage + username + "\n" + messageID + "\n";
  sendToServer(loginMessage);

}*/
  



// Close the server
void QUIT() {
  sendToServer("QUIT");
}



// Automated test of functions
void TEST(){
  // SEND
  // Send 2 messages
  std::cout << "\nSEND TEST: Sending 2 messages to server." << std::endl;
  // Message 1
  std::cout << "Message 1:" << std::endl;
  Message testMessage("1", "This is the subject line.", "This is\nthe Message text. Containing multiple Sentences and\nlinebreaks", "Sarah", "Bert"); 
  testMessage.printMessage();
  SEND(testMessage);
  
  // Message 2
  std::cout << "Message 2:" << std::endl;
  Message testMessage2("2", "This is the other subject line.", "This is\nanother Message text. Containing multiple Sentences and\nlinebreaks", "Klemens", "Bert");
  testMessage2.printMessage();
  SEND(testMessage2);

  // LIST
  std::string listMessage = "LIST\nBert\n";
  std::cout << "LIST TEST:" << std::endl;
  sendToServer(listMessage);
  
  // READ
  // Read 1 of the sent messages
  std::string readMessage = "READ\nBert\n2\n";
  std::cout << "READ TEST:" << std::endl;
  sendToServer(readMessage);

}



// Sends the input string to the server and listens to response
void sendToServer(std::string serverCommand){
  char buffer[BUF];
  
  // Save command to buffer
  strncpy(buffer, serverCommand.c_str(), sizeof(buffer));
  // Complete c String by appending "\0"
  buffer[sizeof(buffer) - 1] = '\0';
  
  int size = strlen(buffer);
   // Remove newline from string at the end
   if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n'){
      size -= 2;
      buffer[size] = 0;
   }
   else if (buffer[size - 1] == '\n'){
      --size;
      buffer[size] = 0;
   }


   // Send buffer to server
   if ((send(SOCKET, buffer, size, 0)) == -1) {
      perror("send error");
      return;
   }

  // Answer from Server
   size = recv(SOCKET, buffer, BUF - 1, 0);
   if (size == -1){
      perror("recv error");
      return;
   }
   else if (size == 0){
      std::cout << "Server closed remote socket" << std::endl;
      return;
   }
   else{
      std::cout << "\nServer Reply:" << std::endl;
      std::cout << buffer << std::endl;
     
      buffer[size] = '\0';
      
    
      if (strcmp("ERR\n", buffer) == 0){
        fprintf(stderr, "<< Server error occured, abort\n");
        return;
      }
   }
}



// Tests message class constructor
void testMessageCreation(){
  std::string formattedForSending;
  std::string garbage = "Garbage\n";
  Message firstMessage;
  formattedForSending = firstMessage.formatForSending();
  formattedForSending = garbage + formattedForSending + garbage;
  
  Message secondMessage = Message(formattedForSending);
  std::cout << "\nFirst Message: " << std::endl;
  firstMessage.printMessage();
  std::cout << "\nSecond Message: " << std::endl;
  secondMessage.printMessage();
  std::cout << "Sent to Server:\n\"" << formattedForSending << "\"\n" << std::endl;
}
