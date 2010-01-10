#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <stdlib.h> /* abort */

#include "threading.h"

#ifdef WIN32
struct thread {
    HANDLE handle;
};

// The callback function needs to respect a non-standard calling convention.
struct run_closure {
    void (*run)(void *data);
    void *data;
};

DWORD WINAPI run_closure_run(LPVOID argument)
{
    struct run_closure *closure = (struct run_closure *)argument;
    void (*run)(void *data) = closure->run;
    void *data = closure->data;
    free(closure);
    run(data);
    return 0;
}
#else
struct thread {
    pthread_t id;
};
#endif

struct thread *thread_start(void* run, void *data)
{
#ifdef WIN32
    struct run_closure *closure;
    HANDLE result;
    struct thread *t;
    
    closure = malloc(sizeof(struct run_closure));
    closure->run = run;
    closure->data = data;
    result = CreateThread(0, 0, run_closure_run, closure, 0, 0);
    if (result == NULL)
        abort();
    t = malloc(sizeof(struct thread));
    if (t == 0) abort();
    t->handle = result;
    return t;
#else
    pthread_t id;
    
    int result = pthread_create(&id, 0, (void *(*)(void *))run, data);
    if (result != 0)
        abort();
    struct thread *t = malloc(sizeof(struct thread));
    if (t == 0) abort();
    t->id = id;
    return t;
#endif
}

void thread_join(struct thread *t)
{
#ifdef WIN32
    DWORD result = WaitForSingleObject(t->handle, INFINITE);
    if (result != WAIT_OBJECT_0) abort();
#else
    int result = pthread_join(t->id, 0);
    if (result != 0) abort();
#endif
    free(t);
}

void thread_dispose(struct thread* t)
{
  free(t);
}

struct lock {
#ifdef WIN32
    CRITICAL_SECTION criticalSection;
    DWORD ownerThreadId;
#else
    pthread_mutex_t mutex;
    pthread_t ownerThreadId;
#endif
};

struct lock *create_lock()
{
    struct lock *lock = malloc(sizeof(struct lock));
#ifdef WIN32
    InitializeCriticalSection(&(lock->criticalSection));
#else
    int result = pthread_mutex_init(&(lock->mutex), 0);
    if (result != 0)
        abort();
#endif
    lock->ownerThreadId = 0;
    return lock;
}

void lock_acquire(struct lock *lock)
{
#ifdef WIN32
    EnterCriticalSection(&(lock->criticalSection));
#else
    pthread_mutex_lock(&(lock->mutex));
#endif
    if (lock->ownerThreadId != 0)
        abort();
#ifdef WIN32
    lock->ownerThreadId = GetCurrentThreadId();
#else
    lock->ownerThreadId = pthread_self();
#endif
}

void lock_release(struct lock *lock)
{
#ifdef WIN32
    if (lock->ownerThreadId != GetCurrentThreadId())
        abort();
    lock->ownerThreadId = 0;
    LeaveCriticalSection(&(lock->criticalSection));
#else
    if (!pthread_equal(lock->ownerThreadId, pthread_self()))
        abort();
    lock->ownerThreadId = 0;
    pthread_mutex_unlock(&(lock->mutex));
#endif
}