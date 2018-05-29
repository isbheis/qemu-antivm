/*
 * parse picture button with location info
 * 2018-03-21
 */

#include "qemu/osdep.h"
#include "antivm/pic_button.h"
#include "qemu/notify.h"
#include "ui/input.h"
#include "qmp-commands.h"
#include "ui/console.h"
#include "qemu/thread.h"


const char* bl_text[] = {"cancel", "back", "exit", "no"};

// rect format: 'left top right botton text'
static void split_rect_info(char* rect_info, int rect[4], char* text){
    char temp[16] = { '\0' };
    char *dst = temp;
    char *p = rect_info;
    int idx = 0;
    while (1){
        while (*p != ' ')
            *dst++ = *p++;
        rect[idx] = atoi(temp);
        memset(temp, 0, sizeof(temp));
        p++;
        idx++;
        if (idx == 4){
            // reach text
            while (*p)
                *text++ = *p++;
            *text = '\0';
        }
    }
    fprintf(stderr, "in recognize file: button loc (%d, %d, %d, %d) text: %s\n",
            rect[0], rect[1], rect[2], rect[3], text);
}

static bool hit_text_black_list(char* btn_text){
    int len = (int)(sizeof(bl_text) / sizeof(NULL));
    for (int i = 0; i < strlen(btn_text); i++)
        btn_text[i] = tolower(btn_text[i]);
    for (int i = 0; i < len; i++){
        if (strcmp((char*)bl_text[i], btn_text) == 0)
            return true;
    }
    return false;
}

#define ABS_MIN 0
#define ABS_MAX 0x7ffff
static void send_click_event(int rect[4]){
    int x, y;
    x = (int)((rect[2] - rect[0]) / 2);
    y = (int)((rect[3] - rect[1]) / 2);
    qemu_input_queue_abs(NULL, INPUT_AXIS_X, x, ABS_MIN, ABS_MAX);
    qemu_input_queue_abs(NULL, INPUT_AXIS_Y, y, ABS_MIN, ABS_MAX);
    qemu_input_event_sync();
}

static bool check_gui_changed(void){
    // exec python script to compare current screen dump with the initial one
    pid_t ppid = getppid();
    pid_t pid = getpid();
    fprintf(stderr, "in check_gui_changed: pid %d, ppid %d\n", pid, ppid);
    char sd_ini[64] = { '\0' };
    char sd_cur[64] = { '\0' };
    sprintf(sd_ini, "/tmp/screen_dump_%d.png", ppid);
    sprintf(sd_cur, "/tmp/screen_dump_%d_cmp.png", pid);
    Error* error = NULL;
    qmp_screendump((const char*)sd_cur, &error);
    const char* python = "/usr/bin/python3";
    char* const script = (char* const)"compare_picture.py";
    char* const args[] = {(char* const)python, script, (char* const)sd_ini, (char* const)sd_cur};
    error = NULL;
    pid_t pid_fork = qemu_fork(&error);
    if (pid_fork == 0){
        // child process
        execv(python, args);
    }else if (pid_fork > 0 ){
        // current process(thread)
        int status;
        pid_t child;
        child = waitpid(pid_fork, &status, 0);
        if (child == -1){
            fprintf(stderr, "waitpid failed!\n");
            return false;
        }
        if (status == 0){
            // screen dumps are similar
            return false;
        }else if (WIFEXITED(status)){
            if (WEXITSTATUS(status) == 1){
                // screen dumps diff
                return true;
            }else{
                // some error occur
                fprintf(stderr, "unkown error! child exit with status: %d\n",
                        WEXITSTATUS(status));
                return false;
            }
        }else{
            // child terminate due to other reasons
            fprintf(stderr, "child exited with status: %d\n", status);
            return false;
        }

    }else{
        fprintf(stderr, "qemu_fork failed in 'check_gui_changed'\n");
        return false;
    }
    return true;
}

