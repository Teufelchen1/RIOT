#include <stdio.h>
#include <assert.h>

#include "architecture.h"
#include "thread.h"
#include "sched.h"

#include "nanocbor/nanocbor.h"
#include "net/nanocoap.h"

#ifdef MODULE_SCHEDSTATISTICS
#include "schedstatistics.h"
#include "ztimer.h"
#endif

#ifdef MODULE_TLSF_MALLOC
#include "tlsf.h"
#include "tlsf-malloc.h"
#endif

int encode_thread(
    nanocbor_encoder_t *enc,
    int pid,
    const char * name,
    thread_status_t state,
    bool active,
    int priority,
    int stacksize,
    int stackusage,
    int stackfree,
    void *base_addr,
    void *current_addr)
{
    nanocbor_fmt_array(enc, 10);
    if (nanocbor_fmt_int(enc, pid) < 0) {
        return -EIO;
    }
    if (nanocbor_put_tstr(enc, name) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(enc, (int) state) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_bool(enc, active) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(enc, priority) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(enc, stacksize) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(enc, stackusage) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(enc, stackfree) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(enc, (int) base_addr) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(enc, (int) current_addr) < 0) {
        return -EIO;
    }
    return 0;
}

ssize_t _ps_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                  coap_request_ctx_t *context)
{
    (void) context;
    uint8_t buffer[500];

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));

    nanocbor_fmt_array(&enc, 2);
    int isr_usage = thread_isr_stack_usage();
    int isr_free = ISR_STACKSIZE - isr_usage;
    void *isr_start = thread_isr_stack_start();
    void *isr_sp = thread_isr_stack_pointer();
    
    nanocbor_fmt_array(&enc, 5);
    if (nanocbor_fmt_int(&enc, ISR_STACKSIZE) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(&enc, isr_usage) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(&enc, isr_free) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(&enc, (int) isr_start) < 0) {
        return -EIO;
    }
    if (nanocbor_fmt_int(&enc, (int) isr_sp) < 0) {
        return -EIO;
    }

    assert(nanocbor_fmt_array_indefinite(&enc) > 0);
    for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; i++) {
        thread_t *p = thread_get(i);

        if (p != NULL) {
            int stacksz = thread_get_stacksize(p);                          /* get stack size */
            int stack_free = thread_measure_stack_free(p);
            stacksz -= stack_free;
            if (
                encode_thread(&enc, 
                    thread_getpid_of(p),
                    thread_get_name(p),
                    thread_get_status(p),
                    thread_is_active(p),
                    thread_get_priority(p),
                    thread_get_stacksize(p), stacksz, stack_free,
                    thread_get_stackstart(p),
                    thread_get_sp(p)
                ) < 0
            ) {
                return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                    COAP_FORMAT_NONE, NULL, 0);
            }
        }
    }
    if (nanocbor_fmt_end_indefinite(&enc) < 0) {
        return coap_reply_simple(pkt, COAP_CODE_INTERNAL_SERVER_ERROR, buf, len,
                    COAP_FORMAT_NONE, NULL, 0);
    }

    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
        COAP_FORMAT_CBOR, buffer, nanocbor_encoded_len(&enc));
}

NANOCOAP_RESOURCE(ps_cbor) { \
    .path = "/jelly/Ps", .methods = COAP_GET, .handler = _ps_handler, .context = NULL \
};
