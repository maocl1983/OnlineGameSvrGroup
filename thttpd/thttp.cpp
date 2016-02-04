/**
 *============================================================
 *
 * @file  thttp.cpp
 *
 * @brief  tiny thttp服务器
 * compiler   gcc4.1.2
 * 
 * platform   Linux
 *
 *============================================================
 */
#include <string>

extern "C" {
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <errno.h>
#include <libcommon/log.h>
#include <libcommon/conf_parser/config.h>
#include <libcommon/time/timer.h>
}
#include <asyn_serv/net_if.hpp>

#include <libproject/utils/strings.hpp> //bin2hex
#if 0
#include <json/json.h> 
#else
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#endif

const int min_len_req_line = 0;
const int max_line_size = 2048 * 2;
char req_line[max_line_size]; //buffer to hold request content
const char* notfound_str = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
size_t notfound_len = 0;

const char* http_ok_header1 = "HTTP/1.1 200 OK\r\nContent-Length: ";
const char* http_ok_header2 = "\r\nConnection: close\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";

static int get_url_field(char* content, const char* field_name, char* filed_value, int max_value_len)
{
	if (!content || !field_name || !filed_value) {
		return -1;
	}

	char* filed_pos = strstr(content, field_name);
	if (!filed_pos) {
		ERROR_LOG("cannot get url field:[%s]", field_name);
		return -1;
	}

	char* start_pos = strchr(filed_pos, '=');
	if (!start_pos) {
		return -1;
	}

	int i = 0;
	int j = 0;
	start_pos += 1;
	while (1) {
		if (start_pos[i] == '&') {
			filed_value[j] = '\0';
			break;
		}
		if (start_pos[i] == '%') {
			i++;
			continue;
		}
		if (j >= max_value_len)
			break;
		filed_value[j] = start_pos[i];
		if (filed_value[j] == '\0')
			break;
		i++;
		j++;
	}
	return 0;
}
/**
  * @brief Initialize service
  *
  */
extern "C" int init_service(int isparent)
{
	/*if (isparent) {
		return 0;
	}*/

	DEBUG_LOG("INIT SERVICE");
	notfound_len = strlen(notfound_str);

	return 0;
}

/**
  * @brief Finalize service
  *
  */
extern "C" int fini_service(int isparent)
{
	/*if (isparent) {
		return 0;
	}*/

	DEBUG_LOG("FINI SERVICE");
	return 0;
}

/**
  * @brief Process events such as timers and signals
  *
  */
extern "C" void proc_events()
{
}

/**
  * @brief Return length of the receiving package
  *
  */
extern "C" int get_pkg_len(int fd, const void *avail_data, int avail_len, int isparent)
{
	/*if (!isparent) {
		return 0;
	}*/

	if (avail_len >= max_line_size) {
		ERROR_LOG("pkg len too large: len=%d fd=%d ip=%s data=%s",
					avail_len, fd, get_server_ip(),
					static_cast<const char*>(avail_data));
		return -1;
	} else if (avail_len <= min_len_req_line) {
		return 0;
	} 
	
	DEBUG_LOG("get pkg len=%d fd=%d", avail_len, fd);
	return avail_len;
}

/**
  * @brief Process packages from clients
  *
  */
