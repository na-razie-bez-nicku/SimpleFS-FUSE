#define FUSE_USE_VERSION 31
#define SIMPLEFS_OPT_KEY(t, p, v) {t, offsetof(struct simplefs_config, p), v}

#include <fuse3/fuse.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <malloc.h>
#include "src/disk.hpp"
#include "src/format.cpp"
#include "src/io.cpp"
#include "src/methods.cpp"
#include "src/files.cpp"

struct simplefs_config
{
    const char *disk;
};

struct simplefs_config conf;
Disk disk;
FSHeader header;

// ------------------------------------------------------------
// getattr – pobieranie metadanych pliku (rozmiar, prawa itd.)
// ------------------------------------------------------------
static int simplefs_getattr(
    const char *path, // ścieżka pliku (np. "/hello.txt")
    struct stat *st,  // struktura do wypełnienia metadanymi
    struct fuse_file_info *fi)
{
    memset(st, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0)
    {
        st->st_mode = S_IFDIR | 0555;
        st->st_nlink = 2;
        return 0;
    }

    const char *filename = path + 1;

    std::vector<DirEntry> entries;

    readRootDir(&disk, header.rootDirOffset, 65536, entries);

    for (DirEntry entry : entries)
    {
        if (strcmp(filename, entry.name) == 0)
        {
            st->st_size = entry.sizeBytes;

            st->st_mode = S_IFREG | 0444;
            st->st_nlink = 1;

            return 0;
        }
    }

    return -ENOENT;
}

// ------------------------------------------------------------
// readdir – listowanie zawartości katalogu
// ------------------------------------------------------------
static int simplefs_readdir(
    const char *path,          // katalog, zwykle "/"
    void *buf,                 // bufor, do którego dodajesz wpisy
    fuse_fill_dir_t filler,    // funkcja pomocnicza do dodawania wpisów
    off_t offset,              // ignoruj zwykle
    struct fuse_file_info *fi, // info o otwarciu katalogu
    enum fuse_readdir_flags flags)
{
    // zawsze dodaj "." i ".."
    filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

    std::vector<DirEntry> entries;

    readRootDir(&disk, header.rootDirOffset, 65536, entries);

    for (DirEntry entry : entries)
    {
        std::cout << entry.name << '\n';
        filler(buf, entry.name, NULL, 0, FUSE_FILL_DIR_PLUS);
    }

    return 0; // 0 = OK
}

// ------------------------------------------------------------
// open – otwieranie pliku
// ------------------------------------------------------------
static int simplefs_open(
    const char *path,         // ścieżka do pliku
    struct fuse_file_info *fi // tryb otwarcia (O_RDONLY, O_WRONLY, itd.)
)
{
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;
    // sprawdź, czy plik istnieje
    // jeśli nie, return -ENOENT
    // jeśli ktoś chce zapisu, a FS jest read-only, return -EACCES

    const char *filename = path + 1;

    std::vector<DirEntry> entries;

    readRootDir(&disk, header.rootDirOffset, 65536, entries);

    for (DirEntry entry : entries)
    {
        if (strcmp(filename, entry.name) == 0)
        {
            return 0;
        }
    }

    return -ENOENT;
}

// ------------------------------------------------------------
// read – odczyt danych z pliku
// ------------------------------------------------------------
static int simplefs_read(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
    const char *filename = path + 1;

    std::vector<DirEntry> entries;

    readRootDir(&disk, header.rootDirOffset, 65536, entries);

    for (DirEntry entry : entries)
    {
        if (strcmp(filename, entry.name) == 0)
        {
            readAllText(&disk, entry, buf);
            return entry.sizeBytes;
        }
    }
    return -ENOENT;
}

// ------------------------------------------------------------
// write – zapis do pliku (jeśli w ogóle chcesz obsługiwać)
// ------------------------------------------------------------
static int simplefs_write(
    const char *path,
    const char *buf, // dane do zapisania
    size_t size,     // ile bajtów
    off_t offset,    // gdzie w pliku
    struct fuse_file_info *fi)
{
    const char *filename = path + 1;

    std::vector<DirEntry> entries;

    readRootDir(&disk, header.rootDirOffset, 65536, entries);

    for (DirEntry entry : entries)
    {
        if (strcmp(filename, entry.name) == 0)
        {
            

            return size;
        }
    }

    return -ENOENT;
}

// ------------------------------------------------------------
// create – tworzenie nowego pliku (opcjonalne)
// ------------------------------------------------------------
static int simplefs_create(
    const char *path,
    mode_t mode, // prawa dostępu (np. 0644)
    struct fuse_file_info *fi)
{
    const char *filename = path + 1;
    // jeśli readonly → return -EROFS
    // jeśli chcesz pozwolić → dodaj plik z pustą zawartością

    makeFile(&disk, filename, "");

    return 0;
}

// ------------------------------------------------------------
// unlink – usuwanie pliku (opcjonalne)
// ------------------------------------------------------------
static int simplefs_unlink(
    const char *path)
{
    // jeśli readonly → return -EROFS
    // jeśli chcesz obsługiwać → usuń plik z listy

    return -EROFS;
}

static void *simplefs_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    disk = Disk(conf.disk);
    std::cout << conf.disk << '\n';
    std::cout << disk.openDisk(OPMD_RDWR) << '\n';

    readHeader(&disk, header);
    printHeader(header);

    // tutaj możesz np. otworzyć plik obrazu dysku
    // albo zaalokować pamięć
    return &disk; // można zwrócić wskaźnik do własnego kontekstu
}

// ------------------------------------------------------------
// Operacje FUSE – przypisanie funkcji
// ------------------------------------------------------------
static const struct fuse_operations simplefs_oper = {
    .getattr = simplefs_getattr,
    .readlink = nullptr,
    .mknod = nullptr,
    .mkdir = nullptr,
    .unlink = simplefs_unlink,
    .rmdir = nullptr,
    .symlink = nullptr,
    .rename = nullptr,
    .link = nullptr,
    .chmod = nullptr,
    .chown = nullptr,
    .truncate = nullptr,
    .open = simplefs_open,
    .read = simplefs_read,
    .write = simplefs_write,
    .statfs = nullptr,
    .flush = nullptr,
    .release = nullptr,
    .fsync = nullptr,
    .readdir = simplefs_readdir,
    .init = simplefs_init,
    .create = simplefs_create,
};

static struct fuse_opt simplefs_opts[] = {
    SIMPLEFS_OPT_KEY("disk=%s", disk, 0),
    FUSE_OPT_END};

int main(int argc, char *argv[])
{
    memset(&conf, 0, sizeof(conf));

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    fuse_opt_parse(&args, &conf, simplefs_opts, NULL);

    printf("Dysk: %s\n", conf.disk);

    return fuse_main(args.argc, args.argv, &simplefs_oper, &conf);
}
