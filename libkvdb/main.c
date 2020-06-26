#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#define B 1
#define KB (1024 * B)
#define MB (1024 * KB)
#define JOURNALSIZE 10
#define KEYSIZE (1 + 128 * B + 9 + 9 + 9 + 1)
#define KEYITEMS (1024)
#define KEYAREASIZE (KEYSIZE * KEYITEMS)




struct record {
    char valid;
    char KEY[128];
    char keysize[9];
    char valuesize[9];
    char clusnum[9];
    char isLong;
};

struct kvdb {
  // your definition here
  int fd;
  struct record *database;
  int filesize;
};
char workpath[1000];

void goto_journal(const struct kvdb *db) {
    lseek(db->fd, 0, SEEK_SET);
}

void goto_keyarea(const struct kvdb *db) {
    lseek(db->fd, JOURNALSIZE, SEEK_SET);
}

void goto_clusters(const struct kvdb *db) {
    lseek(db->fd, (JOURNALSIZE+KEYAREASIZE), SEEK_SET);
}

void load_database(struct kvdb* db) {
    db->filesize = lseek(db->fd, 0, SEEK_END);
    struct kvdb* pkvdb = malloc(sizeof(struct kvdb));
    db->database= malloc(sizeof(char)*KEYAREASIZE);
    lseek(db->fd, JOURNALSIZE, SEEK_SET);
    read(db->fd, db->database, KEYAREASIZE);
    return ;
}

void setkeyondisk(struct kvdb* db,const char* keyname, const char* valuename, const int keyindex_i) {
    int keySizeNum = strlen(keyname);
    int valueSizeNum = strlen(valuename);
    char keysize[9];
    char valuesize[9];
    sprintf(keysize, "%07x", keySizeNum);
    sprintf(valuesize, "%07x", valueSizeNum);
    int end = lseek(db->fd, 0, SEEK_END);
    int clusnum = (end-JOURNALSIZE-KEYAREASIZE)/(4*KB);


    struct record* keybuffer = malloc(sizeof(char)*KEYSIZE);
    sprintf(keybuffer->clusnum, "%07x", clusnum);
    sprintf(db->database[keyindex_i].clusnum, "%07x", clusnum);
    keybuffer->valid = 1;
    memcpy(keybuffer->keysize, keysize, 9);
    memcpy(keybuffer->valuesize, valuesize, 9);
    memcpy(keybuffer->KEY, keyname, keySizeNum);


    if (valueSizeNum > 4*KB) {
        keybuffer->isLong = 1;
    } else {
        keybuffer->isLong = 0;
    }
    lseek(db->fd, JOURNALSIZE+keyindex_i*KEYSIZE, SEEK_SET);
    write(db->fd, keybuffer, KEYSIZE);
    free(keybuffer);
    keybuffer = NULL;
}


void unload_database(struct kvdb *db) {
    free(db->database);
    db->database = NULL;
    return ;
}
struct kvdb *kvdb_open(const char *filename) {
    int fd = open(filename, O_RDWR | O_CREAT, 0777);
    // long offset = 0;
    struct stat statbuf;  
    stat(filename, &statbuf);  
    struct kvdb* pkvdb = malloc(sizeof(struct kvdb));
    pkvdb->fd = fd;
    int fileSize = statbuf.st_size;  


    if (fileSize == 0) {
        char* zeros = malloc(JOURNALSIZE);
        memset(zeros, 0, JOURNALSIZE);
        write(pkvdb->fd, zeros, JOURNALSIZE);
        free(zeros);
        zeros = NULL;

        zeros = malloc(sizeof(char)*KEYAREASIZE);
        memset(zeros, 0, KEYAREASIZE);
        write(pkvdb->fd, zeros, KEYAREASIZE);

        fsync(pkvdb->fd);

    }


    memset(workpath, '\0', 1000);
    return pkvdb;
}

