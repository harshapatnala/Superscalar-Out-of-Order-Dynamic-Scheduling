#include "superscalar_proc.h"
#include <bits/stdc++.h>

/* ----------------------------------- Pipeline Register Methods ------------------------------ */
pipeline_register::pipeline_register(unsigned int WIDTH, unsigned int STAGES) {
    size = 0;
    max_size = WIDTH;
    num_stages = STAGES;
    entries = new register_entry [max_size];
    for(unsigned int i=0; i < max_size; i++) {
        for(unsigned int j=0; j < num_stages; j++) {
            entries[i].stage[j].start_cycle = 0;
            entries[i].stage[j].num_cycles = 0;
        }
        entries[i].valid = entries[i].rs1_ready = entries[i].rs2_ready = false;
    }
}

bool pipeline_register::isValid(unsigned int i) { return entries[i].valid; }
bool pipeline_register::isEmpty() { return (size == 0); }
bool pipeline_register::isNotEmpty() { return (size != 0); }
bool pipeline_register::isReady(unsigned int i) { return (entries[i].valid && (entries[i].latency == 0)); }
unsigned int pipeline_register::get_size() { return size; }
unsigned int pipeline_register::get_max_size() { return max_size; }

void pipeline_register::add_entry(register_entry entry, unsigned int i) {
    entries[i].valid = true;
    entries[i].op_t = entry.op_t;
    entries[i].sr1 = entry.sr1;
    entries[i].sr2 = entry.sr2;
    entries[i].dst = entry.dst;
    entries[i].dst_rob_tag = entry.dst_rob_tag;
    entries[i].rs1_tag = entry.rs1_tag;
    entries[i].rs2_tag = entry.rs2_tag;
    entries[i].rs1_ready = entry.rs1_ready;
    entries[i].rs2_ready = entry.rs2_ready;
    entries[i].count = entry.count;
    for(unsigned int j=0; j < num_stages; j++) {
        entries[i].stage[j] = entry.stage[j];
    }
    if(entries[i].op_t == 0) entries[i].latency = 1;
    if(entries[i].op_t == 1) entries[i].latency = 2;
    if(entries[i].op_t == 2) entries[i].latency = 5;
    size++;
}

void pipeline_register::add_entry_unbounded(register_entry entry, unsigned int stage) {
    for(unsigned int i=0; i < max_size; i++) {
        if(!entries[i].valid) {
            entries[i].valid = true;
            entries[i].op_t = entry.op_t;
            entries[i].sr1 = entry.sr1;
            entries[i].sr2 = entry.sr2;
            entries[i].dst = entry.dst;
            entries[i].dst_rob_tag = entry.dst_rob_tag;
            entries[i].rs1_tag = entry.rs1_tag;
            entries[i].rs2_tag = entry.rs2_tag;
            entries[i].rs1_ready = entry.rs1_ready;
            entries[i].rs2_ready = entry.rs2_ready;
            entries[i].count = entry.count;
            for(unsigned int j=0; j < num_stages; j++) {
                entries[i].stage[j] = entry.stage[j];
            }
            if(entries[i].op_t == 0) entries[i].latency = 1;
            if(entries[i].op_t == 1) entries[i].latency = 2;
            if(entries[i].op_t == 2) entries[i].latency = 5;
            if(stage == EX) {
                entries[i].stage[EX].start_cycle = entries[i].stage[IS].start_cycle + entries[i].stage[IS].num_cycles;
                entries[i].stage[EX].num_cycles = entries[i].latency;
            }
            else if(stage == WB) {
                entries[i].stage[WB].start_cycle = entries[i].stage[EX].start_cycle + entries[i].stage[EX].num_cycles;
                entries[i].stage[WB].num_cycles = 1;
            }
            size++;
            return;
        }
    }
}

