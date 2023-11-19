#include <cstring>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

// Fork includes
#include <sys/types.h>
#include <sys/wait.h>

// Thread includes
#include <thread>
#include <mutex>

// Folder navigation Linux
#include <dirent.h>
#include <errno.h>
#include <filesystem>
#include <fstream>
#include <sys/stat.h> 
#include <fcntl.h>
#include <chrono>

// Socket includes
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Message class
#include "Message.h"

// LDAP includes
#include "myldap.h"


// Main Tasks
bool prepareDirectory();
std::string saveMsgToDB(Message MessageToSave);
Message readMessageFromDB(int messageID, std::string filepath);
std::string LOGIN(std::string username, std::string password, char clientIP[INET_ADDRSTRLEN], bool& isLoggedIn, int& loginTries);
std::string LIST(std::string username);
std::string READ(std::string username, std::string messageID);
std::string DEL(std::string username, std::string messageID);
std::string blacklist(std::string clientIP);
bool isBlacklisted(std::string clientIP);
// Index functions
std::string getUserByIndex(int index, std::string path);
std::string fixInidices(int messageID, std::string accountPath);
std::string fixMessageID(std::string inboxPath, std::string filepath, int messageID);
void removeLastLine(std::string filepath, std::string filename);
int newMessageID(Message message);
// Communication
void *clientCommunication(void *data, char clientIP[INET_ADDRSTRLEN]);
void signalHandler(int sig);

//Mutexes
std::mutex blacklistMutex;

// Buffer & Globals
#define BUF 1024
int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;
std::string mailSpool = "./";

//Input format: twmailer-server <port> <directory>
//Test  command ./twmailer-server 6543 ./MessageDB


//############################################################################################
int main(int argc, char *argv[]) {

  std::string serverPort = argv[optind];
  if(argc == 3){
    mailSpool = argv[optind + 1];
  }
  int port = stoi(serverPort);

  socklen_t addrlen;
  struct sockaddr_in address, cliaddress;
  int reuseValue = 1;

  //build directory structure
  prepareDirectory();

  // SIGNAL HANDLER -- SIGINT (Interrup: ctrl+c)
  if (signal(SIGINT, signalHandler) == SIG_ERR) {
    perror("signal can not be registered");
    return EXIT_FAILURE;
  }

  // CREATE A SOCKET - IPv4, TCP (connection oriented), IP (same as client)
  if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket error");
    return EXIT_FAILURE;
  }

  // SET SOCKET OPTIONS - socket, level, optname, optvalue, optlen
  if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &reuseValue, sizeof(reuseValue)) == -1) {
    perror("set socket options - reuseAddr");
    return EXIT_FAILURE;
  }

  if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEPORT, &reuseValue, sizeof(reuseValue)) == -1) {
    perror("set socket options - reusePort");
    return EXIT_FAILURE;
  }

  // INIT ADDRESS -  Attention: network byte order => big endian
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // ASSIGN AN ADDRESS WITH PORT TO SOCKET
  if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1) {
    perror("bind error");
    return EXIT_FAILURE;
  }

  // ALLOW CONNECTION ESTABLISHING - Socket, Backlog (= count of waiting
  // connections allowed)
  if (listen(create_socket, 5) == -1) {
    perror("listen error");
    return EXIT_FAILURE;
  }

  while (!abortRequested) {
    std::cout << "Waiting for connections..." << std::endl;

    // ACCEPTS CONNECTION SETUP - blocking, might have an accept-error on ctrl+c
    addrlen = sizeof(struct sockaddr_in);
    if ((new_socket = accept(create_socket, (struct sockaddr *)&cliaddress, &addrlen)) == -1) {
      if (abortRequested) {
        perror("accept error after aborted");
      } else {
        perror("accept error");
      }
      break;
    }

    // START CLIENT
    printf("Client connected from %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));

    // ----------- Threading later in this functions --------------

    // Extract and print client IP address
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cliaddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    std::cout << "Client connected from IP: " << clientIP << std::endl;
    
    std::thread clientThread(clientCommunication, &new_socket, clientIP);
    clientThread.detach();  // Thread freigeben, um Ressourcen zu verwalten
    
    new_socket = -1;
  }

  // Frees the descriptor
  if (create_socket != -1) {
    if (shutdown(create_socket, SHUT_RDWR) == -1) {
      perror("shutdown create_socket");
    }
    if (close(create_socket) == -1) {
      perror("close create_socket");
    }
    create_socket = -1;
  }

  return EXIT_SUCCESS;
}



