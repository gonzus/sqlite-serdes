#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#define EUROPE_DB "europe.db"
#define AFRICA_DB "africa.db"

static int query_db(const char* name, sqlite3* db);

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    sqlite3* europe_db = 0;
    sqlite3* africa_db = 0;
    const char* file = 0;
    unsigned char* buf = 0;
    int rc = SQLITE_OK;

    do {
        printf("SQLite library version [%s]\n", sqlite3_libversion());

        rc = sqlite3_open(EUROPE_DB, &europe_db);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot open database %s: %s\n", EUROPE_DB, sqlite3_errmsg(europe_db));
            break;
        }
        fprintf(stderr, "Opened database %s\n", EUROPE_DB);

        rc = sqlite3_open(AFRICA_DB, &africa_db);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot open database %s: %s\n", AFRICA_DB, sqlite3_errmsg(africa_db));
            break;
        }
        fprintf(stderr, "Opened database %s\n", AFRICA_DB);

        file = sqlite3_db_filename(europe_db, 0);
        fprintf(stderr, "File for database %s: %p [%s]\n", EUROPE_DB, file, file ? file : "");
        file = sqlite3_db_filename(africa_db, 0);
        fprintf(stderr, "File for database %s: %p [%s]\n", AFRICA_DB, file, file ? file : "");

        query_db(EUROPE_DB, europe_db);
        query_db(AFRICA_DB, africa_db);

        unsigned int flags = 0;
        sqlite3_int64 len = 0;
        buf = sqlite3_serialize(europe_db, 0, &len, flags);
        if (!buf) {
            fprintf(stderr, "Cannot serialize %s\n", EUROPE_DB);
            break;
        }
        fprintf(stderr, "Serialized %s: %lld bytes @ %p\n", EUROPE_DB, len, buf);

        rc = sqlite3_deserialize(africa_db, 0, buf, len, len, flags);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot deserialize from %s into %s\n", EUROPE_DB, AFRICA_DB);
            break;
        }
        fprintf(stderr, "Deserialized from %s into %s\n", EUROPE_DB, AFRICA_DB);

        file = sqlite3_db_filename(europe_db, 0);
        fprintf(stderr, "File for database %s: %p [%s]\n", EUROPE_DB, file, file ? file : "");
        file = sqlite3_db_filename(africa_db, 0);
        fprintf(stderr, "File for database %s: %p [%s]\n", AFRICA_DB, file, file ? file : "");

        query_db(EUROPE_DB, europe_db);
        query_db(AFRICA_DB, africa_db);
    } while (0);

    if (buf) {
        sqlite3_free(buf);
        fprintf(stderr, "Released serialization buffer for database %s\n", EUROPE_DB);
        buf = 0;
    }

    if (africa_db) {
        sqlite3_close(africa_db);
        fprintf(stderr, "Closed database %s\n", AFRICA_DB);
        africa_db = 0;
    }

    if (europe_db) {
        sqlite3_close(europe_db);
        fprintf(stderr, "Closed database %s\n", EUROPE_DB);
        europe_db = 0;
    }

    return 0;
}

static int query_db(const char* name, sqlite3* db) {
    int rc = SQLITE_OK;
    sqlite3_stmt* stmt = 0;
    int row = 0;

    do {
        const char* query = "SELECT name FROM countries WHERE name LIKE ?";
        rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot open prepare statement [%s] [%s]: %s\n", name, query, sqlite3_errmsg(db));
            break;
        }
        fprintf(stderr, "Prepared statement for database %s: [%s]\n", name, query);

        const char* param = "%land%";
        int plen = strlen(param);
        rc = sqlite3_bind_text(stmt, 1, param, plen, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot bind parameter [%d:%s] for [%s] [%s]: %s\n",
                    plen, param, name, query, sqlite3_errmsg(db));
            break;
        }
        fprintf(stderr, "Bound statement parameter [%d:%s] for database %s\n",
                plen, param, name);

        while (1) {
            int step = sqlite3_step(stmt);
            if (step == SQLITE_DONE) break;
            if (step != SQLITE_ROW) {
                fprintf(stderr, "Cannot fetch row for [%s] [%s]: %s\n", name, query, sqlite3_errmsg(db));
                break;
            }
            ++row;
            printf("%d: [%s]\n", row, sqlite3_column_text(stmt, 0));
        }
    } while (0);

    if (stmt) {
        sqlite3_finalize(stmt);
        fprintf(stderr, "Finalized statement for database %s\n", name);
        stmt = 0;
    }

    return row;
}
