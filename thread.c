#include <stdio.h>
#include <pthread.h>

int num[7];
double avg;
int min;
int max;

void *cal_avg(void *arg) {
    int sum = 0;
    for (int i = 0; i < 7; i++) {
        sum += num[i];
    }
    avg = (double)sum / 7;
    pthread_exit(0);
}

void *cal_min(void *arg) {
    min = num[0];
    for (int i = 1; i < 7; i++) {
        if (num[i] < min)
            min = num[i];
    }
    pthread_exit(0);
}

void *cal_max(void *arg) {
    max = num[0];
    for (int i = 1; i < 7; i++) {
        if (num[i] > max)
            max = num[i];
    }
    pthread_exit(0);
}

void main() {
    printf("Enter 7 integers:\n");
    for (int i = 0; i < 7; i++) {
        scanf("%d", &num[i]);
    }

    pthread_t tid_avg, tid_min, tid_max;

    pthread_create(&tid_avg, NULL, cal_avg, NULL);
    pthread_create(&tid_min, NULL, cal_min, NULL);
    pthread_create(&tid_max, NULL, cal_max, NULL);

    pthread_join(tid_avg, NULL);
    pthread_join(tid_min, NULL);
    pthread_join(tid_max, NULL);

    printf("The average value is %.0f\n", avg);
    printf("The minimum value is %d\n", min);
    printf("The maximum value is %d\n", max);
}
