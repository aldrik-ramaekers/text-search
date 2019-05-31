#ifndef INCLUDE_CAMERA
#define INCLUDE_CAMERA

typedef struct t_camera
{
	float32 x;
	float32 y;
	float32 rotation;
} camera;

void camera_apply_transformations(platform_window *window, camera *camera);

#endif