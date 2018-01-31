/*
 * Josh Hursey, Samantha Foley, Brandon Sinjakovic
 *
 * CS441/541: Project 2
 *
 */
#include "mysh.h"

int main(int argc, char * argv[]) {
    int ret = 0;

    bg_jobs = malloc(sizeof(struct jobs_array));
    bg_jobs->jobs_arr = malloc(sizeof(struct job *));
    
    jobs_history = malloc(sizeof(struct jobs_array));
    jobs_history->jobs_arr = malloc(sizeof(struct job *));
    
    
    /*
     * Parse Command line arguments to check if this is an interactive or batch
     * mode run.
     */
    if( 0 != (ret = parse_args_main(argc, argv)) ) {
        fprintf(stderr, "Error: Invalid command line!\n");
        return -1;
    }

    /*
     * If in batch mode then process all batch files
     */
    if( TRUE == is_batch) {
        if( TRUE == is_debug ) {
            printf("Batch Mode!\n");
        }

        if( 0 != (ret = batch_mode()) ) {
            fprintf(stderr, "Error: Batch mode returned a failure!\n");
        }
    }
    /*
     * Otherwise proceed in interactive mode
     */
    else if( FALSE == is_batch ) {
        if( TRUE == is_debug ) {
            printf("Interactive Mode!\n");
        }

        if( 0 != (ret = interactive_mode()) ) {
            fprintf(stderr, "Error: Interactive mode returned a failure!\n");
        }
    }
    /*
     * This should never happen, but otherwise unknown mode
     */
    else {
        fprintf(stderr, "Error: Unknown execution mode!\n");
        return -1;
    }


    /*
     * Display counts
     */
    printf("-------------------------------\n");
    printf("Total number of jobs               = %d\n", total_jobs);
    printf("Total number of jobs in history    = %d\n", total_history);
    printf("Total number of jobs in background = %d\n", total_jobs_bg);

    /*
     * Cleanup
     */
    if( NULL != batch_files) {
        free(batch_files);
        batch_files = NULL;
        num_batch_files = 0;
    }

    return 0;
}

int parse_args_main(int argc, char **argv)
{
    int i;

    /*
     * If no command line arguments were passed then this is an interactive
     * mode run.
     */
    if( 1 >= argc ) {
        is_batch = FALSE;
        return 0;
    }

    /*
     * If command line arguments were supplied then this is batch mode.
     */
    is_batch = TRUE;
    num_batch_files = argc - 1;
    batch_files = (char **) malloc(sizeof(char *) * num_batch_files);
    if( NULL == batch_files ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }

    for( i = 1; i < argc; ++i ) {
        batch_files[i-1] = strdup(argv[i]);
    }

    return 0;
}

int batch_mode(void)
{
    int i;
    int ret;
    char * command = NULL;
    char * cmd_rtn = NULL;
    FILE *batch_fd = NULL;

    command = (char *) malloc(sizeof(char) * (MAX_COMMAND_LINE+1));
    if( NULL == command ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }

    for(i = 0; i < num_batch_files; ++i) {
        if( TRUE == is_debug ) {
            printf("-------------------------------\n");
            printf("Processing Batch file %2d of %2d = [%s]\n", i, num_batch_files, batch_files[i]);
            printf("-------------------------------\n");
        }

        /*
         * Open the batch file
         * If there was an error then print a message and move on to the next file.
         */
        if( NULL == (batch_fd = fopen(batch_files[i], "r")) ) {
            fprintf(stderr, "Error: Unable to open the Batch File [%s]\n", batch_files[i]);
            continue;
        }

        /*
         * Read one line at a time.
         */
        while( FALSE == exiting && 0 == feof(batch_fd) ) {

            /* Read one line */
            command[0] = '\0';
            if( NULL == (cmd_rtn = fgets(command, MAX_COMMAND_LINE, batch_fd)) ) {
                break;
            }

            /* Strip off the newline */
            if( '\n' == command[strlen(command)-1] ) {
                command[strlen(command)-1] = '\0';
            }

            /*
             * Parse and execute the command
             */
            if( 0 != (ret = split_parse_and_run(command)) ) {
                fprintf(stderr, "Error: Unable to run the command \"%s\"\n", command);
            }
        }

        /*
         * Close the batch file
         */
        fclose(batch_fd);
    }

    /*
     * Cleanup
     */
    if( NULL != command ) {
        free(command);
        command = NULL;
    }

    return 0;
}

