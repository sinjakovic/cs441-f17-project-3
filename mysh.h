/*
 * Josh Hursey, Samantha Foley, Brandon Sinjakovic
 *
 * CS441/541: Project 2
 *
 */
#ifndef MYSHELL_H
#define MYSHELL_H

#include "support.h"

/* For fork, exec, sleep */
#include <sys/types.h>
#include <unistd.h>
/* For waitpid */
#include <sys/wait.h>
/* For open */
#include <fcntl.h>


/******************************
 * Defines
 ******************************/
#define PROMPT ("mysh$ ")


/******************************
 * Structures
 ******************************/
struct job {
    int pid;
    int is_running;
    char ** argv;
    int argc;
    char * full_command;
    int job_number;
    int is_background;
    int display;
};
 
struct jobs_array {
    int size;
    struct job ** jobs_arr;
};

/******************************
 * Global Variables
 ******************************/
/*
 * Arrays
 */

struct jobs_array * bg_jobs;
struct jobs_array * jobs_history;

/*
 * Debugging toggle
 */
int is_debug = FALSE;

/*
 * Interactive or batch mode
 */
int is_batch = FALSE;

/*
 * Batch file names
 */
int num_batch_files = 0;
char **batch_files = NULL;

/*
 * Counts
 */
int total_jobs_display_ctr = 0;
int total_jobs    = 0;
int total_jobs_bg = 0;
int total_history = 0;

int num_bg_jobs = 0;

/*
 * If we are exiting
 */
int exiting = FALSE;


/******************************
 * Function declarations
 ******************************/
/*
 * Parse command line arguments passed to myshell upon startup.
 *
 * Parameters:
 *  argc : Number of command line arguments
 *  argv : Array of pointers to strings
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int parse_args_main(int argc, char **argv);

/*
 * Main routine for batch mode
 *
 * Parameters:
 *  None
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int batch_mode(void);

/*
 * Main routine for interactive mode
 *
 * Parameters:
 *  None
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int interactive_mode(void);

/*
 * Split possible multiple jobs on a command line, then call parse_and_run()
 *
 * Parameters:
 *  command : command line string (may contain multiple jobs)
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int split_parse_and_run(char * command);

/*
 * Parse and execute a single job given to the prompt.
 *
 * Parameters:
 *   loc_job : job to execute
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int parse_and_run(job_t * loc_job);

/*
 * Launch a job
 *
 * Parameters:
 *   loc_job : job to execute
 *
 * Returns:
 *   0 on success
 *   Negative value on error 
 */
int launch_job(job_t * loc_job);

/*
 * Built-in 'exit' command
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int builtin_exit(void);

/*
 * Built-in 'jobs' command
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int builtin_jobs(void);

/*
 * Built-in 'history' command
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int builtin_history(void);

/*
 * Built-in 'wait' command
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int builtin_wait(void);

/*
 * Built-in 'fg' command
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int builtin_fg();

/*
 * Built-in 'fg' command
 *
 * Parameters:
 *   job id
 *
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int builtin_fg_with_id(int id);

/*
 * Adds a job to a job_array
 *
 * Parameters:
 *   -jobs_array: add job to seleced array
 *   -full_command: the full command of the job
 *   -pid: the process id of the job
 *   -argc: the number of args the array has
 *   -job_number: the job id 
 *   -argv: the arguments of the job
 *   -size: the size of the array
 *   -is_background: true if the job is in the background
 *    
 * Returns:
 *   0 on success
 *   Negative value on error
 */
int add_job(struct jobs_array * jobs_array, char * full_command, int pid, 
        int argc, int job_number, char ** argv, int size, int is_background);
        

#endif /* MYSHELL_H */
