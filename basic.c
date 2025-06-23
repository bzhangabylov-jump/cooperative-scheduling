#define _GNU_SOURCE // enables all available features/extensions
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>

int futex_wait(volatile int *addr, int val) {
    // syscall(SYS_futex, uint32_t *uaddr, int futex_op, uint32_t val,
    //                 const struct timespec *timeout,   /* or: uint32_t val2 */
    //                 uint32_t *uaddr2, uint32_t val3);
    return syscall(SYS_futex, addr, FUTEX_WAIT, val, NULL, NULL, 0);
}

int futex_wake(volatile int *addr, int n) {
    return syscall(SYS_futex, addr, FUTEX_WAKE, n, NULL, NULL, 0);
}

int futex_waitv(struct futex_waitv *waiters, unsigned int nr_futexes,
                unsigned int flags) {
    // flags needs to be 0, timeout is 0, none
    // futex_waitv(struct futex_waitv *waiters, unsigned int nr_futexes,
            // unsigned int flags, struct timespec *timeout, clockid_t clockid)
    return syscall(SYS_futex_waitv, waiters, nr_futexes, 0, NULL, 0);
}

void consumer(volatile int *futex) {
    int pid = getpid();
    printf("I'm the consumer process with pid %d\n", pid);
    while (1) {
        // sleep if futex is 0 and wait for wake
        futex_wait(futex, 0);

        // FD_ATOMIC_CAS()
        if (__sync_val_compare_and_swap(futex, 1, 0) == 1) {
            printf("Consumer %d: got work !!!! futex is %d\n", pid, *futex);
        }     
    }
}

void consumerNFutexes(volatile int **futexes, int num_futexes) {
    int pid = getpid();
    printf("I'm the consumer process with %d futexes and pid %d\n", num_futexes, pid);

    struct futex_waitv waiters[num_futexes];
    for (int i = 0; i < num_futexes; i++) {
        waiters[i].uaddr = (uintptr_t) (futexes[i]);
        waiters[i].val = 0;
        // waiters[i].flags = FUTEX2_SIZE_U64;
        waiters[i].flags = FUTEX_32; // says do not use?
        waiters[i].__reserved = 0;
    }

    while (1) {
        // wait for any futex to be set to 1
        int ret = futex_waitv(waiters, num_futexes, 0);
        if (ret >= 0) {
            volatile int *futex = futexes[ret];
            if (__sync_val_compare_and_swap(futex, 1, 0) == 1) {
                printf("Consumer %d: got work from futexes[%d] !!!! futexes[%d] is %d\n", pid, ret, ret, *futex);
            }
        } else if (ret == -1 && errno == EAGAIN) {
            printf("One or more futexes are non-zero, checking futex values...\n");
            for (int i = 0; i < num_futexes; i++) {
                volatile int *futex = futexes[i];
                if (*futex != 0) {
                    if (__sync_val_compare_and_swap(futex, 1, 0) == 1) {
                        printf("Consumer %d: got work from futexes[%d] !!!! futexes[%d] is %d\n", pid, i, i, *futex);
                    }
                }
            }
        } else {
            perror("futex_waitv failed some other way");
            printf("Error code: %d\n", ret);
            printf("errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }
}

void producer(volatile int *futex, int index) {
    while (1) {

        // FD_ATOMIC_ADD_AND_FETCH
        // __sync_add_and_fetch(futex, 1);
        // *futex += 1;

        // FD_ATOMIC_XCHG(p,v)  
        // *futex = 1;
        __sync_lock_test_and_set( (futex), (1) );

        // wake up ALL consumers
        futex_wake(futex, INT_MAX);
        printf("Produced a value, futex (index %d) is now %d\n", index, *futex);
        sleep(5);
    }
}

void oneFutexCase() {
    volatile int *futex = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *futex = 0;

    pid_t producer_pid, consumer_pid;

    producer_pid = fork();
    if (producer_pid == 0) {
        producer(futex, 0);
    }

    consumer_pid = fork();
    consumer_pid = fork();
    consumer_pid = fork();
    if (consumer_pid == 0) {
        consumer(futex);
    }

    while(1) {
        pause(); // keep the application alive
    }
}

void nFutexCase() {
    int num_futexes = 2;
    volatile int *futexes[num_futexes];

    for (int i = 0; i < num_futexes; i++) {
        futexes[i] = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    }

    for (int i = 0; i < num_futexes; i++) {
        *futexes[i] = 0;
    }

    for (int i = 0; i < num_futexes; i++) {
        int producer_pid = fork();
        if (producer_pid == 0) {
            producer(futexes[i], i);
        }
    }

    int consumer_id = fork();
    if (consumer_id == 0) {
        consumerNFutexes(futexes, num_futexes);
    }

    while(1) {
        pause();
    }
}

int main() {
    // oneFutexCase();
    nFutexCase();

    return 0;
}
