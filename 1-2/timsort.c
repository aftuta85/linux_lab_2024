#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "list.h"
#include "sort_impl.h"

static inline size_t run_size(struct list_head *head)
{
    if (!head)
        return 0;
    if (!head->next)
        return 1;
    return (size_t) (head->next->prev);
}

struct pair {
    struct list_head *head, *next;
};

static size_t stk_size;
static size_t min_run;

static struct list_head *merge(void *priv,
                               list_cmp_func_t cmp,
                               struct list_head *a,
                               struct list_head *b)
{
    struct list_head *head;
    struct list_head **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            *tail = a;
            tail = &(*tail)->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &(*tail)->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

static void build_prev_link(struct list_head *head,
                            struct list_head *tail,
                            struct list_head *list)
{
    tail->next = list;
    do {
        list->prev = tail;
        tail = list;
        list = list->next;
    } while (list);

    /* The final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}

static void merge_final(void *priv,
                        list_cmp_func_t cmp,
                        struct list_head *head,
                        struct list_head *a,
                        struct list_head *b)
{
    struct list_head *tail = head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    build_prev_link(head, tail, b);
}

static struct list_head *get_list_mid(struct list_head *start,
                                      struct list_head *end)
{
    struct list_head **indir = &start;
    for (struct list_head *fast = start->next; fast != end && fast->next != end;
         fast = fast->next->next)
        indir = &(*indir)->next;
    return *indir;
}

static struct pair binary_insert_run(void *priv,
                                     struct list_head *head,
                                     struct list_head *tail,
                                     struct list_head *new_node,
                                     list_cmp_func_t cmp)
{
    struct pair result;
    struct list_head *left = head, *right = tail, *prev = head;
    while (left != right) {
        struct list_head *mid = get_list_mid(left, right);
        if (cmp(priv, mid, new_node) > 0) {
            right = mid;
        } else {
            prev = mid;
            left = mid->next;
        }
    }

    // The new_node is the smallest in the list
    if (left == head) {
        new_node->prev = prev->prev;
        new_node->next = prev;
        prev->prev = new_node;
        result.head = new_node;
        result.next = tail;
        return result;
    }
    // The new_node is the largest in the list
    if (left == tail && cmp(priv, tail, new_node) <= 0) {
        new_node->next = tail->next;
        tail->next = new_node;
        new_node->prev = tail;
        tail = new_node;
        result.head = head;
        result.next = tail;
        return result;
    }

    struct list_head *next = prev->next;
    new_node->prev = next->prev;
    new_node->next = next;
    prev->next = new_node;
    if (next)
        next->prev = new_node;
    result.head = head;
    result.next = tail;
    return result;
}

static struct pair find_run(void *priv,
                            struct list_head *list,
                            list_cmp_func_t cmp)
{
    size_t len = 1;
    struct list_head *next = list->next, *head = list, *tail = head,
                     *tmp_next = NULL;
    struct pair result;
    struct pair bir_result;

    if (!next) {
        result.head = head, result.next = next;
        return result;
    }

    if (cmp(priv, list, next) > 0) {
        /* decending run, also reverse the list */
        struct list_head *prev = NULL;
        do {
            len++;
            list->next = prev;
            prev = list;
            list = next;
            next = list->next;
            head = list;
        } while (next && cmp(priv, list, next) > 0);
        list->next = prev;
        while (next && len < min_run) {
            len++;
            tmp_next = next;
            next = next->next;
            bir_result = binary_insert_run(priv, head, tail, tmp_next, cmp);
            head = bir_result.head, tail = bir_result.next;
            list = head;
        }
    } else {
        do {
            len++;
            list = next;
            next = list->next;
        } while (next && cmp(priv, list, next) <= 0);
        list->next = NULL;
        while (next && len < min_run) {
            len++;
            tmp_next = next;
            next = next->next;
            tail = list;
            bir_result = binary_insert_run(priv, head, tail, tmp_next, cmp);
            head = bir_result.head, tail = bir_result.next;
            list = tail;
        }
    }
    head->prev = NULL;
    head->next->prev = (struct list_head *) len;
    result.head = head, result.next = next;
    return result;
}

static struct list_head *merge_at(void *priv,
                                  list_cmp_func_t cmp,
                                  struct list_head *at)
{
    size_t len = run_size(at) + run_size(at->prev);
    struct list_head *prev = at->prev->prev;
    struct list_head *list = merge(priv, cmp, at->prev, at);
    list->prev = prev;
    list->next->prev = (struct list_head *) len;
    --stk_size;
    return list;
}

static struct list_head *merge_force_collapse(void *priv,
                                              list_cmp_func_t cmp,
                                              struct list_head *tp)
{
    while (stk_size >= 3) {
        if (run_size(tp->prev->prev) < run_size(tp)) {
            tp->prev = merge_at(priv, cmp, tp->prev);
        } else {
            tp = merge_at(priv, cmp, tp);
        }
    }
    return tp;
}

static struct list_head *merge_collapse(void *priv,
                                        list_cmp_func_t cmp,
                                        struct list_head *tp)
{
    int n;
    while ((n = stk_size) >= 2) {
        if ((n >= 3 &&
             run_size(tp->prev->prev) <= run_size(tp->prev) + run_size(tp)) ||
            (n >= 4 && run_size(tp->prev->prev->prev) <=
                           run_size(tp->prev->prev) + run_size(tp->prev))) {
            if (run_size(tp->prev->prev) < run_size(tp)) {
                tp->prev = merge_at(priv, cmp, tp->prev);
            } else {
                tp = merge_at(priv, cmp, tp);
            }
        } else if (run_size(tp->prev) <= run_size(tp)) {
            tp = merge_at(priv, cmp, tp);
        } else {
            break;
        }
    }

    return tp;
}

static int get_data_length(const struct list_head *const head)
{
    struct list_head *curr = NULL;
    int count = 0;
    list_for_each (curr, head)
        count++;
    return count;
}

static void set_minrun(int data_length)
{
    if (data_length < 64) {
        min_run = data_length;
        return;
    }

    int zeros = __builtin_clz(data_length);
    int shift = (sizeof(data_length) * CHAR_BIT) - zeros - 6;
    int remain = !!(~((~0) << shift) & data_length);
    min_run = (data_length >> shift) + remain;
}

void timsort(void *priv, struct list_head *head, list_cmp_func_t cmp)
{
    stk_size = 0;
    int data_length = get_data_length(head);
    set_minrun(data_length);

    struct list_head *list = head->next, *tp = NULL;
    if (head == head->prev)
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    do {
        /* Find next run */
        struct pair result = find_run(priv, list, cmp);
        result.head->prev = tp;
        tp = result.head;
        list = result.next;
        stk_size++;
        tp = merge_collapse(priv, cmp, tp);
    } while (list);

    /* End of input; merge together all the runs. */
    tp = merge_force_collapse(priv, cmp, tp);

    /* The final merge; rebuild prev links */
    struct list_head *stk0 = tp, *stk1 = stk0->prev;
    while (stk1 && stk1->prev)
        stk0 = stk0->prev, stk1 = stk1->prev;
    if (stk_size <= 1) {
        build_prev_link(head, head, stk0);
        return;
    }
    merge_final(priv, cmp, head, stk1, stk0);
}