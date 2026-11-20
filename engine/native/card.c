/* the memory card: the backup library's calls over a 128 KB image of a VMU. the
 * runner names the image's file, and a missing file becomes a new card. where
 * the runner names none, no card is in any socket. a call ends as it returns,
 * so buStat is never busy. the image is read as the game first starts the
 * library, after play has made its engine. each change writes it back by the
 * engine's storage where linked, else by the C library. either writes a new
 * file under the card's name and .new. that file takes the card's name once it
 * holds the card whole. the storage waits for the disk before the rename. so a
 * failed write or a run cut short leaves the old card whole. filed says where
 * Windows without an engine differs. the file system is the VMU's own: a root
 * block, a FAT and a directory. the FAT chains the directory's blocks as it
 * does a file's. an image mounts only once damage finds it whole. a call
 * changes the card only once its change is stored. so each call reads chains
 * that end where the directory says */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "sa_engine.h"

#define BLOCK 512
#define BLOCKS 256
#define ROOT 255
#define FAT 254
#define DIRECTORY 253
#define DIRECTORY_BLOCKS 13
#define USER_BLOCKS 200
#define FAT_FREE 0xfffc
#define FAT_END 0xfffa
#define ENTRY 32
#define NAME 12
/* the root block's offsets of the card's layout, then a directory entry's
 * offsets of its file */
#define ROOT_FAT 0x46
#define ROOT_FAT_BLOCKS 0x48
#define ROOT_DIRECTORY 0x4a
#define ROOT_DIRECTORY_BLOCKS 0x4c
#define ROOT_USER_BLOCKS 0x50
#define ENTRY_FIRST 2
#define ENTRY_NAME 4
#define ENTRY_BLOCKS 0x18
#define ENTRY_HEADER 0x1a
/* an entry's first byte, for a file of data and for a game. 0 is no file */
#define FILE_DATA 0x33
#define FILE_GAME 0xcc
/* a formatted card's root block starts with FORMATTED_BYTES of these */
#define FORMATTED 0x55
#define FORMATTED_BYTES 16
/* the most blocks a card's game has, as the console's library sets them */
#define GAME_BLOCKS 128

#define BUD_ERR_OK 0
#define BUD_ERR_INVALID_PARAM ((int32_t)0xffffff00)
#define BUD_ERR_NO_DISK ((int32_t)0xffffff01)
#define BUD_ERR_DISK_FULL ((int32_t)0xffffff04)
#define BUD_ERR_FILE_NOT_FOUND ((int32_t)0xffffff05)
#define BUD_ERR_FILE_EXIST ((int32_t)0xffffff06)
#define BUD_ERR_CANNOT_CREATE ((int32_t)0xffffff08)
#define BUD_ERR_EXECFILE_EXIST ((int32_t)0xffffff09)
#define BUD_ERR_ACCESS_DENIED ((int32_t)0xffffff0b)
#define BUD_ERR_WRITE_ERROR ((int32_t)0xffffff40)
#define BUD_FILETYPE_EXECUTABLE 1
#define BUD_OP_MOUNT 1
#define BUD_OP_DELETEFILE 7
#define BUD_OP_LOADFILE 0xb
#define BUD_OP_SAVEFILE 0xc
#define BUD_OP_SAVEEXECFILE 0xd
#define BUD_OP_LOADFILEEX 0xe
#define BUD_OP_REWRITEEXECFILE 0x14

/* a run without an engine links none of these */
#pragma weak sa_storage_read
#pragma weak sa_storage_write

struct sa_engine *native_card_engine;
/* the mounted card, and a save's copy of it until the copy is stored */
static uint8_t card[BLOCKS * BLOCK];
static uint8_t staged[BLOCKS * BLOCK];
static const char *card_path;
static int loaded;
static int found_at;
/* the game's callback, as BU_COMPLETECALLBACK declares it */
static int (*complete)(int32_t drive, int32_t op, int32_t status,
                       uint32_t param);
static int mounted;

void native_card_open(const char *path)
{
    card_path = path;
}

static uint16_t half(const uint8_t *at)
{
    return (uint16_t)(at[0] | at[1] << 8);
}

