#include "web_server.h"

#include "utils.h"

#include <stdio.h>

#define HTML_BEGIN_TO_TITLE "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>"
#define HTML_TITLE_TO_BODY "</title></head><body>"
#define HTML_BODY_TO_END "</body></html>"

#define HTML_TAG(name) "<" name ">"
#define HTML_END_TAG(name) "</" name ">"

#define HTML_URL(url, name) "<a href=\"" url "\">" name "</a>"
#define HTML_QUICK_URL(url) HTML_URL(url, url)
#define HTML_LI(content) "<li>" content "</li>"

int web_server_index_html(struct http_request *req,
			  struct http_response *resp)
{
	buffer_append_string(&resp->buffer, HTML_BEGIN_TO_TITLE
			     "stm32f429zi index.html" HTML_TITLE_TO_BODY);

	buffer_append_string(&resp->buffer, "<fieldset><legend>URL list</legend><ul>");

	buffer_append_string(&resp->buffer, HTML_LI(HTML_QUICK_URL("/index.html")));
	buffer_append_string(&resp->buffer, HTML_LI(HTML_QUICK_URL("/info")));
	buffer_append_string(&resp->buffer, HTML_LI(HTML_QUICK_URL("/devices")));
	buffer_append_string(&resp->buffer, HTML_LI(HTML_QUICK_URL("/devices/caniot")));
	buffer_append_string(&resp->buffer, HTML_LI(HTML_QUICK_URL("/devices/xiaomi")));
	buffer_append_string(&resp->buffer, HTML_LI(HTML_QUICK_URL("/metrics")));

	/* TODO generate this dynamically using routes map */

	buffer_append_string(&resp->buffer, "</ul></fieldset>" HTML_BODY_TO_END);

	return 0;
}