int interactive_mode(void)
{
    int ret;
    char * command = NULL;
    char * cmd_rtn = NULL;

    /*
     * Display the prompt and wait for input
     */
    command = (char *) malloc(sizeof(char) * (MAX_COMMAND_LINE+1));
    if( NULL == command ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }

    do {
        /*
         * Print the prompt
         */
        printf("%s", PROMPT);
        fflush(NULL);

        /*
         * Read stdin, break out of loop if Ctrl-D
         */
        command[0] = '\0';
        if( NULL == (cmd_rtn = fgets(command, MAX_COMMAND_LINE, stdin)) ) {
            printf("\n");
            fflush(NULL);
            break;
        }

        /* Strip off the newline */
        if( '\n' == command[strlen(command)-1] ) {
            command[strlen(command)-1] = '\0';
        }

        /*
         * Parse and execute the command
         */
        if( 0 != (ret = split_parse_and_run(command)) ) {
            fprintf(stderr, "Error: Unable to run the command \"%s\"\n", command);
            /* This is not critical, just try the next command */
        }

    } while( NULL != cmd_rtn && FALSE == exiting);

    /*
     * Cleanup
     */
    if( NULL != command ) {
        free(command);
        command = NULL;
    }

    return 0;
}

int split_parse_and_run(char * command)
{
    int ret, i, j;
    int    num_jobs = 0;
    job_t *loc_jobs = NULL;
    char * dup_command = NULL;
    int bg_idx;
    int valid = FALSE;

    /*
     * Sanity check
     */
    if( NULL == command ) {
        return 0;
    }

    /*
     * Check for multiple sequential or background operations on the same
     * command line.
     */
    /* Make a duplicate of command so we can sort out a mix of ';' and '&' later */
    dup_command = strdup(command);

    /******************************
     * Split the command into individual jobs
     ******************************/
    /* Just get some space for the function to hold onto */
    loc_jobs = (job_t*)malloc(sizeof(job_t) * 1);
    if( NULL == loc_jobs ) {
        fprintf(stderr, "Error: Failed to allocate memory! Critical failure on %d!", __LINE__);
        exit(-1);
    }
    split_input_into_jobs(command, &num_jobs, &loc_jobs);

    /*
     * For each job, check for background or foreground
     * Walk the command string looking for ';' and '&' to identify each job as either
     * sequential or background
     */
    bg_idx = 0;
    valid = FALSE;
    for(i = 0; i < strlen(dup_command); ++i ) {
        /* Sequential separator */
        if( dup_command[i] == ';' ) {
            if( TRUE == valid ) {
                loc_jobs[bg_idx].is_background = FALSE;
                ++bg_idx;
                valid = FALSE;
            }
            else {
                fprintf(stderr, "Error: syntax error near unexpected token ';'\n");
            }
        }
        /* Background separator */
        else if( dup_command[i] == '&' ) {
            if( TRUE == valid ) {
                loc_jobs[bg_idx].is_background = TRUE;
                ++bg_idx;
                valid = FALSE;
            }
            else {
                fprintf(stderr, "Error: syntax error near unexpected token '&'\n");
            }
        }
        /*
         * Look for valid characters. So we can print an error if the user
         * types: date ; ; date
         */
        else if( dup_command[i] != ' ' ) {
            valid = TRUE;
        }
    }

    /*
     * For each job, parse and execute it
     */
    for( i = 0; i < num_jobs; ++i ) {
        
        /* Add job to history array */
        add_job(jobs_history, loc_jobs->full_command, 0, 0,
                    total_jobs+1, NULL ,total_history, loc_jobs->is_background);
                    
        if( 0 != (ret = parse_and_run( &loc_jobs[i] )) ) {
            fprintf(stderr, "Error: The following job failed! [%s]\n", loc_jobs[i].full_command);
        }
    }

    /*
     * Cleanup
     */
    if( NULL != loc_jobs ) {
        /* Cleanup struct fields */
        for( i = 0; i < num_jobs; ++i ) {
            if( NULL != loc_jobs[i].full_command ) {
                free( loc_jobs[i].full_command );
                loc_jobs[i].full_command = NULL;
            }

            if( NULL != loc_jobs[i].argv ) {
                for( j = 0; j < loc_jobs[i].argc; ++j ) {
                    if( NULL != loc_jobs[i].argv[j] ) {
                        free( loc_jobs[i].argv[j] );
                        loc_jobs[i].argv[j] = NULL;
                    }
                }
                free( loc_jobs[i].argv );
                loc_jobs[i].argv = NULL;
            }

            loc_jobs[i].argc = 0;

            if( NULL != loc_jobs[i].binary ) {
                free( loc_jobs[i].binary );
                loc_jobs[i].binary = NULL;
            }
        }
        /* Free the array */
        free(loc_jobs);
        loc_jobs = NULL;
    }

    if( NULL != dup_command ) {
        free(dup_command);
        dup_command = NULL;
    }

    return 0;
}

