/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
*/

#ifndef INCLUDE_NOTIFICATION
#define INCLUDE_NOTIFICATION

typedef struct t_notification
{
	char *message;
	u16 duration;
} notification;

array global_notifications;

void push_notification(char *message);
void update_render_notifications();

#endif
