/**
 * @file main.c
 * @brief Entrypoint of kernel systems
 *
 * Contains initial setup, and beginning of
 * main superloop for responsible for uninterrupted
 * runtime.
 *
 * @authors Nithin Senthil, Parteek Singh, Jacob Tkeo
 * @date 6/../23
 */

/* Globals */
#include "./scheduler/schedulerGlobals.h"

/* File Includes */
#include "scheduler/scheduler.h"
#include "scheduler/task.h"
#include "scheduler/status.h"

/* Macros */
#define SYSTICK_DUR_U 10000          // Config. of systick timer in usec (1 ms)
#define BATTERY_THRESHOLD 20        // TODO: Min. battery voltage value, below which mode -> CHARGING

/* Misc variables */
int reboot_count;
// volatile uint16_t flagBits = 0;     // Declared in status.h
volatile struct OperationBits flagBits = {0,0};
jmp_buf to_mode_select;

/* Testing Variables */
int max_handler_count;
int systick_handler_count;
int is_unlimited_tick;

// Define signal handler and timer
struct sigaction sysTick;
struct itimerval sysTick_timer;

/* Prototypes */
void sysTickHandler(int signal);


/**
 * @brief Initial startup mode
 *
 * The one-time 30 minute wait when the Intellisat
 * is first released from the ISS.
 *
 * @note Load initial values from flash
 * @todo Replace sleep time to 30 min
 */
void startup() {
    // TODO: Retrieve base info from flash (flagBits, reboot_count, etc.)
    reboot_count++;
    if (!IS_BIT_SET(flagBits.statusBits, START)) {
        printf("First startup detected\nStarting 5 second wait...");
        usleep(5000000); // TODO: replace with correct wait period (30min)

        SET_BIT(flagBits.statusBits, START); // TODO: Intentionally at the end in case of failure during wait state (REVISIT)

    } else {
        printf("Loading Backups\nPlease wait (5s)...\n");
        usleep(5000000); // TESTING: remove when backups implemented
        // TODO: Load backups here
    }
}

/**
 * @brief Timer based interrupt handler
 *
 * Handler called when timer interrupt fires
 * and serves primarily as a wrapper for
 * the main scheduling logic.
 *
 * @param signal Corresponding interrupt bit that was triggered
 * @see scheduler() (scheduler.c)
 * @note Will be merged with master sysTickHandler with
 *       other functionality on hardware Intellisat
 */
void sysTickHandler(int signal) {
    // // Backup guard from other alarms (Likely can be removed)
    // if(signal != SIGALRM) {
    //     return;
    // }

    systick_handler_count--; // TESTING: remove when done debugging
    scheduler(signal, &to_mode_select);
}

/**
 * @brief Virtual Intellisat configuration
 *
 * Limiter on number of cycles before termination.
 * Configures virtual timers.
 *
 * @param argc Number of CLI args
 * @param argv CLI args
 * @note Remove during hardware integration
 */
void virtualTesting(int argc, char *argv[]) {
    // Testing limiter
    if (argc >= 2) {
        max_handler_count = atoi(argv[1]);
        is_unlimited_tick = 0;
    } else {
        max_handler_count = -1;
        is_unlimited_tick = 1;
    }
    
    if (argc >= 3) {
        if (atoi(argv[2]) == 1) {
            SET_BIT(flagBits.statusBits, START);
        }
    }
    systick_handler_count = max_handler_count;
    printf("Inputted handler count: %d\n", max_handler_count);


    /* System handler, timer setup */
    // Install custom ISR as handler for SIGALRM
    sysTick.sa_handler = &sysTickHandler;
    sigaction(SIGALRM, &sysTick, NULL);

    // Configure and start sys timer
    sysTick_timer.it_value.tv_sec = 0;
    sysTick_timer.it_value.tv_usec = SYSTICK_DUR_U;
    sysTick_timer.it_interval.tv_sec = 0;
    sysTick_timer.it_interval.tv_usec = SYSTICK_DUR_U;
}


/**
 * @brief Superloop
 *
 * Contains initial timer and interrupt handler (TESTING) and
 * the main superloop. Serves as standard behavior when
 * there is no scheduler intervention.
 */
int main(int argc, char *argv[]) {

    //  Virtual Intellisat configuration
    virtualTesting(argc, argv);

    // Initial configuration
    startup();

    /* Set up rand function for testing */
	srand(2);
	printf("start\n");
	
    /* Run initial mode decision */
    systemsCheck(); // All other mode decisions done via task ISR
    currTask = taskTable[0];
  
    /* Start sys timer */
    setitimer(ITIMER_REAL, &sysTick_timer, NULL);
    
    /* Define jmp point */
    sigsetjmp(to_mode_select, 1); // PROTOTYPE
    // setjmp(toModeSelect); SWITCH FOR Hardware Intellisat

    /* Run superloop */
    while (1) {
        printf("\n");

        modeSelect();

    	printf("ID: %d\n", currTask.task_id);

        currTask.runPtr();  // usleep(rand) done here
	    CLR_BIT(flagBits.modeBits, currTask.task_id);

        printf("Task %d is successful.\n", currTask.task_id);

        
        // Cycle limiter for testing
        printf("systickHandlerCount: %d\n", max_handler_count - systick_handler_count);
        if (!is_unlimited_tick && systick_handler_count <= 0) {
            printf("Terminating Kernel\n");
            return 0;
        }
    }

    return 0;
}
