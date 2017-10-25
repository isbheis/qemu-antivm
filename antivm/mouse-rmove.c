/*
 * 2017-10-19
 * implement of mouse_move_random_thread function
 */

#include "qemu/osdep.h"
#include "antivm/mouse-rmove.h"
#include "qemu/notify.h"
#include "ui/input.h"
#include "qemu/thread.h"

#define AXIS_X 0
#define AXIS_Y 1
#define MIN_DELAY 100		/* milliseconds */
#define MAX_DELAY 100000	/* milliseconds */


static void * mouse_move_random(void * delay)
{
    int td = 1000;
    usleep(10 * td * 1000);
    int con_count, rfactor;
    bool pulse = false;
    for(;;){
        /* decide continously events count and rest factor for next round */
        srand((unsigned)time(NULL));
        con_count = (rand() % 6) + 1;
        rfactor = (rand() % 4) + 1;
        pulse = rfactor > (con_count + 1);

        while(con_count > 0){
            /* get random point in a fixed square */
            int dx, dy;
            int square = 20;
            dx = (dx = rand() % square) & 0x1 ? -dx : dx;
            dy = (dy = rand() % square) & 0x1 ? -dy : dy;
            if (pulse && con_count == 0){
                dx += (dx > 0 ? 1 : -1) * 40;
                dy += (dy > 0 ? 1 : -1) * 40;
            }

            /* send mouse move events */
            qemu_input_queue_rel(NULL, AXIS_X, dx);
            qemu_input_queue_rel(NULL, AXIS_Y, dy);

            /* synchronize ui*/
            qemu_input_event_sync();

            /* delay */
            usleep(100 * 1000);
            con_count -= 1;
        }

        /* delay for next round */
        usleep(rfactor * td * 1000);
    }
    return NULL;
}


static bool single = false;
void mouse_move_random_thread(void)
{
    /* check if thread exists */
    if (single){
        return;
    }

    /* create thread */
    QemuThread mr_thread;
    const char* name = "mouse-rmove";
    qemu_thread_create(&mr_thread, name, mouse_move_random,
                       NULL, QEMU_THREAD_DETACHED);
    single = true;
}
