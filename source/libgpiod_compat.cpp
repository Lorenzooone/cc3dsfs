#include "libgpiod_compat.h"

#if defined(RASPI) && defined(LIBGPIOD3)
#include <sys/stat.h>
#include <dirent.h>
#include <dirent.h>
#include <string>

// Apparently libgpiod3 removed some useful functions...
// Not only that, but there is no migration guide. :/
// Need to do this horrible thing to fix that. :/

std::string prefix_path = "/dev/";

static std::string entry_to_dev_path(const struct dirent *entry) {
	return prefix_path + std::string(entry->d_name);
}

static int chip_dir_filter(const struct dirent *entry)
{
	if(entry == NULL)
		return 0;

	struct stat sb;
	std::string path = entry_to_dev_path(entry);

	if((lstat(path.c_str(), &sb) == 0) && (!S_ISLNK(sb.st_mode)) && gpiod_is_gpiochip_device(path.c_str()))
		return 1;

	return 0;
}

static int all_chip_paths(std::string **paths_ptr)
{
	struct dirent **entries;
	*paths_ptr = NULL;

	int num_chips = scandir(prefix_path.c_str(), &entries, chip_dir_filter, versionsort);
	if(num_chips < 0)
		return 0;

	if(num_chips == 0) {
		free(entries);
		return 0;
	}

	std::string *paths = new std::string[num_chips];

	for(int i = 0; i < num_chips; i++)
		paths[i] = entry_to_dev_path(entries[i]);

	*paths_ptr = paths;

	for(int i = 0; i < num_chips; i++)
		free(entries[i]);
	free(entries);

	return num_chips;
}

struct gpiod_line* gpiod_line_find(const char *line_name)
{
	struct gpiod_chip *chip;
	std::string *chip_paths;

	int num_chips = all_chip_paths(&chip_paths);
	for(int i = 0; i < num_chips; i++) {
		chip = gpiod_chip_open(chip_paths[i].c_str());
		if(!chip)
			continue;

		int offset = gpiod_chip_get_line_offset_from_name(chip, line_name);
		if(offset != -1) {
			struct gpiod_line* output = new gpiod_line;
			output->chip = chip;
			output->offset = offset;
			delete []chip_paths;
			return output;
		}

		gpiod_chip_close(chip);
	}
	if(num_chips >= 0)
		delete []chip_paths;

	return NULL;
}

static void return_from_gpiod_line_request_input_flags(struct gpiod_request_config* req_cfg, struct gpiod_line_config* line_cfg, struct gpiod_line_settings* settings) {
	if(req_cfg)
		gpiod_request_config_free(req_cfg);

	if(line_cfg)
		gpiod_line_config_free(line_cfg);

	if(settings)
		gpiod_line_settings_free(settings);
}

void gpiod_line_request_input_flags(struct gpiod_line* in, const char* consumer, gpiod_line_bias bias) {
	if(in == NULL)
		return;
	if(in->chip == NULL)
		return;

	struct gpiod_request_config* req_cfg = NULL;
	struct gpiod_line_config* line_cfg = NULL;

	struct gpiod_line_settings* settings = gpiod_line_settings_new();
	if(settings == NULL)
		return return_from_gpiod_line_request_input_flags(req_cfg, line_cfg, settings);

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_bias(settings, bias);

	line_cfg = gpiod_line_config_new();
	if(line_cfg == NULL)
		return return_from_gpiod_line_request_input_flags(req_cfg, line_cfg, settings);

	if(gpiod_line_config_add_line_settings(line_cfg, (const unsigned int*)&in->offset, 1, settings) != 0)
		return return_from_gpiod_line_request_input_flags(req_cfg, line_cfg, settings);

	if(consumer != NULL) {
		req_cfg = gpiod_request_config_new();
		if(req_cfg == NULL)
			return return_from_gpiod_line_request_input_flags(req_cfg, line_cfg, settings);

		gpiod_request_config_set_consumer(req_cfg, consumer);
	}

	in->request = gpiod_chip_request_lines(in->chip, req_cfg, line_cfg);

	return return_from_gpiod_line_request_input_flags(req_cfg, line_cfg, settings);
}

int gpiod_line_get_value(struct gpiod_line* in) {
	return gpiod_line_request_get_value(in->request, in->offset);
}

void gpiod_line_close_chip(struct gpiod_line* in)
{
	if(in == NULL)
		return;
	if(in->request != NULL)
		gpiod_line_request_release(in->request);
	in->request = NULL;
	if(in->chip != NULL)
		gpiod_chip_close(in->chip);
	in->chip = NULL;
}

void gpiod_line_release(struct gpiod_line* in)
{
	if(in == NULL)
		return;
	gpiod_line_close_chip(in);
	delete in;
}
#endif
