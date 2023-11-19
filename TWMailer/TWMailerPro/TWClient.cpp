#include <getopt.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>


// Folder navigation Linux
#include <dirent.h>
#include <errno.h>

// Socket includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Message Class
#include "Message.h"
// Get Password
#include "mypw.h"


//////////////////////////////////////////////////////////////////////////////////////////////
// Function Decalarations

// UI functions
void userInterface();
void SEND();
void SEND(Message message);
void READ();
void LIST();
void DEL();
void QUIT();

// Login functions
std::string LOGIN();
bool isLoggedIn();

// Server functions
std::string sendToServer(std::string serverCommand);

// Debug funcctions
void TEST();
void testMessageCreation();

//////////////////////////////////////////////////////////////////////////////////////////////
// Buffer & Globals
#define BUF 1024
int SOCKET;

//////////////////////////////////////////////////////////////////////////////////////////////
// Input format: twmailer-client <ip> <port>
// Test  command ./twmailer-client 127.0.0.1 6543
//////////////////////////////////////////////////////////////////////////////////////////////

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


//////////////////////////////////////////////////////////////////////////////////////////////
// UI Functions

// Take User input
void userInterface(){
  std::string command;
  std::string loginReply;

  std::cout << "Welcome to TW-Mailer!" << std::endl;

  while (command != "QUIT"){
    if(isLoggedIn() == false){
      std::cout << "Valid commands are QUIT and LOGIN" << std::endl;
      std::cout << ">> ";
      std::cin >> command;
      if(command == "LOGIN"){
        loginReply = LOGIN();
        if(loginReply == "OK\n"){
          std::cout << "Login successful!" << std::endl;
        }
        else if(loginReply == "ERR\n"){
          std::cout << "Login failed" << std::endl;
        }
        else if(loginReply == "LOCKED\n"){
          std::cout << "You have been locked. Try again later." << std::endl;
        }
        else{
          std::cout << "Server error: " << loginReply << std::endl;
        }
      }
      else if (command == "QUIT") {
        QUIT();
        return;
      }
    }
    else{
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
    } 
  };
}



// Create a new message and send it to the server
// Print result
void SEND() {
  Message newMessage;
  std::string response;
  std::string sendMessage = newMessage.formatForSending();

  // Construct protocol message
  sendMessage = "SEND\n" + sendMessage;
  // Send the message
  response = sendToServer(sendMessage);

  //Output to console
  if(response == "OK\n"){
    std::cout << "Message Sent" << std::endl;
  }
  else{
    std::cout << "Sending Message failed. Please try again!" << std::endl;
  }
}



// Send prebuilt message (for Testing)
void SEND(Message message) {
  std::string sendMessage = message.formatForSending();
  sendMessage = "SEND\n" + sendMessage;
  // Send the message
  sendToServer(sendMessage);
}



// Display a specific message of a specific user
void READ() { std::cout << "\n"; 
  std::string readMessage = "READ\n";
  std::string response;
  std::string messageID;

  // Get Input
  std::cout << "\nDislpays a specific message." << std::endl;
  std::cout << "\nEnter Message id: " << std::endl;
  std::cin >> messageID;

  // Build the READ request
  readMessage = readMessage + messageID + "\n";
  response = sendToServer(readMessage);

  //Output response to console
  std::string prefix = "OK\n";
  if (response.substr(0, prefix.size()) == prefix) {  //checks for "OK\n" at the start of the response
    response = response.substr(prefix.size());        //removes the "OK\n" from the string
    Message readMessage(response, "read");            //constructs Message object
    readMessage.printMessage();                       //prints the formatted message
  } else if (response == "ERR\n"){
    std::cout << "Read request unsuccessful. Please try again." << std::endl;
  }
}



// List all messages of a specific user
void LIST() {
  std::string listMessage = "LIST\n";
  std::string response;

  // Send the LIST request
  response = sendToServer(listMessage);
  // Output to console
  std::cout << response << std::endl;
}



// Delete a specific message of a specific user
void DEL() {
  std::string delMessage = "DEL\n";
  std::string messageID;
  std::string response;

  // Get Input
  std::cout << "\ndelete a specific message" << std::endl;
  std::cout << "\nMessage Number: " << std::endl;
  std::cin >> messageID;

  // Build and send the DEL request
  delMessage = delMessage + messageID + "\n";
  response = sendToServer(delMessage);

  //Output to console
  if(response == "OK\n"){
    std::cout << "Message deleted." << std::endl;
  }
  else{
    std::cout << "Message deletion failed. Please try again." << std::endl;
  }
}



// Close the server connection
void QUIT() {
  sendToServer("QUIT");
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Login functions

// Gets user input for LDAP verification
std::string LOGIN(){
  std::string loginMessage = "LOGIN\n";
  std::string username;
  std::string password;

  std::cout << "\nEnter your username: " << std::endl;
  std::cin >> username;

  std::cin.clear();
  
  // Get password - input invisible
  password = getpass();

  // Build the LOGIN request
  loginMessage = loginMessage + username + "\n" + password + "\n";
  return sendToServer(loginMessage); //returns the server response
}



// Checks if user is logged in with verified LDAP credentials on server
bool isLoggedIn(){
  std::string response = sendToServer("LOGINSTATUS\n");

  if(response == "VERIFIED\n"){
    return true;
  }
  else if(response == "LOCKED\n"){
    return false;
  }
  else{
    return false;
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Server Functions

// Sends the input string to the server and listens to response
std::string sendToServer(std::string serverCommand){
  char buffer[BUF];
  
  // Save command to buffer
  strncpy(buffer, serverCommand.c_str(), sizeof(buffer));
  // Complete c String by appending "\0"
  buffer[sizeof(buffer) - 1] = '\0';
  

  // Remove newline from string at the end
  int size = strlen(buffer);
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
    return "send error";
  }

  // Answer from Server
  size = recv(SOCKET, buffer, BUF - 1, 0);
  if (size == -1){
    // Error
    perror("recv error");
    return "recv error";
  }
  else if (size == 0){
    // Socket closed
    std::cout << "Server closed remote socket" << std::endl;
    return "Server closed remote socket";
  }
  else{
    // Success
    buffer[size] = '\0';
    return buffer;
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////
// Test functions

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




// Tests message class constructor
void testMessageCreation(){
  std::string formattedForSending;
  std::string garbage = "Garbage\n";
  Message firstMessage;
  formattedForSending = firstMessage.formatForSending();
  formattedForSending = garbage + formattedForSending + garbage;
  
  Message secondMessage = Message(formattedForSending, "send");
  std::cout << "\nFirst Message: " << std::endl;
  firstMessage.printMessage();
  std::cout << "\nSecond Message: " << std::endl;
  secondMessage.printMessage();
  std::cout << "Sent to Server:\n\"" << formattedForSending << "\"\n" << std::endl;
}
