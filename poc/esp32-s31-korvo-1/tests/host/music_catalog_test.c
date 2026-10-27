#define _XOPEN_SOURCE 700
#include "music_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void touch(const char *path)
{
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    assert(fclose(f) == 0);
}

static void join(char *out, size_t cap, const char *a, const char *b)
{
    int n = snprintf(out, cap, "%s/%s", a, b);
    assert(n > 0 && (size_t)n < cap);
}

int main(void)
{
    char root[] = "build-host/music-catalog-XXXXXX";
    assert(mkdtemp(root) != NULL);
    music_catalog_t cat;
    assert(music_catalog_scan(root, &cat) == ESP_ERR_NOT_FOUND); /* missing MUSIC */

    char music[512]; join(music, sizeof(music), root, "MUSIC");
    assert(mkdir(music, 0700) == 0);
    assert(music_catalog_scan(root, &cat) == ESP_OK && cat.count == 0); /* empty */

    char p[768];
    join(p, sizeof(p), music, "zeta.Mp3"); touch(p);
    join(p, sizeof(p), music, "Alpha.mp3"); touch(p);
    join(p, sizeof(p), music, "beta.MP3"); touch(p);
    join(p, sizeof(p), music, "ignore.wav"); touch(p);
    join(p, sizeof(p), music, "noext"); touch(p);
    join(p, sizeof(p), music, "folder.mp3"); assert(mkdir(p, 0700) == 0);
    assert(music_catalog_scan(root, &cat) == ESP_OK && cat.count == 3);
    assert(strcmp(cat.tracks[0].name, "Alpha.mp3") == 0);
    assert(strcmp(cat.tracks[1].name, "beta.MP3") == 0);
    assert(strcmp(cat.tracks[2].name, "zeta.Mp3") == 0);

    char long_name[256]; memset(long_name, 'n', 240); strcpy(long_name + 240, ".mp3");
    join(p, sizeof(p), music, long_name); touch(p);
    assert(music_catalog_scan(root, &cat) == ESP_OK && cat.count == 4);
    assert(strlen(cat.tracks[2].name) == 244);

    /* The fixed catalogue accepts exactly its declared capacity. */
    for (size_t i = 0; i < MUSIC_CATALOG_MAX_TRACKS - cat.count; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "track_%03u.mp3", (unsigned)i);
        join(p, sizeof(p), music, name); touch(p);
    }
    assert(music_catalog_scan(root, &cat) == ESP_OK);
    assert(cat.count == MUSIC_CATALOG_MAX_TRACKS);
    join(p, sizeof(p), music, "overflow.mp3"); touch(p);
    assert(music_catalog_scan(root, &cat) == ESP_ERR_NO_MEM);
    assert(cat.count == 0 && cat.directory_found == 0); /* failed scans publish no partial state */

    char root2[] = "build-host/music-catalog-file-XXXXXX";
    assert(mkdtemp(root2) != NULL);
    assert(music_catalog_scan(root2, &cat) == ESP_ERR_NOT_FOUND);
    /* A regular file in place of MUSIC is reported as invalid state. */
    join(p, sizeof(p), root2, "MUSIC"); touch(p);
    assert(music_catalog_scan(root2, &cat) == ESP_ERR_INVALID_STATE);

    /* Keep fixtures under build-host; no cleanup can remove unrelated files. */
    puts("music_catalog tests passed");
    return 0;
}