void pipeline_register::copy_entry(register_entry entry, unsigned int i) {
    entries[i].valid = true;
    entries[i].op_t = entry.op_t;
    entries[i].sr1 = entry.sr1;
    entries[i].sr2 = entry.sr2;
    entries[i].dst = entry.dst;
    entries[i].dst_rob_tag = entry.dst_rob_tag;
    entries[i].rs1_tag = entry.rs1_tag;
    entries[i].rs2_tag = entry.rs2_tag;
    entries[i].rs1_ready = entry.rs1_ready;
    entries[i].rs2_ready = entry.rs2_ready;
    entries[i].count = entry.count;
    entries[i].latency = entry.latency;
    for(unsigned int j=0; j < num_stages; j++) {
        entries[i].stage[j] = entry.stage[j];
    }
    size++;
}

register_entry pipeline_register::get_entry(unsigned int i) {
    return entries[i];
}

void pipeline_register::remove_entry(unsigned int i) {
    entries[i].valid = false;
    size--;
}

void pipeline_register::set_cycle_info(unsigned int i , unsigned int current_stage, unsigned int cycle) {
    if(current_stage == FE) {
        entries[i].stage[FE].start_cycle = cycle;
        entries[i].stage[FE].num_cycles = 1;
    }
    else {
        entries[i].stage[current_stage].start_cycle = entries[i].stage[current_stage -1].start_cycle + entries[i].stage[current_stage -1].num_cycles;
        entries[i].stage[current_stage].num_cycles = 1;
    }
}

void pipeline_register::update_cycle_info(unsigned int i, unsigned int current_stage) {
    entries[i].stage[current_stage].num_cycles++;
}

void pipeline_register::update_execution() {
    for(unsigned int i=0; i < max_size; i++) {
        if(entries[i].valid) {
            entries[i].latency--;
        }
    }
}

void pipeline_register::update_ready_after_execute(int dst_tag) {
    for(unsigned int i=0; i < max_size; i++) {
        if (entries[i].valid) {
            if (entries[i].rs1_tag == dst_tag) entries[i].rs1_ready = true;
            if (entries[i].rs2_tag == dst_tag) entries[i].rs2_ready = true;
        }
    }
}

/* ----------------------------------- Rename Map Table Methods ------------------------------ */
rename_table::rename_table(unsigned int size) {
    entries = new RMT_entry [size];
    this->size = size;
    for(unsigned int i=0; i < size; i++) {
        entries[i].valid = false;
        entries[i].rob_tag = (int)i;
    }
}

int rename_table::get_rob_tag(int tag) { return entries[tag].rob_tag; }
bool rename_table::get_valid_bit(int tag) { return entries[tag].valid; }

void rename_table::set_rob_tag(int reg_value, int tag) {
    entries[reg_value].valid = true;
    entries[reg_value].rob_tag = tag;
}

void rename_table::invalidate_entry(int tag) {
    for(unsigned int i=0; i < size; i++) {
        if(entries[i].valid && entries[i].rob_tag == tag) {
            entries[i].valid = false;
            entries[i].rob_tag = (int)i;
            return;
        }
    }
}

/* ---------------------------------- Reorder Buffer Methods ------------------------------ */
reorder_buffer::reorder_buffer(unsigned int max_size, unsigned int stages) {
    this->max_size = max_size;
    head = tail = size = 0;
    entries = new ROB_entry [max_size];
    num_stages = stages;

    for(unsigned int i=0; i < max_size; i++) {
        entries[i].rob_tag = (int)i;
        entries[i].valid = false;
        entries[i].ready = false;
        entries[i].dst = -1;
        for (unsigned int j = 0; j < num_stages; j++) {
            entries[i].stage[j].start_cycle = 0;
            entries[i].stage[j].num_cycles = 0;
        }
    }
}

bool reorder_buffer::isEmpty() { return size == 0; }
bool reorder_buffer::isFull() { return size == max_size; }
bool reorder_buffer::entries_available(unsigned int num) { return ((max_size - size) >=num); }
bool reorder_buffer::isReady() { return (entries[head].valid && entries[head].ready); }
bool reorder_buffer::check_ready_at_rob(int tag) { return entries[tag].ready; }
int reorder_buffer::get_rob_tag_at_head() { return entries[head].rob_tag; }
int reorder_buffer::get_rob_tag_at_tail() { return entries[tail].rob_tag; }

