/*
 * Registry File Generator
 *
 * Copyright (C) 2023 Richard Acayan
 *
 * sscregistrygen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Typical usage with HexagonFS default physical filesystem structure and
 * mainline Linux:
 *
 * registry -p OEM -s $(< /sys/devices/soc0/soc_id) \
 *	    /etc/qcom/sensors.d/ /var/lib/qcom/sensors/registry/
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <json.h>

// Not implemented in GNU C library
#ifndef O_SEARCH
#define O_SEARCH O_RDONLY
#endif

struct config_filter {
	const char *hw_platform;
	// This is represented by a string, so don't bother converting it
	const char *soc_id;
};

void create_registry_file(int dirfd, const char *name, struct json_object *obj);

/*
 * This returns the default reference object. It interacts with static memory
 * and is therefore not thread safe.
 */
struct json_object *get_default_ref()
{
	static struct json_object *ref = NULL;
	struct json_object *type, *ver, *data;

	if (ref == NULL) {
		type = json_object_new_string("grp");
		ver = json_object_new_string("0");
		data = json_object_new_string("");

		ref = json_object_new_object();
		json_object_object_add(ref, "type", type);
		json_object_object_add(ref, "ver", ver);
		json_object_object_add(ref, "data", data);
	}

	/*
	 * Static storage will always hold a reference, unaffected by the
	 * calling functions. That unfortunately means that we cannot free
	 * everything on exit.
	 */
	json_object_get(ref);

	return ref;
}

void add_child(int dirfd,
	       const char *parent_name, const char *name,
	       struct json_object *parent, struct json_object *obj)
{
	struct json_object *ref;
	int n_parent_name, n_name;
	char *filename;

	if (name[0] == '.') {
		n_parent_name = strlen(parent_name);
		n_name = strlen(name);

		filename = malloc(n_parent_name + n_name + 1);
		if (filename == NULL)
			return;

		memcpy(filename, parent_name, n_parent_name);
		memcpy(&filename[n_parent_name], name, n_name);
		filename[n_parent_name + n_name] = '\0';

		create_registry_file(dirfd, filename, obj);

		free(filename);

		ref = get_default_ref();

		// Held reference transferred
		json_object_object_add(parent, &name[1], ref);
	} else {
		// Hold reference for parent
		json_object_get(obj);
		json_object_object_add(parent, name, obj);
	}
}

/*
 * Creates a registry file, and registry files that are nested in the given
 * JSON object.
 */
void create_registry_file(int dirfd, const char *name, struct json_object *obj)
{
	struct json_object *out, *root;
	struct json_object_iterator iter, end;
	const char *child;
	struct json_object *val;
	const char *contents;
	int fd;

	root = json_object_new_object();

	out = json_object_new_object();
	json_object_object_add(root, name, out);

	iter = json_object_iter_begin(obj);
	end = json_object_iter_end(obj);

	while (!json_object_iter_equal(&iter, &end)) {
		child = json_object_iter_peek_name(&iter);
		val = json_object_iter_peek_value(&iter);

		add_child(dirfd, name, child, out, val);

		json_object_iter_next(&iter);
	}

	contents = json_object_to_json_string_ext(root,
						  JSON_C_TO_STRING_PLAIN
						| JSON_C_TO_STRING_NOSLASHESCAPE);

	fd = openat(dirfd, name, O_WRONLY | O_CREAT,
		    (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (fd == -1)
		return;

	write(fd, contents, strlen(contents));

	close(fd);

	json_object_put(root);
}

bool check_config_filter(struct config_filter *filter, struct json_object *config)
{
	struct json_object *hw_plat, *soc_id;
	struct json_object *curr;
	const char *str;
	size_t len, i;

	hw_plat = json_object_object_get(config, "hw_platform");
	if (filter->hw_platform != NULL
	 && hw_plat != NULL
	 && json_object_get_type(hw_plat) == json_type_array) {
		len = json_object_array_length(hw_plat);

		for (i = 0; i < len; i++) {
			curr = json_object_array_get_idx(hw_plat, i);
			str = json_object_get_string(curr);
			if (!strcmp(str, filter->hw_platform))
				break;
		}

		if (i >= len)
			return false;
	}

	soc_id = json_object_object_get(config, "soc_id");
	if (filter->soc_id != NULL
	 && soc_id != NULL
	 && json_object_get_type(soc_id) == json_type_array) {
		len = json_object_array_length(soc_id);

		for (i = 0; i < len; i++) {
			curr = json_object_array_get_idx(soc_id, i);
			str = json_object_get_string(curr);
			if (!strcmp(str, filter->soc_id))
				break;
		}

		if (i >= len)
			return false;
	}

	return true;
}

void apply_sensor_configuration(struct config_filter *filter,
				int indir, int outdir, const char *file)
{
	struct json_object *in;
	struct json_object *config;
	const char *name;
	struct json_object *val;
	struct json_object_iterator iter, end;
	int fd;

	fd = openat(indir, file, O_RDONLY);
	if (fd == -1)
		return;

	in = json_object_from_fd(fd);

	close(fd);

	config = json_object_object_get(in, "config");
	if (config != NULL) {
		if (!check_config_filter(filter, config))
			return;
	}

	iter = json_object_iter_begin(in);
	end = json_object_iter_end(in);

	while (!json_object_iter_equal(&iter, &end)) {
		name = json_object_iter_peek_name(&iter);
		val = json_object_iter_peek_value(&iter);

		if (strcmp(name, "config"))
			create_registry_file(outdir, name, val);

		json_object_iter_next(&iter);
	}
}

int main(int argc, char **argv)
{
	struct dirent *dirent;
	DIR *indir;
	int indirfd, outdirfd;
	struct config_filter filter;
	int opt;

	filter.hw_platform = NULL;
	filter.soc_id = NULL;

	while ((opt = getopt(argc, argv, ":p:s:")) != -1) {
		switch (opt) {
		case 'p':
			filter.hw_platform = strdup(optarg);
			break;
		case 's':
			filter.soc_id = strdup(optarg);
			break;
		case ':':
			fprintf(stderr, "Option -%c requires an argument\n", optopt);
			return 1;
		case '?':
		default:
			fprintf(stderr, "Unknown option: -%c\n", optopt);
			return 1;
		};
	}

	if (argc < optind + 2) {
		printf("Please specify the input and output directories.\n");
		return 1;
	}

	indir = opendir(argv[optind]);
	if (indir == NULL) {
		fprintf(stderr, "Could not open %s: %s\n",
				argv[optind], strerror(-errno));
		return 1;
	}

	indirfd = open(argv[optind], O_SEARCH | O_DIRECTORY);
	if (indirfd == -1) {
		fprintf(stderr, "Could not open %s: %s\n",
				argv[optind], strerror(-errno));
		return 1;
	}

	outdirfd = open(argv[optind + 1], O_SEARCH | O_DIRECTORY);
	if (outdirfd == -1) {
		fprintf(stderr, "Could not open %s: %s\n",
				argv[optind + 1], strerror(-errno));
		return 1;
	}

	dirent = readdir(indir);

	while (dirent != NULL) {
		if (!strcmp(dirent->d_name, ".")
		 || !strcmp(dirent->d_name, ".."))
			goto next;

		apply_sensor_configuration(&filter, indirfd, outdirfd, dirent->d_name);

	next:
		dirent = readdir(indir);
	}

	return 0;
}