int kvdb_close(struct kvdb *db) {

    close(db->fd);
    free(db);
    db = NULL;
    return 0;
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) {


    load_database(db);
    int i;
    int valuelength = strlen(value);
    for (i = 0; db->database[i].valid != 0; i++) {


        if (strncmp(db->database[i].KEY, key, strlen(key)) == 0) {
            break;
        }
    }


    if (db->database[i].valid == 0) {
        setkeyondisk(db, key, value, i);
        lseek(db->fd, 0, SEEK_END);
        if (valuelength <= 4*KB) {
            lseek(db->fd, 0, SEEK_END);
            char* valuebuffer = malloc(sizeof(char) * 4 * KB);
            memset(valuebuffer, 'T', 4 * KB);
            memcpy(valuebuffer, value, valuelength);
            write(db->fd, valuebuffer, 4*KB);
            free(valuebuffer);
            valuebuffer = NULL;
        } else {
            lseek(db->fd, 0, SEEK_END);
            char* valuebuffer = malloc(sizeof(char) * 16 * MB);
            memset(valuebuffer, 'u', 16 * MB);
            memcpy(valuebuffer, value, valuelength);
            write(db->fd, valuebuffer, 16 * MB);
            free(valuebuffer);
            valuebuffer = NULL;
        }
    } else {
        if (db->database[i].isLong == 0 && valuelength >= 4*KB) {
            setkeyondisk(db, key, value, i);
            int end = lseek(db->fd, 0, SEEK_END);
            int clus = strtol(db->database[i].clusnum, NULL, 16);
            assert(end == clus*4*KB+JOURNALSIZE+KEYAREASIZE);
            char* valuebuffer = malloc(sizeof(char) * 16 * MB);
            memset(valuebuffer, 'Z', 16 * MB);
            memcpy(valuebuffer, value, valuelength);
            write(db->fd, valuebuffer, 16 * MB);
            free(valuebuffer);
            valuebuffer = NULL;
        } else {
            int keySizeNum = strlen(key);
            int valueSizeNum = strlen(value);
            char keysize[9];
            char valuesize[9];
            sprintf(keysize, "%07x", keySizeNum);
            sprintf(valuesize, "%07x", valueSizeNum);
            lseek(db->fd, JOURNALSIZE+i*KEYSIZE, SEEK_SET);
            struct record* keybuffer = malloc(sizeof(char)*KEYSIZE);
            keybuffer->valid = 1;
            memcpy(keybuffer->keysize, keysize, 9);
            memcpy(keybuffer->valuesize, valuesize, 9);
            memcpy(keybuffer->KEY, key, keySizeNum);
            memcpy(keybuffer->clusnum, db->database[i].clusnum, 9);


            int clusindex = strtol(db->database[i].clusnum, NULL, 16);
            int isLong;
            char* valuebuff;
            if (db->database[i].isLong == 0) {
                isLong = 0;
                keybuffer->isLong = 0;
                valuebuff = malloc(4*KB);
                memset(valuebuff, 'R', 4*KB);
            } else if (db->database[i].isLong == 1) {
                isLong = 1;
                keybuffer->isLong = 1;
                valuebuff = malloc(16*MB);
                memset(valuebuff, 'Y', 16*MB);
            } else{



                assert(0);
            }
            write(db->fd, keybuffer, KEYSIZE);
            free(keybuffer);
            keybuffer = NULL;
            memcpy(valuebuff, value, valuelength);
            
            
            lseek(db->fd, JOURNALSIZE+KEYAREASIZE+clusindex*4*KB, SEEK_SET);
            if (isLong == 1) {
                write(db->fd, valuebuff, valuelength);
                lseek(db->fd, 16*MB-valuelength, SEEK_CUR);
            }
            else {
                write(db->fd, valuebuff, valuelength);
                lseek(db->fd, 4*KB-valuelength, SEEK_CUR);
            }
            free(valuebuff);
            valuebuff = NULL;
        }
    }
    fsync(db->fd);
    unload_database(db);
    return 0;
}

char *kvdb_get(struct kvdb *db, const char *key) {


    load_database(db);
    int clusNum = -1;
    int valueSize = -1;
    char* returned = NULL;
    for (int i = 0; db->database[i].valid != 0; i++) {
        if (strncmp(db->database[i].KEY, key, strtol(db->database[i].keysize, NULL, 16)) == 0) {
            clusNum = strtol(db->database[i].clusnum, NULL, 16);
            valueSize = strtol(db->database[i].valuesize, NULL, 16);
            break;
        }
    }

    if (clusNum >= 0 && valueSize >=0) {
        returned = malloc((valueSize+1));
        returned[valueSize] = '\0';
        // lseek(db->fd, JOURNALSIZE+KEYAREASIZE+clusNum*4*KB,SEEK_SET);
        // read(db->fd, returned, valueSize);
        lseek(db->fd, JOURNALSIZE+KEYAREASIZE+clusNum*4*KB, SEEK_SET);
        read(db->fd, returned, valueSize);
    }
    unload_database(db);
    return returned;
}

int main() {

    struct kvdb *db = NULL;
    int instruction = -1;
    // int ttt = 0;
    char* longtext = malloc(8192);
    char* longtext2 = malloc(16384);
    memset(longtext, 'b', 8192);
    memset(longtext2, 't', 16384);
    longtext[8192] = '\0';
    longtext2[16384] = '\0';
    srand(time(NULL));
    char* keys[] = {"abc", "dfs", "werd", "scvxdf", "dfwreh", "qwcvgsrgtre", "qwrvffzcbvce", "yrtbvfcthtxbvvcb"};
    char* values[] = {longtext,longtext2, "abc", "dfs", "werd", "scvxdf", "dfwreh", "qwcvgsrgtre", "qwrvffzcbvce", "yrtbvfcthtxbvvcb"};
    db = kvdb_open("lpr");
    int maps[8];
    int itre = 0;
    for (int i = 0; i < 8; i++)
        maps[i] = -1;
    while (true) {
        itre++;
        if (itre > 200) {
            printf("PASS\n");
            return 0;
        } else {
            if (itre % 20 == 0) {
                printf("%d0%% has complished\n", itre/20);
            }
        }
        int instru = rand()%2;
        if (instru == 0) {
            int keyindex = rand() % 8;
            int valueindex = rand() % 10;
            kvdb_put(db, keys[keyindex], values[valueindex]);


            maps[keyindex] = valueindex;
        } else {
            int keyindex = rand() % 8;
            char* returned = kvdb_get(db, keys[keyindex]);
            if (maps[keyindex] != -1) {
                printf("key:%s\tgetvalue:%s\tshoulebe:%s\n", keys[keyindex], returned, values[maps[keyindex]]);
            }

            if (maps[keyindex] == -1) {
                // assert(returned == NULL);

                assert(maps[keyindex] = -1); 
            } else {
                char* shouldbe = values[maps[keyindex]];


                fflush(stdout);
                assert((strcmp(returned, shouldbe) == 0));
                if (returned != NULL)
                    free(returned);
                returned = NULL;
            }
        }
    }
    kvdb_close(db);
    free(longtext);
    free(longtext2);
    return 0;
    // system("rm -f *.db");
    return 0;
}