/*
 * 2017-12-12
 * implement of hit_random_key_sequence_timer func
 */

#include "qemu/osdep.h"
#include "antivm/key-rhit.h"
#include "qemu/notify.h"
#include "qapi-types.h"
#include "ui/input.h"
#include "qemu/timer.h"

static const int key_qcodebook[] = {
    9, 10, 11, 12, 13, 14, 15, 16, 17, 18, /* 1~9, 0 */
    19, 21, 22, /* '-', 'backspace, tab */
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, /* 'q'~'p' */
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, /* 'q'~'p' */
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, /* 'q'~'p' */
    35, 35, 35, /* ret/enter */
    36, 37, 38, 39, 40, 41, 42, 43, 44, /* 'a'~'l' */
    36, 37, 38, 39, 40, 41, 42, 43, 44, /* 'a'~'l' */
    36, 37, 38, 39, 40, 41, 42, 43, 44, /* 'a'~'l' */
    45, 46, /* ';', ''' */
    49, 50, 51, 52, 53, 54, 55, /* 'z'~'m' */
    49, 50, 51, 52, 53, 54, 55, /* 'z'~'m' */
    49, 50, 51, 52, 53, 54, 55, /* 'z'~'m' */
    56, 57, /* ',', '.' */
};
static const int qcodebook_len = sizeof(key_qcodebook) / sizeof(int);

static const int64_t seq_gap[] = {
    1, 1, 1, 1, 1, /* 1 second */
    2, 2, 2, /* 2 seconds */
    3, 3, /* 3 seconds */
    6, 7, 8, 9, /* 6s, 7s, 8s, 9s */
    20, /* 20s */
};
static const int seq_gap_len = sizeof(seq_gap) / sizeof(int64_t);

#define MAX_SEQ_LEN 8

static void hit_random_key_sequence(void *opaque){
    /* 1~8 characters per seq */
    int seq_len, key_idx, qcode, sgap_idx;
    bool hit_shift = false;

    srand((unsigned)time(NULL));
    seq_len = (rand() % MAX_SEQ_LEN) + 1;
    for (int i = 0; i < seq_len; i++){
        /* get a random key from key_qcodebook */
        key_idx = rand() % qcodebook_len;
        qcode = key_qcodebook[key_idx];

        /* send qcode */
        hit_shift = (rand() % 59) == 0 ? true : false;
        if (hit_shift){
            qemu_input_event_send_key_qcode(NULL, Q_KEY_CODE_SHIFT, true);
        }
        qemu_input_event_send_key_qcode(NULL, qcode, true);
        qemu_input_event_send_key_qcode(NULL, qcode, false);
        if (hit_shift){
            qemu_input_event_send_key_qcode(NULL, Q_KEY_CODE_SHIFT, false);
            hit_shift = false;
        }
    }
    /* split with spaces */
    qemu_input_event_send_key_qcode(NULL, Q_KEY_CODE_SPC, true);
    qemu_input_event_send_key_qcode(NULL, Q_KEY_CODE_SPC, false);

    /* reset timer, 1~30 seconds a seq */
    sgap_idx = rand() % seq_gap_len;
    int64_t sgap = seq_gap[sgap_idx];
    QEMUTimer *ts = opaque;
    sgap *= 1000; /* milliseconds */
    timer_mod(ts, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + sgap);
}

void hit_random_key_sequence_timer(void){
    /* create a timer on default timer list*/
    QEMUTimer *ts = timer_new_ms(QEMU_CLOCK_VIRTUAL, hit_random_key_sequence, NULL);
    ts->opaque = ts; /* brutal */
    int64_t init_delay = 20 * 1000; /* wait for system setup, in milliseconds */
    timer_mod(ts, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + init_delay);
}
