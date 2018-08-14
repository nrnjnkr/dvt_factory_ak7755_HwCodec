/*
 * agent_dev.c
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
#include "device.h"
#include "impl_dev.h"
#include "agent_dev.h"
#include "proc_dev.h"

#include <semaphore.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/time.h>

typedef struct {
    CTX_COMMON

    pthread_t           thread;
    pthread_mutex_t     lock;
    sem_t               sem;

    device_ptr_t        p_device;

    loop_ptr_t          p_loop;
    channel_ptr_t       p_channel_agent;
    channel_ptr_t       p_channel_task;

    int32_t             agent_reconnect_times;
    handle_t            h_opaque;
    msg_notify_cb_t     msg_notify_cb;
} agent_dev_t, *agent_dev_ptr_t, **agent_dev_pptr_t;

typedef struct {
    queue_elem_t        elem;

    ctx_ptr_t           p_ctx;
    int32_t             task_id;
    msg_t               msg;
    msg_notify_cb_t     msg_notify_cb;
} task_entry_t, *task_entry_ptr_t;

typedef struct {
    CHANNEL_COMMON

    pthread_mutex_t     lock;

    agent_dev_ptr_t     p_agent_dev;
    queue_t             queue_task;
    int32_t             task_id;
} channel_task_t, *channel_task_ptr_t, **channel_task_pptr_t;

// channel_agent_dev
static int32_t channel_agent_dev_handle_read(channel_agent_dev_ptr_t p_client) {
    if (p_client->r_off == p_client->buf_size) {
        LOGD("buf full");
        return RET_ERROR_IN_BUF_FULL;
    }

    int32_t len = (int32_t)read(p_client->watcher.io.fd, p_client->r_buf + p_client->r_off, p_client->buf_size - p_client->r_off);
    if (len > 0) {
        p_client->r_off += len;

        // loop
        // 1. find next sync byte
        // 2. check whether it is a message indeed.
        //    if NO, log error and skip, goto 1;
        //    if YES, parse message -> handle it -> output
        // 3. send output

        int32_t beg, end;
        beg = end = len = 0;

        while (beg < p_client->r_off) {
            char *p_end = (char *)memchr(p_client->r_buf + beg, MSG_DELIMITER, p_client->r_off - beg);
            if (!p_end) { break; }

            end = (int32_t)(p_end - p_client->r_buf);
            len = end - beg;

            if (agent_cloud_msg_proc((channel_ptr_t)p_client, p_client->r_buf + beg, len) < 0) {
                return RET_ERROR_CLIENT;
            }

            beg = end + 1;
        }

        while (beg < p_client->r_off && p_client->r_buf[beg] == '\0') { beg++; }

        int32_t left = p_client->r_off - beg;
        if (beg != 0 && left > 0) {
            // skip zero
            memmove(p_client->r_buf, p_client->r_buf + beg, left);
        }
        p_client->r_off = left;
    } else if (len == 0) {
        return RET_ERROR_EOF;
    } else {
        if (errno != EINTR && errno != EAGAIN) { return RET_ERROR_READ; }
    }
    return RET_OK;
}

static int32_t channel_agent_dev_handle_write(channel_agent_dev_ptr_t p_client) {
    if (p_client->w_off == 0) { return RET_OK; }

    int32_t ret = (int32_t)write(p_client->watcher.io.fd, p_client->w_buf, p_client->w_off);
    if (ret == -1) {
        if (errno != EINTR && errno != EAGAIN) { return RET_ERROR_WRITE; }
    } else {
        int32_t left = p_client->w_off - ret;
        if (left > 0) { memmove(p_client->w_buf, p_client->w_buf + ret, left); }
        p_client->w_off = left;
    }
    return RET_OK;
}

static void channel_agent_dev_cb(EV_P_ ev_io *watcher, int32_t revents) {
    if (EV_ERROR & revents) {
        LOGE("revents:%x", revents);
        return ;
    }

    channel_agent_dev_ptr_t p_client = (channel_agent_dev_ptr_t)(((char*)watcher) - offsetof(channel_t, watcher));
    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t)p_client->p_ctx;

    int32_t ret;
    if (EV_READ & revents) {
        ret = channel_agent_dev_handle_read(p_client);
        if (ret < 0) {
            LOGD("read_error: fd=%d ret=%d", watcher->fd, ret);
            p_agent_dev->p_channel_agent->destroy(&p_agent_dev->p_channel_agent);
            return ;
        }
    }

    if (EV_WRITE & revents) {
        ret = channel_agent_dev_handle_write(p_client);
        if (ret < 0) {
            LOGD("write_error: fd=%d ret=%d", watcher->fd, ret);
            p_agent_dev->p_channel_agent->destroy(&p_agent_dev->p_channel_agent);
            return ;
        }
    }

    int32_t flags = EV_READ;
    if (p_client->w_off > 0) { flags |= EV_WRITE; }
    channel_io_update((channel_ptr_t)p_client, flags);
}

static channel_ptr_t channel_agent_dev_create(agent_dev_ptr_t p_ctx, int32_t fd) {
    channel_agent_dev_ptr_t p_channel = MEM_ALLOCZ(channel_agent_dev_t);
    do {
        if (!p_channel) {
            LOGE("alloc failure");
            break;
        }

        p_channel->buf_size = BUF_SIZE_2048;
        p_channel->start = channel_io_start;
        p_channel->stop = channel_io_stop;
        p_channel->destroy = channel_io_destroy;

        p_channel->p_ctx = (ctx_ptr_t) p_ctx;

        int32_t flag = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, flag | O_NONBLOCK);

        ev_io_init(&p_channel->watcher.io, channel_agent_dev_cb, fd, EV_READ);
        return (channel_ptr_t)p_channel;
    } while (0);

    if (p_channel) { p_channel->destroy((channel_pptr_t)&p_channel); }
    return NULL;
}

// channel_task
static void task_init(task_entry_ptr_t p_task,
                      ctx_ptr_t        p_ctx,
                      int32_t          task_id,
                      msg_ptr_t        p_msg,
                      msg_notify_cb_t  msg_notify_cb)
{
    p_task->p_ctx = p_ctx;
    p_task->task_id = task_id;
    memmove(&p_task->msg, p_msg, sizeof(msg_t));
    p_task->msg_notify_cb = msg_notify_cb;
}

static int connect_non_blocking(int sockfd, const struct sockaddr_in *saptr, socklen_t salen, int nsec) {
    int flags,n,error;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    if ((n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0){
        if (errno != EINPROGRESS){
            return(-1);
        }
    }
    /* Do whatever we want while the connect is taking place. */
    if (n == 0) {
        goto done;/* connect completed immediately */
    }

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;
    while ((n = select(sockfd+1, &rset, &wset, NULL,nsec ? &tval : NULL)) < 0 && errno == EINTR);
    if (n == 0) {
        errno = ETIMEDOUT;
        return(-1);
    }
    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0){
            return(-1);
        }
    } else{
        return (-1);/*select error*/
    }
