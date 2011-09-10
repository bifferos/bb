/*
  A couple of examples to demonstrate RTAI programming on the Bifferboard.
 
  Useage: rtai-led-demo [blink|pulsate]
 
 
 */

#define REALLY_SLOW_IO

#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <rtai_lxrt.h>
#include <rtai_sem.h>
#include <rtai_usi.h>
#include <rtai_fifos.h>
#include <time.h>
#include <sys/io.h>

RT_TASK *maint;
int squarethread;


static volatile int exec_end = 1;



// Defines necessary to control the red LED

#define CONTROL 0x80003848
#define DATA 0x8000384c
#define BIT_LED  (1<<16)   // GPIO 16, red LED.
#define LED_LOW  outl(0, 0xcfc);    // start off with it high.
#define LED_HIGH  outl(BIT_LED, 0xcfc);



// This is the simplest example of a periodic task, it just blinks the LED on 
// and off.  Of course, you can do the same thing with a bash script, but this
// demonstrates the mechanics of setting up the realtime task.
static void *blink_task(void *args) {
    
        // in nanoseconds (0.5 seconds)
        unsigned BLINK_PERIOD = 500000000;
        
        RT_TASK *handler;
        RTIME period;
        int led_state=0;

        if (!(handler = rt_task_init_schmod(nam2num("ACHLR"), 
                 0, 0, 0, SCHED_FIFO, 0xF))) {
                printf("Unable to init timer handler task.\n");
                exit(1);
        }

        rt_allow_nonroot_hrt();
        // no swapping
        mlockall(MCL_CURRENT | MCL_FUTURE);

        rt_set_oneshot_mode();
        start_rt_timer(0);
        period = nano2count(BLINK_PERIOD);
        rt_make_hard_real_time();
        exec_end = 0;
        rt_task_make_periodic(handler, rt_get_time() + period, period);
        rt_task_wait_period();

        while (!exec_end) {
                rt_task_wait_period();

                if (led_state) {
                  LED_LOW;
                  led_state = 0;                
                } else {
                  LED_HIGH;                
                  led_state = 1;
                }
        }
        
        stop_rt_timer();
        rt_make_soft_real_time();
        rt_task_delete(handler);
        return 0;
}



static volatile int pulsate_brightness = 0;
static char pulsate_duty[10];
static void pulsate_set_brightness(int level)
{
  int i;
  for (i=0;i<sizeof(pulsate_duty);i++)
  {
    pulsate_duty[i] = (i<level) ? 1 : 0;
  }
}



// Pulsing dim-bright-dim
static void *pulsate_task(void *args) {
        unsigned pulsate_period = 1000000;
        RT_TASK *handler;
        RTIME period;
        int duty_step = 0;
        int brightness = 0;
        int bright_count = 0;
        int bright_inc = 1;
        
        pulsate_set_brightness(brightness);
        
        if (!(handler = rt_task_init_schmod(nam2num("ACHLR"), 
                 0, 0, 0, SCHED_FIFO, 0xF))) {
                printf("Unable to init timer handler task.\n");
                exit(1);
        }

        rt_allow_nonroot_hrt();
        // no swapping
        mlockall(MCL_CURRENT | MCL_FUTURE);

        rt_set_oneshot_mode();
        start_rt_timer(0);
        period = nano2count(pulsate_period);
        rt_make_hard_real_time();
        exec_end = 0;
        rt_task_make_periodic(handler, rt_get_time() + period, period);
        rt_task_wait_period();

        while (!exec_end) {
                rt_task_wait_period();

                if (pulsate_duty[duty_step])
                {
                  LED_LOW;
                } else {
                  LED_HIGH;
                }
                duty_step++;
                if (duty_step >= sizeof(pulsate_duty)) 
                  duty_step = 0;
                bright_count++;
                if (bright_count>50)
                {
                  bright_count = 0;
                  
                  brightness += bright_inc;
                  
                  if (brightness > 9)
                    bright_inc = -1;
                  if (brightness < 0)
                    bright_inc = 1;
                  pulsate_set_brightness(brightness);
                }
        }
        
        stop_rt_timer();
        rt_make_soft_real_time();
        rt_task_delete(handler);
        return 0;
}





void cleanup(int sig) {
        exec_end = 1;
        return;
}


void Help(void)
{
    printf("Usage: rtai-led-demo [blink|pulsate]\n");
    exit(-1);
}

typedef enum
{
    blink,
    pulsate
} Operation;


int main(int argc, char *argv[]) {
        unsigned long tmp;
        Operation op = blink;  // 1 == blink, 2 = pulsate
        
        if (argc != 2)
        {
            Help();
        }
        
        if (strcmp(argv[1],"blink")==0)
        {
            op = blink;
        } 
        else if (strcmp(argv[1],"pulsate")==0)
        {
            op = pulsate;
        }
        else
        {
            Help();
        }
        

        // Catch some signals, because we need to clean up        
        signal(SIGTERM, cleanup);
        signal(SIGINT, cleanup);
        signal(SIGKILL, cleanup);
        
        if (!(maint = rt_task_init(nam2num("MAIN"), 1, 0, 0))) {
                printf("Cannot initialise main task.\n");
                exit(1);
        }

        // ask for permission to access the port from user-space
        ioperm(0xcf8, 8, 1);
        ioperm(0x80,1,1);  // we need this, for the port delays


        // Set lines as GPIO
        outl(CONTROL, 0xcf8);  // Set control register
        tmp = inl(0xcfc);
        tmp |= BIT_LED;
        outl(tmp, 0xcfc);     
        outl(DATA, 0xcf8);     // leave pointing to register data.
                    

        
        squarethread = rt_thread_create(
                (op == blink) ? blink_task : pulsate_task, NULL, 10000);

        // Wait here until start-up
        while (exec_end) {   
                usleep(100000);
        }
        printf("Task is now realtime!\n");
        while (!exec_end) {
          usleep(200000);
        }

        rt_thread_join(squarethread);
        rt_task_delete(maint);
        return 0;
}

