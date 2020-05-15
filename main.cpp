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
    for(bool i : readers_done){
        if(!i){
            return false;
        }
    }
    return true;
}

void *thr_fn_write(void *arg) {
   while (true) {
        if (checkAllReadersDone()) {
            pthread_rwlock_wrlock(&myLockRead);
            buffer = new char [BUFFER_SIZE];
            read(0, buffer, 1024);
            for(bool & i : readers_done){
                i = false;
            }
            readers_done_count = 0;
            if (strcmp(buffer, "quit\n") == 0) {
                pthread_rwlock_unlock(&myLockRead);
                pthread_exit((void *) arg);
            }
            else {
                pthread_rwlock_unlock(&myLockRead);
            }

        }
   }

}

void *thr_fn_read(void *arg) {
    while(true) {
        pthread_rwlock_rdlock(&myLockRead);
        if (strcmp(buffer , "quit\n") == 0) {
            pthread_rwlock_unlock(&myLockRead);
            pthread_exit((void *) arg);
        }
        else {
            if (readers_done_count < READERS_COUNT) {
                readers_done[readers_done_count] = true;
                readers_done_count++;

                pthread_rwlock_wrlock(&myLockFile);
                write(fileno(file), buffer, BUFFER_SIZE);
                pthread_rwlock_unlock(&myLockFile);
            }
            pthread_rwlock_unlock(&myLockRead);
        }
    }
}


int main() {
    if(file == nullptr){
        cout<<"произошла ошибка открытия файла";
        return 0;
    }

    pthread_rwlock_init(&myLockRead, nullptr);
    pthread_rwlock_init(&myLockFile, nullptr);

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
    return 0;
}
