/* 
*  BSD 2-Clause “Simplified” License
*  Copyright (c) 2019, Aldrik Ramaekers, aldrik.ramaekers@protonmail.com
*  All rights reserved.
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