#include "chat_bot.h"

string ChatBot::getResponse(const string &question, const string &model)
{
    CURL *curl;
    CURLcode res;
    string buf;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        string api_key = "sk-CgYJpV36YnkjqaRyDNzTMmGnawswGsSa9MtOQO6g9DkeBJZl";
        headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        json request_body = {
            {"model", model}, 
            {"store", true},
            {"messages", {{{"role", "user"}, {"content", question}}}}};

        string data = request_body.dump();

        curl_easy_setopt(curl, CURLOPT_URL, "https://www.blueshirtmap.com/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            try 
            { 
                json jsonResponse = json::parse(buf);
                cout << jsonResponse << endl;
                if (jsonResponse.contains("choices") && !jsonResponse["choices"].empty() &&
                    jsonResponse["choices"][0].contains("message") &&
                    jsonResponse["choices"][0]["message"].contains("content"))
                {
                    std::string content = jsonResponse["choices"][0]["message"]["content"];
                    return content;
                }
                else
                {
                    std::cerr << "Unexpected JSON structure: " << jsonResponse.dump() << std::endl;
                }
            }
            catch (json::parse_error &e)
            {
                std::cerr << "Failed to parse the JSON response: " << e.what() << std::endl;
            }
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
    return "";
}
