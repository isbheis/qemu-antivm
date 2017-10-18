/*
 * 2017-10-12
 * implement of mouse_move_random function
 */

#include "qemu/osdep.h"
#include "antivm/mouse-rmove.h"
#include "qemu/notify.h"
#include "ui/input.h"

#define AXIS_X 0
#define AXIS_Y 1


void mouse_move_random(void){
    /* get random point in a fixed square */
    int dx, dy;
    int square = 24;
    srand((unsigned)time(NULL));
    dx = (dx = rand() % square) & 0x1 ? -dx : dx;
    dy = (dy = rand() % square) & 0x1 ? -dy : dy;

    /* send mouse move events */
    qemu_input_queue_rel(NULL, AXIS_X, dx);
    qemu_input_queue_rel(NULL, AXIS_Y, dy);

    /* synchronize ui*/
    qemu_input_event_sync();
}