static void set_half(uint8_t *at, uint16_t value)
{
    at[0] = (uint8_t)value;
    at[1] = (uint8_t)(value >> 8);
}

/* a word of image's root */
static uint32_t root(const uint8_t *image, uint32_t at)
{
    return half(image + ROOT * BLOCK + at);
}

/* block's word of image's FAT */
static uint8_t *fat_at(uint8_t *image, uint32_t block)
{
    return image + root(image, ROOT_FAT) * BLOCK + block * 2;
}

/* a new card, as a console formats one. every block is free but the root, the
 * FAT and the directory, whose chain runs down */
static void format(uint8_t *image)
{
    /* the layout's nonzero words, by their offset in the root. no call here
     * reads the words with no name */
    static const uint16_t layout[][2] = {
        {0x40, 255},
        {0x44, 255},
        {ROOT_FAT, FAT},
        {ROOT_FAT_BLOCKS, 1},
        {ROOT_DIRECTORY, DIRECTORY},
        {ROOT_DIRECTORY_BLOCKS, DIRECTORY_BLOCKS},
        {ROOT_USER_BLOCKS, USER_BLOCKS},
        {0x52, 31},
        {0x56, 128},
    };

    memset(image, 0, sizeof card);
    memset(image + ROOT * BLOCK, FORMATTED, FORMATTED_BYTES);
    for (uint32_t i = 0; i < sizeof layout / sizeof *layout; i++) {
        set_half(image + ROOT * BLOCK + layout[i][0], layout[i][1]);
    }
    for (uint32_t block = 0; block < BLOCKS; block++) {
        set_half(fat_at(image, block), FAT_FREE);
    }
    for (uint32_t block = DIRECTORY; block > DIRECTORY - DIRECTORY_BLOCKS + 1;
         block--) {
        set_half(fat_at(image, block), (uint16_t)(block - 1));
    }
    set_half(fat_at(image, DIRECTORY - DIRECTORY_BLOCKS + 1), FAT_END);
    set_half(fat_at(image, FAT), FAT_END);
    set_half(fat_at(image, ROOT), FAT_END);
}

static int stored(void)
{
    return native_card_engine && sa_storage_read && sa_storage_write;
}

/* the engine's storage, called under the host's FPU mode. the card takes the
 * call's result as the request's and reuses image once it returns. this needs
 * a storage that ends a request in its call, as the PC backend's does */
static sa_result storage(int write, uint8_t *image)
{
    uint32_t game = native_fpu_host();
    sa_handle request;
    sa_result result;

    result = write ? sa_storage_write(native_card_engine, card_path, image,
                                      sizeof card, &request)
                   : sa_storage_read(native_card_engine, card_path, image,
                                     sizeof card, &request);
    native_fpu_game(game);
    return result;
}

/* the name a new card is written under, beside the card's file. it gives 0
 * where the name doesn't fit */
static int fresh_at(char *name, size_t size)
{
    return (size_t)snprintf(name, size, "%s.new", card_path) < size;
}

/* the file at path, into image. it gives SA_OK only for exactly a card's
 * bytes, and SA_ERR_MISSING where no file is there */
static sa_result unfiled(const char *path, uint8_t *image)
{
    FILE *in = fopen(path, "rb");
    sa_result result;

    if (!in) {
        return errno == ENOENT ? SA_ERR_MISSING : SA_ERR_IO;
    }
    result = fread(image, 1, sizeof card, in) == sizeof card && fgetc(in) == EOF
                 ? SA_OK
                 : SA_ERR_INVALID;
    fclose(in);
    return result;
}

/* write image as the card's file. a new file beside it takes its name once it
 * holds the card whole. it gives 1 where it did. else it gives 0, with the
 * card's file as it was and the new one removed. Windows' rename takes no
 * file's place, so there the old card goes first. a failed rename or a run
 * stopped between the two leaves the new card under its own name. adopted
 * finds it there */
