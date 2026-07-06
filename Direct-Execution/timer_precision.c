#include <stdio.h>
#include <sys/time.h>

// How precise is gettimeofday()? Two experiments:
//   1) back-to-back calls: what's the smallest non-zero delta we ever see?
//   2) spin until the reported time actually changes, and count how many
//      calls that took -- that's the real "tick" granularity of the clock.

// int main(void) {
//     const int trials = 1000000;
//     long min_nonzero_delta = -1;
//     long zero_count = 0;

//     struct timeval prev, cur;
//     gettimeofday(&prev, NULL);
//     for (int i = 0; i < trials; i++) {
//         gettimeofday(&cur, NULL);
//         long delta = (cur.tv_sec - prev.tv_sec) * 1000000L + (cur.tv_usec - prev.tv_usec);
//         if (delta == 0) {
//             zero_count++;
//         } else if (delta > 0 && (min_nonzero_delta < 0 || delta < min_nonzero_delta)) {
//             min_nonzero_delta = delta;
//         }
//         prev = cur;
//     }

//     printf("back-to-back gettimeofday(): %ld/%d calls returned the same value as the previous call\n",
//            zero_count, trials);
//     printf("smallest observed nonzero delta between back-to-back calls: %ld us\n", min_nonzero_delta);

//     // second experiment: how many calls does it take for the clock to
//     // visibly advance from a fixed starting reading?
//     struct timeval start, now;
//     gettimeofday(&start, NULL);
//     now = start;
//     long iterations = 0;
//     while (now.tv_sec == start.tv_sec && now.tv_usec == start.tv_usec) {
//         gettimeofday(&now, NULL);
//         iterations++;
//     }
//     long tick_us = (now.tv_sec - start.tv_sec) * 1000000L + (now.tv_usec - start.tv_usec);
//     printf("clock advanced by %ld us after %ld calls (tick granularity)\n", tick_us, iterations);

//     return 0;
// }


int main(void) {
    
    const int trials = 1000000;
    long min_zero_delta = -1;

    long zero_count = 0;

    struct timeval prev, cur;

    gettimeofday(&prev, NULL);

    for (int i = 0; i < trials; i++) {
        gettimeofday(&cur, NULL);
        long delta = (cur.tv_sec - prev.tv_sec) * 1000000L + (cur.tv_usec - prev.tv_usec);
        if ( delta == 0) {
            zero_count++;
        } else if (delta > 0 && (min_zero_delta < 0 || delta < min_zero_delta)) {
            min_zero_delta = delta;
        }
        prev = cur;
    }

    printf("back-to-back gettimeofday(): %ld/%d calls returned the same value as the previous call\n",
           zero_count, trials);
    printf("smallest observed nonzero delta between back-to-back calls: %ld us\n", min_zero_delta);


    struct timeval start, now;
    gettimeofday(&start, NULL);
    now = start;
    long iterations = 0;
    while (now.tv_sec == start.tv_sec && now.tv_usec == start.tv_usec) {
        gettimeofday(&now, NULL);
        iterations++;
    }
    long tick_us = (now.tv_sec - start.tv_sec) * 1000000L + (now.tv_usec - start.tv_usec);
    printf("clock advanced by %ld us after %ld calls (tick granularity)\n", tick_us, iterations);
    

    return 0;
}