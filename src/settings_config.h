#ifndef INCLUDE_SETTINGS_CONFIG
#define INCLUDE_SETTINGS_CONFIG

typedef struct t_config_setting
{
	char *name;
	char *value;
} config_setting;

typedef struct t_settings_config
{
	array settings;
} settings_config;

/* Example of file:
*  NAME = "Aldrik Ramaekers"
*  AGE = "69"
*  NUMBER = "15"
*/

settings_config settings_config_load_from_file(char *path);
void settings_config_destroy(settings_config *config);

#endif