// Communication with client
void *clientCommunication(void *data, char clientIP[INET_ADDRSTRLEN]) {
  char buffer[BUF];
  int size;
  int *current_socket = (int *)data;

  int loginTries = 0;// TODO delete later
  bool isLoggedIn = false; // TODO delete later

  // SEND welcome message
  strcpy(buffer, "Connection to Server successful\r\n");
  if (send(*current_socket, buffer, strlen(buffer), 0) == -1) {
    perror("send failed");
    return NULL;
  } 

  //#########################vvv Running Server vvv##############################################
  do {
    // Receive client command
    size = recv(*current_socket, buffer, BUF - 1, 0);
    if (size == -1) {
      if (abortRequested) {
        perror("recv error after aborted");
      } 
      else {
        perror("recv error");
      }
      break;
    }

    if (size == 0) {
      std::cout << "Client closed remote socket" << std::endl;
      break;
    }

    // Remove ugly debug message, because of the sent newline of client
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
      size -= 2;
    } 
    else if (buffer[size - 1] == '\n') {
      --size;
    }

    buffer[size] = '\0';
    std::cout << "Message received: " << buffer << "\n" << std::endl;

    // Server Logic --------------------------------------------
    std::istringstream ss(buffer);
    char delimiter = '\n';
    std::string command;
    std::string username;
    std::string password;
    std::string messageID;
    std::string path;
    std::string responseMessage;
    char responseCharArray[1000];

    // Check sent command
    std::getline(ss, command, delimiter);

    // LOGIN
    if (command == "LOGIN") {
      std::getline(ss, username, delimiter);
      std::getline(ss, password, delimiter);
      responseMessage = LOGIN(username, password, clientIP, isLoggedIn, loginTries);
    }
    // SEND
    else if (command == "SEND") {
      Message ReceivedMessage(buffer);
      ReceivedMessage.setSender(username);
      responseMessage = saveMsgToDB(ReceivedMessage);
    }
    // LIST
    else if (command == "LIST") {
      // Extract username
      responseMessage = LIST(username);
    }
    // READ
    else if (command == "READ") {
      // Extract username and messageID
      std::getline(ss, messageID, delimiter);
      responseMessage = READ(username, messageID);
    }
    // DEL
    else if (command == "DEL") {
      std::getline(ss, messageID, delimiter);
      responseMessage = DEL(username, messageID);
    }


    // Save response to charArray
    strncpy(responseCharArray, responseMessage.c_str(), sizeof(responseCharArray));
    // Complete c String by using a  "\0"
    responseCharArray[sizeof(responseCharArray) - 1] = '\0';
    // Send response to client
    if (send(*current_socket, responseCharArray, BUF-2, 0) == -1) {
      perror("send answer failed");
      return NULL;
    }
  } while (strcmp(buffer, "QUIT") != 0 && !abortRequested);
  //#########################^^^ Running Server ^^^##############################################

  
  // Closes/frees the socket
  if (*current_socket != -1) {
    if (shutdown(*current_socket, SHUT_RDWR) == -1) {
      perror("shutdown new_socket");
    }
    
    if (close(*current_socket) == -1) {
      perror("close new_socket");
    }
    *current_socket = -1;
  }

  return NULL;
}