static void click_rect(char* res_filename){
        FILE *fh = NULL;
        if ((fh = fopen(res_filename, "rb")) == NULL){
            fprintf(stderr, "fopen %s failed\n", res_filename);
            exit(0);
        }
        char rect_buf[256] = { '\0' };
        int rect[4] = { 0 };
        char text[256] = { '\0' };
        while (!feof(fh)){
            // one line per time
            if (fgets(rect_buf, sizeof(rect_buf), fh) == NULL){
                fprintf(stderr, "fgets failed or end of file in 'click_rect'\n");
                return;
            }
            if (rect_buf[0] > 0x39 || rect_buf[0] < 0x30){
                // not 0 ~ 9, skip
                fprintf(stderr, "invalid recognized result line: %s\n", rect_buf);
                continue;
            }
            split_rect_info(rect_buf, rect, text);
            if (hit_text_black_list(text)){
                memset(text, 0, sizeof(text));
                memset(rect, 0, sizeof(rect));
                continue;
            }
            fprintf(stderr, "click rect: (%d %d %d %d), '%s'\n", rect[0], rect[1], rect[2], rect[3], text);
            send_click_event(rect);
            usleep(500);    // wait gui to change
            if (!check_gui_changed()){
                memset(text, 0, sizeof(text));
                memset(rect, 0, sizeof(rect));
                continue;
            }else{
                break;
            }
        }
        // gui changed or no button to handle
        exit(0);
}

static void* parse_picture_button_impl(void* opaque){
    pid_t pid = getpid();
    // get screen dump
    char sd_filename[64] = { '\0' };
    sprintf(sd_filename, "/tmp/screen_dump_%d.png", pid);
    Error* error = NULL;
    qmp_screendump((const char*)sd_filename, &error);
    fprintf(stderr, "initial screen dump save to file: %s\n", sd_filename);

    // execute python script to parse pic button
    char loc_filename[64] = { '\0' };
    char res_filename[64] = { '\0' };
    sprintf(loc_filename, "/tmp/screen_loc_%d.txt", pid);
    sprintf(res_filename, "/tmp/screen_res_%d.txt", pid);
    error = NULL;
    pid_t pid_fork = qemu_fork(&error);
    if (pid_fork == 0){
        // child process
        fprintf(stderr, "recognize script process id: %d, parent id: %d\n", getpid(), getppid());
        const char* python = "/usr/bin/python3";
        char* const script = (char* const)"/tmp/recognize_pic_button.py";
        char* const option = (char* const)"-o";
        char* const args[] = {(char* const)python, script, option, (char* const)res_filename, (char* const)sd_filename, (char* const)loc_filename, NULL};
        execv(python, args);
    }else if (pid_fork > 0){
        // parent process(thread)
        usleep(6000);
        // wait for child process
        while (access(res_filename, F_OK) == -1){
            usleep(1000);
        }
        // file is null
        struct stat file_stat;
        memset(&file_stat, 0, sizeof(file_stat));
        stat(res_filename, &file_stat);
        if (file_stat.st_size == 0){
            fprintf(stderr, "recognize failed!\n");
            return NULL;
        }
        // read result file and click button
        click_rect(res_filename);
    }else{
        fprintf(stderr, "qemu_fork failed in 'parse_picture_button_impl'\n");
        return NULL;
    }
    return NULL;
}


void parse_picture_button(int loc[4]){
    // save button laction info
    char loc_filename[64] = { '\0' };
    sprintf(loc_filename, "/tmp/screen_loc_%d.txt", getpid());
    FILE* fh = fopen(loc_filename, "w");
    if (fh == NULL){
        fprintf(stderr, "fopen %s failed\n", loc_filename);
        return;
    }
    char loc_buf[96] = { '\0' };
    sprintf(loc_buf, "%d %d %d %d\n", loc[0], loc[1], loc[2], loc[3]);
    fputs(loc_buf, fh);
    fclose(fh);
    fprintf(stderr, "pic button found: %d %d %d %d\n", loc[0], loc[1], loc[2], loc[3]);
    // create thread
    QemuThread ppb_thread;
    const char* name = "parse_pic_button";
    fprintf(stderr, "qemu main thread id: %d\n", getpid());
    qemu_thread_create(&ppb_thread, name, parse_picture_button_impl,
                        (int *)loc, QEMU_THREAD_DETACHED);
    return;
}
