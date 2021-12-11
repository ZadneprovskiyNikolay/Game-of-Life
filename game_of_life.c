#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef unsigned int uint16;
typedef unsigned long uint32;
typedef unsigned char byte;

struct CmdArgs { 
    char* input_file_name;
    char* output_directory;
    int max_iters, interval;
};


inline void strcpy_alloc(char** strp, char* src) {
    int string_len  = strlen(src);
    *strp = (char*) malloc((string_len + 1) * sizeof(char));
    strcpy(*strp, src);
}

// Returns 1 for valid arguments, 0 otherwise.
int parse_cmdargs(int argc, char* argv[], struct CmdArgs* cmdargs) {     
    if (argc < 3) {
        printf("You didn't provide enough arguments.\n");
        return 0;
    }
    
    for (int i = 0; i < argc; i++) {
        // Input file name.
        if (*(argv[i] + 2) == 'i')
            strcpy_alloc(&(cmdargs->input_file_name), argv[i] + 8);

        // Output directory.
        else if (*(argv[i] + 2) == 'o')
            strcpy_alloc(&(cmdargs->output_directory), argv[i] + 9);

        // Maximum ages.
        else if (*(argv[i] + 2) == 'm') {
            cmdargs->max_iters = strtol(argv[i] + 11, NULL, 10);
            if (cmdargs->max_iters == 0) {
                exit(EXIT_SUCCESS);
            }
        }

        // Picture updating frequency in seconds.
        else if (*(argv[i] + 2) == 'd') {  
            cmdargs->interval = strtol(argv[i] + 12, NULL, 10);
        } 
    }

    if (cmdargs->input_file_name == NULL || cmdargs->output_directory == NULL)
        return 0;

    return 1;
}

int main(int argc, char* argv[]) {    
    // Parse cmd arguments.
    struct CmdArgs cmdargs = { NULL, NULL, 0, .interval = 1 };
    if (!parse_cmdargs(argc, argv, &cmdargs)) {
        exit(EXIT_FAILURE);
    }   

    return 0;
}