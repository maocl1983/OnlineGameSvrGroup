#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "global_data.hpp"
#include "http_request.hpp"

using namespace std;

size_t recv_data(void *content, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;

    recv_buffer_t *buff = (recv_buffer_t*)userp;
    if(!buff) return 0;
    if (buff->size + realsize >= RECVBUFF_LEN) {
        ERROR_LOG("recv buff size max[%d]", buff->size + realsize);
        return 0;
    }
    strncat(buff->buffer, (char*)content, realsize);
    buff->size += realsize;
    buff->buffer[buff->size] = '\0';
    return realsize;
}

RequestInfo::RequestInfo()
{
    curl_handle = NULL;
    
    recv_buff.size = 1L;
    recv_buff.buffer[0] = '\0';
}

RequestInfo::~RequestInfo()
{
    if (curl_handle) 
        curl_easy_cleanup(curl_handle);
    curl_handle = NULL;
        
    recv_buff.size = 0;
}

HttpRequest::HttpRequest()
{
    curl_m = curl_multi_init();
    request_nums = 0;
}

HttpRequest::~HttpRequest()
{
    if (curl_m) {
        curl_multi_cleanup(curl_m);
    }
}

int HttpRequest::add_request(uint32_t uid, request_type_t type, const char *url, const char *content)
{
    request_t info;
    info.userid = uid;
    info.type = type;

    if (!url || strlen(url) >= URL_LEN) {
        if (url) KERROR_LOG(uid, "url len error[%d]", strlen(url));
        else KERROR_LOG(uid, "url nil error[%d]");
        return -1;
    }
    if (content && strlen(content) >= CONTENT_LEN) {
        KERROR_LOG(uid, "request content len error[%d]", strlen(content));
        return -1;
    }

    strcpy(info.url, url);
    if (content) {
        strcpy(info.content, content);
    }
    request_list.push_back(info);
    KDEBUG_LOG(uid, "ADD REQUEST[type=%d url=%s]", type, url);
    return 0;
}

int HttpRequest::add_request_handle(RequestInfo *p_info)
{
    if (!p_info || !curl_m) return -1;

    CURLMcode ret;
    curl_multi_add_handle(curl_m, p_info->get_handle());
    ret = curl_multi_perform(curl_m, &request_nums);
    if (ret != CURLM_OK && ret != CURLM_CALL_MULTI_PERFORM) {
        ERROR_LOG("add request failed ret=%d!\n", ret);
    }
     
    return 0;
}

int HttpRequest::add_get_request(const char* url)
{
    CURLcode ret;
    RequestInfo *p_info = new RequestInfo();

    p_info->set_handle(curl_easy_init());
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_URL, url);
    if (ret != CURLE_OK) {
        ERROR_LOG("get request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_HEADER, 0L);
    if (ret != CURLE_OK) {
        ERROR_LOG("get request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_WRITEFUNCTION, recv_data);
    if (ret != CURLE_OK) {
        ERROR_LOG("get request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_WRITEDATA, (void *)(p_info->buff()));
    if (ret != CURLE_OK) {
        ERROR_LOG("get request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_PRIVATE, p_info);
    if (ret != CURLE_OK) {
        ERROR_LOG("get request setopt error[%d]", ret);
        return ret;
    }

    add_request_handle(p_info);
    return ret;
}

int HttpRequest::add_post_request(const char *url, const char *post_content)
{
    CURLcode ret;
    RequestInfo *p_info = new RequestInfo();

    p_info->set_handle(curl_easy_init());
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_URL, url);
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_HEADER, 0L);
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_CUSTOMREQUEST, "post");
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_POST, strlen(post_content));
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_POSTFIELDS, post_content);
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_PRIVATE, p_info);
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }

    struct curl_slist *list = NULL;
    char cl_str[128] = {0x00};
    sprintf(cl_str, "Content-Length:%ld", strlen(post_content));
    list = curl_slist_append(list, "Content-Type: application/json");
    list = curl_slist_append(list, cl_str);
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_HTTPHEADER, list);
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }

    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_WRITEFUNCTION, recv_data);
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }
    ret = curl_easy_setopt(p_info->get_handle(), CURLOPT_WRITEDATA, (void *)p_info->buff());
    if (ret != CURLE_OK) {
        ERROR_LOG("post request setopt error[%d]", ret);
        return ret;
    }

    add_request_handle(p_info);
    return ret;
}

int HttpRequest::request_select()
{
    CURLMcode ret;
    int max_fd = -1;
    struct timeval timeout_tv;

    fd_set fd_read;
    fd_set fd_write;
    fd_set fd_except;
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_except);

    timeout_tv.tv_sec = 1;
    timeout_tv.tv_usec = 0;

    ret = curl_multi_fdset(curl_m, &fd_read, &fd_write, &fd_except, &max_fd);
    if (ret != CURLM_OK) {
        ERROR_LOG("curl multi fdset error[%d]", ret);
        return -1;
    }

    if (max_fd >= 0) {
        int rc = select(max_fd+1, &fd_read, &fd_write, &fd_except, &timeout_tv);
        switch (rc) {
        case -1:
            return -1;
        case 0:
        default:
            curl_multi_perform(curl_m, &request_nums);       
            break;
        }
    } else {
        //important
        curl_multi_perform(curl_m, &request_nums);       
    }

    return 0;
}

int HttpRequest::request_done()
{
    struct CURLMsg *m;
    int msg_left;

    m = curl_multi_info_read(curl_m, &msg_left);
    if (m && m->msg == CURLMSG_DONE) {
        CURL *curl = m->easy_handle;
        RequestInfo *p_request = NULL;
        curl_easy_getinfo(curl, CURLINFO_PRIVATE, &p_request);
        curl_multi_remove_handle(curl_m, curl);
        if (p_request) {
            //DEBUG_LOG("REQUEST FINISH[%s]", p_request->buff()->buffer);
            delete p_request;
        }
    }

    return msg_left;
}

int HttpRequest::pop_request_list()
{
    while(request_list.size() > 0) {
        request_t &info = request_list.front();
        if (info.type == em_get_request) {
            add_get_request(info.url);
        } else if (info.type == em_post_request) {
            add_post_request(info.url, info.content);
        }
        request_list.pop_front();
    }

    return 0;
}

void HttpRequest::request_deal_loop()
{
    pop_request_list();
    //while (get_request_nums)
        request_select();
    request_done();
    return;
}

void HttpRequest::DoTest(uint32_t uid)
{
    const char *get_url = "http://121.41.52.86:8080/index.php?userid=1001&&nick=%mark%";
    for (int i = 0; i < 100; i++)
        add_request(uid, em_get_request, get_url, NULL);

    const char *post_url = "http://121.41.52.86:8080/index.php";
    const char *json_str = "{\"userid\":100001, \"nick\":\"markmao\"}";
    for (int i = 0; i < 100; i++)
        add_request(uid, em_post_request, post_url, json_str);
    return;
}

void *deal_request_thread(void *arg)
{
	pthread_detach(pthread_self());
    while(1) {
        http_request->request_deal_loop();
        usleep(1000);
    }
    pthread_exit(0);

    return NULL;
}