void reorder_buffer::add_entry(register_entry entry) {
    if(isFull()) return;
    entries[tail].dst = entry.dst;
    entries[tail].sr1 = entry.sr1;
    entries[tail].sr2 = entry.sr2;
    entries[tail].op_t = entry.op_t;
    entries[tail].valid = true;
    entries[tail].ready = false;
    tail = (tail+1) % max_size;
    size++;
}

ROB_entry reorder_buffer::remove_entry() {
    ROB_entry entry = entries[head];
    entries[head].valid = false;

    if(size > 1) {
        head = (head + 1) % max_size;
    }
    else head = tail;
    size--;
    return entry;
}

void reorder_buffer::update_ready(register_entry entry) {
    for(unsigned int i=0; i < max_size; i++) {
        if(entries[i].valid && (entries[i].rob_tag == entry.dst_rob_tag)) {
            entries[i].ready = true;
            for(unsigned int j=0; j < num_stages; j++) {
                entries[i].stage[j] = entry.stage[j];
            }
            entries[i].stage[RT].start_cycle = entry.stage[WB].start_cycle + entry.stage[WB].num_cycles;
            entries[i].stage[RT].num_cycles = 1;
            return;
        }
    }
}

void reorder_buffer::update_cycles() {
    for(unsigned int i=0; i < max_size; i++) {
        if(entries[i].valid) {
            entries[i].stage[RT].num_cycles++;
        }
    }
}

/* ---------------------------------- Issue Queue Methods ------------------------------ */
issue_queue::issue_queue(unsigned int max_size, unsigned int stages) {
    this->max_size = max_size;
    size = 0;
    entries = new IQ_entry [max_size];
    num_stages = stages;
    for(unsigned int i=0; i < max_size; i++) {
        entries[i].valid = entries[i].rs1_ready = entries[i].rs2_ready = false;
        entries[i].dst_tag = entries[i].rs1_tag = entries[i].rs2_tag = -1;
        entries[i].count = INT_MAX;
        for(unsigned int j=0; j < num_stages; j++) {
            entries[i].stage[j].start_cycle = 0;
            entries[i].stage[j].num_cycles = 0;
        }
    }
}

bool issue_queue::isFull() { return size == max_size; }
bool issue_queue::isEmpty() { return size == 0; }
bool issue_queue::entries_available(unsigned int num) { return ((max_size - size) >= num); }
bool issue_queue::isReady(unsigned int num) { return (entries[num].valid && entries[num].rs1_ready && entries[num].rs2_ready); }
unsigned int issue_queue::get_max_size() { return max_size; }

void issue_queue::add_entry(register_entry entry) {
    if(isFull()) return;
    for(unsigned int i=0; i < max_size; i++) {
        if(!entries[i].valid) {
            entries[i].op_t = entry.op_t;
            entries[i].sr1 = entry.sr1;
            entries[i].sr2 = entry.sr2;
            entries[i].dst = entry.dst;
            entries[i].rs1_ready = entry.rs1_ready;
            entries[i].rs2_ready = entry.rs2_ready;
            entries[i].rs1_tag = entry.rs1_tag;
            entries[i].rs2_tag = entry.rs2_tag;
            entries[i].dst_tag = entry.dst_rob_tag;
            entries[i].count = entry.count;
            for(unsigned int j=0; j < 9; j++) {
                entries[i].stage[j] = entry.stage[j];
            }
            entries[i].stage[IS].start_cycle = entry.stage[DI].start_cycle + entry.stage[DI].num_cycles;
            entries[i].stage[IS].num_cycles = 1;
            entries[i].valid = true;
            size++;
            return;
        }
    }
}

IQ_entry issue_queue::remove_entry(unsigned int num) {
    IQ_entry entry;
    entry = entries[num];
    entries[num].valid = false;
    size--;
    return entry;
}

void issue_queue::sort_queue() {
    for(unsigned int i=0; i < max_size-1; i++) {
        for(unsigned int j=0; j < max_size -1; j++) {
            if(entries[j].count > entries[j+1].count) {
                IQ_entry temp;
                temp = entries[j];
                entries[j] = entries[j+1];
                entries[j+1] = temp;
            }
        }
    }
}

