#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

void consumer() {
    while (1) {
        printf("I'm the consumer process\n");
        sleep(1);
    }
}

void producer() {
    while (1) {
        printf("I'm the producer process\n");
        sleep(1);
    }
}

int main() {
    pid_t producer_pid, consumer_pid;

    producer_pid = fork();
    
    if (producer_pid == 0) {
        producer();
    }

    consumer_pid = fork();
            
    if (consumer_pid == 0) {
        consumer();
    }

    printf("both children have been created\n");

    while(1) {
        pause(); // keep the application alive
    }

    return 0;
}
