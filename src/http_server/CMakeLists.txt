SET(app_sources http_server.c http_utils.c http_conn.c http_request.c http_response.c routes.c rest_server.c web_server.c prometheus_client.c files_server.c)
target_sources(app PRIVATE ${app_sources})

zephyr_sources_ifdef(CONFIG_HTTP_TEST_SERVER test_server.c)

# zephyr_sources(
#     http_server.c
#     http_utils.c
#     http_conn.c
#     http_request.c
#     http_response.c
#     routes.c
#     rest_server.c
#     web_server.c
#     prometheus_client.c
# )

# zephyr_sources_ifdef(CONFIG_HTTP_TEST_SERVER test_server.c)