static int filed(const uint8_t *image)
{
    char fresh[FILENAME_MAX + sizeof ".new"];
    FILE *out;
    int kept;

    if (!fresh_at(fresh, sizeof fresh) || !(out = fopen(fresh, "wb"))) {
        return 0;
    }
    kept = fwrite(image, 1, sizeof card, out) == sizeof card;
    kept = !fclose(out) && kept;
#ifdef _WIN32
    if (kept) {
        remove(card_path);
        return !rename(fresh, card_path);
    }
#else
    kept = kept && !rename(fresh, card_path);
#endif
    if (!kept) {
        remove(fresh);
    }
    return kept;
}

/* image where the card is kept: 1 once it is there */
static int store(uint8_t *image)
{
    int kept = stored() ? storage(1, image) == SA_OK : filed(image);

    if (!kept) {
        fprintf(stderr, "sa1: can't write the card at %s\n", card_path);
    }
    return kept;
}

/* the nth entry of image's directory, or null past its end */
static uint8_t *entry_at(uint8_t *image, int n)
{
    uint32_t block = root(image, ROOT_DIRECTORY);
    uint32_t per_block = BLOCK / ENTRY;

    if ((uint32_t)n >= root(image, ROOT_DIRECTORY_BLOCKS) * per_block) {
        return NULL;
    }
    for (uint32_t i = (uint32_t)n / per_block; i; i--) {
        block = half(fat_at(image, block));
    }
    return image + block * BLOCK + (uint32_t)n % per_block * ENTRY;
}

/* the nth entry a card reads, or null past the last. the console's library
 * reads one entry for each user block, though the directory has room for
 * more */
static uint8_t *slot_at(uint8_t *image, int n)
{
    return (uint32_t)n < root(image, ROOT_USER_BLOCKS) ? entry_at(image, n)
                                                       : NULL;
}

static uint8_t *entry_named(uint8_t *image, const char *name)
{
    uint8_t *entry;

    for (int n = 0; (entry = slot_at(image, n)); n++) {
        if (entry[0] &&
            !strncmp((const char *)entry + ENTRY_NAME, name, NAME)) {
            return entry;
        }
    }
    return NULL;
}

/* the entry of image's game, or null where it holds none */
static uint8_t *game_of(uint8_t *image)
{
    uint8_t *entry;

    for (int n = 0; (entry = slot_at(image, n)) && entry[0] != FILE_GAME; n++) {
    }
    return entry;
}

/* what keeps image from being a card to mount, or null where nothing does.
 * *file is the entry of a file at fault, or null */
static const char *damage(uint8_t *image, const uint8_t **file)
{
    uint8_t taken[BLOCKS] = {0};
    uint32_t fat = root(image, ROOT_FAT);
    uint32_t user = root(image, ROOT_USER_BLOCKS);
    uint32_t block = root(image, ROOT_DIRECTORY);
    uint8_t *entry;

    *file = NULL;
    if (fat >= ROOT) {
        return "its FAT doesn't lie below the root";
    }
    if (root(image, ROOT_FAT_BLOCKS) != 1) {
        return "its FAT isn't one block long";
    }
    if (user > fat) {
        return "its user blocks reach its FAT";
    }
    taken[fat] = 1;
    for (uint32_t left = root(image, ROOT_DIRECTORY_BLOCKS); left; left--) {
        if (block < user) {
            return "its directory lies among its user blocks";
        }
        if (block >= ROOT || taken[block]) {
            return "its directory's chain is broken";
        }
        taken[block] = 1;
        block = half(fat_at(image, block));
    }
    if (block != FAT_END || !root(image, ROOT_DIRECTORY_BLOCKS)) {
        return "its directory's chain is broken";
    }
    for (int n = 0; (entry = entry_at(image, n)); n++) {
        uint32_t blocks = entry[0] ? half(entry + ENTRY_BLOCKS) : 0;

        *file = entry;
        block = half(entry + ENTRY_FIRST);
        for (uint32_t left = blocks; left; left--) {
            if (block == FAT_END) {
                return "a file's chain ends before the file";
            }
            if (block >= user) {
                return "a file's chain leaves the user blocks";
            }
            if (taken[block]) {
                return "a file's chain takes a block twice";
            }
            taken[block] = 1;
            block = half(fat_at(image, block));
        }
        /* a file of no blocks has no chain to end */
        if (blocks && block != FAT_END) {
            return "a file's chain doesn't end with the file";
        }
    }
    *file = NULL;
    return NULL;
}

