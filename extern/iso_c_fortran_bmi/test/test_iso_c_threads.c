/* Concurrency regression test for the iso_c_bmi shim's C-to-Fortran string
 * conversion.
 *
 * Several threads repeatedly call the bind(C) BMI entry points that take a
 * variable name, using names of differing lengths.  Every call must return the
 * value that the test model documents for that name.  A shared piece of state
 * in the name conversion shows up here as a valid name being reported as
 * unknown (BMI_FAILURE), or as a value belonging to some other name.
 *
 * Each thread drives its own model handle, so the only state under test is the
 * shim's own.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int register_bmi(void *);
extern int initialize(void *, char *);
extern int finalize(void *);
extern int get_var_grid(void *, char *, int *);
extern int get_var_itemsize(void *, char *, int *);

#define BMI_SUCCESS 0
#define BMI_FAILURE 1

#define NAME_BUF_LEN 2048

#define DEFAULT_NUM_THREADS 32
#define DEFAULT_NUM_ITERS 200000

struct var_case {
    const char *name;
    int grid;
    int itemsize;
};

/* name          | grid | itemsize */
static const struct var_case VAR_CASES[] =
{
      { "INPUT_VAR_1"  ,    0 ,        8 }
    , { "INPUT_VAR_2"  ,    0 ,        4 }
    , { "INPUT_VAR_3"  ,    0 ,        4 }
    , { "GRID_VAR_1"   ,    1 ,        8 }
    , { "OUTPUT_VAR_1" ,    0 ,        8 }
    , { "OUTPUT_VAR_2" ,    0 ,        4 }
    , { "OUTPUT_VAR_3" ,    0 ,        4 }
    , { "GRID_VAR_2"   ,    1 ,        4 }
    , { "GRID_VAR_3"   ,    2 ,        8 }
    , { "GRID_VAR_4"   ,    2 ,        8 }
};

static const int NUM_VAR_CASES = (int)(sizeof(VAR_CASES) / sizeof(VAR_CASES[0]));

struct worker_args {
    void *handle;
    long iters;
    long bad_status;
    long bad_value;
};

static void *worker(void *raw)
{
    struct worker_args *args = (struct worker_args *)raw;
    char name[NAME_BUF_LEN];
    long i;

    for (i = 0; i < args->iters; i++) {
        const struct var_case *vc = &VAR_CASES[i % NUM_VAR_CASES];
        int grid = -99;
        int itemsize = -99;
        int status;

        memset(name, 0, sizeof(name));
        strcpy(name, vc->name);

        status = get_var_grid(&args->handle, name, &grid);
        if (status != BMI_SUCCESS) {
            args->bad_status++;
        } else if (grid != vc->grid) {
            args->bad_value++;
        }

        status = get_var_itemsize(&args->handle, name, &itemsize);
        if (status != BMI_SUCCESS) {
            args->bad_status++;
        } else if (itemsize != vc->itemsize) {
            args->bad_value++;
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int num_threads = DEFAULT_NUM_THREADS;
    long num_iters = DEFAULT_NUM_ITERS;
    char init_file[NAME_BUF_LEN];
    pthread_t *threads;
    struct worker_args *args;
    long total_bad_status = 0;
    long total_bad_value = 0;
    long total_calls;
    int t;

    if (argc > 1) {
        num_threads = atoi(argv[1]);
    }
    if (argc > 2) {
        num_iters = atol(argv[2]);
    }
    if (num_threads < 1 || num_iters < 1) {
        fprintf(stderr, "usage: %s [num_threads] [num_iters]\n", argv[0]);
        return 2;
    }

    memset(init_file, 0, sizeof(init_file));
    strcpy(init_file, "test_bmi_fortran_config_0.txt");

    threads = malloc(sizeof(pthread_t) * (size_t)num_threads);
    args = calloc((size_t)num_threads, sizeof(struct worker_args));
    if (threads == NULL || args == NULL) {
        fprintf(stderr, "allocation failed\n");
        return 2;
    }

    /* Register and initialize every handle before any thread starts, so that
     * only the per-call paths run concurrently. */
    for (t = 0; t < num_threads; t++) {
        int status = register_bmi(&args[t].handle);
        if (status != BMI_SUCCESS) {
            fprintf(stderr, "register_bmi failed for handle %d\n", t);
            return 2;
        }
        status = initialize(&args[t].handle, init_file);
        if (status != BMI_SUCCESS) {
            fprintf(stderr, "initialize failed for handle %d\n", t);
            return 2;
        }
        args[t].iters = num_iters;
    }

    for (t = 0; t < num_threads; t++) {
        if (pthread_create(&threads[t], NULL, worker, &args[t]) != 0) {
            fprintf(stderr, "pthread_create failed for thread %d\n", t);
            return 2;
        }
    }

    for (t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
        total_bad_status += args[t].bad_status;
        total_bad_value += args[t].bad_value;
    }

    for (t = 0; t < num_threads; t++) {
        finalize(&args[t].handle);
    }

    total_calls = (long)num_threads * num_iters * 2;
    printf("threads: %d, iterations per thread: %ld, calls: %ld\n",
           num_threads, num_iters, total_calls);
    printf("unexpected BMI_FAILURE: %ld\n", total_bad_status);
    printf("wrong result value: %ld\n", total_bad_value);

    free(args);
    free(threads);

    if (total_bad_status != 0 || total_bad_value != 0) {
        printf("test_iso_c_threads: FAILURE\n");
        return 1;
    }

    printf("test_iso_c_threads: SUCCESS\n");
    return 0;
}
