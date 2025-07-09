#ifndef CHAT_BOT_H
#define CHAT_BOT_H
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

class ChatBot {
public:
    ChatBot() {}
    string getResponse(const string &question, const string &model);

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
};

#endif