done:
    fcntl(sockfd, F_SETFL, flags);/* restore file status flags */
    if (error) {
        errno = error;
        return(-1);
    }
    return(0);
}

static int32_t task_proc(task_entry_ptr_t p_task_entry) {
    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t)p_task_entry->p_ctx;

    switch (p_task_entry->msg.what) {
        case MSG_SELF_BREAK_LOOP: {
            ev_break (p_agent_dev->p_loop, EVBREAK_ALL);
        } break;
        case MSG_SELF_CHANNEL_UPDATE: {
            channel_agent_dev_ptr_t p_client = (channel_agent_dev_ptr_t)p_task_entry->msg.obj;
            int32_t flags = EV_READ;
            if (p_client->w_off > 0) { flags |= EV_WRITE; }
            channel_io_update((channel_ptr_t)p_client, flags);
        } break;
        case MSG_SELF_CONNECT_CLOUD: {
            const char *p_ip = (const char*) p_agent_dev->p_device->param.srv_ip;
            int port = p_agent_dev->p_device->param.srv_dev_port;

            int sock = 0;
            do {
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) {
                    LOGE("create socket failure");
                    break;
                }

                //query wlan0 address
                struct ifreq ifr;
                strcpy(ifr.ifr_name, "wlan0");
                if (ioctl(sock, SIOCGIFADDR, &ifr) <  0) {
                    LOGE("failed to query wlan0 address\n");
                    msg_t msg;
                    msg.what = MSG_DEVICE_AGENT_CANNOT_CONNECT;
                    agent_dev_notify(p_task_entry->p_ctx, &msg, NULL);
                    break;
                }
                struct sockaddr_in clientAddr;
                int flag=1;
                clientAddr.sin_family = AF_INET;
                clientAddr.sin_addr.s_addr = ((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr.s_addr;
                clientAddr.sin_port = htons(CONF_DEVICE_APP_PORT);
                setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
                if(bind(sock, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
                    LOGE("failed to bind %s:%d\n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr), CONF_DEVICE_APP_PORT);
                    msg_t msg;
                    msg.what = MSG_DEVICE_AGENT_CANNOT_CONNECT;
                    agent_dev_notify(p_task_entry->p_ctx, &msg, NULL);
                    break;
                }

                struct sockaddr_in srvAddr;
                srvAddr.sin_family = AF_INET;
                srvAddr.sin_addr.s_addr = inet_addr(p_ip);
                srvAddr.sin_port = htons(port);

                int ret = connect_non_blocking(sock, &srvAddr, sizeof(srvAddr), CONF_APP_CONNECT_TIMEOUT);
                if (ret < 0) {
                    LOGE("can't connect cloud %s:%d\n", p_ip, port);
                    msg_t msg;
                    msg.what = MSG_DEVICE_AGENT_CANNOT_CONNECT;
                    agent_dev_notify(p_task_entry->p_ctx, &msg, NULL);
                    break;
                }

                {
                    msg_t msg;
                    msg.what = MSG_DEVICE_AGENT_CONNECTED;
                    agent_dev_notify(p_task_entry->p_ctx, &msg, NULL);
                }

                channel_t *p_channel = channel_agent_dev_create(p_agent_dev, sock);
                if (!p_channel) {
                    LOGE("create agent_dev failure");
                    break;
                }

                p_agent_dev->p_channel_agent = p_channel;
                p_channel->start(p_channel, p_agent_dev->p_loop);

                sprintf(g_tmp_buf, "{\"%s\":%d, \"%s\":\"%s\", \"%s\":%d, \"%s\":%d, \"%s\":\"%s\"}\n",
                        KEY_WHAT, MSG_DEVICE_LOGIN,
                        KEY_UID, p_agent_dev->p_device->param.dev_uid,
                        KEY_STATE, DEVICE_STATE_ONLINE,
                        KEY_MODE, p_agent_dev->p_device->param.dev_mode,
                        KEY_CATEGORY, p_agent_dev->p_device->param.dev_category);
                agent_dev_cloud_send_sync(p_agent_dev->p_channel_agent, g_tmp_buf, (int32_t)strlen(g_tmp_buf));
                LOGD("MSG_SELF_CONNECT_CLOUD %s:%d Done\n", p_ip, port);
                return 0;
            } while (0);

            if (sock > 0) { close(sock); }

            // TODO
            // use timer instead
            /*
            if (p_agent_dev->agent_reconnect_times < AGENT_RECONNECT_TIMES_THRESHOLD) {
                p_agent_dev->agent_reconnect_times++;

                sleep(1);
                agent_dev_msg_post(p_task_entry->p_ctx, MSG_SELF_CONNECT_CLOUD);
            } else {
                p_agent_dev->agent_reconnect_times = 0;
            }
            */
        } break;
        case MSG_SELF_CONNECT_CLOUD_FD:{
            const char *p_ip = (const char*) p_agent_dev->p_device->param.srv_ip;
            int port = p_agent_dev->p_device->param.srv_dev_port;
            do {
                {
                    msg_t msg;
                    msg.what = MSG_DEVICE_AGENT_CONNECTED;
                    agent_dev_notify(p_task_entry->p_ctx, &msg, NULL);
                }

                channel_t *p_channel = channel_agent_dev_create(p_agent_dev, p_task_entry->msg.para1);
                if (!p_channel) {
                    LOGE("create agent_dev failure");
                    break;
                }

                p_agent_dev->p_channel_agent = p_channel;
                p_channel->start(p_channel, p_agent_dev->p_loop);

                sprintf(g_tmp_buf, "{\"%s\":%d, \"%s\":\"%s\", \"%s\":%d, \"%s\":%d, \"%s\":\"%s\"}\n",
                        KEY_WHAT, MSG_DEVICE_LOGIN,
                        KEY_UID, p_agent_dev->p_device->param.dev_uid,
                        KEY_STATE, DEVICE_STATE_ONLINE,
                        KEY_MODE, p_agent_dev->p_device->param.dev_mode,
                        KEY_CATEGORY, p_agent_dev->p_device->param.dev_category);
                agent_dev_cloud_send_sync(p_agent_dev->p_channel_agent, g_tmp_buf, (int32_t)strlen(g_tmp_buf));
                LOGD("MSG_SELF_CONNECT_CLOUD %s:%d Done\n", p_ip, port);
            } while (0);
        }break;
        case MSG_SELF_DISCONNECT_CLOUD: {
            if (p_agent_dev->p_channel_agent) {
                p_agent_dev->p_channel_agent->destroy(&p_agent_dev->p_channel_agent);
            }
        } break;
        case MSG_SELF_STANDBY_TCP: {
            if (!p_agent_dev->p_channel_agent) {
                LOGE("can't notify cloud");
                break;
            }
            sprintf(g_tmp_buf, "{\"%s\":%d, \"%s\":\"%s\", \"%s\":%d, \"%s\":%d, \"%s\":\"%s\", \"%s\":\"%s\"}\n",
                    KEY_WHAT, MSG_DEVICE_STANDBY_TCP,
                    KEY_UID, p_agent_dev->p_device->param.dev_uid,
                    KEY_STATE, DEVICE_STATE_STANDBY,
                    KEY_MODE, p_agent_dev->p_device->param.dev_mode,
                    KEY_TOKEN, p_agent_dev->p_device->param.dev_token,
                    KEY_CATEGORY, p_agent_dev->p_device->param.dev_category);
            agent_dev_cloud_send_sync(p_agent_dev->p_channel_agent, g_tmp_buf, (int32_t)strlen(g_tmp_buf));
        }
        break;
        case MSG_SELF_STREAMING: {
            if (!p_agent_dev->p_channel_agent) {
                LOGE("can't notify cloud");
                break;
            }
            sprintf(g_tmp_buf, "{\"%s\":%d, \"%s\":\"%s\", \"%s\":%d, \"%s\":%d, \"%s\":\"%s\", \"%s\":\"%s\"}\n",
                    KEY_WHAT, MSG_DEVICE_LOGIN,
                    KEY_UID, p_agent_dev->p_device->param.dev_uid,
                    KEY_STATE, DEVICE_STATE_ONLINE,
                    KEY_MODE, p_agent_dev->p_device->param.dev_mode,
                    KEY_TOKEN, p_agent_dev->p_device->param.dev_token,
                    KEY_CATEGORY, p_agent_dev->p_device->param.dev_category);
            agent_dev_cloud_send_sync(p_agent_dev->p_channel_agent, g_tmp_buf, (int32_t)strlen(g_tmp_buf));
        }
        break;
        case MSG_DEVICE_BATTERY: {
            if (!p_agent_dev->p_channel_agent) {
                LOGE("can't notify cloud");
                break;
            }

            json_object *p_out_json = NULL;
            json_object *p_json_what = NULL;
            json_object *p_json_stat = NULL;
            json_object *p_json_quantity = NULL;
            do {
                msg_ptr_t p_msg = &p_task_entry->msg;

                p_json_what = json_object_new_int(p_msg->what);
                if (!p_json_what) {
                    LOGE("create what object failure: msg:%d", p_msg->what);
                    break;
                }

                p_json_stat = json_object_new_int(p_msg->para1);
                if (!p_json_stat) {
                    LOGE("create stat object failure: msg:%d", p_msg->what);
                    break;
                }

                p_json_quantity = json_object_new_int(p_msg->para2);
                if (!p_json_quantity) {
                    LOGE("create quantity object failure: msg:%d", p_msg->what);
                    break;
                }

                p_out_json = json_object_new_object();
                if (!p_out_json) {
                    LOGE("create object failure: msg:%d", p_msg->what);
                    break;
                }

                json_object_object_add(p_out_json, KEY_WHAT, p_json_what);
                json_object_object_add(p_out_json, KEY_STAT, p_json_stat);
                json_object_object_add(p_out_json, KEY_QUANTITY, p_json_quantity);

                const char *p_str = json_object_to_json_string(p_out_json);
                sprintf(g_tmp_buf, "%s\n", p_str);
                agent_dev_cloud_send_sync(p_agent_dev->p_channel_agent, g_tmp_buf, (int32_t)strlen(g_tmp_buf));
            } while (0);

            if (p_out_json) {
                json_object_put(p_out_json);
            } else {
                if (p_json_what) { json_object_put(p_json_what); }
                if (p_json_stat) { json_object_put(p_json_stat); }
                if (p_json_quantity) { json_object_put(p_json_quantity); }
            }
        } break;
        case MSG_DEVICE_CHARGE:
        case MSG_DEVICE_DC:
        case MSG_DEVICE_PIR: {
            if (!p_agent_dev->p_channel_agent) {
                LOGE("can't notify cloud");
                break;
            }

            json_object *p_out_json = NULL;
            json_object *p_json_what = NULL;
            json_object *p_json_stat = NULL;
            do {
                msg_ptr_t p_msg = &p_task_entry->msg;

                p_json_what = json_object_new_int(p_msg->what);
                if (!p_json_what) {
                    LOGE("create what object failure: msg:%d", p_msg->what);
                    break;
                }

                p_json_stat = json_object_new_int(p_msg->para1);
                if (!p_json_stat) {
                    LOGE("create stat object failure: msg:%d", p_msg->what);
                    break;
                }

                p_out_json = json_object_new_object();
                if (!p_out_json) {
                    LOGE("create object failure: msg:%d", p_msg->what);
                    break;
                }

                json_object_object_add(p_out_json, KEY_WHAT, p_json_what);
                json_object_object_add(p_out_json, KEY_STAT, p_json_stat);

                const char *p_str = json_object_to_json_string(p_out_json);
                sprintf(g_tmp_buf, "%s\n", p_str);
                agent_dev_cloud_send_sync(p_agent_dev->p_channel_agent, g_tmp_buf, (int32_t)strlen(g_tmp_buf));
            } while (0);

            if (p_out_json) {
                json_object_put(p_out_json);
            } else {
                if (p_json_what) { json_object_put(p_json_what); }
                if (p_json_stat) { json_object_put(p_json_stat); }
            }
        } break;
    }

    return RET_OK;
}

static void channel_task_cb (EV_P_ ev_async *watcher, int32_t revents) {
    channel_task_ptr_t p_channel_task = (channel_task_ptr_t)(((char*)watcher) - offsetof(channel_task_t, watcher));

    queue_t queue;
    pthread_mutex_lock(&p_channel_task->lock);
    queue_swap(&queue, &p_channel_task->queue_task);
    pthread_mutex_unlock(&p_channel_task->lock);

    do {
        task_entry_ptr_t p_task = QUEUE_DEQ(&queue, task_entry_ptr_t);
        if (!p_task) { break; }

        task_proc(p_task);
        MEM_FREE_PTR(p_task);
    } while (1);
}

channel_ptr_t channel_task_create(agent_dev_ptr_t p_agent_dev) {
    channel_task_ptr_t p_channel = MEM_ALLOCZ(channel_task_t);
    do {
        if (!p_channel) {
            LOGE("alloc failure");
            break;
        }

        p_channel->p_agent_dev = p_agent_dev;
        p_channel->start = channel_async_start;
        p_channel->stop = channel_async_stop;
        p_channel->destroy = channel_async_destroy;

        pthread_mutex_init(&p_channel->lock, NULL);
        queue_init(&p_channel->queue_task);

        ev_async_init(&p_channel->watcher.async, channel_task_cb);
        return (channel_ptr_t)p_channel;
    } while (0);

    if (p_channel) { p_channel->destroy((channel_pptr_t)&p_channel); }
    return NULL;
}

int32_t agent_dev_msg_post(ctx_ptr_t p_ctx, int32_t what) {
    msg_t msg;
    msg.what = what;
    return agent_dev_msg_request(p_ctx, &msg, NULL);
}

int32_t agent_dev_msg_request(ctx_ptr_t p_ctx, msg_ptr_t p_msg, msg_notify_cb_t msg_proc_cb) {
    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t) p_ctx;
    channel_task_ptr_t p_channel_task = (channel_task_ptr_t)p_agent_dev->p_channel_task;

    int32_t task_id = task_id = p_channel_task->task_id + 1;
    task_entry_ptr_t p_task_entry = MEM_ALLOCZ(task_entry_t);
    if (!p_task_entry) {
        LOGE("create task failure");
        return RET_ERROR;
    }

    pthread_mutex_lock(&p_channel_task->lock);
    task_init(p_task_entry, p_ctx, task_id, p_msg, msg_proc_cb);
    QUEUE_ENQ(&p_channel_task->queue_task, p_task_entry);
    pthread_mutex_unlock(&p_channel_task->lock);

    ev_async_send(p_channel_task->p_loop, &p_channel_task->watcher.async);
    p_channel_task->task_id++;
    return task_id;
}

int32_t agent_dev_msg_cancel(ctx_ptr_t p_ctx, int32_t task_id) {
    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t) p_ctx;
    channel_task_ptr_t p_channel_task = (channel_task_ptr_t)p_agent_dev->p_channel_task;

    pthread_mutex_lock(&p_channel_task->lock);
    task_entry_ptr_t p_task_entry = QUEUE_HEAD(&p_channel_task->queue_task, task_entry_ptr_t);
    while (p_task_entry) {
        if (p_task_entry->task_id == task_id) {
            QUEUE_REMOVE(&p_task_entry->elem);
            MEM_FREE_PTR(p_task_entry);
            break;
        }
        p_task_entry = QUEUE_NEXT(&p_task_entry->elem, task_entry_ptr_t);
    }
    pthread_mutex_unlock(&p_channel_task->lock);
    return RET_OK;
}

