#include <iostream>
#include "apue.h"
#include <pthread.h>
#include <map>

using namespace std;
const int BUFFER_SIZE = 1024;
const int READERS_COUNT = 5;

int err;
pthread_t writer;

pthread_t readers[READERS_COUNT];
bool readers_done[READERS_COUNT];
int readers_done_count = 0;

pthread_rwlock_t myLockRead;
pthread_rwlock_t myLockFile;
pthread_rwlock_t myLockFlags;
pthread_rwlock_t myLockCount;

FILE *file = fopen ("test.txt","w");

char *buffer = new char [BUFFER_SIZE];

void err_quit(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

//проверка на то, все ли читатели прочитали данные из буфера
bool checkAllReadersDone(){
    pthread_rwlock_rdlock(&myLockFlags);
    for(bool i : readers_done){
        if(!i){
            pthread_rwlock_unlock(&myLockFlags);
            return false;
        }
    }
    pthread_rwlock_unlock(&myLockFlags);
    return true;
}

void *thr_fn_write(void *arg) {
    while (true) {
        if (checkAllReadersDone()) {
            string bufferData;

            pthread_rwlock_wrlock(&myLockRead);
            buffer = new char [BUFFER_SIZE];
            read(0, buffer, 1024);
            bufferData = buffer;
            pthread_rwlock_unlock(&myLockRead);

            pthread_rwlock_wrlock(&myLockFlags);
            for(bool & i : readers_done){
                i = false;
            }
            pthread_rwlock_unlock(&myLockFlags);

            pthread_rwlock_wrlock(&myLockCount);
            readers_done_count = 0;
            pthread_rwlock_unlock(&myLockCount);

            if (strcmp(bufferData.c_str(), "quit\n") == 0) {
                pthread_exit((void *) arg);
            }
        }
    }

}

void *thr_fn_read(void *arg) {
    while(true) {
        string bufferData;

        pthread_rwlock_rdlock(&myLockRead);
        bufferData = buffer;
        pthread_rwlock_unlock(&myLockRead);


        if(strcmp(bufferData.c_str(),"quit\n")==0){
            pthread_exit((void *)arg);
        }

        pthread_rwlock_wrlock(&myLockCount);
        if(readers_done_count < READERS_COUNT){
            pthread_rwlock_wrlock(&myLockFlags);
            readers_done[readers_done_count] = true;
            pthread_rwlock_unlock(&myLockFlags);

            readers_done_count++;

            pthread_rwlock_wrlock(&myLockFile);
            write(1, bufferData.c_str(), bufferData.size());
            pthread_rwlock_unlock(&myLockFile);
        }
        pthread_rwlock_unlock(&myLockCount);
    }
}


int main() {
    if(file == nullptr){
        cout<<"произошла ошибка открытия файла";
        return 0;
    }

    pthread_rwlock_init(&myLockRead, nullptr);
    pthread_rwlock_init(&myLockFile, nullptr);
    pthread_rwlock_init(&myLockFlags, nullptr);
    pthread_rwlock_init(&myLockCount, nullptr);

    for(bool & i : readers_done){
        i = true;
    }

    err = pthread_create(&writer, nullptr, thr_fn_write,nullptr);
    if (err != 0) {
        err_quit("невозможно создать поток-писатель");
    }

    for(auto & reader : readers){
        err = pthread_create(&reader, nullptr, thr_fn_read, nullptr);
        if (err != 0) {
            err_quit("невозможно создать поток-читатель");
        }
    }

    err = pthread_join(writer, nullptr);
    if (err != 0) {
        err_quit("невозможно присоединить поток-писатель");
    }

    for(auto & reader : readers){
        err = pthread_join(reader, nullptr);
        if (err != 0) {
            err_quit("невозможно присоединить поток-читатель ");
        }
    }

    fclose(file);
    delete[] buffer;
    pthread_rwlock_destroy(&myLockRead);
    pthread_rwlock_destroy(&myLockFile);
    pthread_rwlock_destroy(&myLockFlags);
    pthread_rwlock_destroy(&myLockCount);
    return 0;
}