// Handles Signals
void signalHandler(int sig) {
  if (sig == SIGINT) {
    std::cout << "abort Requested..." << std::endl;
    abortRequested = 1;

    if (new_socket != -1) {
      if (shutdown(new_socket, SHUT_RDWR) == -1) {
        perror("shutdown new_socket");
      }
      if (close(new_socket) == -1) {
        perror("close new_socket");
      }
      new_socket = -1;
    }

    if (create_socket != -1) {
      if (shutdown(create_socket, SHUT_RDWR) == -1) {
        perror("shutdown create_socket");
      }
      if (close(create_socket) == -1) {
        perror("close create_socket");
      }
      create_socket = -1;
    }
  } 
  else {
    exit(sig);
  }
}



// Handles LOGIN request
std::string LOGIN(std::string username, std::string password, char clientIP_char[INET_ADDRSTRLEN], bool& isLoggedIn, int& loginTries){
  std::cout << "LDAP Output: " << ldapAuthentication(username, password) << std::endl;
  std::string clientIP = clientIP_char;

if(!isBlacklisted(clientIP)){
  if(ldapAuthentication(username, password) == "OK\n"){

    isLoggedIn = true;

    return "OK\n";
  }
  ++loginTries;
   
  if(loginTries >= 1){
    // write to blacklist when mutex is unlocked
    blacklistMutex.lock();
    blacklist(clientIP);
    blacklistMutex.unlock();

    loginTries = 0;
    return "LOCKED\n";
  }
  return "ERR\n";
}
else{
  std::cout << "on blacklist" << std::endl;
  return "LOCKED\n";
}

}

// Handles LIST request
// Returns:
//   <count of messages>
//   <messageID>. <subject>
std::string LIST(std::string username) {
  std::ifstream input_file(mailSpool + "/" + username + "/index.txt");
  int counter = 0;
  std::string line;
  std::string response = "";
  
  if (!input_file.is_open()) {
    std::cerr << "Error: file couldn't be opened." << std::endl;
    return "ERR\n";
  }

  //fill buffer string with every subject plus associated ID
  while(std::getline(input_file, line)){
    ++counter;
    if (counter % 3 == 1){
      response += line + ": ";
    }
    else if(counter % 3 == 0){
      response += line + "\n";
    }
  }

  response = std::to_string(counter/3) + "\n" + response;

  return response;
}


// Handles READ request
// Returns
//  "OK\n
//  <complete message>"
// or "ERR\n"
std::string READ(std::string username, std::string messageID){
  // Build path
  std::string path = mailSpool + "/" + username;
  std::cout << path << std::endl;
  // Create message object from DB
  Message serverResponse = readMessageFromDB(std::stoi(messageID), path);
  serverResponse.printMessage();
  // Save formatted response
  if (serverResponse.getMessageID() == "ERR\n"){
    return serverResponse.getMessageID();
  }
  else{
    return "OK\n" + serverResponse.formatForSending() + "\n";
  }
}