static void agent_init(agent_dev_ptr_t p_agent_dev) {
    channel_ptr_t p_channel = p_agent_dev->p_channel_task;
    p_channel->start(p_channel, p_agent_dev->p_loop);
}

static void agent_deinit(agent_dev_ptr_t p_agent_dev) {
    channel_ptr_t p_channel = p_agent_dev->p_channel_agent;
    if (p_channel) { p_channel->stop(p_channel); }

    p_channel = p_agent_dev->p_channel_task;
    if (p_channel) { p_channel->stop(p_channel); }

    // purge task-related
    channel_task_ptr_t p_channel_task = (channel_task_ptr_t)p_agent_dev->p_channel_task;
    pthread_mutex_lock(&p_channel_task->lock);
    queue_purge(&p_channel_task->queue_task);
    pthread_mutex_unlock(&p_channel_task->lock);

    p_agent_dev->agent_reconnect_times = 0;
}

void* agent_dev_thread(void *arg) {
    LOGD("thread start");

    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t)arg;

    // start
    agent_init(p_agent_dev);

    sem_post(&p_agent_dev->sem);

    // running
    ev_loop(p_agent_dev->p_loop, 0);

    // stop
    agent_deinit(p_agent_dev);

    LOGD("thread end");

    return NULL;
}

