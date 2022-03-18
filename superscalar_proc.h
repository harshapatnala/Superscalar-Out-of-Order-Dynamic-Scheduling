#ifndef SUPERSCALAR_PROC_H
#define SUPERSCALAR_PROC_H
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;
enum {FE = 0, DE=1, RN=2, RR=3, DI=4, IS=5, EX=6, WB=7, RT=8}; /* Enum for pipeline stages */

typedef struct { /* Structure for recording cycle times */
    unsigned int start_cycle;
    unsigned int num_cycles;
} cycle_info;

typedef struct { /* Rename Map Table Entry */
    bool valid;
    int rob_tag;
} RMT_entry;

typedef struct { /* Issue Queue Entry */
    bool valid, rs1_ready, rs2_ready;
    int dst_tag, rs1_tag, rs2_tag;
    unsigned count;
    int sr1, sr2, dst, op_t;
    cycle_info stage[9];
} IQ_entry;

typedef struct { /* Reorder Buffer Entry */
    bool ready, valid;
    int sr1, sr2, dst, op_t;
    int rob_tag;
    cycle_info stage[9];
} ROB_entry;

typedef struct { /* Pipeline Register Entry */
    bool valid, rs1_ready, rs2_ready;
    int op_t, sr1, sr2, dst, rs1_tag, rs2_tag, dst_rob_tag;
    unsigned int latency, count;
    cycle_info stage[9];
} register_entry;

class pipeline_register { /* PIPELINE REGISTER BETWEEN STAGES */
    private:
        register_entry* entries;
        unsigned int size, num_stages, max_size;

    public:
        pipeline_register(unsigned int, unsigned int); /* Constructor */

        /* Methods for managing data flow in register */
        bool isEmpty();
        bool isNotEmpty();
        bool isValid(unsigned int);
        void add_entry(register_entry, unsigned int);
        void add_entry_unbounded(register_entry, unsigned int);
        void set_cycle_info(unsigned int, unsigned int, unsigned int);
        void update_cycle_info(unsigned int, unsigned int);
        void remove_entry(unsigned int);
        void copy_entry(register_entry, unsigned int);
        register_entry get_entry(unsigned int);
        void update_execution();
        bool isReady(unsigned int);
        void update_ready_after_execute(int);
        unsigned int get_size();
        unsigned int get_max_size();
};


class rename_table { /* RENAME MAP TABLE */
    private:
        RMT_entry* entries;
        unsigned int size;
    public:
        rename_table(unsigned int);

        void set_rob_tag(int, int);
        void invalidate_entry(int);
        int get_rob_tag(int);
        bool get_valid_bit(int);
};


class issue_queue { /* ISSUE QUEUE */
    private:
        unsigned int max_size, size, num_stages;
        IQ_entry * entries;

    public:
        issue_queue(unsigned int, unsigned int); /* Constructor */

        /* Methods for managing Issue Queue */
        bool isFull();
        bool isEmpty();
        bool isReady(unsigned int);
        unsigned int get_max_size();
        bool entries_available(unsigned int);
        void add_entry(register_entry);
        IQ_entry remove_entry(unsigned int);
        void sort_queue();
        void update_cycles();
        void IQ_wakeup(int);
};

class reorder_buffer { /* REORDER BUFFER */
    private:
        unsigned int max_size, head, tail, size, num_stages;
        ROB_entry * entries;

    public:
        reorder_buffer(unsigned int, unsigned int); /* Constructor */

        /* Methods for managing ROB */
        bool isEmpty();
        bool isFull();
        void add_entry(register_entry);
        ROB_entry remove_entry();
        int get_rob_tag_at_head();
        int get_rob_tag_at_tail();
        bool entries_available(unsigned int);
        void update_ready(register_entry);
        bool isReady();
        void update_cycles();
        bool check_ready_at_rob(int);
};

class superscalar_proc { /* OOO PIPELINED SIMULATOR */
    private:
        /* Structures in OOO Scheduling */
        rename_table* RMT;
        issue_queue* IQ;
        reorder_buffer* ROB;

        /* Pipeline Registers */
        pipeline_register* Decode_Register;
        pipeline_register* Rename_Register;
        pipeline_register* RR_Register;
        pipeline_register* DI_Register;
        pipeline_register* Execute_Register;
        pipeline_register* Writeback_Register;

        /* Simulator Parameters */
        unsigned int iq_size, rob_size, width;
        char* trace_file;
        ifstream file;

        /* Global variables used during simulation */
        unsigned int current_cycle, pc, instruction_count, num_stages, retire_count;
        int source_1, source_2, dest, operation_type;
        double ipc;
        bool end_of_trace;

    public:
        superscalar_proc(unsigned int, unsigned int, unsigned int , char*); /* Constructor */

        /* Simulator Methods */
        void Simulate();
        void Fetch();
        void Decode();
        void Rename();
        void RegRead();
        void Dispatch();
        void Issue();
        void Execute();
        void Writeback();
        void Retire();
        bool Advance_Cycle();
        void print_stats();
};



#endif