static int formatted(const uint8_t *image)
{
    for (int i = 0; i < FORMATTED_BYTES; i++) {
        if (image[ROOT * BLOCK + i] != FORMATTED) {
            return 0;
        }
    }
    return 1;
}

#ifdef _WIN32
/* whether a whole new card that filed left beside no card took the card's
 * name. a partial or damaged new card stays */
static int adopted(void)
{
    char fresh[FILENAME_MAX + sizeof ".new"];
    const uint8_t *file;

    return fresh_at(fresh, sizeof fresh) && unfiled(fresh, staged) == SA_OK &&
           formatted(staged) && !damage(staged, &file) &&
           !rename(fresh, card_path);
}
#endif

/* the card's file into card, by the engine's storage or the C library */
static sa_result fetched(void)
{
    return stored() ? storage(0, card) : unfiled(card_path, card);
}

/* the image, or a new card where no file is there. any other file must hold a
 * whole formatted card that damage accepts. else the run ends, and the file
 * stays as it was */
static void load(void)
{
    const uint8_t *file;
    const char *why;
    sa_result result;

    loaded = 1;
    result = fetched();
#ifdef _WIN32
    /* with an engine too: a run without one may have left a new card */
    if (result == SA_ERR_MISSING && adopted()) {
        result = fetched();
    }
#endif
    if (result == SA_ERR_MISSING) {
        format(card);
        fprintf(stderr, "sa1: a new card at %s\n", card_path);
        store(card);
        return;
    }
    if (result == SA_OK && !formatted(card)) {
        result = SA_ERR_INVALID;
    }
    if (result != SA_OK) {
        fprintf(stderr, "sa1: no card at %s\n", card_path);
        exit(2);
    }
    if ((why = damage(card, &file))) {
        fprintf(stderr, "sa1: a damaged card at %s: %s%s%.*s\n", card_path, why,
                file ? ": " : "", file ? NAME : 0,
                file ? (const char *)file + ENTRY_NAME : "");
        exit(2);
    }
}

/* whether a card is in the socket. the first call reads its image */
static int inserted(void)
{
    if (card_path && !loaded) {
        load();
    }
    return card_path != NULL;
}

static uint32_t free_blocks(uint8_t *image)
{
    uint32_t count = 0;

    for (uint32_t block = 0; block < root(image, ROOT_USER_BLOCKS); block++) {
        count += half(fat_at(image, block)) == FAT_FREE;
    }
    return count;
}

/* the blocks image has free for a game. a game lies in the card's blocks from
 * the first up, in at most GAME_BLOCKS of the user blocks. so its room ends at
 * the first of those that isn't free */
static uint32_t game_room(uint8_t *image)
{
    uint32_t most = root(image, ROOT_USER_BLOCKS);
    uint32_t count = 0;

    if (most > GAME_BLOCKS) {
        most = GAME_BLOCKS;
    }
    while (count < most && half(fat_at(image, count)) == FAT_FREE) {
        count++;
    }
    return count;
}

static int32_t done(int32_t drive, int32_t op, int32_t result)
{
    if (complete) {
        complete(drive, op, result, 0);
    }
    return result;
}

/* the library starts afresh, as the console's does. buExecServer then mounts
 * the card again and reports it to the game's callback. card_init clears the
 * game's record of its drives. a save waits for that report */
int32_t buInit(int32_t capacity, int32_t drives, void *work,
               void (*ready)(void))
{
    (void)capacity, (void)drives, (void)work;
    mounted = 0;
    inserted();
    if (ready) {
        ready();
    }
    return 0;
}

void buExecServer(void)
{
    if (complete && !mounted && inserted()) {
        mounted = 1;
        complete(0, BUD_OP_MOUNT, BUD_ERR_OK, 0);
    }
}

void buDisableServer(void)
{
}

void buSetCompleteCallback(int (*function)(int32_t drive, int32_t op,
                                           int32_t status, uint32_t param))
{
    complete = function;
}