int parse_and_run(job_t * loc_job)
{
    int ret;
    int file_in = -1;
    int file_out = -1;
    int i;
    int new_argc = 1;

    /*
     * Sanity check
     */
    if( NULL == loc_job ||
        NULL == loc_job->full_command ) {
        return 0;
    }

    /*
     * No command specified
     */
    if(0 >= strlen( loc_job->full_command ) ) {
        return 0;
    }

    if( TRUE == is_debug ) {
        printf("        \"%s\"\n", loc_job->full_command );
    }
    
    

    ++total_history;

    /******************************
     * Parse the string into the binary, and argv
     ******************************/
    split_job_into_args(loc_job);
    
        for(i = 1; i < loc_job->argc; i++) {
        if(strcmp(loc_job->argv[i], ">") == 0) {
            loc_job->argv[i] = NULL;
            if(i+1 < loc_job->argc) {
                i++;
                file_out = open(loc_job->argv[i], O_WRONLY | O_CREAT | O_TRUNC, 
                                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                    
                loc_job->stdout = dup(STDOUT_FILENO);
                dup2(file_out, STDIN_FILENO);
            }
            else {
                fprintf(stderr,"Error: no out file specified");
            }
        }
        else if(strcmp(loc_job->argv[i], "<") == 0) {
            loc_job->argv[i] = NULL;
            if(i+1 < loc_job->argc) {
                i++;
                file_in = open(loc_job->argv[i], O_RDONLY);
                loc_job->stdin = dup(STDIN_FILENO);
                dup2(file_in, STDIN_FILENO);
                
            }
            else {
                fprintf(stderr,"Error: no in file specified");
            }
        }
        else {
            new_argc++;
        }
    }
    
    loc_job->argc = new_argc;
    loc_job->file_in = file_in;
    loc_job->file_out = file_out;

    /* Check if the command was just spaces */
    if( 0 >= loc_job->argc ) {
        return 0;
    }

    /* Grab the binary from the list of arguments */
    if( 0 < loc_job->argc ) {
        loc_job->binary = strdup(loc_job->argv[0]);
    }
    
    /******************************
     * Check for built-in commands:
     * - jobs
     * - exit
     * - history
     * - wait
     * - fg
     ******************************/
    if( 0 == strncmp("exit", loc_job->binary, strlen(loc_job->binary)) ) {
        if( 0 != (ret = builtin_exit() ) ) {
            fprintf(stderr, "Error: exit command failed!\n");
        }
    }
    else if( 0 == strncmp("jobs", loc_job->binary, strlen(loc_job->binary)) ) {
        if( 0 != (ret = builtin_jobs() ) ) {
            fprintf(stderr, "Error: jobs command failed!\n");
        }
    }
    else if( 0 == strncmp("history", loc_job->binary, strlen(loc_job->binary)) ) {
        if( 0 != (ret = builtin_history() ) ) {
            fprintf(stderr, "Error: history command failed!\n");
        }
    }
    else if( 0 == strncmp("wait", loc_job->binary, strlen(loc_job->binary)) ) {
        if( 0 != (ret = builtin_wait() ) ) {
            fprintf(stderr, "Error: wait command failed!\n");
        }
    }
    else if( 0 == strncmp("fg", loc_job->binary, strlen(loc_job->binary)) ) {
        if(loc_job->argc == 2) {
            if (0 != (ret = builtin_fg_with_id(strtol(loc_job->argv[1], NULL, 10)))) {
                fprintf(stderr, "Error: fg command failed!\n");
            }
        }
        else if( 0 != (ret = builtin_fg() ) ) {
            fprintf(stderr, "Error: fg command failed!\n");
        }
    }
    /*
     * Launch the job
     */
    else {
        if( 0 != (ret = launch_job(loc_job)) ) {
            fprintf(stderr, "Error: Unable to launch the job! \"%s\"\n", loc_job->binary);
        }
    }

    return 0;
}

int launch_job(job_t * loc_job)
{
    int c_id;

    /*
     * Launch the job in either the foreground or background
     */
     
    c_id = fork();
    
    if (c_id < 0) {
        fprintf(stderr, "Error: Fork failed\n");
        return -1;
    }
    else if (c_id == 0) {
        execvp(loc_job->binary, loc_job->argv);
        fprintf(stderr, "Error: Unkown command\n");
        exit(-1);
    } 
    else {
        if(loc_job->is_background) {
            
            waitpid(c_id, NULL, WNOHANG);
            
            loc_job->pid = c_id;
            loc_job->is_running = TRUE;
            loc_job->job_number = total_jobs+1;
            
            add_job(bg_jobs, loc_job->full_command, loc_job->pid, loc_job->argc,
                    loc_job->job_number, loc_job->argv ,num_bg_jobs, TRUE);
            num_bg_jobs++;
            
        }
        else {
            waitpid(c_id, NULL, 0);
            
            if(loc_job->file_in != -1) {
                dup2(loc_job->stdin, STDIN_FILENO);
                close(loc_job->file_in);
            }
            if(loc_job->file_out != -1) {
                dup2(loc_job->stdout, STDOUT_FILENO);
                close(loc_job->file_out);
            }
        }
    }
    
    /*
     * Some accounting
     */
    ++total_jobs;
    ++total_jobs_display_ctr;
    if( TRUE == loc_job->is_background ) {
        ++total_jobs_bg;
    }

    return 0;
}

int builtin_exit(void)
{
    int i;
    int j;
    int ret;
    int num_jobs_waiting = 0;
    struct job * courser = NULL;
    
    exiting = TRUE;

    for(i = 0; i < bg_jobs->size; i++) {
        courser = bg_jobs->jobs_arr[i];
        if(courser){
            ret = waitpid(courser->pid, NULL, WNOHANG);
            if(!ret) {
                num_jobs_waiting++;
            }
        }
    }
    if(num_jobs_waiting){
        printf("Waiting on %2d jobs to finish running in the background!\n", num_jobs_waiting);
    }
    
    for(i = 0; i < bg_jobs->size; i++) {
        courser = bg_jobs->jobs_arr[i];
        if(courser->is_running) {
            waitpid(courser->pid, NULL, 0);
            courser->is_running = FALSE;
        }
        for(j = 0; j < courser->argc; j++) {
            free(courser->argv[j]);
        }
        free(courser->argv);
        free(courser->full_command);
    }
    
    for(i = 0; i < jobs_history->size; i++) {
        courser = jobs_history->jobs_arr[i];
        for(j = 0; j < courser->argc; j++) {
            free(courser->argv[j]);
        }
        free(courser->argv);
        free(courser->full_command);
    }
    
    free(bg_jobs->jobs_arr);
    free(jobs_history->jobs_arr);
    free(bg_jobs);
    free(jobs_history);
    
    ++total_jobs_display_ctr;
    fflush(NULL);

    return 0;
}

int builtin_jobs(void)
{
    int i;
    int j;
    int ret;

    for(i = 0; i < bg_jobs->size; i++) {
        struct job * bg_job = bg_jobs->jobs_arr[i];
        ret = waitpid(bg_job->pid, NULL, WNOHANG);
        
        if(bg_job->is_running && ret == bg_job->pid) {
            bg_job->is_running = FALSE;
        }
        if(bg_job->display) {
            printf("[%d]   %-7s  ",
            bg_job->job_number, 
            (TRUE == bg_job->is_running ? "running" : "Done"));
            for(j = 0; j < bg_job->argc; j++) {
                printf(" %s", bg_job->argv[j]);
            }
            printf("\n");
        }
        if(bg_job->is_running == FALSE) {
            bg_job->display = FALSE;
        }
    }
    ++total_jobs_display_ctr;
    fflush(NULL);

    return 0;
}

int builtin_history(void)
{
    int i;

    for(i = 0; i < total_history; i++) {
        printf("  %2d   %s %c\n",
           i+1, jobs_history->jobs_arr[i]->full_command,
           (TRUE == jobs_history->jobs_arr[i]->is_background ? '&' : ' '));
    }

    ++total_jobs_display_ctr;
    fflush(NULL);

    return 0;
}

int builtin_wait(void)
{
    int i;
    struct job * courser;
    
    for(i = 0; i < bg_jobs->size; i++) {
        courser = bg_jobs->jobs_arr[i];
        if(courser->is_running) {
            waitpid(courser->pid, NULL, 0);
            courser->is_running = FALSE;
        }
    }
    ++total_jobs_display_ctr;
    fflush(NULL);

    return 0;
}

int builtin_fg(void)
{
    struct job * last_job;
    struct job * courser = NULL;
    int i;
    int ret;

    last_job = bg_jobs->jobs_arr[0];
    for(i = 1; i < bg_jobs->size; i++) {
        courser = bg_jobs->jobs_arr[i];
        if(courser) {
            ret = waitpid(courser->pid, NULL, WNOHANG);
            if(ret == 0 && courser->is_running) {
                if(courser->job_number > last_job->job_number) {
                    last_job = courser;
                }
            }
        }
    }
    if(last_job){
        waitpid(last_job->pid, NULL, 0);
        last_job->display = FALSE;
    }
    else {
        printf("Error: no running jobs in background\n");
    }
    
    ++total_jobs_display_ctr;
    fflush(NULL);

    return 0;
}

int builtin_fg_with_id(int id) {
    struct job * courser;
    int ret;
    int i;
    
    for(i = 0; i < bg_jobs->size; i++) {
        if(bg_jobs->jobs_arr[i]->job_number == id) {
            courser = bg_jobs->jobs_arr[i];
            ret = waitpid(courser->pid, NULL, WNOHANG);
        }
    }
    
    if(courser) {
        if(ret == 0 && courser->is_running) {
            waitpid(courser->pid, NULL, 0);
            courser->display = FALSE;
        }
        else {
            printf("Error: Job is not running\n");
        }
    }
    else {
        printf("Error: Job not found\n");
    }
    return 0;
}

int add_job(struct jobs_array * jobs_array, char * full_command, int pid, 
        int argc, int job_number, char ** argv, int size, int is_background) {
    
    int i;
    struct job * new_job = malloc(sizeof(struct job));
    
    jobs_array->jobs_arr = realloc(jobs_array->jobs_arr, sizeof(struct job *) * size+1 );
    
    new_job->full_command = strdup(full_command);
    new_job->pid = pid;
    new_job->is_running = TRUE;
    new_job->argc = argc;
    new_job->job_number = job_number;
    new_job->argv = malloc(sizeof(char*)*argc);
    new_job->is_background = is_background;
    new_job->display = TRUE;
    
    for (i = 0; i < argc; i++) {
        new_job->argv[i] = strdup(argv[i]);
    }
    
    jobs_array->jobs_arr[size] = new_job;
    jobs_array->size++;
    
    return 0;
}