// Handles DEL request
// Returns "OK\n" or "ERR\n"
std::string DEL(std::string username, std::string messageID){

  std::string accountPath = mailSpool + "/" + username;
  std::string line;
  std::string content;
  bool messageLines = false;    // Flag for message lines that are part of the message to be deleted
  bool messageDeleted = false;  // Flag for passing the deleted message

  // Returns a path based on the associated index : like index = 4 - testuser -> .../inbox/testuser.txt
  std::string contactPath = getUserByIndex(stoi(messageID), accountPath);
  std::string tempPath = accountPath + "/inbox/temp.txt";

  std::cout << "account Path: " << accountPath << std::endl;
  std::cout << "contact Path: " << contactPath << std::endl;
  std::cout << "tmp Path: " << tempPath << std::endl;

  // Create temp file - needed to remove message lines
  std::ofstream tempFile(tempPath);

  // Open file 
  std::ifstream input_file(contactPath);
  if (!input_file.is_open()) {
    std::cerr << "Error: file couldn't be opened.(382)" << std::endl;
    return "ERR\n";
  }
  
  // Copy every line except the message we want to delete into a temporary file
  while (std::getline(input_file, line)) {    // Iterate through the file
    if (line == "{" + messageID + "}") {      // Search for ID-tag
      messageLines = true;                    // The following lines are treated as part of the message
    }
    
    if (messageLines) {                       // If we are in the message body
      if (line == "{end of message}") {       // If we reached the end-tag
        messageLines = false;                 // Stop ignoring the message
        messageDeleted = true;                // Mark message as deleted
        std::cout << "end of message recognized." << std::endl;
      }
    }
    else{                                     // Everything that shouldn't be deleted
      tempFile << line << std::endl;          // Write line in temp.txt
    }
  }

  // Close files
  input_file.close();
  tempFile.close();

  //Replace original with temp file
  // Delete original file 
  if (remove(contactPath.c_str()) != 0) {
      std::cerr << "Error, removing the original file" << std::endl;
      return "ERR\n";
  }

  // Rename temp file
  if (rename(tempPath.c_str(), contactPath.c_str()) != 0) {
      std::cerr << "Error, rename the temp file" << std::endl;
      return "ERR\n";
  }


  //-----------------------------------------------------------------------
  // Fix indices in all files
  fixInidices(stoi(messageID), accountPath);

  std::cout << "Operation del succesfull" << std::endl;


  //Check if DEL was successful
  if(messageDeleted == true){
    return "OK\n";
  }
  else{
    return "ERR\n";
  }
}



