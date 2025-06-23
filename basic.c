#define _GNU_SOURCE // enables all available features/extensions
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <limits.h>
#include <stdlib.h>

int futex_wait(volatile int *addr, int val) {
    return syscall(SYS_futex, addr, FUTEX_WAIT, val, NULL, NULL, 0);
}

int futex_wake(volatile int *addr, int n) {
    return syscall(SYS_futex, addr, FUTEX_WAKE, n, NULL, NULL, 0);
}

void consumer(volatile int *flag) {
    int pid = getpid();
    printf("I'm the consumer process with pid %d\n", pid);
    while (1) {
        // sleep if flag is 0 and wait for wake
        futex_wait(flag, 0);

        // FD_ATOMIC_CAS()
        if (__sync_val_compare_and_swap(flag, 1, 0) == 1) {
            printf("Consumer %d: got work !!!! flag is %d\n", pid, *flag);
        }     
    }
}

void producer(volatile int *flag) {
    while (1) {

        // FD_ATOMIC_ADD_AND_FETCH
        // __sync_add_and_fetch(flag, 1);
        // *flag += 1;

        // NOT doing producer synchronization yet
        // FD_ATOMIC_XCHG(p,v)  
        // *flag = 1;
        __sync_lock_test_and_set( (flag), (1) );

        // wake up ALL consumers
        futex_wake(flag, INT_MAX);
        printf("Produced a value, flag is now %d\n", *flag);
        sleep(1);
    }
}

int main() {
    volatile int *flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *flag = 0;

    pid_t producer_pid, consumer_pid;

    producer_pid = fork();
    
    if (producer_pid == 0) {
        producer(flag);
    }

    consumer_pid = fork();
    consumer_pid = fork();
    consumer_pid = fork();

    if (consumer_pid == 0) {
        consumer(flag);
    }

    while(1) {
        pause(); // keep the application alive
    }

    return 0;
}
