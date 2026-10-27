#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define MUSIC_CATALOG_MAX_TRACKS 256
#define MUSIC_CATALOG_NAME_MAX 255

typedef struct {
    char name[MUSIC_CATALOG_NAME_MAX + 1];
} music_track_t;

typedef struct {
    music_track_t tracks[MUSIC_CATALOG_MAX_TRACKS];
    size_t count;
    int directory_found;
} music_catalog_t;

/* Scans <mount_point>/MUSIC. The caller owns catalog and needs no cleanup. */
esp_err_t music_catalog_scan(const char *mount_point, music_catalog_t *catalog);
