#include "../gluethread/glthread.h"
#include "../BitOp/bitsop.h"
#include "../Threads/refcount.h"
#include "../graph.h"
#include "../notif.h"
#include "rt_notif.h"
#include "rt_table/nexthop.h"
#include "layer3.h"
#include "../LinuxMemoryManager/uapi_mm.h"

void
rt_table_add_route_to_notify_list (
                rt_table_t *rt_table, 
                l3_route_t *l3route,
                uint8_t flag) {

    uint8_t old_flags = l3route->rt_flags;

    if (!IS_GLTHREAD_LIST_EMPTY(&l3route->notif_glue)) {
        remove_glthread(&l3route->notif_glue);
        if (ref_count_dec(l3route->ref_count)){
            assert(0);
        }
    }

    UNSET_BIT8(l3route->rt_flags, RT_ADD_F);
    UNSET_BIT8(l3route->rt_flags, RT_DEL_F);
    UNSET_BIT8(l3route->rt_flags, RT_UPDATE_F);

    if (IS_BIT_SET(old_flags, RT_DEL_F) &&
         IS_BIT_SET(flag, RT_ADD_F)) {
            SET_BIT(l3route->rt_flags, RT_UPDATE_F);
    } else {
            SET_BIT(l3route->rt_flags, flag);
    }
    glthread_add_next(&rt_table->rt_notify_list_head, &l3route->notif_glue);
    ref_count_inc(l3route->ref_count);
}

static void
rt_table_notif_job_cb(event_dispatcher_t *ev_dis, void *arg, uint32_t arg_size) {

    glthread_t *curr;
    l3_route_t *l3route;
    rt_table_t *rt_table = (rt_table_t *)arg;
    
    rt_table->notif_job = NULL;

    rt_route_notif_data_t rt_route_notif_data;

    pthread_rwlock_rdlock(&rt_table->rwlock);

    /* Start Sending Notifications Now */
    ITERATE_GLTHREAD_BEGIN_REVERSE(&rt_table->rt_notify_list_head, curr) {

        l3route = notif_glue_to_l3_route(curr);
        rt_route_notif_data.l3route = l3route;
        rt_route_notif_data.node = rt_table->node;
        thread_using_route(l3route);

        nfc_invoke_notif_chain(NULL,
                                               &rt_table->nfc_rt_updates, 
                                               &rt_route_notif_data,
                                               sizeof(rt_route_notif_data), 0, 0,
                                               TASK_PRIORITY_COMPUTE);

        remove_glthread(&l3route->notif_glue);
        if (ref_count_dec(l3route->ref_count)) {
            assert(0);
        }

        if ( IS_BIT_SET(l3route->rt_flags, RT_DEL_F) ) {
                thread_using_route_done(l3route);
                continue;
        }

        UNSET_BIT8(l3route->rt_flags, RT_ADD_F);
        UNSET_BIT8(l3route->rt_flags, RT_UPDATE_F);
        thread_using_route_done(l3route);

    } ITERATE_GLTHREAD_END_REVERSE(&rt_table->rt_notify_list_head, curr)
    pthread_rwlock_unlock(&rt_table->rwlock);
}

void
rt_table_kick_start_notif_job(rt_table_t *rt_table) {

    if (rt_table->notif_job) return;
    rt_table->notif_job = task_create_new_job(
                                        EV(rt_table->node),
                                        rt_table, 
                                        rt_table_notif_job_cb,
                                        TASK_ONE_SHOT,
                                        TASK_PRIORITY_COMPUTE);
}

void
nfc_ipv4_rt_subscribe (node_t *node, nfc_app_cb cbk) {

    notif_chain_elem_t nfce_template;

    memset(&nfce_template, 0, sizeof(notif_chain_elem_t));
    nfce_template.app_cb = cbk;

    nfc_register_notif_chain(&NODE_RT_TABLE(node)->nfc_rt_updates,
                                            &nfce_template);
}

void
nfc_ipv4_rt_un_subscribe (node_t *node, nfc_app_cb cbk) {

    notif_chain_elem_t nfce_template;

    memset(&nfce_template, 0, sizeof(notif_chain_elem_t));
    nfce_template.app_cb = cbk;

    nfc_de_register_notif_chain(&node->node_nw_prop.rt_table->nfc_rt_updates,
                                                    &nfce_template);
}

/* Begin : Route flash on Application Demand */

typedef struct flash_data_ {

    rt_table_t *rt_table;
    nfc_app_cb cbk;
} flash_data_t;