void buSetProgressCallback(void *function)
{
    (void)function;
}

int32_t buStat(int32_t drive)
{
    return drive == 0 && mounted ? 0 : BUD_ERR_NO_DISK;
}

int32_t buIsReady(int32_t drive)
{
    return drive == 0 && mounted;
}

/* BUS_DISKINFO, as shinobi.h lays it out */
struct disk_info {
    uint8_t volume[32];
    uint16_t total_blocks, total_user_blocks, free_blocks, free_user_blocks,
        total_exe_blocks, free_exe_blocks, block_size, icon_no;
    uint8_t time[8];
};

/* a card the server has yet to mount answers nothing, and info stays as it
 * was */
int32_t buGetDiskInfo(int32_t drive, struct disk_info *info)
{
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    memset(info, 0, sizeof *info);
    info->total_blocks = BLOCKS;
    info->total_user_blocks = (uint16_t)root(card, ROOT_USER_BLOCKS);
    info->free_blocks = (uint16_t)free_blocks(card);
    info->free_user_blocks = info->free_blocks;
    info->block_size = BLOCK;
    return BUD_ERR_OK;
}

/* the blocks a new file of type has room for, as the console's library counts
 * them. a game's are game_room's. any other file's are the free user blocks */
int32_t buGetDiskFree(int32_t drive, int32_t type)
{
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    return (int32_t)(type == BUD_FILETYPE_EXECUTABLE ? game_room(card)
                                                     : free_blocks(card));
}

static int32_t find_from(char *name)
{
    uint8_t *entry;

    while ((entry = slot_at(card, found_at))) {
        found_at++;
        if (entry[0]) {
            memcpy(name, entry + ENTRY_NAME, NAME);
            name[NAME] = '\0';
            return BUD_ERR_OK;
        }
    }
    return BUD_ERR_FILE_NOT_FOUND;
}

int32_t buFindFirstFile(int32_t drive, char *name)
{
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    found_at = 0;
    return find_from(name);
}

int32_t buFindNextFile(int32_t drive, char *name)
{
    return drive == 0 && mounted ? find_from(name) : BUD_ERR_NO_DISK;
}

/* the name of the card's game. a card with none answers
 * BUD_ERR_FILE_NOT_FOUND, and name stays as it was */
int32_t buFindExecFile(int32_t drive, char *name)
{
    uint8_t *entry;

    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    if (!(entry = game_of(card))) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    memcpy(name, entry + ENTRY_NAME, NAME);
    name[NAME] = '\0';
    return BUD_ERR_OK;
}

int32_t buGetFileSize(int32_t drive, const char *name)
{
    uint8_t *entry = drive == 0 && mounted ? entry_named(card, name) : NULL;

    return entry ? half(entry + ENTRY_BLOCKS) : BUD_ERR_FILE_NOT_FOUND;
}

/* BUD_ERR_OK where the card holds name's file, and BUD_ERR_FILE_NOT_FOUND where
 * it holds none. a drive with no mounted card answers BUD_ERR_NO_DISK */
int32_t buIsExistFile(int32_t drive, const char *name)
{
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    return entry_named(card, name) ? BUD_ERR_OK : BUD_ERR_FILE_NOT_FOUND;
}

/* read name's blocks into buffer from its block first on, at most blocks of
 * them. a read past the file's end stops there and succeeds */
static int32_t load_file(int32_t drive, int32_t op, const char *name,
                         void *buffer, uint32_t first, uint32_t blocks)
{
    uint8_t *entry = drive == 0 && mounted ? entry_named(card, name) : NULL;
    uint8_t *to = buffer;
    uint32_t block;

    if (!entry) {
        return done(drive, op, BUD_ERR_FILE_NOT_FOUND);
    }
    block = half(entry + ENTRY_FIRST);
    for (uint32_t at = 0; at < half(entry + ENTRY_BLOCKS) && blocks; at++) {
        if (at >= first) {
            memcpy(to, card + block * BLOCK, BLOCK);
            to += BLOCK;
            blocks--;
        }
        block = half(fat_at(card, block));
    }
    return done(drive, op, BUD_ERR_OK);
}