// Takes a message object and creates an entry in the correct location
// Returns "OK\n" or "ERR\n"
std::string saveMsgToDB(Message message) {
  if (message.messageVerify()) { // Validate message content
    
    // Get sender and receiver usernames
    std::string filePath;
    struct stat st; // Library struct to help us check directories - see line 336

    // Create relevant paths
    std::string userDirectory = mailSpool + "/" + message.getReceiver();
    std::string inboxDirectory = userDirectory + "/inbox";
    std::string indexFilePath = userDirectory + "/index.txt";

    
    // Check if needed directories exist and create them if not
    if((stat(userDirectory.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) == 0){
      // Create the User Directory
      if (mkdir(userDirectory.c_str(), 0777) == 0) {
        std::cout << "Created new receiver directory " << userDirectory << std::endl;
        // Create inbox directory
        if (mkdir(inboxDirectory.c_str(), 0777) == 0){
          std::cout << "Created new inbox directory" << inboxDirectory << std::endl;
        }
        else{
          std::cerr << "Error, new inbox directory couldn't be created" << std::endl;
          return "ERR\n";
        }
      
        // Create index.txt
        int fd = open(indexFilePath.c_str(), O_CREAT | O_WRONLY, 0777);
        if (fd != -1) {
          close(fd);
          std::cout << "Created new index.txt file" << std::endl;
        }
        else {
          std::cerr << "Error, new index.txt file couldn't be created" << std::endl;
          return "ERR\n";
        }
        
      }
      else {
        std::cerr << "Error, new receiver directory couldn't be created" << std::endl;
        return "ERR\n";
      }
    }


    // Get a new Message ID and save it in the Message object
    int newID = newMessageID(message);
    if (newID < 0){
      std::cerr << "Error, failed to create Message ID" << std::endl;
      return "ERR\n";
    }
    message.setMessageID(newID);
    
    // Construct the file path
    filePath = mailSpool + "/" + message.getReceiver() + "/inbox/" + message.getSender() + ".txt";
  
    // Open the file in append mode
    std::ofstream outputFile(filePath, std::ios::app);

    if (outputFile.is_open()) {
      // Write the message in the desired format using formatForSaving()
      outputFile << message.formatForSaving();
      outputFile.close();
      return "OK\n";
      
    } 
    else {
      std::cerr << "Error: Unable to open file for saving the message." << std::endl;
      return "ERR\n";
    }
    
  }
  else {
    std::cerr << "Message corrupted." << std::endl;
    return "ERR\n";
  }
}




// Creates a message object from a DB entry
Message readMessageFromDB(int messageID, std::string filepath) {
  // filepath = ./MessageDB/*username*
  std::string newFilePath;

  // Look up correct file using the message index
  newFilePath = getUserByIndex(messageID, filepath);
  // Open file
  std::ifstream input_file(newFilePath);
  if (!input_file.is_open()) {
    std::cerr << "Error: file couldn't be opened. (543)" << std::endl;
    // Create a message object with error code as ID
    Message errorMsg("ERR\n", "", "", "", "");
    return errorMsg;
  }

  // Search variables
  std::string content;
  std::string IDTag = std::to_string(messageID);
  bool messageLines = false;

  std::string line;

  while (std::getline(input_file, line)) {    // Iterate through the file
    if (line == "{" + IDTag + "}") {          // Search for ID-tag
      messageLines = true;                    // The following lines are treated as part of the message
      continue;
    }
    
    if (messageLines) {                       // If we are in the message body
      if (line == "{end of message}") {       // If we reached the end-tag
        messageLines = false;                 // Stop reading the message             
      }
      else{
        content += line;                      // Append line to the message content
        content += "\n";
      }   
    }
  }

  // Close file
  input_file.close(); 

  Message foundMessage(content);
  std::cout << "Found Message:\n" << std::endl;
  foundMessage.printMessage();

  return foundMessage;
}



// Find the corresponding contact file to the input Index in index.txt
std::string getUserByIndex(int index, std::string path) {
  // path = ./MessageDB/*username*

  std::ifstream input_file(path + "/index.txt"); // Get the relevant index file
  std::string line;
  std::string sender = "";
  std::string stringIndex = std::to_string(index);

  if (!input_file.is_open()) {
    std::cerr << "Error: file couldn't be opened.(595)" << std::endl;
    return "ERR\n";
  }

  // Compares messageId with index. If they are equal, return the username beneath
  do {
    std::getline(input_file, line);
    if (line == stringIndex) {
      std::getline(input_file, sender);
    }

  } while (line != stringIndex && !input_file.eof());

  // If sender is empty return an error
  if (sender == "") {
    return "ERR\n";
  }

  // Otherwise return the correct path
  return path + "/inbox/" + sender + ".txt";
}



// Fix indices
std::string fixInidices(int messageID, std::string accountPath){
  
  std::string tempIndexPath = accountPath + "/temp.txt";
  std::string indexPath = accountPath + "/index.txt";
  std::string inboxPath = accountPath + "/inbox";
  std::string filePath;
  std::string line;
  bool messageDeleted = false;  // Flag for passing the deleted message
  bool firstEntry = true;
  int tempID = -1;

  // Open the index file in read mode
  std::ifstream input_file(indexPath);
  // Open a temp file in append mode
  std::ofstream tempFile(tempIndexPath);

  if (!input_file.is_open()) {
    std::cerr << "Error: file couldn't be opened.(631)" << std::endl;
    return "ERR\n";
  }

  // Fix indices in index.txt
  do {
    std::getline(input_file, line);
    
    if (line == std::to_string(messageID)) { // If index of message to be deleted is found
      std::getline(input_file, line);        // skip 3 lines
      std::getline(input_file, line);  
      std::getline(input_file, line);
      messageDeleted = true;
    }

    if(messageDeleted){
      if(std::isdigit(line[0])){            // If the line is the Index part of an entry
        tempID = stoi(line);                
        tempID--;                           // Decrement Index
        if(firstEntry){
          tempFile << tempID;
          firstEntry = false;
        }
        else{
          tempFile << "\n" << tempID;       // Replace Index
        }
        
      }
      else{
        if(firstEntry){
          tempFile << line;
          firstEntry = false;
        }
        else{
          tempFile << "\n" << line;       // Replace Index
        }
      }
    }
    else{                                    // Save every line before the deleted message as is
      if(firstEntry){
        tempFile << line;
        firstEntry = false;
      }
      else{
        tempFile << "\n" << line;         // Replace Index
      }
    }
  } while (!input_file.eof());

  //Replace original with temp file
  // Delete original file 
  if (remove(indexPath.c_str()) != 0) {
    std::cerr << "Error, removing the original file" << std::endl;
    return "ERR\n";
  }

  // Rename temp file
  if (rename(tempIndexPath.c_str(), indexPath.c_str()) != 0) {
    std::cerr << "Error, rename the temp file" << std::endl;
    return "ERR\n";
  }
    
  // Fix indices in all contact files
  

  // Open directory
  DIR* directory = opendir(inboxPath.c_str());
  if (directory) {
    struct dirent* entry;

    // Run through every .txt file
    while ((entry = readdir(directory)) != nullptr) {
      // Checks if the file is a correct file
      if (entry->d_type == DT_REG) { 
        // filename of the current txt
        const char* filename = entry->d_name;

        filePath = inboxPath + "/" + filename;
        fixMessageID(inboxPath, filePath, messageID);
        removeLastLine(inboxPath, filename);
      }
    }

    closedir(directory);
  } 
  else {
    std::cerr << "Error, trying to open directory" << std::endl;
  }

  return "OK\n";
}



// Returns a new unique message ID by searching through the index file
int newMessageID(Message message) {
  std::string filepath = mailSpool + "/" + message.getReceiver() + "/index.txt";
  // Open file in read mode
  std::ifstream input_file(filepath); 

  if (!input_file.is_open()) {
    std::cerr << "Error: file couldn't be opened. (newMessageID)" << std::endl;
    return -1;
  }

  std::string IDTag;
  int messageID = 1;
  std::string line;

  // Run through indices to find the highest ID
  while (std::getline(input_file, line)) {
    IDTag = std::to_string(messageID);
    if (line == IDTag) {
      messageID++;
    }
  }

  input_file.close(); // Close file

  
  // Open in append mode
  std::ofstream outputFile(filepath, std::ios::app); 
  // Append new index
  if (outputFile.is_open()) {
    outputFile << messageID << "\n" << message.getSender() << "\n" << message.getSubject() << std::endl;
    outputFile.close(); // Close the output file
  } 
  else {
    std::cerr << "Error: Unable to open the file for appending. (newMessageID)" << std::endl;
    return -1;
  }
  return messageID;
}



std::string fixMessageID(std::string inboxPath, std::string filepath, int messageID) {
  // Open the file to adjust it
  std::ifstream file(filepath);
  std::string tempFilePath = inboxPath + "/temp.txt";
  std::ofstream tempFile(tempFilePath);
  
  std::string line;
  std::string id = "";
  int newID = 0;
  
  if (file.is_open()) {
    while(!file.eof()) {
      std::getline(file, line);
      if(line[0] == '{' && line[1] != 'e'){ 
        //extract number eg. {456} -> 456
        for (char character : line) {
          if (std::isdigit(character)) {
            id += character;
          }
        }
        

        if(stoi(id) > messageID){
          newID = stoi(id) - 1;
          tempFile << "{" << newID << "}" << std::endl;
        }
        else{
          tempFile << line << std::endl;
        }

        //reset id 
        id = "";
      }
      else{
        tempFile << line << std::endl;
      }
    }
  }
  
  //close file after adjustment
  file.close();
  tempFile.close();

  //Replace original with temp file
  // Delete original file 
  if (remove(filepath.c_str()) != 0) {
    std::cerr << "Error, removing the original file" << std::endl;
    return "ERR\n";
  }

  // Rename temp file
  if (rename(tempFilePath.c_str(), filepath.c_str()) != 0) {
    std::cerr << "Error, rename the temp file" << std::endl;
    return "ERR\n";
  }
  return "OK\n";
}



void removeLastLine(std::string filepath, std::string filename) {
  std::ifstream file(filepath + "/" + filename);
  std::ofstream tempFile(filepath + "/temp.txt");
  std::string line;

  // Copy first line without newline before
  if (file.is_open()) {
    std::getline(file, line);
    tempFile << line;
    // Copy all other lines with a newline at the start
    while(std::getline(file, line)) {
      tempFile << "\n" << line;
    }
  } 

  //close file after adjustment
  file.close();
  tempFile.close();

  //Replace original with temp file
  // Delete original file 
  if (remove((filepath + "/" + filename).c_str()) != 0) {
    std::cerr << "Error, removing the original file" << std::endl;
  }

  // Rename temp file
  if (rename((filepath + "/temp.txt").c_str(), (filepath + "/" + filename).c_str()) != 0) {
    std::cerr << "Error, rename the temp file" << std::endl;
  }
}



//prepare Directory structure
bool prepareDirectory(){
  // Create Mail directory
  if (mkdir(mailSpool.c_str(), 0777) == 0){
    std::cout << "Created new directory: " << mailSpool << std::endl;
  }
  else{
    std::cerr << "Error, new directory couldn't be created: " << mailSpool << "\nThis Directory might already exist." <<  std::endl;
    return false;
  }

  std::string blacklistFilePath = mailSpool + "/blacklist.txt";
  // Create index.txt
  int fd = open(blacklistFilePath.c_str(), O_CREAT | O_WRONLY, 0777);
  if (fd != -1) {
    close(fd);
    std::cout << "Created blacklist.txt" << std::endl;
  }
  else {
    std::cerr << "Error, blacklist.txt couldn't be created" << std::endl;
    return "ERR\n";
  }


  return true;
}

std::string blacklist(std::string clientIP){
  std::ofstream file(mailSpool + "/blacklist.txt", std::ios::app);


  auto currentTime = std::chrono::system_clock::now();
  auto timestampInSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch());

  if (file.is_open()) {
    // Write the message in the desired format using formatForSaving()
    file << clientIP << std::endl;
    file << timestampInSeconds.count() << std::endl;
    file.close();
    return "OK\n";
  } 
  else {
    std::cerr << "Error: Unable to open blacklist.txt for writing." << std::endl;
    return "ERR\n";
  }
  
}



