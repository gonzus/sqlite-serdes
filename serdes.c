#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

enum DB {
    EUROPE,
    AFRICA,
    MEMORY,
    DB_LAST,
};

static struct {
    const char* file;
    sqlite3* handle;
} data[DB_LAST] = {
    { "europe.db", 0 },
    { "africa.db", 0 },
    { ":memory:", 0 },
};

static int check_sqlite(void);
static void query_db(int db);
static void show_db_info(int db);

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    unsigned char* buf = 0;
    int rc = SQLITE_OK;

    do {
        if (!check_sqlite()) {
            fprintf(stderr, "SQLite library does not support required features\n");
            break;
        }

        printf("==============================\n");
        int bail = 0;
        for (int db = 0; db < DB_LAST; ++db) {
            int flags = SQLITE_OPEN_READONLY;
            rc = sqlite3_open_v2(data[db].file, &data[db].handle, flags, 0);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "Cannot open database %s - %s\n", data[db].file, sqlite3_errmsg(data[db].handle));
                bail = 1;
                break;
            }
            fprintf(stderr, "Opened database %s\n", data[db].file);
        }
        if (bail) break;

        printf("==============================\n");
        for (int db = 0; db < DB_LAST; ++db) {
            show_db_info(db);
        }

        for (int db = 0; db < DB_LAST; ++db) {
            printf("==============================\n");
            query_db(db);
        }

        printf("==============================\n");
        unsigned int flags = 0;
        sqlite3_int64 len = 0;
        buf = sqlite3_serialize(data[EUROPE].handle, 0, &len, flags);
        if (!buf) {
            fprintf(stderr, "Cannot serialize %s\n", data[EUROPE].file);
            break;
        }
        fprintf(stderr, "Serialized %s - %lld bytes @ %p\n", data[EUROPE].file, len, buf);

        printf("==============================\n");
        rc = sqlite3_deserialize(data[AFRICA].handle, 0, buf, len, len, flags);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot deserialize from %s into %s\n", data[EUROPE].file, data[AFRICA].file);
            break;
        }
        fprintf(stderr, "Deserialized from %s into %s\n", data[EUROPE].file, data[AFRICA].file);

        printf("==============================\n");
        rc = sqlite3_deserialize(data[MEMORY].handle, 0, buf, len, len, flags);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot deserialize from %s into %s\n", data[EUROPE].file, data[MEMORY].file);
            break;
        }
        fprintf(stderr, "Deserialized from %s into %s\n", data[EUROPE].file, data[MEMORY].file);

        printf("==============================\n");
        for (int db = 0; db < DB_LAST; ++db) {
            show_db_info(db);
        }

        for (int db = 0; db < DB_LAST; ++db) {
            printf("==============================\n");
            query_db(db);
        }
    } while (0);

    printf("==============================\n");
    if (buf) {
        sqlite3_free(buf);
        fprintf(stderr, "Released serialization buffer for database %s\n", data[EUROPE].file);
        buf = 0;
    }

    printf("==============================\n");
    for (int db = 0; db < DB_LAST; ++db) {
        if (!data[db].handle) continue;
        sqlite3_close_v2(data[db].handle);
        fprintf(stderr, "Closed database %s\n", data[db].file);
        data[db].handle = 0;
    }

    return 0;
}

static int query_countries_like(int db, const char* pattern) {
    int rc = SQLITE_OK;
    sqlite3_stmt* stmt = 0;
    int row = 0;

    do {
        const char* query = "SELECT name FROM countries WHERE name LIKE ?";
        rc = sqlite3_prepare_v2(data[db].handle, query, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot open prepare statement [%s] [%s] - %s\n", data[db].file, query, sqlite3_errmsg(data[db].handle));
            break;
        }
        fprintf(stderr, "Prepared statement for database %s - [%s]\n", data[db].file, query);

        int plen = strlen(pattern);
        rc = sqlite3_bind_text(stmt, 1, pattern, plen, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot bind parameter [%d:%s] for [%s] [%s] - %s\n",
                    plen, pattern, data[db].file, query, sqlite3_errmsg(data[db].handle));
            break;
        }
        fprintf(stderr, "Bound statement parameter [%d:%s] for database %s\n",
                plen, pattern, data[db].file);

        while (1) {
            int step = sqlite3_step(stmt);
            if (step == SQLITE_DONE) break;
            if (step != SQLITE_ROW) {
                fprintf(stderr, "Cannot fetch row for [%s] [%s] - %s\n", data[db].file, query, sqlite3_errmsg(data[db].handle));
                break;
            }
            ++row;
            printf("%d: [%s]\n", row, sqlite3_column_text(stmt, 0));
        }
    } while (0);

    if (stmt) {
        sqlite3_finalize(stmt);
        fprintf(stderr, "Finalized statement for database %s\n", data[db].file);
        stmt = 0;
    }

    return row;
}

static int query_tables_in_db(int db) {
    int rc = SQLITE_OK;
    sqlite3_stmt* stmt = 0;
    int row = 0;

    do {
        const char* query = "SELECT name FROM sqlite_master WHERE type='table'";
        rc = sqlite3_prepare_v2(data[db].handle, query, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot open prepare statement [%s] [%s] - %s\n", data[db].file, query, sqlite3_errmsg(data[db].handle));
            break;
        }
        fprintf(stderr, "Prepared statement for database %s - [%s]\n", data[db].file, query);

        while (1) {
            int step = sqlite3_step(stmt);
            if (step == SQLITE_DONE) break;
            if (step != SQLITE_ROW) {
                fprintf(stderr, "Cannot fetch row for [%s] [%s] - %s\n", data[db].file, query, sqlite3_errmsg(data[db].handle));
                break;
            }
            ++row;
            printf("%d: [%s]\n", row, sqlite3_column_text(stmt, 0));
        }
    } while (0);

    if (stmt) {
        sqlite3_finalize(stmt);
        fprintf(stderr, "Finalized statement for database %s\n", data[db].file);
        stmt = 0;
    }

    return row;
}

static int check_sqlite(void) {
    assert(sqlite3_libversion_number() == SQLITE_VERSION_NUMBER);
    assert(strcmp(sqlite3_libversion(), SQLITE_VERSION) == 0);
    assert(strcmp(sqlite3_sourceid(), SQLITE_SOURCE_ID) == 0);

    printf("SQLite library version: %d [%s]\n", sqlite3_libversion_number(), sqlite3_libversion());
    printf("SQLite source id: [%s]\n", sqlite3_sourceid());
    printf("SQLite threadsafe: %s\n", sqlite3_threadsafe() ? "YES" : "NO");

    int serdes = 1;
#if 0
    // Commented out because ENABLE_DESERIALIZE is one of the options that
    // SQLite mistakenly (as of 3.35.4) does not store in its internal list of
    // compile options.
    for (int opt = 0; 1; ++opt) {
        const char* name = sqlite3_compileoption_get(opt);
        if (!name) break;
        printf("SQLite option %d: [%s]\n", opt, name);
    }

    int serdes = sqlite3_compileoption_used("ENABLE_DESERIALIZE");
    printf("SQLite support for seralization / deserealization: %s\n", serdes ? "YES" : "NO");
#endif
    return serdes;
}

static void query_db(int db) {
    int tables = query_tables_in_db(db);
    if (tables <= 0) return;

    int countries = query_countries_like(db, "%land%");
    if (countries <= 0) return;
}

static void show_db_info(int db) {
    const char* file = sqlite3_db_filename(data[db].handle, 0);
    fprintf(stderr, "File for database %s is %p - [%s]\n", data[db].file, file, file ? file : ":NULL:");
}
