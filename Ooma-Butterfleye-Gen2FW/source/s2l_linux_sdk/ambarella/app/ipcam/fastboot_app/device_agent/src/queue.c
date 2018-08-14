/*
 * queue.c
 *
 * History:
 *       2015/03/10 - [jywang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "defs.h"
#include "queue.h"

static void elem_destroy_cb_default(queue_elem_pptr_t pp_elem) {
    MEM_FREE_PPTR(pp_elem);
}

queue_ptr_t queue_create() {
    queue_ptr_t p_queue = MEM_ALLOCZ(queue_t);
    return p_queue;
}

void queue_destroy(queue_pptr_t pp_queue) {
    ASSERT(pp_queue);
    ASSERT(*pp_queue);
    queue_deinit(*pp_queue);
    MEM_FREE_PPTR(pp_queue);
}

int32_t queue_init(queue_ptr_t p_queue) {
    ASSERT(p_queue);
    p_queue->sentinel.p_next = &p_queue->sentinel;
    p_queue->sentinel.p_prev = &p_queue->sentinel;
    p_queue->length = 0;
    return RET_OK;
}

void queue_deinit(queue_ptr_t p_queue) {
    queue_purge(p_queue);
}

void queue_set_elem_destroy_cb(queue_ptr_t p_queue, elem_destroy_cb_t cb) {
    ASSERT(p_queue);
    p_queue->destroy_cb = cb;
}

void queue_enq(queue_ptr_t p_queue, queue_elem_ptr_t p_elem) {
    ASSERT(p_queue);
    ASSERT(p_elem);
    if (p_elem->p_queue == p_queue) {
        return ;
    }

    ASSERT(p_elem->p_queue == NULL);
    p_elem->p_next = p_queue->sentinel.p_next;
    p_elem->p_prev = &p_queue->sentinel;
    p_elem->p_queue = p_queue;
    p_queue->sentinel.p_next->p_prev = p_elem;
    p_queue->sentinel.p_next = p_elem;
    p_queue->length++;
}

queue_elem_ptr_t queue_deq(queue_ptr_t p_queue) {
    ASSERT(p_queue);
    if (p_queue->length > 0) {
        queue_elem_ptr_t p_elem = p_queue->sentinel.p_prev;
        ASSERT(p_elem);
        ASSERT(p_queue->sentinel.p_prev != &p_queue->sentinel);
        p_elem->p_prev->p_next = &p_queue->sentinel;
        p_queue->sentinel.p_prev = p_elem->p_prev;
        p_elem->p_queue = NULL;
        p_queue->length--;
        return p_elem;
    }
    return NULL;
}

queue_elem_ptr_t queue_head(queue_ptr_t p_queue) {
    ASSERT(p_queue);
    return (p_queue->length > 0 ? p_queue->sentinel.p_prev : NULL);
}

queue_elem_ptr_t queue_tail(queue_ptr_t p_queue) {
    ASSERT(p_queue);
    return (p_queue->length > 0 ? p_queue->sentinel.p_next : NULL);
}

int32_t queue_length(queue_ptr_t p_queue) {
    ASSERT(p_queue);
    return p_queue->length;
}

void queue_purge(queue_ptr_t p_queue) {
    ASSERT(p_queue);
    
    elem_destroy_cb_t elem_destroy_cb = p_queue->destroy_cb;
    if (!elem_destroy_cb) { elem_destroy_cb = elem_destroy_cb_default; }
    
    do {
        queue_elem_t *p_elem = queue_deq(p_queue);
        if (!p_elem) { break; }
        
        elem_destroy_cb(&p_elem);
    } while (0);
}

void queue_remove(queue_ptr_t p_queue, queue_elem_ptr_t p_elem) {
    ASSERT(p_queue);
    ASSERT(p_elem);
    ASSERT(p_elem != &p_queue->sentinel);

    if (p_elem->p_queue == p_queue) {
        p_elem->p_next->p_prev = p_elem->p_prev;
        p_elem->p_prev->p_next = p_elem->p_next;
        p_elem->p_prev = NULL;
        p_elem->p_next = NULL;
        p_elem->p_queue = NULL;
        p_queue->length--;
    }
}

void queue_swap(queue_ptr_t p_dst_queue, queue_ptr_t p_src_queue) {
    ASSERT(p_src_queue);
    ASSERT(p_dst_queue);

    queue_init(p_dst_queue);
    if (p_src_queue->length > 0) {
        queue_elem_ptr_t p_head = queue_head(p_src_queue);
        p_head->p_next = &p_dst_queue->sentinel;
        p_dst_queue->sentinel.p_prev = p_head;

        queue_elem_ptr_t p_tail = queue_tail(p_src_queue);
        p_tail->p_prev = &p_dst_queue->sentinel;
        p_dst_queue->sentinel.p_next = p_tail;

        p_dst_queue->length = p_src_queue->length;

        queue_init(p_src_queue);
    }
}

queue_elem_ptr_t queue_elem_next(queue_elem_ptr_t p_elem) {
    ASSERT(p_elem);
    
    return ((p_elem->p_prev == (queue_elem_ptr_t)p_elem->p_queue) ? NULL : p_elem->p_prev);
}

void queue_elem_remove(queue_elem_ptr_t p_elem) {
    ASSERT(p_elem);
    if (p_elem->p_queue) {
        queue_remove(p_elem->p_queue, p_elem);
    }
}

