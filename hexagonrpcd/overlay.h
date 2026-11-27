/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef OVERLAY_H
#define OVERLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 * Writable overlay for the HexagonFS.
 *
 * The DSP-visible tree served through -R is read-only, but the sensors PD
 * opens its registry for writing.  The overlay is a real directory (given by
 * -O) that mirrors the DSP-visible paths: the first write to a file copies
 * the -R file into the overlay, and from then on the overlay version is what
 * the DSP sees.
 */
int overlay_init(const char *dir);
bool overlay_enabled(void);

/* Map a DSP-visible path to the path below the overlay directory. */
int overlay_phys_path(const char *vpath, char *buf, size_t size);

int overlay_open(const char *vpath, int flags, mode_t mode);
bool overlay_exists(const char *vpath);
int overlay_stat(const char *vpath, struct stat *st);

int overlay_remove(const char *vpath);
int overlay_rename(const char *from, const char *to);

#endif /* OVERLAY_H */
