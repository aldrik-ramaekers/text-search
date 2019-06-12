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
void settings_config_write_to_file(settings_config *config, char *path);
void settings_config_destroy(settings_config *config);

config_setting* settings_config_get_setting(settings_config *config, char *name);
char* settings_config_get_string(settings_config *config, char *name);
s64 settings_config_get_number(settings_config *config, char *name);

void settings_config_set_string(settings_config *config, char *name, char *value);
void settings_config_set_number(settings_config *config, char *name, s64 value);



#endif