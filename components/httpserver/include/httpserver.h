#ifndef __HTTPD_TESTS_H__
#define __HTTPD_TESTS_H__
#include "cJSON.h"
#include "esp_event_loop.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <esp_log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
extern httpd_handle_t start_tests(void);
extern void stop_tests(httpd_handle_t hd);

#endif // __HTTPD_TESTS_H__
