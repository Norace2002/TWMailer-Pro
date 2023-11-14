#include "Message.h"

// Construct from String
Message::Message(std::string textContainingMessage) {
  std::istringstream ss(textContainingMessage);
  char delimiter = '\n';

  std::string temp;
  // Discard anything before
  std::getline(ss, temp, delimiter);
  // Read and set the variables
  std::getline(ss, sender, delimiter);
  std::getline(ss, receiver, delimiter);
  std::getline(ss, subject, delimiter);
  //std::getline(ss, messageText, ".\n");

  // Read the message text
  messageText.clear();
  std::string line;

  while (std::getline(ss, line, delimiter)) {
      if (line == ".") {
          // Check if it's a standalone "."
          if (messageText.empty() || messageText.back() == '\n') {
              // End of message reached
              break;
          }
      }
      messageText += line + '\n';
  }
}



// Construct with initializer list (only for debugging)
Message::Message(std::string messageID, std::string subject, std::string messageText, std::string sender, std::string receiver)
    : messageID(messageID), subject(subject), messageText(messageText), sender(sender), receiver(receiver) {
}



// Construct from user input
Message::Message(){
  while (std::cin.get() != '\n') {
      // Discard characters until a newline character is found
  }
  
  // Get user input data

  sender = "placeholder";
  std::cout << "\nReceiver: " << std::endl;
  receiver = this->getInputString(60);
  std::cout << "\nSubject: " << std::endl;
  subject = this->getInputString(80);

  // Get message
  char c = 'a';
  bool endOfMessage = false;
  std::cout << "\nEnter your message (end with '.'):\n";

  while (!endOfMessage) {
    c = std::cin.get();          // Read a character
    messageText += c;            // Add the character to the message

    if (c == '.') {              //catching ".\n" in two steps
      c = std::cin.get();        //check the char after '.'
      if(c == '\n'){             //if the next char is a newline
        messageText.pop_back();  //remove the '.'
        endOfMessage = true;     //end the loop
      }
      else{                      //otherwise
        messageText += c;        //add the '.' back to the message
      }
    }
  }
}



// Getters
std::string Message::getMessageID() const {
    return messageID;
}

std::string Message::getSubject() const {
    return subject;
}

std::string Message::getMessageText() const {
    return messageText;
}

std::string Message::getSender() const {
    return sender;
}

std::string Message::getReceiver() const {
    return receiver;
}



// Setters
void Message::setMessageID(int newID) {
  this->messageID = std::to_string(newID);
}

void Message::setSubject(const std::string& subject) {
    this->subject = subject;
}

void Message::setMessageText(const std::string& messageText) {
    this->messageText = messageText;
}

void Message::setSender(const std::string& sender) {
    this->sender = sender;
}

void Message::setReceiver(const std::string& receiver) {
    this->receiver = receiver;
}



// Returns a saveable string
std::string Message::formatForSaving(){
  std::string delimiter = "\n";
  std::string formattedMessage = "{" + messageID + "}\n" + sender + delimiter + receiver+ delimiter + subject + delimiter + messageText + "{end of message}\n";
  return formattedMessage;
}

// Returns a sendable string
std::string Message::formatForSending(){
  std::string delimiter = "\n";
  std::string formattedMessage = sender + delimiter + receiver+ delimiter + subject + delimiter + messageText + ".\n";
  return formattedMessage;
}



// Prints the message to commandline
void Message::printMessage(){
  if(messageID != ""){
    std::cout << "Message ID: " << messageID << std::endl;
  }
  std::cout << "Sent by " << sender << " to " << receiver << std::endl;
  std::cout << "Subject:\t" << subject << std::endl;
  std::cout << "Message Text:\t" << messageText << std::endl;
}



// Gets a string from user
std::string Message::getInputString(long unsigned int length){
  std::string inputString = "0";
  
  do {
    std::cout << "~ Please use a maximum of " << length <<" characters ~" << std::endl;
    
    std::getline(std::cin, inputString);
    
  } while (inputString.length() > length);
  
  return inputString;
}



// Checks whether all values are set (except for messageID)
bool Message::messageVerify(){
  bool isValid = true;
  if(receiver == ""){
    std::cout << "Receiver is empty" << std::endl;
    isValid = false;
  }
  if(subject == ""){
    std::cout << "Subject is empty" << std::endl;
    isValid = false;
  }
  if(messageText == ""){
    std::cout << "Message is empty" << std::endl;
    isValid = false;
  }
  return isValid;
}