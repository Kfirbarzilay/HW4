/* 046267 Computer Architecture - Spring 2019 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

//#define DEBUG1

#ifdef DEBUG1
#define DEBUG_PRINT1(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT1(...) do{ } while ( false )
#endif

#define DEBUG2

#ifdef DEBUG2
#define DEBUG_PRINT2(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT2(...) do{ } while ( false )
#endif

#ifdef DEBUG2

#define FOREACH_OPCODE(OPCODE) \
        OPCODE(NOP)   \
        OPCODE(ADD)  \
        OPCODE(SUB)  \
        OPCODE(ADDI)   \
        OPCODE(SUBI)  \
        OPCODE(LOAD)  \
        OPCODE(STORE)  \
        OPCODE(HALT)  \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum FRUIT_ENUM {
    FOREACH_OPCODE(GENERATE_ENUM)
};

static const char *OP_STRING[] = {
        FOREACH_OPCODE(GENERATE_STRING)
};

#endif
void op_add_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread);
void op_sub_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread);
void op_addi_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread);
void op_subi_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread);
void op_add_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread);
void op_sub_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread);
void op_addi_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread);
void op_subi_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread);


int ThreadsNum;
int * my_instructions_blocked;
int * my_instructions_fg;
tcontext * block_regs; // where the instructions are kept
tcontext * finegrained_regs; // where the instructions are kept
int base_address = 12960;
int debug;
// Init the global cycles counter
int globalCyclesCounter;
int globalInstructionsCounter;

bool isLastThread_blocked(int threadNum)
{
    int numOfThreadsRunning = 0;
    for (int i=0; i<ThreadsNum; i++)
    {
        if(threadNum == i) continue;
        if(my_instructions_blocked[i]!= -1)
        {
            numOfThreadsRunning++;
        }
    }
    return numOfThreadsRunning == 0;
}

bool isLastThread_fg(int threadNum)
{
    int numOfThreadsRunning = 0;
    for (int i=0; i<ThreadsNum; i++)
    {
        if (threadNum == i) continue;
        if(my_instructions_fg[i]!= -1)
        {
            numOfThreadsRunning++;
        }
    }
    return numOfThreadsRunning == 0;
}

bool noThreadsInCore() {
    bool finished = true;
    for (int i = 0; i < ThreadsNum; i++) {
        if (my_instructions_fg[i] != -1) {
            finished = false;
        }
    }
    return finished;
}



Status Core_blocked_Multithreading(){
    globalCyclesCounter = 0;
    globalInstructionsCounter = 0;
	Instuction* CurIns_blocked = (Instuction*)malloc(sizeof(Instuction));
	int cur_thread = 0;
	int cur_ins_per_thread = 0;
	int32_t *dest_blocked = malloc(sizeof(*dest_blocked));
	uint32_t TargetAddress;
	// bool if the processing is over
	bool finished = false;
	// update the Threads Number
	ThreadsNum = Get_thread_number();
	// initiate the contexts
	block_regs = malloc(sizeof(*block_regs)*ThreadsNum);
	for(int i=0; i<ThreadsNum; i++){
		for (int j=0; j<8; j++)
		{
			block_regs[i].reg[j] = 0;
		}
	}
	// initiate the global instructions array
	my_instructions_blocked = malloc(sizeof(*my_instructions_blocked)*ThreadsNum);
	int* threadsWaitingLatency = malloc(sizeof(int) * ThreadsNum);
	bool* threadsInIO = malloc(sizeof(bool) * ThreadsNum);
	for (int i=0; i<ThreadsNum; i++)
	{
		my_instructions_blocked[i] = 0;
		threadsWaitingLatency[i] = 0;
        threadsInIO[i] = false;
	}

    // Get latency penalties for load and store
    int loadTime, storeTime;
    int latencies[2];
    Mem_latency(latencies);
    loadTime = latencies[0];
    storeTime = latencies[1];

    // Get penalty for context switch
    int contextPenalty = Get_switch_cycles();


	// the process itself
	while (1)
	{
		if(my_instructions_blocked[cur_thread] != -1) // Thread didn't finish commands
		{
            // TODO Every operation is at least 1 cycle
            globalCyclesCounter++;
            DEBUG_PRINT1("globalCyclesCounter = %d\n",globalCyclesCounter);
            DEBUG_PRINT1("thread %d is %s\n", cur_thread,threadsInIO[cur_thread] ? "IO" : "not waiting" );
            DEBUG_PRINT1("threadsWaitingLatency[%d] = %d\n",cur_thread, threadsWaitingLatency[cur_thread]);

			// for(int i=0; i<3; i++)
            // {
            // DEBUG_PRINT1("%d %d\n", i, my_instructions_blocked[i]);
            // }
            cur_ins_per_thread = my_instructions_blocked[cur_thread];
            DEBUG_PRINT1("my_instructions_blocked[%d] = %d\n", cur_thread,cur_ins_per_thread);
            SIM_MemInstRead(cur_ins_per_thread, CurIns_blocked, cur_thread);
            DEBUG_PRINT1("CurIns_blocked->opcode = %d\n\n", CurIns_blocked->opcode);
            // update the instructions array
            // DEBUG_PRINT1("before everything: \n");
            // DEBUG_PRINT1("CurIns_blocked->opcode: %d\n", CurIns_blocked->opcode);
            // DEBUG_PRINT1("cur_ins_per_thread: %d\n", cur_ins_per_thread);
            // DEBUG_PRINT1("cur_thread: %d\n", cur_thread);
            if (CurIns_blocked!=NULL && CurIns_blocked->opcode == CMD_HALT && !threadsInIO[cur_thread]) // if it is HALT OPERATION
            {
                my_instructions_blocked[cur_thread] = -1;
            }
            else if( threadsInIO[cur_thread] )
            {
                if  (threadsWaitingLatency[cur_thread] - globalCyclesCounter >= 0)
                {
                    DEBUG_PRINT1("Thread %d is still waiting\n\n",cur_thread);
                    if (!isLastThread_blocked(cur_thread)) {
                        cur_thread = (cur_thread + 1) % ThreadsNum;
                        globalCyclesCounter += contextPenalty;
                    }
                    continue;
                }
                else // finished IO process
                {
                    DEBUG_PRINT1("Thread %d finished IO process\n\n",cur_thread);
                    threadsInIO[cur_thread] = false;
                    threadsWaitingLatency[cur_thread] = 0;
                    globalInstructionsCounter++;
                    DEBUG_PRINT1("finished IO command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                    continue;
                }
            } else
            {
                my_instructions_blocked[cur_thread] = cur_ins_per_thread+1;
            }
            // Thread is in IO
            //make the operation
			// DEBUG_PRINT1("before operation:");
			// scanf("%d", &debug);

			if (CurIns_blocked!=NULL)
			{
				switch (CurIns_blocked->opcode) {
					case CMD_NOP:
						break;
					case CMD_ADD:
						// DEBUG_PRINT1("in add\n");
						// DEBUG_PRINT1("CurIns_blocked->dst_index: %d\n", CurIns_blocked->dst_index);
						// DEBUG_PRINT1("CurIns_blocked->src1_index: %d\n", CurIns_blocked->src1_index);
						// DEBUG_PRINT1("CurIns_blocked->src2_index_imm: %d\n", CurIns_blocked->src2_index_imm);
						op_add_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        DEBUG_PRINT1("finished ADD command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_SUB:
						// DEBUG_PRINT1("in sub\n");
						op_sub_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
                        globalInstructionsCounter++;
                        DEBUG_PRINT1("finished SUB command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_ADDI:
						// DEBUG_PRINT1("in addi\n");
						op_addi_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
                        globalInstructionsCounter++;
                        DEBUG_PRINT1("finished ADDI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_SUBI:
						// DEBUG_PRINT1("in subi\n");
						op_subi_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
                        globalInstructionsCounter++;
                        DEBUG_PRINT1("finished SUBI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_LOAD:
					// DEBUG_PRINT1("in load\n");
					// TODO note that it may not be an immediate
					    if (CurIns_blocked->isSrc2Imm)
						    TargetAddress = block_regs[cur_thread].reg[CurIns_blocked->src1_index] + CurIns_blocked->src2_index_imm;
                        else
                            TargetAddress = block_regs[cur_thread].reg[CurIns_blocked->src1_index] + block_regs[cur_thread].reg[CurIns_blocked->src2_index_imm];
						SIM_MemDataRead(TargetAddress, dest_blocked);
						block_regs[cur_thread].reg[CurIns_blocked->dst_index] = (int)(*dest_blocked);
						// TODO make a context switch to next thread.
						// TODO state that the thread is in IO operation and store the clock cycle where memory data is ready
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + loadTime;
                        threadsInIO[cur_thread] = true;
                        if (!isLastThread_blocked(cur_thread))
                            globalCyclesCounter += contextPenalty;
                        // TODO: basically should be in the previous condition
                        cur_thread = (cur_thread+1)%ThreadsNum;
                        break;
					case CMD_STORE:
						// DEBUG_PRINT1("in store\n");
                        // TODO note that it may not be an immediate
                        if (CurIns_blocked->isSrc2Imm)
						    SIM_MemDataWrite(block_regs[cur_thread].reg[CurIns_blocked->dst_index] + CurIns_blocked->src2_index_imm, block_regs[cur_thread].reg[CurIns_blocked->src1_index]);
                        else
                            SIM_MemDataWrite(block_regs[cur_thread].reg[CurIns_blocked->dst_index] + block_regs[cur_thread].reg[CurIns_blocked->src2_index_imm], block_regs[cur_thread].reg[CurIns_blocked->src1_index]);
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + storeTime;
                        threadsInIO[cur_thread] = true;
                        if (!isLastThread_blocked(cur_thread))
                            globalCyclesCounter += contextPenalty;
                        cur_thread = (cur_thread+1)%ThreadsNum;
                        break;
					case CMD_HALT:
						// op_halt();
						if(!isLastThread_blocked(cur_thread))
						        globalCyclesCounter += contextPenalty;
                        globalInstructionsCounter++;
                        DEBUG_PRINT1("finished HALT command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						cur_thread = (cur_thread+1)%ThreadsNum;
						//DEBUG_PRINT1("Should never get here\n");
						break;
				}
			}
		}
		// proceed to next thread
		else
		{
			cur_thread = (cur_thread+1)%ThreadsNum;
		}
		// stop condition
		finished = true;
		for (int i=0; i<ThreadsNum; i++)
		{
			if(my_instructions_blocked[i]!= -1)
			{
				finished = false;
			}
		}
		// TODO: If the last op was halt add the penalty.
		if (finished)
		{
			break;
		}
		// DEBUG_PRINT1("cur_thread: %d\n", cur_thread);
		// DEBUG_PRINT1("my_instructions_blocked[cur_thread]: %d\n", my_instructions_blocked[cur_thread]);
	}

	return Success;
}


Status Core_fineGrained_Multithreading(){
    globalCyclesCounter = 0;
    globalInstructionsCounter = 0;
    Instuction* CurIns_fg=(Instuction*)malloc(sizeof(Instuction));
	int cur_thread = 0;
	int cur_ins_per_thread = 0;
	int32_t *dest_fg = malloc(sizeof(*dest_fg));
	uint32_t TargetAddress;
	// bool if the processing is over
	bool finished = false;
	// update the Threads Number
	ThreadsNum = Get_thread_number();
	// initiate the contexts
	finegrained_regs = malloc(sizeof(*finegrained_regs)*ThreadsNum);
    for(int i=0; i<ThreadsNum; i++){
		for (int j=0; j<8; j++)
		{
			finegrained_regs[i].reg[j] = 0;
		}
	}
	// initiate the global instructions array
	my_instructions_fg = malloc(sizeof(*my_instructions_fg)*ThreadsNum);
    int* threadsWaitingLatency = malloc(sizeof(int) * ThreadsNum);
    bool* threadsInIO = malloc(sizeof(bool) * ThreadsNum);
	for (int i=0; i<ThreadsNum; i++)
	{
		my_instructions_fg[i] = 0;
        threadsWaitingLatency[i] = 0;
        threadsInIO[i] = false;
	}

    int loadTime, storeTime;
    int latencies[2];
    Mem_latency(latencies);
    loadTime = latencies[0];
    storeTime = latencies[1];

	// the process itself
	DEBUG_PRINT2("| # | T | PC |   OP   | dst | s1 | s2 | imm | res | \n");
	while (1)
	{
		if(my_instructions_fg[cur_thread] != -1)
		{
            globalCyclesCounter++;
            DEBUG_PRINT1("globalCyclesCounter = %d\n",globalCyclesCounter);
            DEBUG_PRINT1("thread %d is %s\n", cur_thread,threadsInIO[cur_thread] ? "IO" : "not waiting" );
            DEBUG_PRINT1("threadsWaitingLatency[%d] = %d\n",cur_thread, threadsWaitingLatency[cur_thread]);
			// for(int i=0; i<3; i++)
			// {
				// DEBUG_PRINT1("%d %d\n", i, my_instructions_fg[i]);
			// }
			cur_ins_per_thread = my_instructions_fg[cur_thread];
            DEBUG_PRINT1("my_instructions_fg[%d] = %d\n", cur_thread,cur_ins_per_thread);
			SIM_MemInstRead(cur_ins_per_thread, CurIns_fg, cur_thread);
            DEBUG_PRINT1("CurIns_blocked->opcode = %d\n\n", CurIns_fg->opcode);

            DEBUG_PRINT2("| %d ", globalCyclesCounter);
            DEBUG_PRINT2("| T%d ", cur_thread);
            DEBUG_PRINT2("| %d ", cur_ins_per_thread);
            DEBUG_PRINT2("| %s ", OP_STRING[CurIns_fg->opcode]);
            DEBUG_PRINT2("| %d ", CurIns_fg->dst_index);
            DEBUG_PRINT2("| %d ", CurIns_fg->src1_index);
            DEBUG_PRINT2("| %X ", CurIns_fg->src2_index_imm);
            DEBUG_PRINT2("| %s ", CurIns_fg->isSrc2Imm ? "true" : "false");

            // update the instructions array
				// DEBUG_PRINT1("before everything: \n");
				// DEBUG_PRINT1("CurIns_fg->opcode: %d\n", CurIns_fg->opcode);
				// DEBUG_PRINT1("cur_ins_per_thread: %d\n", cur_ins_per_thread);
				// DEBUG_PRINT1("cur_thread: %d\n", cur_thread);
			if (CurIns_fg!=NULL && CurIns_fg->opcode == CMD_HALT && !threadsInIO[cur_thread]) // if it is HALT OPERATION
			{
				my_instructions_fg[cur_thread] = -1;
            }
			else if( threadsInIO[cur_thread] )
            {
                if  (threadsWaitingLatency[cur_thread] - globalCyclesCounter >= 0)
                {
                    DEBUG_PRINT1("Thread %d is still waiting\n\n",cur_thread);
                    if(!isLastThread_fg(cur_thread))
                    {
                        cur_thread = (cur_thread + 1) % ThreadsNum;
                    }
                    DEBUG_PRINT2("| waiting pc=%d|\n",cur_ins_per_thread);
                    continue;
                }
                else // finished IO process
                {
                    DEBUG_PRINT1("Thread %d finished IO process\n\n",cur_thread);
                    threadsInIO[cur_thread] = false;
                    threadsWaitingLatency[cur_thread] = 0;
                    globalInstructionsCounter++;
                    DEBUG_PRINT1("finished IO command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                    if(!isLastThread_fg(cur_thread))
                    {
                        cur_thread = (cur_thread + 1) % ThreadsNum;
                    }
                    DEBUG_PRINT2("| prev IO done pc=%d|\n",cur_ins_per_thread);
                    continue;
                }
            } else
            {
                my_instructions_fg[cur_thread] = cur_ins_per_thread+1;
            }

			if (CurIns_fg!=NULL)
			{
				switch (CurIns_fg->opcode) {
					case CMD_NOP:
						break;
					case CMD_ADD:
						// DEBUG_PRINT1("in add\n");
						// DEBUG_PRINT1("CurIns_fg->dst_index: %d\n", CurIns_fg->dst_index);
						// DEBUG_PRINT1("CurIns_fg->src1_index: %d\n", CurIns_fg->src1_index);
						// DEBUG_PRINT1("CurIns_fg->src2_index_imm: %d\n", CurIns_fg->src2_index_imm);
						// DEBUG_PRINT1("block_regs[cur_thread].reg[src1_index]: %d\n",block_regs[cur_thread].reg[CurIns_fg->src1_index]);
						// DEBUG_PRINT1("block_regs[cur_thread].reg[src2_index_imm]: %d\n",block_regs[cur_thread].reg[CurIns_fg->src2_index_imm]);
						op_add_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        DEBUG_PRINT1("finished ADD command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						// DEBUG_PRINT1("finegrained_regs[cur_thread].reg[CurIns_fg->dst_index: %d\n", finegrained_regs[cur_thread].reg[CurIns_fg->dst_index]);
						// DEBUG_PRINT1("cur_thread: %d\n", cur_thread);
						break;
					case CMD_SUB:
						// DEBUG_PRINT1("in sub\n");
						op_sub_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        DEBUG_PRINT1("finished SUB command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                        break;
					case CMD_ADDI:
						// DEBUG_PRINT1("in addi\n");
						op_addi_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        DEBUG_PRINT1("finished ADDI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                        break;
					case CMD_SUBI:
						// DEBUG_PRINT1("in subi\n");
						op_subi_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        DEBUG_PRINT1("finished SUBI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                        break;
					case CMD_LOAD:
						// DEBUG_PRINT1("in load\n");
                        // TODO note that it may not be an immediate
                        if (CurIns_fg->isSrc2Imm)
						    TargetAddress = finegrained_regs[cur_thread].reg[CurIns_fg->src1_index] + CurIns_fg->src2_index_imm;
                        else
                            TargetAddress = finegrained_regs[cur_thread].reg[CurIns_fg->src1_index] + finegrained_regs[cur_thread].reg[CurIns_fg->src2_index_imm];
						SIM_MemDataRead(TargetAddress, dest_fg);
						finegrained_regs[cur_thread].reg[CurIns_fg->dst_index] = (int)(*dest_fg);
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + loadTime;
                        threadsInIO[cur_thread] = true;
                        DEBUG_PRINT2("| %d |\n", finegrained_regs[cur_thread].reg[CurIns_fg->dst_index]);
						break;
					case CMD_STORE:
						// DEBUG_PRINT1("in store\n");
                        if (CurIns_fg->isSrc2Imm)
						    SIM_MemDataWrite(finegrained_regs[cur_thread].reg[CurIns_fg->dst_index] + CurIns_fg->src2_index_imm,block_regs[cur_thread].reg[CurIns_fg->src1_index]);
                        else
                            SIM_MemDataWrite(finegrained_regs[cur_thread].reg[CurIns_fg->src1_index] + finegrained_regs[cur_thread].reg[CurIns_fg->src2_index_imm],block_regs[cur_thread].reg[CurIns_fg->src1_index]);
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + storeTime;
                        threadsInIO[cur_thread] = true;
                        DEBUG_PRINT2("| %d |\n", finegrained_regs[cur_thread].reg[CurIns_fg->src1_index]);
                        break;
					case CMD_HALT:
						// op_halt();
						// cur_thread = (cur_thread+1)%ThreadsNum;
						// DEBUG_PRINT1("in halt\n");
                        globalInstructionsCounter++;
                        DEBUG_PRINT1("finished HALT command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                        DEBUG_PRINT2("\n");
                        break;
				}
			}
		}
        // If there are threads left other then this thread context switch to next thread
        if (!isLastThread_fg(cur_thread))
            cur_thread = (cur_thread+1)%ThreadsNum;
		finished = true;
		for (int i=0; i<ThreadsNum; i++)
		{
			if(my_instructions_fg[i]!= -1)
			{
				finished = false;
			}
		}
		if (finished)
		{
			break;
		}
		// DEBUG_PRINT1("cur_thread: %d\n", cur_thread);
		// DEBUG_PRINT1("my_instructions_fg[cur_thread]: %d\n", my_instructions_fg[cur_thread]);
	}

	return Success;
}



double Core_finegrained_CPI(){
	return (double)globalCyclesCounter/globalInstructionsCounter;
}
double Core_blocked_CPI(){
	return (double)globalCyclesCounter/globalInstructionsCounter;
}

Status Core_blocked_context(tcontext* bcontext,int threadid){
	// *bcontext = block_regs[threadid];
	for(int i=0; i<8; i++)
	{
		bcontext[threadid].reg[i] = block_regs[threadid].reg[i];
	}
	return Success;
}

Status Core_finegrained_context(tcontext* finegrained_context,int threadid){
	for(int i=0; i<8; i++)
	{
		finegrained_context[threadid].reg[i] = block_regs[threadid].reg[i];
	}
	return Success;
}


void op_add_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	block_regs[cur_thread].reg[dst_index] = block_regs[cur_thread].reg[src1_index] + block_regs[cur_thread].reg[src2_index_imm];
	// DEBUG_PRINT1("block_regs[cur_thread].reg[src1_index]: %d\n",block_regs[cur_thread].reg[src1_index]);
	// DEBUG_PRINT1("block_regs[cur_thread].reg[src2_index_imm]: %d\n",block_regs[cur_thread].reg[src2_index_imm]);
	// DEBUG_PRINT1("in func: block_regs[cur_thread].reg[dst_index]: %d\n",block_regs[cur_thread].reg[dst_index]);
	DEBUG_PRINT2("| %d |\n", block_regs[cur_thread].reg[dst_index]);
}

void op_sub_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	block_regs[cur_thread].reg[dst_index] = block_regs[cur_thread].reg[src1_index] - block_regs[cur_thread].reg[src2_index_imm];
    DEBUG_PRINT2("| %d |\n", block_regs[cur_thread].reg[dst_index]);
}

void op_addi_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	block_regs[cur_thread].reg[dst_index] = block_regs[cur_thread].reg[src1_index] + src2_index_imm;
    DEBUG_PRINT2("| %d |\n", block_regs[cur_thread].reg[dst_index]);
}

void op_subi_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	block_regs[cur_thread].reg[dst_index] = block_regs[cur_thread].reg[src1_index] - src2_index_imm;
    DEBUG_PRINT2("| %d |\n", block_regs[cur_thread].reg[dst_index]);
}

void op_add_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] + finegrained_regs[cur_thread].reg[src2_index_imm];
	// DEBUG_PRINT1("finegrained_regs[cur_thread].reg[src1_index]: %d\n",finegrained_regs[cur_thread].reg[src1_index]);
	// DEBUG_PRINT1("finegrained_regs[cur_thread].reg[src2_index_imm]: %d\n",finegrained_regs[cur_thread].reg[src2_index_imm]);
	// DEBUG_PRINT1("in func: finegrained_regs[cur_thread].reg[dst_index]: %d\n",finegrained_regs[cur_thread].reg[dst_index]);
    DEBUG_PRINT2("| %d |\n", finegrained_regs[cur_thread].reg[dst_index]);

}

void op_sub_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] - finegrained_regs[cur_thread].reg[src2_index_imm];
    DEBUG_PRINT2("| %d |\n", finegrained_regs[cur_thread].reg[dst_index]);

}

void op_addi_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] + src2_index_imm;
    DEBUG_PRINT2("| %d |\n", finegrained_regs[cur_thread].reg[dst_index]);
}

void op_subi_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
    finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] - src2_index_imm;
    DEBUG_PRINT2("| %d |\n", finegrained_regs[cur_thread].reg[dst_index]);
}