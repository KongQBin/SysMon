#include "rbtree.h"

struct rb_root g_cbtree = RB_ROOT;
struct syscall
{
    struct rb_node node;
    int id;
    void *func;
};

struct syscall *cb_search(int id);
int cb_insert(struct syscall *data);

