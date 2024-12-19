#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Thread {
    SDL_Thread *thread;
    void (*task)(void *);
    void *task_data;
} Thread;

void _RT_kill(void *data) {
    // you are WORTHLESS
    // you serve ZERO purpose
    // you should kill yourself, NOW
}

void _RT_Thread(void *data) {
    Thread *thread = data;
    while (true) {
        if (thread->task != NULL) {

            if (thread->task == _RT_kill) {
                thread->task = NULL;
                thread->task_data = NULL;
                break;
            }

            thread->task(thread->task_data);
            thread->task = NULL;
            thread->task_data = NULL;
        }
    }
}

Thread *RT_alloc_thread() {
    Thread *thread = malloc(sizeof(Thread));
    thread->task = NULL;
    thread->thread = SDL_CreateThread(_RT_Thread, "RT thread", thread);

    return thread;
}

void RT_activate_thread(Thread *thread, void (*task)(void *), void *task_data) {
    thread->task_data = task_data;
    thread->task = task;
}

void RT_wait_thread(Thread *thread) {
    while (thread->task != NULL) {
        //    wait 
        //     :)
    }
}

void RT_free_thread(Thread *thread) {
    RT_activate_thread(thread->thread, _RT_kill, NULL);
    SDL_DetachThread(thread->thread);

    free(thread);
    
}

