#include <string.h>
#include <alsa/asoundlib.h>

static array load_audio_devices()
{
	array devices = array_create(sizeof(audio_device));
	
	s32 card_num = -1;
	s32 err;
	
	while (1)
	{
		snd_ctl_t *card_handle;
		if ((err = snd_card_next(&card_num)) < 0)
		{
			printf("Can't get the next card number: %s\n", snd_strerror(err));
			break;
		}
		
		// no more cards left.
		if (card_num < 0) break;
		
		// open device
		{
			char str[64];
			sprintf(str, "hw:%i", card_num);
			if ((err = snd_ctl_open(&card_handle, str, 0)) < 0)
			{
				printf("Can't open card %i: %s\n", card_num, snd_strerror(err));
				continue;
			}
		}
		
		// query info
		{
			snd_ctl_card_info_t *card_info;
			snd_ctl_card_info_alloca(&card_info);
			
			if ((err = snd_ctl_card_info(card_handle, card_info)) < 0)
			{
				printf("Can't get info for card %i: %s\n", card_num, snd_strerror(err));
			}
			else
			{
				char *name = mem_alloc(80); // TODO(Aldrik): can device names be longer?
				bool is_output = false;
				
				strcpy(name, snd_ctl_card_info_get_name(card_info));
				
				char **hints;
				int err = snd_device_name_hint(card_num, "pcm", (void***)&hints);
				if (err != 0)
					return devices;
				
				char** n = hints;
				while (*n != NULL) {
					
					char *type = snd_device_name_get_hint(*n, "IOID");
					
					if (type != NULL && 0 == strcmp("Output", type)) {
						is_output = true;
					}
					mem_free(type);
					n++;
				}
				
				audio_device new_device;
				new_device.is_output = is_output;
				new_device.card_num = card_num;
				new_device.name = name;
				array_push(&devices, &new_device);
				
				snd_device_name_free_hint((void**)hints);
			}
		}
		
		snd_ctl_close(card_handle);
	}
	
	return devices;
}

int audio_system_create()
{
	global_audio_system.audio_devices = load_audio_devices();
	
#if 0
	for (int i = 0; i < global_audio_system.audio_devices.length; i++)
	{
		audio_device *device = array_at(&global_audio_system.audio_devices, i);
		printf("device: %s output=%d\n", device->name, device->is_output);
	}
#endif
	
	return 0;
}

void audio_system_destroy()
{
	for (int i = 0; i < global_audio_system.audio_devices.length; i++)
	{
		audio_device *device = array_at(&global_audio_system.audio_devices, i);
		free(device->name);
	}
	
	array_destroy(&global_audio_system.audio_devices);
}