bool isBlacklisted(std::string clientIP){
  std::ifstream file(mailSpool + "/blacklist.txt");
  std::ofstream tempFile(mailSpool + "/temp.txt");
  std::string line;
  bool blacklisted = false;

  //Get timestamp;
  auto currentTime = std::chrono::system_clock::now();
  auto timestampInSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch());

  // Copy first line without newline before
  if (file.is_open()) {

    while(std::getline(file, line) || !file.eof()){

      //check values
      if(line == clientIP){
        std::getline(file, line);
        blacklisted = true;

        //verify time
        if(timestampInSeconds.count() - std::stoi(line) < 30){
          tempFile << clientIP << std::endl;
          tempFile << line << std::endl;
          
        }
        else{
          blacklisted = false;
        }
      }
      else{
        tempFile << line << std::endl;
      }
    }

    

    //close file after adjustment
    file.close();
    tempFile.close();

    //Replace original with temp file
    // Delete original file 
    if (remove((mailSpool + "/blacklist.txt").c_str()) != 0) {
      std::cerr << "Error, removing the original file" << std::endl;
    }

    // Rename temp file
    if (rename((mailSpool + "/temp.txt").c_str(), (mailSpool + "/blacklist.txt").c_str()) != 0) {
      std::cerr << "Error, rename the temp file" << std::endl;
    }

  } 
  return blacklisted;

}