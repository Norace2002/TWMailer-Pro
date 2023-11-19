#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <iostream>
#include <vector>
#include <sstream>


class Message {
public:
    // Construct from string
    Message(std::string textContainingMessage, std::string method);
    // Construct with initializer list (only for debugging)
    Message(std::string messageID, std::string subject, std::string messageText, std::string sender, std::string receiver);
    // Construct from user input
    Message();

    // Getters for member variables
    std::string getMessageID() const;
    std::string getSubject() const;
    std::string getMessageText() const;
    std::string getSender() const;
    std::string getReceiver() const;

    // Setters for member variables (optional)
    void setMessageID(int newID);
    void setSubject(const std::string& subject);
    void setMessageText(const std::string& messageText);
    void setSender(const std::string& sender);
    void setReceiver(const std::string& receiver);

    // Message handling
    std::string formatForSaving();
    std::string formatForSending();
    void printMessage();
    bool messageVerify();

private:
    std::string messageID = "";
    std::string subject;
    std::string messageText;
    std::string sender;
    std::string receiver;

    std::string getInputString(long unsigned int length);
};

#endif