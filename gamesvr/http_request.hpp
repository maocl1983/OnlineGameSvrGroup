#ifndef HTTP_REQUEST_HPP_
#define HTTP_REQUEST_HPP_

#include "common_def.hpp"
#include <list>
#include <curl/curl.h>

#define URL_LEN         1024
#define CONTENT_LEN     8192
#define RECVBUFF_LEN    8192

typedef enum request_type_t {
    em_get_request = 1,
    em_post_request,
} request_type_t;

typedef struct request_t {
    uint32_t userid;
    char url[URL_LEN+1];
    char content[CONTENT_LEN+1];
    request_type_t type;
} request_t;

typedef struct recv_buffer_t {
    char buffer[RECVBUFF_LEN];
    size_t size;
} recv_buffer_t;

class RequestInfo {
public:
    RequestInfo();
    ~RequestInfo();

    CURL *get_handle()
    { return curl_handle; }
    void set_handle(CURL *handle)
    { curl_handle = handle; }

    recv_buffer_t *buff()
    { return &recv_buff; }
private:
    CURL *curl_handle;
    recv_buffer_t recv_buff;
};

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    int add_request(uint32_t uid, request_type_t type, const char *url, const char *content);
    void request_deal_loop();

    void DoTest(uint32_t uid);
private:
    int pop_request_list();
    int request_select();
    int request_done();

    int add_get_request(const char* url);
    int add_post_request(const char *url, const char *post_content);
    int add_request_handle(RequestInfo *p_info);
private:
    CURLM* curl_m;
    int request_nums;
    std::list<request_t> request_list;
};

void *deal_request_thread(void *arg);
#endif