static int32_t agent_start(ctx_ptr_t p_ctx) {
    // TODO: for safe init later

    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t)p_ctx;
    LOGD("before agent_dev_thread create");
    pthread_create(&p_agent_dev->thread, NULL, agent_dev_thread, p_agent_dev);
    LOGD("before agent_dev_thread created and waiting sem");
    sem_wait(&p_agent_dev->sem);
    LOGD("init sem gotten");
    return RET_OK;
}

static int32_t agent_stop(ctx_ptr_t p_ctx) {
    // TODO: for safe deinit later

    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t)p_ctx;
    if (p_agent_dev->thread != (pthread_t)INVALID_ID) {
        channel_task_ptr_t p_channel_task = (channel_task_ptr_t)p_agent_dev->p_channel_task;
        if (p_channel_task) {
            agent_dev_msg_post(p_ctx, MSG_SELF_BREAK_LOOP);
            pthread_join(p_agent_dev->thread, NULL);
            p_agent_dev->thread = (pthread_t) INVALID_ID;
        }
    }
    return RET_OK;
}

static void agent_destroy(ctx_pptr_t pp_ctx) {
    if (pp_ctx && *pp_ctx) {
        agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t)*pp_ctx;
        p_agent_dev->stop((ctx_ptr_t)p_agent_dev);

        channel_pptr_t pp_channel = &p_agent_dev->p_channel_agent;
        if (*pp_channel) { (*pp_channel)->destroy(pp_channel); }

        pp_channel = &p_agent_dev->p_channel_task;
        if (*pp_channel) { (*pp_channel)->destroy(pp_channel); }

        ev_loop_destroy(p_agent_dev->p_loop);

        pthread_mutex_destroy(&p_agent_dev->lock);
        sem_destroy(&p_agent_dev->sem);

        MEM_FREE_PPTR(pp_ctx);
    }
}

