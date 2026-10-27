#include "music_catalog.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int ascii_lower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

static int name_compare(const void *left, const void *right)
{
    const music_track_t *a = left;
    const music_track_t *b = right;
    const unsigned char *pa = (const unsigned char *)a->name;
    const unsigned char *pb = (const unsigned char *)b->name;
    while (*pa && *pb) {
        int la = ascii_lower(*pa), lb = ascii_lower(*pb);
        if (la != lb) return la - lb;
        ++pa;
        ++pb;
    }
    if (*pa || *pb) return (int)*pa - (int)*pb;
    return strcmp(a->name, b->name); /* deterministic tie break */
}

static int is_mp3(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && dot != name && strlen(dot) == 4 && ascii_lower((unsigned char)dot[1]) == 'm' &&
           ascii_lower((unsigned char)dot[2]) == 'p' &&
           ascii_lower((unsigned char)dot[3]) == '3' && dot[4] == '\0';
}

esp_err_t music_catalog_scan(const char *mount_point, music_catalog_t *catalog)
{
    if (!mount_point || !catalog || mount_point[0] == '\0') return ESP_ERR_INVALID_ARG;
    memset(catalog, 0, sizeof(*catalog));
    char path[512];
    int written = snprintf(path, sizeof(path), "%s%sMUSIC",
                           mount_point, mount_point[strlen(mount_point) - 1] == '/' ? "" : "/");
    if (written < 0 || (size_t)written >= sizeof(path)) return ESP_ERR_INVALID_SIZE;

    struct stat st;
    if (stat(path, &st) != 0) return errno == ENOENT ? ESP_ERR_NOT_FOUND : ESP_FAIL;
    if (!S_ISDIR(st.st_mode)) return ESP_ERR_INVALID_STATE;
    catalog->directory_found = 1;

    DIR *dir = opendir(path);
    if (!dir) return ESP_FAIL;
    esp_err_t result = ESP_OK;
    errno = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || !is_mp3(entry->d_name)) {
            errno = 0;
            continue;
        }
        size_t len = strlen(entry->d_name);
        if (len > MUSIC_CATALOG_NAME_MAX) { result = ESP_ERR_INVALID_SIZE; break; }
        char file_path[sizeof(path) + MUSIC_CATALOG_NAME_MAX + 2];
        int n = snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);
        if (n < 0 || (size_t)n >= sizeof(file_path)) { result = ESP_ERR_INVALID_SIZE; break; }
        if (stat(file_path, &st) != 0) { result = ESP_FAIL; break; }
        if (!S_ISREG(st.st_mode)) { errno = 0; continue; }
        FILE *probe = fopen(file_path, "rb");
        if (!probe) { result = ESP_FAIL; break; }
        unsigned char first_byte;
        (void)fread(&first_byte, 1, 1, probe);
        int read_failed = ferror(probe);
        if (fclose(probe) != 0 || read_failed) { result = ESP_FAIL; break; }
        if (catalog->count == MUSIC_CATALOG_MAX_TRACKS) { result = ESP_ERR_NO_MEM; break; }
        memcpy(catalog->tracks[catalog->count].name, entry->d_name, len + 1);
        ++catalog->count;
        errno = 0;
    }
    if (result == ESP_OK && errno != 0) result = ESP_FAIL;
    if (closedir(dir) != 0 && result == ESP_OK) result = ESP_FAIL;
    if (result != ESP_OK) {
        /* A failed scan never publishes partial or success-only metadata. */
        memset(catalog, 0, sizeof(*catalog));
        return result;
    }
    qsort(catalog->tracks, catalog->count, sizeof(catalog->tracks[0]), name_compare);
    return ESP_OK;
}
