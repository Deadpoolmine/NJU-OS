#include "co.h"
#include "stdint.h"
#include "unistd.h"
#include <setjmp.h>
#include <stdlib.h>

#define STACK_SIZE 8192
#define MAX_CO_NUM 128
#define SWITCH_OUT 0
#define SWITCH_IN  1
// https://unix.stackexchange.com/questions/425013/why-do-i-have-to-set-ld-library-path-before-running-a-program-even-though-i-alr

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg)
{
    asm volatile(
#if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
        : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
#else
        "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
        : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
#endif
    );
}

enum co_status {
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
    const char *name;
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;

    enum co_status status;     // 协程的状态
    struct co *waiter;         // 是否有其他协程在等待当前协程
    jmp_buf context;           // 寄存器现场 (setjmp.h)
    uint8_t stack[STACK_SIZE]; // 协程的堆栈
};

struct co_pool {
    struct co *co[MAX_CO_NUM];
    int co_num;
};

struct co *current;
struct co_pool co_pool; 

static inline int manage_co(struct co *co)
{
    for (int i = 0; i < MAX_CO_NUM; ++i) {
        if (co_pool.co[i] == NULL) {
            co_pool.co[i] = co;
            co_pool.co_num++;
            return 0;
        }
    }
    return -1;
}

static inline int unmanage_co(struct co *co)
{
    for (int i = 0; i < MAX_CO_NUM; ++i) {
        if (co_pool.co[i] == co) {
            co_pool.co[i] = NULL;
            co_pool.co_num--;
            return 0;
        }
    }
    return -1;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg)
{
    struct co *co = malloc(sizeof(struct co));
    if (!co)
        return NULL;

    co->name = name;
    co->func = func;
    co->arg = arg;

    co->status = CO_NEW;
    co->waiter = NULL;

    memset(&co->stack, 0, STACK_SIZE);

    manage_co(co);

    stack_switch_call(co->stack + STACK_SIZE, func, (uintptr_t)arg);

    return co;
}

void co_wait(struct co *co)
{
}

void co_yield (void)
{
    int val = setjmp(current->context);
    if (val == SWITCH_OUT) {
        /* save context using setjmp */
        // 
    } else {
        /* context is restored by longjmp, do nothing */
        return;
    }
}

__attribute__((constructor)) co_main_init()
{
    current = co_start("main", NULL, NULL);
    for (int i = 0; i < MAX_CO_NUM; ++i) {
        co_pool.co[i] = NULL;
    }
    co_pool.co_num = 0;
}