ctx_ptr_t agent_dev_create(device_ptr_t p_device) {
    agent_dev_ptr_t p_agent_dev = MEM_ALLOCZ(agent_dev_t);
    do {
        if (!p_agent_dev) {
            LOGE("alloc failure");
            break;
        }

        p_agent_dev->start = agent_start;
        p_agent_dev->stop = agent_stop;
        p_agent_dev->destroy = agent_destroy;

        p_agent_dev->thread = (pthread_t) INVALID_ID;
        pthread_mutex_init(&p_agent_dev->lock, NULL);
        sem_init(&p_agent_dev->sem, 0, 0);

        p_agent_dev->p_device = p_device;

        p_agent_dev->p_loop = ev_loop_new(0);
        if (!p_agent_dev->p_loop) {
            LOGE("create loop failure");
            break;
        }

        p_agent_dev->p_channel_task = channel_task_create(p_agent_dev);
        if (!p_agent_dev->p_channel_task) {
            LOGE("create task failure");
            break;
        }
        return (ctx_ptr_t)p_agent_dev;
    } while (0);

    if (p_agent_dev) { p_agent_dev->destroy((ctx_pptr_t)&p_agent_dev); }
    return NULL;
}

void agent_dev_set_msg_notify_cb(ctx_ptr_t p_ctx, handle_t h_opaque, msg_notify_cb_t notify_cb) {
    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t) p_ctx;
    p_agent_dev->h_opaque = h_opaque;
    p_agent_dev->msg_notify_cb = notify_cb;
}

