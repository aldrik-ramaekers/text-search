
settings_config settings_config_load_from_file(char *path)
{
	settings_config config;
	config.settings = array_create(sizeof(config_setting));
	
	
	
	return config;
}

void settings_config_destroy(settings_config *config)
{
	array_destroy(&config->settings);
}