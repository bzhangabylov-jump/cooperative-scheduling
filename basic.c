#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

void consumer(volatile int *flag) {
    while (1) {
        printf("I'm the consumer process\n");
        if (*flag > 0) {
            *flag -= 1;
            printf("Consumed a value, flag is now %d\n", *flag);
        }
        sleep(1);
    }
}

void producer(volatile int *flag) {
    while (1) {
        printf("I'm the producer process\n");
        *flag = 1;
        printf("Produced a value, flag is now %d\n", *flag);
        sleep(5);
    }
}

int main() {
    volatile int *flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *flag = 0;
    printf("flag has been defined %d\n", *flag);
    printf("flag address is %p\n", (void *) flag);
    pid_t producer_pid, consumer_pid;

    producer_pid = fork();
    
    if (producer_pid == 0) {
        producer(flag);
    }

    consumer_pid = fork();
            
    if (consumer_pid == 0) {
        consumer(flag);
    }

    printf("both children have been created\n");

    while(1) {
        pause(); // keep the application alive
    }

    return 0;
}
