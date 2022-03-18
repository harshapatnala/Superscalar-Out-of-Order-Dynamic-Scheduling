#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_proc.h"
#include "superscalar_proc.h"





int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    //int op_type, dest, src1, src2;  // Variables are read from trace file
    //unsigned long int pc; // Variable holds the pc read from input file

    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }

    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    /*
    printf("rob_size:%lu "
           "iq_size:%lu "
           "width:%lu "
           "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    */

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    superscalar_proc* Simulator;
    Simulator = new superscalar_proc(params.iq_size, params.rob_size, params.width, trace_file);
    Simulator->Simulate();

    printf("# === Simulator Command =========\n");
    printf("# %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
    Simulator->print_stats();

    return 0;
}