/* 
 * Single semaphore for producer & consumer solution
 * Copyfree, please modify to meet your needs.
 * Rhino, 2019-7-22, China
 * vim: sw=4 ts=4 expandtab
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

// Define your max slots here
#define N 100
#define PRODUCER_COUNT  3
#define CONSUMER_COUNT  10

struct task_t {
    int data;
};

struct unisem_t {
    pthread_mutex_t mutex;
    sem_t sem;
    struct task_t * slots[N];
    int head;
    int tail;
    volatile int n;
    volatile int run;
    pthread_t producers[PRODUCER_COUNT];
    pthread_t consumers[CONSUMER_COUNT];
};

typedef struct unisem_t unisem_t;
typedef struct task_t task_t;
void * producer(void *);
void * consumer(void *);
int wait(sem_t * sem, int ms);

// constructor
void unisem_init(unisem_t * ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->run = 1;
    ctx->head = 0;
    ctx->tail = 0;

    srand(time(NULL));
    pthread_mutex_init(&ctx->mutex, NULL);
    sem_init(&ctx->sem, 0, 0);

    // producers
    for (int i = 0; i < PRODUCER_COUNT; i++) 
        pthread_create(&ctx->producers[i], NULL, producer, ctx);

    // consumers
    for (int i = 0; i < CONSUMER_COUNT; i++) 
        pthread_create(&ctx->consumers[i], NULL, consumer, ctx);
}

int unisem_push(unisem_t * ctx, task_t * task)
{
    int ret = -1;
    long unsigned id = pthread_self();

    // wait for a slot with 10ms, 
    // 10ms is the OS scheduling time slice. In practical,
    // the producers are faster than consumers, so a busy 
    // wait would suffice.
    while (ctx->run && (ctx->n == N))
        usleep(10*1000);

    pthread_mutex_lock(&ctx->mutex);
    if (ctx->n < N) {
        // enqueue
        assert(ctx->slots[ctx->head] == NULL);
        printf("+ producer %lX push into slot #%d, data = %d, n = %d\n", id, ctx->head, task->data, ctx->n);
        ctx->slots[ctx->head] = task;
        ctx->head = (ctx->head + 1) % N;
        // signal
        ++ctx->n;
        sem_post(&ctx->sem);
        ret = 0;
        assert(ctx->n <= N);
    }
    pthread_mutex_unlock(&ctx->mutex);

    return (ret);
}

task_t * unisem_pop(unisem_t * ctx, int ms) 
{
    long unsigned id = pthread_self();
    int ret = -1;
    task_t * task = NULL;

    if ((ret = wait(&ctx->sem, ms)) == 0) {
        pthread_mutex_lock(&ctx->mutex);

        // dequeue
        task = ctx->slots[ctx->tail];
        ctx->slots[ctx->tail] = NULL;
        assert(task);
        printf("- consumer %lX pop from slot #%d, data = %d, n = %d\n", id, ctx->tail, task->data, ctx->n);
        ctx->tail = (ctx->tail + 1) % N;

        // signal
        --ctx->n;
        assert(ctx->n >= 0);
        pthread_mutex_unlock(&ctx->mutex);
    }

    return (task);
}

// destructor
void unisem_destroy(unisem_t * ctx)
{

    printf("terminating all threads ...\n");
    ctx->run = 0;

    for (int i = 0; i < CONSUMER_COUNT; i++) 
        pthread_join(ctx->consumers[i], NULL);

    for (int i = 0; i < PRODUCER_COUNT; i++) 
        pthread_join(ctx->producers[i], NULL);

    for (int i = 0; i < N; i++) {
        if (ctx->slots[i]) {
            free(ctx->slots[i]);
            ctx->slots[i] = NULL;
        }
    }

    sem_destroy(&ctx->sem);
}

int wait(sem_t * sem, int ms)
{
    struct timespec ts = {};
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000LL;
    return sem_timedwait(sem, &ts);
}

void * producer(void * param)
{
    task_t * task;
    unisem_t * ctx = (unisem_t *)param;
    long unsigned id = pthread_self();

    printf("producer %lX created\n", id);

    // keep producing random data
    while (ctx->run) {
        task = (task_t *)malloc(sizeof(task_t));
        task->data = rand();
        unisem_push(ctx, task);
#if 0
        // slow it down for demo
        usleep((rand() % 10 + 1)*10*1000);
#endif
    }

    printf("producer %lX terminated!\n", id);

    return (NULL);
}

void process(task_t *task)
{
    // TODO: JUST DO IT!
    (void)task;
}

void * consumer(void * param)
{
    task_t * task;
    unisem_t * ctx = (unisem_t *)param;
    long unsigned id = pthread_self();
    printf("consumer %lX created\n", id);

    while (ctx->run) {
        // dequeue
        if ((task = unisem_pop(ctx, 100)) == NULL)
            continue;
        process(task);
        free(task);
    }

    printf("consumer %lX terminated!\n", id);

    return (NULL);
}

int main(int argc, const char * argv[])
{
    unisem_t ctx = {};
    unisem_init(&ctx);

    printf("press any key to stop\n");
    getchar();

    unisem_destroy(&ctx);
    printf("unisem demo terminated!\n");

    return (0);
}
