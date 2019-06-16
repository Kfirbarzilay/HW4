/* 046267 Computer Architecture - Spring 2019 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
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

bool isLastThread()
{
    int numOfThreadsRunning = 0;
    for (int i=0; i<ThreadsNum; i++)
    {
        if(my_instructions_blocked[i]!= -1)
        {
            numOfThreadsRunning++;
        }
    }
    return numOfThreadsRunning == 1;
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
            printf("globalCyclesCounter = %d\n",globalCyclesCounter);
            printf("thread %d is %s\n", cur_thread,threadsInIO[cur_thread] ? "IO" : "not waiting" );
            printf("threadsWaitingLatency[%d] = %d\n",cur_thread, threadsWaitingLatency[cur_thread]);

			// for(int i=0; i<3; i++)
            // {
            // printf("%d %d\n", i, my_instructions_blocked[i]);
            // }
            cur_ins_per_thread = my_instructions_blocked[cur_thread];
            printf("my_instructions_blocked[%d] = %d\n", cur_thread,cur_ins_per_thread);
            SIM_MemInstRead(cur_ins_per_thread, CurIns_blocked, cur_thread);
            printf("CurIns_blocked->opcode = %d\n\n", CurIns_blocked->opcode);
            // update the instructions array
            // printf("before everything: \n");
            // printf("CurIns_blocked->opcode: %d\n", CurIns_blocked->opcode);
            // printf("cur_ins_per_thread: %d\n", cur_ins_per_thread);
            // printf("cur_thread: %d\n", cur_thread);
            if (CurIns_blocked!=NULL && CurIns_blocked->opcode == CMD_HALT) // if it is HALT OPERATION
            {
                my_instructions_blocked[cur_thread] = -1;
            }
            else if( threadsInIO[cur_thread] )
            {
                if  (threadsWaitingLatency[cur_thread] - globalCyclesCounter > 0)
                {
                    printf("Thread %d is still waiting\n\n",cur_thread);
                    if (!isLastThread()) {
                        cur_thread = (cur_thread + 1) % ThreadsNum;
                        globalCyclesCounter += contextPenalty;
                    }
                    continue;
                }
                else // finished IO process
                {
                    printf("Thread %d finished IO process\n\n",cur_thread);
                    threadsInIO[cur_thread] = false;
                    threadsWaitingLatency[cur_thread] = 0;
                    globalInstructionsCounter++;
                    printf("finished IO command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                    continue;
                }
            } else
            {
                my_instructions_blocked[cur_thread] = cur_ins_per_thread+1;
            }
            // Thread is in IO
            //make the operation
			// printf("before operation:");
			// scanf("%d", &debug);

			if (CurIns_blocked!=NULL)
			{
				switch (CurIns_blocked->opcode) {
					case CMD_NOP:
						break;
					case CMD_ADD:
						// printf("in add\n");
						// printf("CurIns_blocked->dst_index: %d\n", CurIns_blocked->dst_index);
						// printf("CurIns_blocked->src1_index: %d\n", CurIns_blocked->src1_index);
						// printf("CurIns_blocked->src2_index_imm: %d\n", CurIns_blocked->src2_index_imm);
						op_add_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        printf("finished ADD command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_SUB:
						// printf("in sub\n");
						op_sub_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
                        globalInstructionsCounter++;
                        printf("finished SUB command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_ADDI:
						// printf("in addi\n");
						op_addi_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
                        globalInstructionsCounter++;
                        printf("finished ADDI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_SUBI:
						// printf("in subi\n");
						op_subi_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
                        globalInstructionsCounter++;
                        printf("finished SUBI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						break;
					case CMD_LOAD:
					// printf("in load\n");
						TargetAddress = block_regs[cur_thread].reg[CurIns_blocked->src1_index] + CurIns_blocked->src2_index_imm;
						SIM_MemDataRead(TargetAddress, dest_blocked);
						block_regs[cur_thread].reg[CurIns_blocked->dst_index] = (int)(*dest_blocked);
						// TODO make a context switch to next thread.
						// TODO state that the thread is in IO operation and store the clock cycle where memory data is ready
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + loadTime;
                        threadsInIO[cur_thread] = true;
                        if (!isLastThread())
                            globalCyclesCounter += contextPenalty;
                        cur_thread = (cur_thread+1)%ThreadsNum;
                        break;
					case CMD_STORE:
						// printf("in store\n");
						SIM_MemDataWrite(CurIns_blocked->dst_index, block_regs[cur_thread].reg[CurIns_blocked->src1_index] + CurIns_blocked->src2_index_imm);
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + storeTime;
                        threadsInIO[cur_thread] = true;
                        if (!isLastThread())
                            globalCyclesCounter += contextPenalty;
                        cur_thread = (cur_thread+1)%ThreadsNum;
                        break;
					case CMD_HALT:
						// op_halt();
						globalCyclesCounter += contextPenalty;
                        globalInstructionsCounter++;
                        printf("finished HALT command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						cur_thread = (cur_thread+1)%ThreadsNum;
						//printf("Should never get here\n");
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
            globalCyclesCounter -= contextPenalty;
			break;
		}
		// printf("cur_thread: %d\n", cur_thread);
		// printf("my_instructions_blocked[cur_thread]: %d\n", my_instructions_blocked[cur_thread]);
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
	while (1)
	{
		if(my_instructions_fg[cur_thread] != -1)
		{
            globalCyclesCounter++;
            printf("globalCyclesCounter = %d\n",globalCyclesCounter);
            printf("thread %d is %s\n", cur_thread,threadsInIO[cur_thread] ? "IO" : "not waiting" );
            printf("threadsWaitingLatency[%d] = %d\n",cur_thread, threadsWaitingLatency[cur_thread]);
			// for(int i=0; i<3; i++)
			// {
				// printf("%d %d\n", i, my_instructions_fg[i]);
			// }
			cur_ins_per_thread = my_instructions_fg[cur_thread];
            printf("my_instructions_fg[%d] = %d\n", cur_thread,cur_ins_per_thread);
			SIM_MemInstRead(cur_ins_per_thread, CurIns_fg, cur_thread);
            printf("CurIns_blocked->opcode = %d\n\n", CurIns_fg->opcode);
			// update the instructions array
				// printf("before everything: \n");
				// printf("CurIns_fg->opcode: %d\n", CurIns_fg->opcode);
				// printf("cur_ins_per_thread: %d\n", cur_ins_per_thread);
				// printf("cur_thread: %d\n", cur_thread);
			if (CurIns_fg!=NULL && CurIns_fg->opcode == CMD_HALT) // if it is HALT OPERATION
			{
				my_instructions_fg[cur_thread] = -1;
            }
			else if( threadsInIO[cur_thread] )
            {
                if  (threadsWaitingLatency[cur_thread] - globalCyclesCounter > 0)
                {
                    printf("Thread %d is still waiting\n\n",cur_thread);
                    if(!isLastThread())
                    {
                        cur_thread = (cur_thread + 1) % ThreadsNum;
                    }
                    continue;
                }
                else // finished IO process
                {
                    printf("Thread %d finished IO process\n\n",cur_thread);
                    threadsInIO[cur_thread] = false;
                    threadsWaitingLatency[cur_thread] = 0;
                    globalInstructionsCounter++;
                    printf("finished IO command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                    if(!isLastThread())
                    {
                        cur_thread = (cur_thread + 1) % ThreadsNum;
                    }
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
						// printf("in add\n");
						// printf("CurIns_fg->dst_index: %d\n", CurIns_fg->dst_index);
						// printf("CurIns_fg->src1_index: %d\n", CurIns_fg->src1_index);
						// printf("CurIns_fg->src2_index_imm: %d\n", CurIns_fg->src2_index_imm);
						// printf("block_regs[cur_thread].reg[src1_index]: %d\n",block_regs[cur_thread].reg[CurIns_fg->src1_index]);
						// printf("block_regs[cur_thread].reg[src2_index_imm]: %d\n",block_regs[cur_thread].reg[CurIns_fg->src2_index_imm]);
						op_add_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        printf("finished ADD command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
						// printf("finegrained_regs[cur_thread].reg[CurIns_fg->dst_index: %d\n", finegrained_regs[cur_thread].reg[CurIns_fg->dst_index]);
						// printf("cur_thread: %d\n", cur_thread);
						break;
					case CMD_SUB:
						// printf("in sub\n");
						op_sub_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        printf("finished SUB command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                        break;
					case CMD_ADDI:
						// printf("in addi\n");
						op_addi_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        printf("finished ADDI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                        break;
					case CMD_SUBI:
						// printf("in subi\n");
						op_subi_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						globalInstructionsCounter++;
                        printf("finished SUBI command. globalInstructionsCounter = %d\n", globalInstructionsCounter);
                        break;
					case CMD_LOAD:
						// printf("in load\n");
						TargetAddress = finegrained_regs[cur_thread].reg[CurIns_fg->src1_index] + CurIns_fg->src2_index_imm;
						SIM_MemDataRead(TargetAddress, dest_fg);
						finegrained_regs[cur_thread].reg[CurIns_fg->dst_index] = (int)(*dest_fg);
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + loadTime;
                        threadsInIO[cur_thread] = true;
						break;
					case CMD_STORE:
						// printf("in store\n");
						SIM_MemDataWrite(CurIns_fg->dst_index, finegrained_regs[cur_thread].reg[CurIns_fg->src1_index] + CurIns_fg->src2_index_imm);
                        threadsWaitingLatency[cur_thread] = globalCyclesCounter + storeTime;
                        threadsInIO[cur_thread] = true;
                        break;
					case CMD_HALT:
						// op_halt();
						// cur_thread = (cur_thread+1)%ThreadsNum;
						// printf("in halt\n");
                        globalInstructionsCounter++;
                        printf("finished HALT command. globalInstructionsCounter = %d\n", globalInstructionsCounter);

                        break;
				}
			}
		}

        if (!isLastThread())
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
		// printf("cur_thread: %d\n", cur_thread);
		// printf("my_instructions_fg[cur_thread]: %d\n", my_instructions_fg[cur_thread]);
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
	// printf("block_regs[cur_thread].reg[src1_index]: %d\n",block_regs[cur_thread].reg[src1_index]);
	// printf("block_regs[cur_thread].reg[src2_index_imm]: %d\n",block_regs[cur_thread].reg[src2_index_imm]);
	// printf("in func: block_regs[cur_thread].reg[dst_index]: %d\n",block_regs[cur_thread].reg[dst_index]);
}

void op_sub_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	block_regs[cur_thread].reg[dst_index] = block_regs[cur_thread].reg[src1_index] - block_regs[cur_thread].reg[src2_index_imm];
}

void op_addi_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	block_regs[cur_thread].reg[dst_index] = block_regs[cur_thread].reg[src1_index] + src2_index_imm;
}

void op_subi_blocked(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	block_regs[cur_thread].reg[dst_index] = block_regs[cur_thread].reg[src1_index] - src2_index_imm;
}

void op_add_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] + finegrained_regs[cur_thread].reg[src2_index_imm];
	// printf("finegrained_regs[cur_thread].reg[src1_index]: %d\n",finegrained_regs[cur_thread].reg[src1_index]);
	// printf("finegrained_regs[cur_thread].reg[src2_index_imm]: %d\n",finegrained_regs[cur_thread].reg[src2_index_imm]);
	// printf("in func: finegrained_regs[cur_thread].reg[dst_index]: %d\n",finegrained_regs[cur_thread].reg[dst_index]);
}

void op_sub_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] - finegrained_regs[cur_thread].reg[src2_index_imm];
}

void op_addi_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
	finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] + src2_index_imm;
}

void op_subi_fg(int dst_index, int src1_index, int src2_index_imm, int cur_thread)
{
    finegrained_regs[cur_thread].reg[dst_index] = finegrained_regs[cur_thread].reg[src1_index] - src2_index_imm;
}