static void
 rt_table_purge_flash_route_queue(rt_table_t *rt_table) {

     glthread_t *curr;
     l3_route_t *l3route;

     ITERATE_GLTHREAD_BEGIN(&rt_table->rt_flash_list_head, curr) {

            l3route = flash_glue_to_l3_route(curr);
            UNSET_BIT8(l3route->rt_flags, RT_FLASH_REQ_F);
            remove_glthread(&l3route->flash_glue);

            if ( IS_BIT_SET(l3route->rt_flags, RT_DEL_F)) {
                /* Will delete automatically as it is reference count object now */
                //l3_route_unlock(l3route); 
            }
            
            if (ref_count_dec(l3route->ref_count)) {
                l3_route_free(l3route);
            }
     } ITERATE_GLTHREAD_END(&rt_table->rt_flash_list_head, curr)
 }

static void
rt_table_process_one_flash_client (rt_table_t *rt_table,  nfc_app_cb cbk) {

    glthread_t *curr;
    l3_route_t *l3route;
    rt_route_notif_data_t route_notif_data;

    ITERATE_GLTHREAD_BEGIN_REVERSE (&rt_table->rt_flash_list_head, curr) {

        l3route = flash_glue_to_l3_route(curr);
        route_notif_data.l3route = l3route;
        route_notif_data.node = rt_table->node;
        thread_using_route(l3route);
        cbk(NULL, &route_notif_data, sizeof(route_notif_data));
        thread_using_route_done(l3route);

    }ITERATE_GLTHREAD_END_REVERSE (&rt_table->rt_flash_list_head, curr) 
}

static void
rt_table_flash_job (event_dispatcher_t *ev_dis, void *arg, uint32_t arg_size) {

    glthread_t *curr;
    l3_route_t *l3route;
    rt_route_flash_request_t *flash_req;

    rt_table_t *rt_table = (rt_table_t *)arg;

    rt_table->flash_job = NULL;

    curr = dequeue_glthread_first(&rt_table->flash_request_list_head);

    flash_req = glue_to_route_flash_request(curr);
    rt_table_process_one_flash_client (rt_table, flash_req->cbk);
    free (flash_req);

    if (!IS_GLTHREAD_LIST_EMPTY(&rt_table->flash_request_list_head)) {
        rt_table->flash_job = task_create_new_job(EV(rt_table->node), 
                                                  rt_table,
                                                  rt_table_flash_job, TASK_ONE_SHOT,
                                                  TASK_PRIORITY_COMPUTE);
    }
    else {
        rt_table_purge_flash_route_queue(rt_table);
    }
}

static void
rt_table_add_route_to_flash_list (rt_table_t *rt_table,
                                                      l3_route_t *l3route) {
    
    if (!IS_GLTHREAD_LIST_EMPTY(&l3route->flash_glue)) {
        remove_glthread (&l3route->flash_glue);
        if (ref_count_dec(l3route->ref_count)) {
            l3_route_free(l3route);
            assert(0);
        }
    }

    SET_BIT(l3route->rt_flags, RT_FLASH_REQ_F);
    glthread_add_next(&rt_table->rt_flash_list_head, &l3route->flash_glue);
    ref_count_inc(l3route->ref_count);
}

static void
 rt_table_kick_start_flash_job(rt_table_t *rt_table) {

    glthread_t *curr;
    l3_route_t *l3route;
    mtrie_node_t *mnode;
    
     if (rt_table->flash_job) return;

    pthread_rwlock_rdlock(&rt_table->rwlock);

     ITERATE_GLTHREAD_BEGIN(&rt_table->route_list.list_head, curr) {

        mnode = list_glue_to_mtrie_node(curr);
        l3route = (l3_route_t *)mnode->data;
        thread_using_route(l3route);
        rt_table_add_route_to_flash_list (rt_table, l3route);
        thread_using_route_done(l3route);

    } ITERATE_GLTHREAD_END(&rt_table->route_list, curr)

    pthread_rwlock_unlock(&rt_table->rwlock);

    if (! rt_table->flash_job) {
        rt_table->flash_job = task_create_new_job( 
                                                EV(rt_table->node) ,
                                                rt_table,
                                                rt_table_flash_job,
                                                TASK_ONE_SHOT,
                                                TASK_PRIORITY_COMPUTE);
    }
 }

void
nfc_ipv4_rt_request_flash (node_t *node, nfc_app_cb cbk) {

    rt_route_flash_request_t *flash_req =
        (rt_route_flash_request_t *)calloc(1, sizeof(rt_route_flash_request_t));

    flash_req->cbk = cbk;
    init_glthread(&flash_req->glue);

    glthread_add_last(&(NODE_RT_TABLE(node)->flash_request_list_head), &flash_req->glue);
    rt_table_kick_start_flash_job(NODE_RT_TABLE(node));
}

/* End : Route flash on Application Demand */

void
nfc_ipv4_rt_subscribe_per_route (node_t *node, uint32_t ip, uint8_t mask) {

}

void
nfc_ipv4_rt_un_subscribe_per_route (node_t *node, uint32_t ip, uint8_t mask) {

}
