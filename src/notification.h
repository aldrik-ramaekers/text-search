/* 
*  Copyright 2019 Aldrik Ramaekers
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.

*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.

*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef INCLUDE_NOTIFICATION
#define INCLUDE_NOTIFICATION

typedef struct t_notification_manager
{
	u64 current_notification_display_stamp;
	array messages;
} notification_manager;

notification_manager global_notification_manager;

void notification_manager_init();
void show_notification(char message[MAX_INPUT_LENGTH]);
void update_render_notifications();
void notification_manager_destroy();

#endif