void issue_queue::IQ_wakeup(int dst) {
    for(unsigned int i=0; i < max_size; i++) {
        if(entries[i].valid) {
            if(entries[i].rs1_tag == dst && !entries[i].rs1_ready) {
                entries[i].rs1_ready = true;
            }
            if(entries[i].rs2_tag == dst && !entries[i].rs2_ready) {
                entries[i].rs2_ready = true;
            }
        }
    }
}

void issue_queue::update_cycles() {
    for(unsigned int i=0; i< max_size; i++) {
        if(entries[i].valid) entries[i].stage[IS].num_cycles++;
    }
}

/* ---------------------------------- OOO Superscalar Simulator Methods ------------------------------ */
superscalar_proc::superscalar_proc(unsigned int IQ_size, unsigned int ROB_size, unsigned int WIDTH, char * t_file) {
    iq_size = IQ_size;
    rob_size = ROB_size;
    width = WIDTH;
    trace_file = t_file;
    this->file.open(trace_file);
    current_cycle = instruction_count = 0;
    end_of_trace = false;
    num_stages = 9;
    retire_count = 0;
    ipc = 0;

    RMT = new rename_table(67);
    IQ = new issue_queue(iq_size, num_stages);
    ROB = new reorder_buffer(rob_size, num_stages);

    Decode_Register = new pipeline_register(width, num_stages);
    Rename_Register = new pipeline_register(width, num_stages);
    RR_Register = new pipeline_register(width, num_stages);
    DI_Register = new pipeline_register(width, num_stages);
    Execute_Register = new pipeline_register(width*5, num_stages);
    Writeback_Register = new pipeline_register(width*5, num_stages);
}

void superscalar_proc::Fetch() {
    if(Decode_Register->isEmpty()) { /* Check if decode register is empty, if yes then fetch upto WIDTH instructions */
        for(unsigned int i=0; i < width; i++) {
            if (file >> hex >> pc >> dec >> operation_type >> dest >> source_1 >> source_2) {
                register_entry entry;
                entry.sr1 = source_1;
                entry.sr2 = source_2;
                entry.dst = dest;
                entry.op_t = operation_type;
                entry.count = instruction_count;
                Decode_Register->add_entry(entry, i);
                Decode_Register->set_cycle_info(i, FE, current_cycle);
                Decode_Register->set_cycle_info(i, DE, current_cycle);
                instruction_count++;
            }
            else {
                end_of_trace = true;
            }
        }
    }
    else return;
}

void superscalar_proc::Decode() {
    if(Decode_Register->isNotEmpty()) { /* First check if Decode register has entries, if yes then check Rename register is empty */
        if(Rename_Register->isEmpty()) {
            for(unsigned int i=0; i < width; i++) {
                if(Decode_Register->isValid(i)) {
                    register_entry entry = Decode_Register->get_entry(i);
                    Rename_Register->copy_entry(entry, i);
                    Rename_Register->set_cycle_info(i, RN, current_cycle);
                    Decode_Register->remove_entry(i);
                }
            }
            return;
        }
        else { /* If Rename Register is not empty, stall at Decode, update stalled cycles at this stage*/
            for(unsigned int i=0; i< width; i++) {
                if(Decode_Register->isValid(i)) {
                    Decode_Register->update_cycle_info(i, DE);
                }
            }
            return;
        }
    }
    else return;
}