void agent_dev_notify(ctx_ptr_t p_ctx, msg_ptr_t p_msg, result_ptr_t p_result) {
    agent_dev_ptr_t p_agent_dev = (agent_dev_ptr_t) p_ctx;
    if (p_agent_dev->msg_notify_cb) {
        p_agent_dev->msg_notify_cb(p_agent_dev->h_opaque, p_msg, p_result);
    }
}

static int _do_write( int sockfd, const unsigned char  *inBuf, size_t inBufLen,int timeout/*seconds*/){
    ssize_t writeResult;
    size_t numWritten;
    fd_set writeSet;
    int selectResult;
    struct timeval tv;

    numWritten = 0;
    do {
select_again_:
        FD_ZERO( &writeSet );
        FD_SET( sockfd, &writeSet );
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        selectResult = select(sockfd + 1, NULL, &writeSet, NULL, &tv);
        if(selectResult < 0 && errno == EINTR) goto select_again_;
        if(selectResult == 0/*Timeout*/) return -1;
write_again_:
        writeResult = write( sockfd, ( inBuf + numWritten ), ( inBufLen - numWritten ) );
        if(writeResult < 0 && (errno == EINTR)){
            goto write_again_;
        }
        if(writeResult < 0 &&( errno == EAGAIN || errno == EWOULDBLOCK)){
            usleep(1000);
            goto write_again_;
        }
        if(writeResult < 0){
            return -1;
        }
        numWritten += writeResult;
    } while( numWritten < inBufLen );

    if(numWritten == inBufLen){
        return numWritten;
    }
    return -1;
}

int32_t agent_dev_standby(int32_t fd, const char *uid, const char *category, int32_t mode, unsigned char *p_token, int32_t token_len) {
    char token[MAX_WAKE_TOKEN_LEN+1];
    snprintf(token, MAX_WAKE_TOKEN_LEN, "%s", p_token);

    char data[MAX_DBG_BUF_SIZE];
    sprintf(data, "{\"%s\":%d, \"%s\":\"%s\", \"%s\":%d, \"%s\":%d, \"%s\":\"%s\", \"%s\":\"%s\"}\n",
        KEY_WHAT, MSG_DEVICE_STANDBY_TCP,
        KEY_UID, uid,
        KEY_STATE, DEVICE_STATE_STANDBY,
        KEY_MODE, mode,
        KEY_TOKEN, token,
        KEY_CATEGORY,category);
    int32_t data_len = (int32_t)strlen(data);
    int32_t ret = (int32_t)_do_write(fd, (unsigned char *)data, data_len,5);
    return ((ret == data_len) ? RET_OK : RET_ERROR);
}
