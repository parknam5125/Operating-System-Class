#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
//#include <sys/wait.h>

#define SIZE 5

int nums[SIZE] = {0, 1, 2, 3, 4};

int main()
{
    int i;
    pid_t pid;

    pid = fork();

    if (pid == 0) {
        for (i = 0; i < SIZE; i++) {
            nums[i] *= -1;
            printf("CHILD: %d ", nums[i]);  /* LINE X */
        }
        printf("\n");
    }
    else if (pid > 0) {
        wait(NULL);
        for (i = 0; i < SIZE; i++)
            printf("PARENT: %d ", nums[i]);  /* LINE Y */
        printf("\n");
    }

    return 0;
}