void superscalar_proc::Rename() {
    if(Rename_Register->isNotEmpty()) { /* First check if Rename register has entries, if yes then check if ROB has enough entries */
        if(RR_Register->isNotEmpty() || !(ROB->entries_available(Rename_Register->get_size()))) {
            for(unsigned int i=0; i < width; i++) {
                if(Rename_Register->isValid(i)) {
                    Rename_Register->update_cycle_info(i, RN); /* ROB doesn't have enough entries, stall at Rename, update stalled cycles */
                }
            }
        }
        else {
            for(unsigned int i=0; i < width; i++) {
                if (Rename_Register->isValid(i)) {
                    /* Allocate entry in ROB for destination. Rename source registers according to RMT. Update RMT entry for allocated ROB entry */
                    int dst_rob_tag = ROB->get_rob_tag_at_tail();
                    register_entry entry = Rename_Register->get_entry(i);
                    ROB->add_entry(entry);
                    if(entry.sr1 != -1) {
                        entry.rs1_tag = RMT->get_rob_tag(entry.sr1);
                        if(RMT->get_valid_bit(entry.sr1)) entry.rs1_ready = false;
                        else entry.rs1_ready = true;
                    }
                    else {
                        entry.rs1_ready = true;
                        entry.rs1_tag = -1;
                    }
                    if(entry.sr2 != -1) {
                        entry.rs2_tag = RMT->get_rob_tag(entry.sr2);
                        if(RMT->get_valid_bit(entry.sr2)) entry.rs2_ready = false;
                        else entry.rs2_ready = true;
                    }
                    else {
                        entry.rs2_ready = true;
                        entry.rs2_tag = -1;
                    }
                    if(entry.dst != -1) {
                        RMT->set_rob_tag(entry.dst, dst_rob_tag);
                    }
                    entry.dst_rob_tag = dst_rob_tag;
                    RR_Register->copy_entry(entry, i);
                    RR_Register->set_cycle_info(i, RR, current_cycle);
                    Rename_Register->remove_entry(i);
                }
            }
        }
    }
    else return;
}

void superscalar_proc::RegRead() {
    if(RR_Register->isNotEmpty()) { /* Check if RegRead register is not empty, if yes, then re-check for readiness according to ROB entries. */
        if(DI_Register->isEmpty()) {
            for(unsigned int i=0; i < width; i++) {
                if(RR_Register->isValid(i)) {
                    register_entry entry = RR_Register->get_entry(i);
                    if(!entry.rs1_ready) {
                        entry.rs1_ready = ROB->check_ready_at_rob(entry.rs1_tag);
                    }
                    if(!entry.rs2_ready) {
                        entry.rs2_ready = ROB->check_ready_at_rob(entry.rs2_tag);
                    }
                    DI_Register->copy_entry(entry, i);
                    DI_Register->set_cycle_info(i, DI, current_cycle);
                    RR_Register->remove_entry(i);
                }
            }
        }
        else {
            for(unsigned int i=0; i < width; i++) {
                if(RR_Register->isValid(i)) {
                    RR_Register->update_cycle_info(i, RR);
                }
            }
        }
    }
    else return;
}

void superscalar_proc::Dispatch() {

    if(DI_Register->isNotEmpty()) { /* Check if Dispatch Register is not empty, if yes then allocate entry in Issue Queue */
        if(IQ->entries_available(DI_Register->get_size())) {
            for(unsigned int i=0; i < width; i++) {
                if(DI_Register->isValid(i)) {
                    register_entry entry = DI_Register->get_entry(i);
                    IQ->add_entry(entry);
                    DI_Register->remove_entry(i);
                }
            }
        }
        else { /* Issue queue doesn't have enough entries, stall at Dispatch. Update stalled cycles at this stage */
            for(unsigned int i=0; i < width; i++) {
                if(DI_Register->isValid(i)) {
                    DI_Register->update_cycle_info(i, DI);
                }
            }
        }
    }
    else return;
}

void superscalar_proc::Issue() {
    if(!IQ->isEmpty()) { // Only check for readyness only if IQ isn't empty.
        IQ->sort_queue(); // Sort IQ to issue oldest ready instructions first.
        for (unsigned int i = 0; i < width; i++) {
            for (unsigned int j = 0; j < IQ->get_max_size(); j++) {
                if (IQ->isReady(j)) {
                    IQ_entry entry = IQ->remove_entry(j);
                    register_entry reg_entry;
                    reg_entry.op_t = entry.op_t;
                    reg_entry.sr1 = entry.sr1;
                    reg_entry.sr2 = entry.sr2;
                    reg_entry.dst = entry.dst;
                    reg_entry.dst_rob_tag = entry.dst_tag;
                    reg_entry.rs1_tag = entry.rs1_tag;
                    reg_entry.rs2_tag = entry.rs2_tag;
                    reg_entry.rs1_ready = entry.rs1_ready;
                    reg_entry.rs2_ready = entry.rs2_ready;

                    for(unsigned int k=0; k < num_stages; k++) {
                        reg_entry.stage[k].start_cycle = entry.stage[k].start_cycle;
                        reg_entry.stage[k].num_cycles = entry.stage[k].num_cycles;
                    }
                    Execute_Register->add_entry_unbounded(reg_entry, EX);
                    break;
                }
            }
        }
        IQ->update_cycles();
    }
    else return;
}

