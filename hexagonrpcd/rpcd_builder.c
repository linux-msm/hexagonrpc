/*
 * FastRPC virtual filesystem builder
 *
 * Copyright (C) 2023 The Sensor Shell Contributors
 *
 * This file is part of sensh.
 *
 * Sensh is free software: you can redistribute it and/or modify
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "hexagonfs.h"

// These paths are relative, with a prepended '/' if missing from previous segments
#define ACDBDATA		"/acdb/"
#define DSP_LIBS		"/dsp/"
#define SENSORS_CONFIG		"/sensors/config/"
#define SENSORS_REGISTRY	"/sensors/registry/"
#define SNS_REG_VERSION		"/sensors/sns_reg_version"
#define SNS_REG_CONFIG		"/sensors/sns_reg.conf"
#define SYSFS_SOCINFO		"/socinfo/"

static struct hexagonfs_dirent *hfs_mkdir(const char *name, size_t n_ents, ...)
{
	struct hexagonfs_dirent *dir;
	struct hexagonfs_dirent **list;
	struct hexagonfs_dirent *ent;
	va_list va;
	size_t i;

	list = malloc(sizeof(struct hexagonfs_dirent *) * (n_ents + 1));
	if (list == NULL)
		return NULL;

	dir = malloc(sizeof(struct hexagonfs_dirent));
	if (dir == NULL)
		goto err_free_list;

	va_start(va, n_ents);
	for (i = 0; i < n_ents; i++) {
		ent = va_arg(va, struct hexagonfs_dirent *);
		if (ent == NULL)
			goto err_free_dir;

		list[i] = ent;
	}
	va_end(va);

	list[n_ents] = NULL;

	dir->name = name;
	dir->ops = &hexagonfs_virt_dir_ops;
	dir->u.dir = list;

	return dir;

err_free_dir:
	free(dir);
err_free_list:
	free(list);
	return NULL;
}

static struct hexagonfs_dirent *hfs_map(const char *name, const char *path)
{
	struct hexagonfs_dirent *file;

	file = malloc(sizeof(struct hexagonfs_dirent));
	if (file == NULL)
		return NULL;

	file->name = name;
	file->ops = &hexagonfs_mapped_ops;
	file->u.phys = path;

	return file;
}

static struct hexagonfs_dirent *hfs_map_or_empty(const char *name, const char *path)
{
	struct hexagonfs_dirent *file;

	file = malloc(sizeof(struct hexagonfs_dirent));
	if (file == NULL)
		return NULL;

	file->name = name;
	file->ops = &hexagonfs_mapped_or_empty_ops;
	file->u.phys = path;

	return file;
}

/*
 * Construct the root directory
 *
 * TODO: Make this free()-able with reference counts
 */
struct hexagonfs_dirent *construct_root_dir(const char *prefix, const char *dsp)
{
	char *acdbdata, *dsp_libs, *sns_cfg, *sns_reg, *sns_reg_version, *sns_reg_config, *socinfo;
	size_t n_prefix;
	struct hexagonfs_dirent *persist_dir, *vendor_dir;

	n_prefix = strlen(prefix);

	acdbdata = malloc(n_prefix + strlen(ACDBDATA) + 1);
	sns_cfg = malloc(n_prefix + strlen(SENSORS_CONFIG) + 1);
	sns_reg = malloc(n_prefix + strlen(SENSORS_REGISTRY) + 1);
	sns_reg_version = malloc(n_prefix + strlen(SNS_REG_VERSION) + 1);
	sns_reg_config = malloc(n_prefix + strlen(SNS_REG_CONFIG) + 1);
	socinfo = malloc(n_prefix + strlen(SYSFS_SOCINFO) + 1);

	dsp_libs = malloc(n_prefix + strlen(DSP_LIBS) + strlen(dsp) + 1);

	if (acdbdata != NULL) {
		strcpy(acdbdata, prefix);
		strcat(acdbdata, ACDBDATA);
	}

	if (sns_cfg != NULL) {
		strcpy(sns_cfg, prefix);
		strcat(sns_cfg, SENSORS_CONFIG);
	}

	if (sns_reg != NULL) {
		strcpy(sns_reg, prefix);
		strcat(sns_reg, SENSORS_REGISTRY);
	}

	if (sns_reg_version != NULL) {
		strcpy(sns_reg_version, prefix);
		strcat(sns_reg_version, SNS_REG_CONFIG);
	}

	if (sns_reg_config != NULL) {
		strcpy(sns_reg_config, prefix);
		strcat(sns_reg_config, SNS_REG_CONFIG);
	}

	if (socinfo != NULL) {
		strcpy(socinfo, prefix);
		strcat(socinfo, SYSFS_SOCINFO);
	}

	if (dsp_libs != NULL) {
		strcpy(dsp_libs, prefix);
		strcat(dsp_libs, DSP_LIBS);
		strcat(dsp_libs, dsp);
	}

	/*
	 * Some platforms need this in / and some need it in /mnt/vendor. Form
	 * a hard link between both locations.
	 */
	persist_dir = hfs_mkdir("persist", 1,
				hfs_mkdir("sensors", 1,
					hfs_mkdir("registry", 2,
						hfs_map("registry", sns_reg),
						hfs_map("sns_reg_version", sns_reg_version)
					)
				)
		      );

	/*
	 * Some platforms need vendor in / and some need it in /system. Form
	 * a hard link between both locations.
	 */
	vendor_dir = hfs_mkdir("vendor", 1,
				hfs_mkdir("etc", 2,
					hfs_mkdir("sensors", 2,
						hfs_map_or_empty("config", sns_cfg),
						hfs_map("sns_reg_config", sns_reg_config)
					),
					hfs_map("acdbdata", acdbdata)
				)
			);

	return hfs_mkdir("/", 6,
			hfs_mkdir("mnt", 1,
				hfs_mkdir("vendor", 1,
					persist_dir
				)
			),
			persist_dir,
			hfs_mkdir("sys", 1,
				hfs_mkdir("devices", 1,
					hfs_map_or_empty("soc0", socinfo)
				)
			),
			hfs_mkdir("system", 1,
				vendor_dir
			),
			hfs_mkdir("usr", 1,
				hfs_mkdir("lib", 1,
					hfs_mkdir("qcom", 1,
						hfs_map_or_empty("adsp", dsp_libs)
					)
				)
			),
			vendor_dir
		);
}
