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
Status Core_blocked_Multithreading(){
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
	for (int i=0; i<ThreadsNum; i++)
	{
		my_instructions_blocked[i] = 0;
	} 
	
	// the process itself
	while (1)
	{
		if(my_instructions_blocked[cur_thread] != -1) // Finished 
		{
			// for(int i=0; i<3; i++)
			// {
				// printf("%d %d\n", i, my_instructions_blocked[i]);
			// }
			cur_ins_per_thread = my_instructions_blocked[cur_thread];
			SIM_MemInstRead(cur_ins_per_thread, CurIns_blocked, cur_thread);
			// update the instructions array
				// printf("before everything: \n");
				// printf("CurIns_blocked->opcode: %d\n", CurIns_blocked->opcode);
				// printf("cur_ins_per_thread: %d\n", cur_ins_per_thread);
				// printf("cur_thread: %d\n", cur_thread);
			if (CurIns_blocked!=NULL && CurIns_blocked->opcode == 7) // if it is HALT OPERATION
			{
				my_instructions_blocked[cur_thread] = -1;
			}
			else
			{
				//precede the instructions
				my_instructions_blocked[cur_thread] = cur_ins_per_thread+1;
				// if (CurIns_blocked!=NULL && (CurIns_blocked->opcode == 5 || CurIns_blocked->opcode == 6)) // if it is LOAD/STORE OPERATION
				// {
					// printf("in load/store: \n");
					// scanf("%d", &debug);
					// cur_thread = (cur_thread+1)%ThreadsNum;
				// }
			} 
			// make the operation
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
						break;						
					case CMD_SUB:
						// printf("in sub\n");					
						op_sub_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
						break;
					case CMD_ADDI:
						// printf("in addi\n");					
						op_addi_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
						break;
					case CMD_SUBI:
						// printf("in subi\n");					
						op_subi_blocked(CurIns_blocked->dst_index, CurIns_blocked->src1_index, CurIns_blocked->src2_index_imm, cur_thread);
						break;
					case CMD_LOAD:
					// printf("in load\n"); 
						TargetAddress = block_regs[cur_thread].reg[CurIns_blocked->src1_index] + CurIns_blocked->src2_index_imm;
						SIM_MemDataRead(TargetAddress, dest_blocked);
						block_regs[cur_thread].reg[CurIns_blocked->dst_index] = (int)(*dest_blocked);
						cur_thread = (cur_thread+1)%ThreadsNum;
						break;
					case CMD_STORE:
						// printf("in store\n");
						SIM_MemDataWrite(CurIns_blocked->dst_index, block_regs[cur_thread].reg[CurIns_blocked->src1_index] + CurIns_blocked->src2_index_imm);
						cur_thread = (cur_thread+1)%ThreadsNum;
						break;						
					case CMD_HALT:
						// op_halt();
						cur_thread = (cur_thread+1)%ThreadsNum;
						// printf("in halt\n");
						break;						
				}
			}
		}
		
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
		// printf("cur_thread: %d\n", cur_thread);
		// printf("my_instructions_blocked[cur_thread]: %d\n", my_instructions_blocked[cur_thread]);
	}

	return Success;
}


Status Core_fineGrained_Multithreading(){

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
	for (int i=0; i<ThreadsNum; i++)
	{
		my_instructions_fg[i] = 0;
	} 
	
	// the process itself
	while (1)
	{
		if(my_instructions_fg[cur_thread] != -1)
		{
			// for(int i=0; i<3; i++)
			// {
				// printf("%d %d\n", i, my_instructions_fg[i]);
			// }
			cur_ins_per_thread = my_instructions_fg[cur_thread];
			SIM_MemInstRead(cur_ins_per_thread, CurIns_fg, cur_thread);
			// update the instructions array
				// printf("before everything: \n");
				// printf("CurIns_fg->opcode: %d\n", CurIns_fg->opcode);
				// printf("cur_ins_per_thread: %d\n", cur_ins_per_thread);
				// printf("cur_thread: %d\n", cur_thread);
			if (CurIns_fg!=NULL && CurIns_fg->opcode == 7) // if it is HALT OPERATION
			{
				my_instructions_fg[cur_thread] = -1;
			}
			else
			{
				//precede the instructions
				my_instructions_fg[cur_thread] = cur_ins_per_thread+1;
				// if (CurIns_fg!=NULL && (CurIns_fg->opcode == 5 || CurIns_fg->opcode == 6)) // if it is LOAD/STORE OPERATION
				// {
					// printf("in load/store: \n");
					// scanf("%d", &debug);
					// cur_thread = (cur_thread+1)%ThreadsNum;
				// }
			} 
			// make the operation
			// printf("before operation:");
			// scanf("%d", &debug);

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
						// printf("finegrained_regs[cur_thread].reg[CurIns_fg->dst_index: %d\n", finegrained_regs[cur_thread].reg[CurIns_fg->dst_index]);
						// printf("cur_thread: %d\n", cur_thread);
						break;						
					case CMD_SUB:
						// printf("in sub\n");					
						op_sub_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						break;
					case CMD_ADDI:
						// printf("in addi\n");					
						op_addi_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						break;
					case CMD_SUBI:
						// printf("in subi\n");					
						op_subi_fg(CurIns_fg->dst_index, CurIns_fg->src1_index, CurIns_fg->src2_index_imm, cur_thread);
						break;
					case CMD_LOAD:
						// printf("in load\n");
						TargetAddress = finegrained_regs[cur_thread].reg[CurIns_fg->src1_index] + CurIns_fg->src2_index_imm;
						SIM_MemDataRead(TargetAddress, dest_fg);
						finegrained_regs[cur_thread].reg[CurIns_fg->dst_index] = (int)(*dest_fg);
						// printf("finegrained_regs[cur_thread].reg[CurIns_fg->dst_index: %d\n", finegrained_regs[cur_thread].reg[CurIns_fg->dst_index]);
						// printf("cur_thread: %d\n", cur_thread);
						// cur_thread = (cur_thread+1)%ThreadsNum;
						break;
					case CMD_STORE:
						// printf("in store\n");
						SIM_MemDataWrite(CurIns_fg->dst_index, finegrained_regs[cur_thread].reg[CurIns_fg->src1_index] + CurIns_fg->src2_index_imm);
						// cur_thread = (cur_thread+1)%ThreadsNum;
						break;						
					case CMD_HALT:
						// op_halt();
						// cur_thread = (cur_thread+1)%ThreadsNum;
						// printf("in halt\n");
						break;						
				}
			}
		}
		
		// else
		// {
		cur_thread = (cur_thread+1)%ThreadsNum;
		// }
		// stop condition
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
	return 0;
}
double Core_blocked_CPI(){
	return 0;
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