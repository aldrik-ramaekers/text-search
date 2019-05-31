#ifndef INCLUDE_AUDIO
#define INCLUDE_AUDIO

typedef struct t_audio_device
{
	char *name;
	u16 card_num;
	bool is_output;
} audio_device;

typedef struct t_audio_system
{
	array audio_devices;
} audio_system;

audio_system global_audio_system;

int audio_system_create();
void audio_system_destroy();

#endif