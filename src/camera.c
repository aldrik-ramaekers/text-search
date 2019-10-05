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

void camera_apply_transformations(platform_window *window, camera *camera)
{
	s32 x = (window->width/2)+(camera->x);
	s32 y =  (window->height/2)+(camera->y);
	glTranslatef(x, y, 0.0f);
	glRotatef(camera->rotation, 0.0f, 0.0f, 1.0f);
	glTranslatef(-x, -y, 0.0f);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    
    glOrtho(camera->x, window->width+camera->x, 
            window->height+camera->y, camera->y, -100, 100);
    
	glMatrixMode(GL_MODELVIEW);
}