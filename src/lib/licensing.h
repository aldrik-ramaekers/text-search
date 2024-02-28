/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_LICENSING
#define INCLUDE_LICENSING

typedef enum t_license_status
{
	LICENSE_STATUS_VALID = 1,
	LICENSE_STATUS_INVALID = 2,
	LICENSE_STATUS_TOO_MANY_USERS = 3,
} license_status;

// NOTE DO NOT TOUCH THIS!
char license_key[18] = { "[LICENSELOCATION]" };
// NOTE DO NOT TOUCH THIS!

license_status global_license_status = LICENSE_STATUS_VALID;
void validate_license();
bool license_check_status();

#endif