int32_t buLoadFile(int32_t drive, const char *name, void *buffer,
                   uint32_t blocks)
{
    return load_file(drive, BUD_OP_LOADFILE, name, buffer, 0, blocks);
}

int32_t buLoadFileEx(int32_t drive, const char *name, void *buffer,
                     uint32_t first, uint32_t blocks)
{
    return load_file(drive, BUD_OP_LOADFILEEX, name, buffer, first, blocks);
}

/* the flash holds no internet settings. its partition never opens, so mail.c
 * reads nothing from it */
int32_t fmMngInit(int32_t partition, void *work)
{
    (void)partition, (void)work;
    return -1;
}

/* free the blocks of entry's file */
static void release(uint8_t *image, const uint8_t *entry)
{
    uint32_t block = half(entry + ENTRY_FIRST);

    for (uint32_t held = half(entry + ENTRY_BLOCKS); held; held--) {
        uint8_t *word = fat_at(image, block);

        block = half(word);
        set_half(word, FAT_FREE);
    }
}

/* the first entry of image's directory that holds no file, with name alone in
 * it. null where each holds one */
static uint8_t *new_entry(uint8_t *image, const char *name)
{
    uint8_t *entry;

    for (int n = 0; (entry = slot_at(image, n)) && entry[0]; n++) {
    }
    if (entry) {
        memset(entry, 0, ENTRY);
        strncpy((char *)entry + ENTRY_NAME, name, NAME);
    }
    return entry;
}

/* write name's file, blocks long, from from into image, in place of any file
 * of that name. it gives BUD_ERR_OK, or the error that refuses it with image
 * as it was. the replaced file's blocks count as room, and a new name needs a
 * free entry */
static int32_t put(uint8_t *image, const char *name, const uint8_t *from,
                   uint32_t blocks)
{
    uint8_t *entry = entry_named(image, name);
    uint32_t held = entry ? half(entry + ENTRY_BLOCKS) : 0;
    uint32_t block;
    uint8_t *link;

    if (blocks > free_blocks(image) + held) {
        return BUD_ERR_DISK_FULL;
    }
    if (!entry && !(entry = new_entry(image, name))) {
        return BUD_ERR_CANNOT_CREATE;
    }
    release(image, entry);
    entry[0] = FILE_DATA;
    set_half(entry + ENTRY_BLOCKS, (uint16_t)blocks);
    /* each block in turn takes the highest free user block, as the console's
     * library gives them. so the file's chain runs down from its first block.
     * link is the word that names the next block: the entry's, then each
     * block's of the FAT */
    link = entry + ENTRY_FIRST;
    block = root(image, ROOT_USER_BLOCKS);
    for (uint32_t i = 0; i < blocks; i++) {
        while (half(fat_at(image, --block)) != FAT_FREE) {
        }
        memcpy(image + block * BLOCK, from + i * BLOCK, BLOCK);
        set_half(link, (uint16_t)block);
        link = fat_at(image, block);
    }
    set_half(link, FAT_END);
    return BUD_ERR_OK;
}

/* write name's game, blocks long, from from into image. a card holds one
 * game, in its blocks from the first up in order. the game's entry gives its
 * header's offset as 1, as the console's library writes it. it gives
 * BUD_ERR_OK, or the error that refuses the game with image as it was. the
 * errors come in the library's order: no room, a file of that name, a game on
 * the card, then a first block or an entry that isn't free */
static int32_t put_game(uint8_t *image, const char *name, const uint8_t *from,
                        uint32_t blocks)
{
    uint8_t *entry;
    uint8_t *link;

    if (blocks > game_room(image)) {
        return BUD_ERR_DISK_FULL;
    }
    if (entry_named(image, name)) {
        return BUD_ERR_FILE_EXIST;
    }
    if (game_of(image)) {
        return BUD_ERR_EXECFILE_EXIST;
    }
    if (half(fat_at(image, 0)) != FAT_FREE ||
        !(entry = new_entry(image, name))) {
        return BUD_ERR_CANNOT_CREATE;
    }
    entry[0] = FILE_GAME;
    set_half(entry + ENTRY_BLOCKS, (uint16_t)blocks);
    set_half(entry + ENTRY_HEADER, 1);
    link = entry + ENTRY_FIRST;
    for (uint32_t block = 0; block < blocks; block++) {
        memcpy(image + block * BLOCK, from + block * BLOCK, BLOCK);
        set_half(link, (uint16_t)block);
        link = fat_at(image, block);
    }
    set_half(link, FAT_END);
    return BUD_ERR_OK;
}

