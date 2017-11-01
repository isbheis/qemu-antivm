/*
 * 2017-10-19
 * implement of mouse_move_random_thread function
 */

#include "qemu/osdep.h"
#include "antivm/mouse-rmove.h"
#include "qemu/notify.h"
#include "ui/input.h"
#include "qemu/timer.h"

#define AXIS_X 0
#define AXIS_Y 1

static void mouse_move_random(void *opaque)
{
    /* get random point in a fixed square */
    int dx, dy;
    srand((unsigned)time(NULL));
    int square = 20;
    dx = (dx = rand() % square) & 0x1 ? -dx : dx;
    dy = (dy = rand() % square) & 0x1 ? -dy : dy;

    /* send mouse move events */
    qemu_input_queue_rel(NULL, AXIS_X, dx);
    qemu_input_queue_rel(NULL, AXIS_Y, dy);

    /* synchronize ui*/
    qemu_input_event_sync();

    /* reset timer */
    QEMUTimer *ts = opaque;
    int64_t delay = 2 * 1000; /* milliseconds */
    timer_mod(ts, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + delay);
}

void mouse_move_random_timer(void)
{
    /* create a timer on default timer list*/
    QEMUTimer *ts = timer_new_ms(QEMU_CLOCK_VIRTUAL, mouse_move_random, NULL);
    ts->opaque = ts; /* brutal */
    int64_t init_delay = 10 * 1000; /* milliseconds */
    timer_mod(ts, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + init_delay);
}
