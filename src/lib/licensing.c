/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#include "external/cJSON.h"

static void* validate_license_thread(void *arg)
{
	char *mac_address = arg;
	
	char params[80];
	sprintf(params, "can_run?ti=%s&addr=%s", license_key, mac_address);
	
	char response[MAX_INPUT_LENGTH];
	while (main_window->is_open)
	{
		// send activity ping
		if (platform_send_http_request("api.aldrik.org", params, response))
		{
			cJSON *result = cJSON_Parse(response);
			if (!result) return false;
			cJSON *response = cJSON_GetObjectItem(result, "response");
			
			if (response)
				global_license_status = response->valueint;
		}
		
		thread_sleep(5*1000*1000); // 5s
	}
	
	mem_free(mac_address);
	return 0;
}

void validate_license()
{
	char *mac_address_buffer = mem_alloc(30);
	platform_get_mac_address(mac_address_buffer, 30);
	
#ifdef MODE_DEVELOPER
	return;
#endif
	
	thread license_thread = thread_start(validate_license_thread, mac_address_buffer);
	thread_detach(&license_thread);
}

bool license_check_status()
{
	if (global_license_status == LICENSE_STATUS_VALID)
	{
		return true;
	}
	else if (global_license_status == LICENSE_STATUS_INVALID)
	{
		char message[200];
		sprintf(message, localize("invalid_license"), license_key);
		platform_show_message(main_window, message, localize("license_error"));
		main_window->is_open = false;
		return false;
	}
	else if (global_license_status == LICENSE_STATUS_TOO_MANY_USERS)
	{
		char message[200];
		sprintf(message, localize("too_many_users_using_license"), license_key);
		platform_show_message(main_window, message, localize("license_error"));
		main_window->is_open = false;
		return false;
	}
	
	return true;
}