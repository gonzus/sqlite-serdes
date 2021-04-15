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

static void query_db(int db);
static void show_db_info(int db);

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    unsigned char* buf = 0;
    int rc = SQLITE_OK;

    do {
        printf("SQLite library version [%s]\n", sqlite3_libversion());

        int bail = 0;
        for (int db = 0; db < DB_LAST; ++db) {
            rc = sqlite3_open(data[db].file, &data[db].handle);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "Cannot open database %s - %s\n", data[db].file, sqlite3_errmsg(data[db].handle));
                bail = 1;
                break;
            }
            fprintf(stderr, "Opened database %s\n", data[db].file);
        }
        if (bail) break;

        for (int db = 0; db < DB_LAST; ++db) {
            show_db_info(db);
        }

        for (int db = 0; db < DB_LAST; ++db) {
            query_db(db);
        }

        unsigned int flags = 0;
        sqlite3_int64 len = 0;
        buf = sqlite3_serialize(data[EUROPE].handle, 0, &len, flags);
        if (!buf) {
            fprintf(stderr, "Cannot serialize %s\n", data[EUROPE].file);
            break;
        }
        fprintf(stderr, "Serialized %s - %lld bytes @ %p\n", data[EUROPE].file, len, buf);

        rc = sqlite3_deserialize(data[AFRICA].handle, 0, buf, len, len, flags);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot deserialize from %s into %s\n", data[EUROPE].file, data[AFRICA].file);
            break;
        }
        fprintf(stderr, "Deserialized from %s into %s\n", data[EUROPE].file, data[AFRICA].file);

        rc = sqlite3_deserialize(data[MEMORY].handle, 0, buf, len, len, flags);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot deserialize from %s into %s\n", data[EUROPE].file, data[MEMORY].file);
            break;
        }
        fprintf(stderr, "Deserialized from %s into %s\n", data[EUROPE].file, data[MEMORY].file);

        for (int db = 0; db < DB_LAST; ++db) {
            show_db_info(db);
        }

        for (int db = 0; db < DB_LAST; ++db) {
            query_db(db);
        }
    } while (0);

    if (buf) {
        sqlite3_free(buf);
        fprintf(stderr, "Released serialization buffer for database %s\n", data[EUROPE].file);
        buf = 0;
    }

    for (int db = 0; db < DB_LAST; ++db) {
        if (!data[db].handle) continue;
        sqlite3_close(data[db].handle);
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
