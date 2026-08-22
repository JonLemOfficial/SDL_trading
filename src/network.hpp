#pragma once
#include <string>
#include <curl/curl.h>

std::string http_get(const std::string& url);
bool http_post(const std::string& url,const std::string& body,
               const std::string& content_type="application/x-www-form-urlencoded");
bool telegram_send(const std::string& bot_token,const std::string& chat_id,const std::string& text);
