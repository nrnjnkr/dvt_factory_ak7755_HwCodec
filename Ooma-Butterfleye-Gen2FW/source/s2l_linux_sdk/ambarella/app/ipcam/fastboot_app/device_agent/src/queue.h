/*
 * queue.h
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
#ifndef __QUEUE_H__
#define __QUEUE_H__

struct queue;

typedef struct queue_elem {
    struct queue      	*p_queue;
    struct queue_elem	*p_next;
    struct queue_elem	*p_prev;
} queue_elem_t, *queue_elem_ptr_t, **queue_elem_pptr_t;


typedef void (*elem_destroy_cb_t)(queue_elem_pptr_t pp_elem);
typedef struct queue {
    queue_elem_t        sentinel;
    int32_t             length;
    elem_destroy_cb_t   destroy_cb;
} queue_t, *queue_ptr_t, **queue_pptr_t;

queue_ptr_t			queue_create();
void				queue_destroy(queue_pptr_t pp_queue);

int32_t             queue_init(queue_ptr_t p_queue);
void 				queue_deinit(queue_ptr_t p_queue);
void                queue_set_elem_destroy_cb(queue_ptr_t p_queue, elem_destroy_cb_t cb);

void           	 	queue_enq(queue_ptr_t p_queue, queue_elem_ptr_t p_elem);
queue_elem_ptr_t    queue_deq(queue_ptr_t p_queue);

queue_elem_ptr_t   	queue_head(queue_ptr_t p_queue);
queue_elem_ptr_t   	queue_tail(queue_ptr_t p_queue);

int32_t  			queue_length(queue_ptr_t p_queue);
void                queue_purge(queue_ptr_t p_queue);
void 				queue_remove(queue_ptr_t p_queue, queue_elem_ptr_t p_elem);
void 				queue_swap(queue_ptr_t p_src_queue, queue_ptr_t p_dst_queue);

queue_elem_ptr_t 	queue_elem_next(queue_elem_ptr_t p_elem);
void 			 	queue_elem_remove(queue_elem_ptr_t p_elem);

#define QUEUE_ENQ(p_queue, p_elem)      queue_enq(p_queue, (queue_elem_ptr_t)p_elem)
#define QUEUE_DEQ(p_queue, ptr_type)    (ptr_type)queue_deq(p_queue)

#define QUEUE_HEAD(p_queue, ptr_type)   (ptr_type)queue_head(p_queue)
#define QUEUE_TAIL(p_queue, ptr_type)   (ptr_type)queue_tail(p_queue)

#define QUEUE_NEXT(p_elem, ptr_type)    (ptr_type)queue_elem_next((queue_elem_ptr_t)p_elem)
#define QUEUE_REMOVE(p_elem)            queue_elem_remove((queue_elem_ptr_t)p_elem)

#endif
