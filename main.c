#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <errno.h>

/* file 1                   file 4
editor    5   0.87          biggie    20    0.99
compiler  40  0.53          nextone   10    0.99
adventure 30  0.72
*/

struct cpu {
    char *status;
    int stop_run; //time at which the currently running job should stop running

};

struct io {
    char *status;
    int stop_run;

};

struct job {
    char name[10]; // name no more than 10 chars
    //int priority;

    float prob_block;
    int time_to_run; //the time it is to run
    int time_remain; //how much run time remains

    int time_completed; // the time at which it was completed
    int given_cpu; // number of times given the cpu
    int io_blocks; // number of times the process is blocked for i/o
    int time_io;  // time spent doing i/o


};

int main(int argc, char *argv[]) {
    /*
    int n = 10;
    (void) srand(12345);
    for (int i=0; i < n; i++) {
      int r = rand();
      printf("%d\n", r);
      printf("f: %f\n", (double)r / RAND_MAX);
      printf("in: %d\n", (r % 5 + 1));
      printf("in: %d\n", (r % 30 + 1));
    }
    */

    // Opens the file given
    FILE *fptr;
    if ((fptr = fopen(argv[2],"r")) == NULL){
        printf("Error opening file\n");

        // Program exits if the file pointer returns NULL.
        exit(1);
    }

    // Creating string variables
    const unsigned MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];
    char copyValue[MAX_LENGTH];


    // Creating an array of structs
    int arrLimit = 50;
    struct job arr_job[arrLimit];
    int readCounter = 0;
    int jobCounter = 0;

    // Cycles through file word by word and fills out struct for array. readcounter keeps track of each of which part of the line it is on. 1=name, 2=time, 3=prob_block
    while (fscanf(fptr, "%1023s", buffer) == 1){
        readCounter++;
        if (readCounter == 1){
            strcpy(arr_job[jobCounter].name, buffer);
            arr_job[jobCounter].time_completed = -1;
            arr_job[jobCounter].given_cpu = 0;
            arr_job[jobCounter].io_blocks = 0;
            arr_job[jobCounter].time_io = 0;
        }
        if (readCounter == 2){
            arr_job[jobCounter].time_to_run = atoi(buffer);
            arr_job[jobCounter].time_remain = atoi(buffer);
        }
        if (readCounter == 3){
            arr_job[jobCounter].prob_block = atof(buffer);
            readCounter = 0;
            jobCounter++;
        }

    }

    // close the file
    fclose(fptr);

    int cur_time = 0;
    struct cpu *cpu1 = malloc(sizeof(*cpu1));
    struct io *io1 = malloc(sizeof(*io1));
    cpu1->status = "idle";
    io1->status = "idle";
    cpu1->stop_run = -1;
    io1->stop_run = -1;


    queue_t readyq;
    readyq = queue_create();
    queue_t ioq;
    ioq = queue_create();

    struct job *cpu = NULL;
    struct job *iodev = NULL;
    int tick = 1;     // changed from 1 to 0 bc it was one tick ahead

    /* for process information */
    for (int i = 0; i < jobCounter; i++) {
        queue_enqueue(readyq, &arr_job[i]);
    }

    (void) srand(12345);
    while (cpu!= NULL || iodev!=NULL || queue_length(readyq) > 0 || queue_length(ioq) > 0) {
        struct job* ptr;
        int same_tick = 0;


        if (strcmp(cpu1->status, "idle") == 0 && (queue_length(readyq) > 0)) {
            queue_dequeue(readyq, (void**)&ptr);
            cpu = ptr;
            cpu->given_cpu++;  // cpu disp stat (increment for when given cpu)
            cpu1->status = "active";


            if (cpu->time_remain > 2) {
                int r = rand();

                if (cpu->prob_block > (double)r / RAND_MAX ) {
                    //block for i/o

                    int r = rand();

                    cpu1->stop_run = tick + (r % cpu->time_remain + 1) - 1;

                }
            } else {
                if (cpu->time_remain == 0) {
                    //printf("PRINT stuff\n");
                    cpu->time_completed = tick; // when done stat
                    //printf("process ended? 1\n");
                    cpu = NULL;
                    cpu1->status = "idle";
                }
            }
        } else if (cpu != NULL) {
            cpu->time_remain--;
            //printf("%d tick cpu with remianing runtime %d\n",tick, cpu->time_remain);
            if (cpu->time_remain == 0) {
                //printf("PRINT stuff\n");
                cpu->time_completed = tick; // when done stat
                //printf("process ended? 2\n");
                cpu = NULL;
                cpu1->status = "idle";
            }
        }

        if (tick == cpu1->stop_run && cpu->time_remain > 0) {
            queue_enqueue(ioq, cpu);
            cpu = NULL;
            cpu1->status = "idle";
            same_tick = 1;
        }




        if (queue_length(ioq) > 0 || iodev != NULL) {
            if (same_tick == 1) {
                same_tick = 0;
                if (strcmp(io1->status, "active") == 0){
                    iodev->time_io++;  // i/o time stat
                }
            } else if (queue_length(ioq) > 0 || iodev != NULL) {
                if (strcmp(io1-> status, "idle") == 0) {
                    queue_dequeue(ioq, (void**)&ptr);
                    iodev = ptr;
                    iodev->io_blocks++; // i/o disp stat (increment for when blocked for i/o)
                    io1->status = "active";
                    iodev->time_remain--;
                }

                if (strcmp(io1->status, "active") == 0){
                    iodev->time_io++; // i/o time stat  again
                }

                if (io1->stop_run == -1) {
                    //random
                    //i/o in i/o
                    if (iodev->time_remain == 0) {
                        io1->stop_run = tick + 1;
                    } else {
                        int r = rand();
                        io1->stop_run = tick + (r % 30 + 1) - 1;
                    }

                }

                if (tick == io1->stop_run) {
                    queue_enqueue(readyq, iodev);
                    iodev = NULL;
                    io1->status = "idle";
                    io1->stop_run = -1;
                }



                //printf("%d tick   has %d time units on i/o device\n", tick, io1->stop_run - tick);
            }
        }



        tick++;
    }

    /*
       * printing process information
       */

    printf("Program output (to stdout):\n------------------\n");
    /* header line */
    printf("Processes:\n\n");
    printf("   name     CPU time  when done  cpu disp  i/o disp  i/o time\n"
    );
    /* for process information */
    for (int i = 0; i < jobCounter; i++) {
        printf("%-10s %6d     %6d    %6d    %6d    %6d\n", arr_job[i].name, arr_job[i].time_to_run,
               arr_job[i].time_completed, arr_job[i].given_cpu, arr_job[i].io_blocks, arr_job[i].time_io);
    }

    /* print clock time at end */
    printf("\nSystem:\n");
    printf("The wall clock time at which the simulation finished: %d\n", tick);

    /* CPU and IO info */
    int cpu_busy = 0;
    int cpu_idle = 0;
    int cpu_dispatch = 0;
    int io_busy = 0;
    int io_idle = 0;
    int io_dispatch = 0;

    for (int i = 0; i < jobCounter; i++) {
        cpu_busy += arr_job[i].time_to_run;
        cpu_dispatch += arr_job[i].given_cpu;
        io_busy += arr_job[i].time_io;
        io_dispatch += arr_job[i].io_blocks;
    }

    int total_time = tick-1;
    io_idle = total_time-io_busy;
    cpu_idle = total_time-cpu_busy;

    /* print cpu statistics */     //need to edit cpu stats and i/o stats to incorporate a variable amount of processes
    printf("\nCPU:\n");
    printf("Total time spent busy: %d\n", cpu_busy);
    printf("Total time spent idle: %d\n", cpu_idle);
    printf("CPU utilization: %.2f\n", (double)((double)cpu_busy/(double)total_time));
    printf("Number of dispatches: %d\n", cpu_dispatch);
    printf("Overall throughput: %.2f\n", (double)(3/(double)total_time));  // need to change to include variable amount of processes

    /* print i/o statistics */
    printf("\nI/O device:\n");
    printf("Total time spent busy: %d\n", io_busy);
    printf("Total time spent idle: %d\n", io_idle);
    printf("I/O utilization: %.2f\n", (double)((double)io_busy/(double)total_time));
    printf("Number of dispatches: %d\n", io_dispatch);
    printf("Overall throughput: %.2f\n", (double)(3/(double)total_time)); // need to change to include variable amount of processes

    /*
     * error messages
     * the arguments for %s(%d) are file name and line number,
     *     respectively
     * all cause exit with status code 1 (ie, exit(1))
     */
    printf("\n\n\nProgram error output (to stderr):\n------------------\n");
    /* bad option (not -f or -r) */
    fprintf(stderr, "Usage: %s [-r | -f] file\n", argv[0]);
    /* can't open file */
    errno = ENOENT;
    perror("filename");
    /* the line in the input file is malformed */
    fprintf(stderr, "Malformed line %s(%d)\n", "filename", 100);
    /* process name is to long (over 10 characters) */
    fprintf(stderr, "name is too long %s(%d)\n", "filename", 100);
    /* runtime is 0 or less */
    fprintf(stderr, "runtime is not positive integer %s(%d)\n", "filename", 100);
    /* probability is not between 0 and 1 */
    fprintf(stderr, "probability < 0 or > 1 %s(%d)\n", "filename", 100);






    return 0;
}