extern "C" int proc_pkg_from_client(void* data, int len, fdsession_t* fdsess)
{
	//int sockfd = fdsess->fd;
	//int pkg_len = 0;
	memcpy(req_line, data, len);
	req_line[len] = '\0';

	//get method: "GET" or "POST"
	char method[64];
	char* first_blank = strstr(req_line, " ");
	if (!first_blank) {
		ERROR_LOG("Invalid pkg whithout blank: %s", req_line);
		return -1;
	}
	int method_len = first_blank - req_line;
	if (method_len >= 64) {
		ERROR_LOG("Invalid pkg method len error: %d", method_len);
		return -1;
	}
	memcpy(method, req_line, method_len);
	method[method_len] = '\0';
	DEBUG_LOG("req method=[%s]", method);

	//get content: such as "/index.php?userid=1001"
	char content[256];
	if (!strcasecmp(method, "GET") || !strcasecmp(method, "POST")) {
		char* second_blank = strstr(first_blank+1, " ");
		if (!second_blank) {
			ERROR_LOG("Invalid pkg whithout second blank: %s", req_line);
			return -1;
		}
		int content_len = second_blank - (first_blank + 1);
		if (content_len >= 256) {
			ERROR_LOG("Invalid pkg get content len error: %d", content_len );
			return -1;
		}
		memcpy(content, first_blank + 1, content_len);
		content[content_len] = '\0';
		DEBUG_LOG("req content=[%s]", content);
	} else {
		ERROR_LOG("Invalid pkg no method: %s", method);
		return -1;
	}

	//process
	if (!strcasecmp(method, "GET")) {
		//test
		char userid_value[32];
		get_url_field(content, "userid", userid_value, 32);
		char nick_value[64];
		get_url_field(content, "nick", nick_value, 64);
		DEBUG_LOG("req get filed [%s %s]", userid_value, nick_value);
		
		char respond_str[2048];
		sprintf(respond_str, "HTTP/1.1 200 OK\r\nContent-Length: %ld", strlen(nick_value));
		strcat(respond_str, http_ok_header2);
		strncat(respond_str, nick_value, strlen(nick_value)+1);
		return send_pkg_to_client(fdsess, respond_str, strlen(respond_str));  		
	} else if (!strcasecmp(method, "POST")) {
		//content length
		int content_length = 0;
		char* length_ptr = strstr(req_line, "Content-Length:");
		if (length_ptr)
			content_length = atoi(length_ptr + 15);
		DEBUG_LOG("req post content length=%d", content_length);

		//url data : such as json data
		char data[1024];
		char* data_ptr = strstr(req_line, "\r\n\r\n");
		if (!data_ptr) {
			return -1;
		}
		int head_length = data_ptr + 4 - req_line;
		int data_length = len - head_length;
		if (data_length != content_length && data_length >= 1024) {
			ERROR_LOG("Invalid pkg post content len error: datalen=%d contentlen=%d", data_length, content_length);
			return -1;
		}

		memcpy(data, data_ptr+4, data_length);
		data[data_length] = '\0';
		DEBUG_LOG("post data len=%d str=[%s]", data_length, data);

#if 0
		//json parse
		Json::Reader Parser;
		Json::Value root;
		int userid = 0;
		std::string nick;
		if (Parser.parse(data, data+data_length, root, false)) {
			if (root["userid"].isInt()) {
				userid = root["userid"].asInt();
			}
			if (root["nick"].isString()) {
				nick = root["nick"].asString();
			}
		}
		DEBUG_LOG("json data userid=%d nick=%s", userid, nick.c_str());

		//json respond
		Json::Value outRoot;
		outRoot["userid"] = userid+1;
		outRoot["nick"] = nick;
		std::string outStr = outRoot.toStyledString();
		char respond_str[2048];
		sprintf(respond_str, "HTTP/1.1 200 OK\r\nContent-Length: %ld", strlen(outStr.c_str()));
		strcat(respond_str, http_ok_header2);
		strncat(respond_str, outStr.c_str(), strlen(outStr.c_str())+1);
        return send_pkg_to_client(fdsess, respond_str, strlen(respond_str));  		
#else
        //rapidjson parse
        rapidjson::Document document;
        if (document.ParseInsitu(data).HasParseError()) {
            ERROR_LOG("docment parse json error!");
            return -1;
        }
        int userid = 0;
        std::string nick;
        if (document["userid"].IsInt()) userid = document["userid"].GetInt();
        if (document["nick"].IsString()) nick = document["nick"].GetString();
		DEBUG_LOG("rapidjson data userid=%d nick=%s", userid, nick.c_str());
        rapidjson::Value &items = document["iteminfo"];
        if (items.IsArray()) {
            for (rapidjson::SizeType i = 0; i < items.Size(); i++) {
                int id = 0, cnt = 0;
                rapidjson::Value &item = items[i];
                if (item.IsObject()) {
                    if (item["itemid"].IsInt()) id = item["itemid"].GetInt();
                    if (item["itemcnt"].IsInt()) cnt = item["itemcnt"].GetInt();
                }
		        DEBUG_LOG("rapidjson item id=%d cnt=%d", id, cnt);
            }
        }

        //rapidjson repond
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        writer.String("userid");
        writer.Uint(userid+100);
        writer.String("nick");
        writer.String(nick.c_str());
        writer.String("iteminfo");
        writer.StartArray();
        for (uint i = 0; i < 2; i++) {
            writer.StartObject();
            writer.String("itemid");
            writer.Uint(10001+i);
            writer.String("itemcnt");
            writer.Uint(10+i);
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();
		char respond_str[2048];
		sprintf(respond_str, "HTTP/1.1 200 OK\r\nContent-Length: %ld", sb.GetSize());
		strcat(respond_str, http_ok_header2);
		strncat(respond_str, sb.GetString(), sb.GetSize());
        return send_pkg_to_client(fdsess, respond_str, strlen(respond_str));  		
#endif
	}
			
	send_pkg_to_client(fdsess, notfound_str, notfound_len);
	return 0;
}

/**
  * @brief Process packages from servers
  *
  */
extern "C" void proc_pkg_from_serv(int fd, void* data, int len)
{
}

/**
  * @brief Called each time on client connection closed
  *
  */
extern "C" void on_client_conn_closed(int fd)
{
}

/**
  * @brief Called each time on close of the fds created by the child process
  *
  */
extern "C" void on_fd_closed(int fd)
{
}

/**
  * @brief Called to process mcast package from the address and port configured in the config file
  */
extern "C" void proc_mcast_pkg(const void* data, int len)
{
}

/**
  * @brief Called to process udp package from the address and port configured in the config file
  */
extern "C" void proc_udp_pkg(int fd, const void* avail_data, int avail_len, struct sockaddr_in *from, socklen_t fromlen)
{
}




