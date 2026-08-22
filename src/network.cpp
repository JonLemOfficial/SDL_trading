#include "network.hpp"

static size_t wcb(void* p,size_t sz,size_t nm,void* u){
    ((std::string*)u)->append((char*)p,sz*nm); return sz*nm;
}
std::string http_get(const std::string& url){
    CURL* curl=curl_easy_init(); if(!curl)return "";
    std::string buf;
    curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,wcb);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&buf);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,10L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0L);
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return buf;
}
bool http_post(const std::string& url,const std::string& body,const std::string& ct){
    CURL* curl=curl_easy_init(); if(!curl)return false;
    std::string resp;
    struct curl_slist* hdr=nullptr;
    std::string ct_hdr="Content-Type: "+ct;
    hdr=curl_slist_append(hdr,ct_hdr.c_str());
    curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,body.c_str());
    curl_easy_setopt(curl,CURLOPT_HTTPHEADER,hdr);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,wcb);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&resp);
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,10L);
    curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0L);
    CURLcode res=curl_easy_perform(curl);
    curl_slist_free_all(hdr); curl_easy_cleanup(curl);
    return res==CURLE_OK;
}
bool telegram_send(const std::string& bot,const std::string& chat,const std::string& text){
    std::string url="https://api.telegram.org/bot"+bot+"/sendMessage";
    std::string esc;
    for(char ch:text){
        if(ch==' ')esc+="%20";
        else if(ch=='\n')esc+="%0A";
        else esc+=ch;
    }
    return http_post(url,"chat_id="+chat+"&text="+esc);
}