void superscalar_proc::Execute() {
    Execute_Register->update_execution(); /* Updating Execution latency for entries in execute register */
    for(unsigned int i=0; i <Execute_Register->get_max_size(); i++) {
        if(Execute_Register->isReady(i)) {
            register_entry entry = Execute_Register->get_entry(i);
            /* Wake-up dependent instructions in Issue Queue, Dispatch register and RegRead register */
            IQ->IQ_wakeup(entry.dst_rob_tag);
            DI_Register->update_ready_after_execute(entry.dst_rob_tag);
            RR_Register->update_ready_after_execute(entry.dst_rob_tag);
            Writeback_Register->add_entry_unbounded(entry, WB);
            Execute_Register->remove_entry(i);
        }
    }
}

void superscalar_proc::Writeback() { /* Update ready bit at ROB entries */
    for(unsigned int i=0; i < Writeback_Register->get_max_size(); i++) {
        if(Writeback_Register->isValid(i)) {
            register_entry entry = Writeback_Register->get_entry(i);
            ROB->update_ready(entry);
            Writeback_Register->remove_entry(i);
        }
    }
}

void superscalar_proc::Retire() { /* Retire upto WIDTH instructions from the head. */
    for(unsigned int i=0; i < width; i++) {
        if(ROB->isReady()) {
            int rmt_rob_tag = ROB->get_rob_tag_at_head();
            ROB_entry entry = ROB->remove_entry();
            RMT->invalidate_entry(rmt_rob_tag);

            cout << retire_count << " fu{" << entry.op_t << "} src{" << entry.sr1 <<"," << entry.sr2 << "} dst{" << entry.dst <<
                 "} FE{" << entry.stage[FE].start_cycle <<"," << entry.stage[FE].num_cycles << "} DE{" << entry.stage[DE].start_cycle <<","<<entry.stage[DE].num_cycles
                 <<"} RN{"<<entry.stage[RN].start_cycle<<","<<entry.stage[RN].num_cycles<<"} RR{"<<entry.stage[RR].start_cycle<<","<<entry.stage[RR].num_cycles <<
                 "} DI{"<<entry.stage[DI].start_cycle<<","<<entry.stage[DI].num_cycles<<"} IS{"<<entry.stage[IS].start_cycle<<","<<entry.stage[IS].num_cycles <<
                 "} EX{"<<entry.stage[EX].start_cycle<<","<<entry.stage[EX].num_cycles<<"} WB{"<<entry.stage[WB].start_cycle<<","<<entry.stage[WB].num_cycles <<
                 "} RT{"<<entry.stage[RT].start_cycle<<","<<entry.stage[RT].num_cycles<<"}"<<endl;

            retire_count++;
        }
        else {
            ROB->update_cycles();
            return;
        }
    }
    ROB->update_cycles();
}

bool superscalar_proc::Advance_Cycle() {
    if(end_of_trace && ROB->isEmpty()) {
        current_cycle++;
        return false;
    }
    else {
        current_cycle++;
        return true;
    }
}

void superscalar_proc::Simulate() {
    do {
        Retire();
        Writeback();
        Execute();
        Issue();
        Dispatch();
        RegRead();
        Rename();
        Decode();
        Fetch();
    } while (Advance_Cycle());
}

void superscalar_proc::print_stats() {
    printf("# === Processor Configuration ===\n");
    printf("# ROB_SIZE = %d\n", rob_size);
    printf("# IQ_SIZE  = %d\n", iq_size);
    printf("# WIDTH    = %d\n", width);

    ipc = (double) instruction_count/current_cycle;

    printf("# === Simulation Results ========\n");
    printf("# Dynamic Instruction Count    = %d\n", instruction_count);
    printf("# Cycles                       = %d\n", current_cycle);
    printf("# Instructions Per Cycle (IPC) = %.2f\n", ipc);
}