/* write blocks of from over those of name's game in image, from the game's
 * block first on. the write follows the game's chain. the console's library
 * writes the card's blocks of those numbers, which are the blocks of a game
 * it saved. it gives BUD_ERR_OK, or the library's error with image as it was:
 * for no file of that name, a file that is no game, and blocks past the
 * game's end */
static int32_t rewrite(uint8_t *image, const char *name, const uint8_t *from,
                       uint32_t first, uint32_t blocks)
{
    uint8_t *entry = entry_named(image, name);
    uint32_t held;
    uint32_t block;

    if (!entry) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    if (entry[0] != FILE_GAME) {
        return BUD_ERR_ACCESS_DENIED;
    }
    held = half(entry + ENTRY_BLOCKS);
    if (first >= held || blocks > held - first) {
        return BUD_ERR_INVALID_PARAM;
    }
    block = half(entry + ENTRY_FIRST);
    for (uint32_t at = 0; at < first + blocks; at++) {
        if (at >= first) {
            memcpy(image + block * BLOCK, from, BLOCK);
            from += BLOCK;
        }
        block = half(fat_at(image, block));
    }
    return BUD_ERR_OK;
}

/* take name's file off image: its blocks are free, and its entry holds no
 * file. it gives BUD_ERR_OK, or BUD_ERR_FILE_NOT_FOUND with image as it was */
static int32_t drop(uint8_t *image, const char *name)
{
    uint8_t *entry = entry_named(image, name);

    if (!entry) {
        return BUD_ERR_FILE_NOT_FOUND;
    }
    release(image, entry);
    memset(entry, 0, ENTRY);
    return BUD_ERR_OK;
}

/* a copy of the card for a call to change */
static uint8_t *stage(void)
{
    memcpy(staged, card, sizeof card);
    return staged;
}

/* end a call's change to staged, which gave result. staged is stored where
 * that is BUD_ERR_OK, and then takes the card's place. it gives result, or
 * BUD_ERR_WRITE_ERROR where store can't keep staged. either error leaves the
 * card as it was */
static int32_t commit(int32_t result)
{
    if (result == BUD_ERR_OK && !store(staged)) {
        result = BUD_ERR_WRITE_ERROR;
    }
    if (result == BUD_ERR_OK) {
        memcpy(card, staged, sizeof card);
    }
    return result;
}

/* each of the four calls below changes a copy of the card, which takes the
 * card's place once it is stored. a change the card refuses, or one store
 * can't keep, leaves the card as it was. on a mounted card the result goes to
 * the game's callback too, where the console's library sends it */
int32_t buSaveFile(int32_t drive, const char *name, const void *buffer,
                   uint32_t blocks, const void *time, uint32_t flag)
{
    (void)time, (void)flag;
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    return done(drive, BUD_OP_SAVEFILE,
                commit(put(stage(), name, buffer, blocks)));
}

int32_t buSaveExecFile(int32_t drive, const char *name, const void *buffer,
                       uint32_t blocks, const void *time, uint32_t flag)
{
    (void)time, (void)flag;
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    return done(drive, BUD_OP_SAVEEXECFILE,
                commit(put_game(stage(), name, buffer, blocks)));
}

int32_t buRewriteExecFile(int32_t drive, const char *name, const void *buffer,
                          uint32_t first, uint32_t blocks)
{
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    return done(drive, BUD_OP_REWRITEEXECFILE,
                commit(rewrite(stage(), name, buffer, first, blocks)));
}

int32_t buDeleteFile(int32_t drive, const char *name)
{
    if (drive != 0 || !mounted) {
        return BUD_ERR_NO_DISK;
    }
    return done(drive, BUD_OP_DELETEFILE, commit(drop(stage(), name)));
}
