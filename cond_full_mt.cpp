//
//  cond_pin.cpp
//  
//
//  Created by Surya Narayanan on 20/09/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <fstream>
#include "pin.H"
#include <map>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include "instlib.H"
#define INCLUDEPRED
#include "predictor.h"
using namespace std;

//map<string, string> InstAndCat;
// uop type encoding
/*
 #define IS_BR_CONDITIONAL (1 << 0)   // conditional branch
 #define IS_BR_INDIRECT    (1 << 1)   // indirect branch
 #define IS_BR_CALL        (1 << 2)   // call
*/

#define MAX_THREAD 8
//structure to compute result
ofstream OutFile;

struct br_pre
{
INT64 total_br;
INT64 mispredict;
INT64 predict;
bool gpred;
my_predictor mypred;
}pred[MAX_THREAD];

//structure to store the per-branch result
typedef struct branch_data {
    INT tid;
    INT64 pc;
    INT64 total_ind_br;
    INT64 pred_ind_br;
    INT64 mispred_ind_br;
    string fn_name_br;
}ind_br;

map<INT64,ind_br>branch;

PIN_LOCK lock;
INT32 numThreads = 0;
static  TLS_KEY tls_key;


class thread_data_t
{
  public:
    thread_data_t() : _count(0) {} 
    UINT64 _count;
//    UINT8 _pad[PADSIZE];
};


thread_data_t* get_tls(THREADID threadid)
{
    thread_data_t* tdata = 
          static_cast<thread_data_t*>(PIN_GetThreadData(tls_key, threadid));
    return tdata;
}

VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    GetLock(&lock, threadid+1);
    numThreads++;
    cout << " Starting thread " << threadid << endl; 
    ReleaseLock(&lock);
    thread_data_t* tdata = new thread_data_t;

    PIN_SetThreadData(tls_key, tdata, threadid);
}

typedef struct inputs {
    INT tid;
    INT64 pc;
    INT type;
    bool taken;
    INT64 target;
    struct inputs *next;
}in;

int x,y,total_br,mispredict,predict = 0;
in *head = NULL;
static my_predictor *mypred;


VOID collect (ADDRINT add_pc, bool tk, ADDRINT target_add, UINT32 value,THREADID th_id)
	{/*
	       total_br++;
              
	       bool gpred = mypred->predict_brcond (add_pc,value );
	       mypred->FetchHistoryUpdate (add_pc & 0x3ffff, value ,tk,target_add & 0x7f );
		if (gpred == tk)
			predict++;
	       else
			mispredict++;*/
//cout << th_id;
    GetLock(&lock, th_id+1);
		in *root;
		root = new in;
		root -> tid = th_id;
		root->pc = add_pc;
		root->type = value;
		root->taken =tk;
		root->target = target_add;
		root->next=head;
		head = root;
    ReleaseLock(&lock);
//		cout << ++y << " address :" << hex << add_pc << " taken: " << tk << " target :" << hex << target_add << endl;
	}
// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    
    if (INS_Category(ins) == XED_CATEGORY_COND_BR) 
   // if(INS_IsBranchOrCall(ins)) 
	{
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32,IS_BR_CONDITIONAL,IARG_THREAD_ID,IARG_END);
    }
    else if ( INS_Category(ins) == XED_CATEGORY_UNCOND_BR)
  //  else if (INS_IsIndirectBranchOrCall(ins))

	{
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32, IS_BR_INDIRECT,IARG_THREAD_ID,IARG_END);
    }
    else if ( INS_Category(ins) == XED_CATEGORY_CALL || INS_Category(ins) == XED_CATEGORY_RET) 
 //   else if (INS_IsRet(ins))
	{
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32, IS_BR_CALL,IARG_THREAD_ID,IARG_END);
    }
/*	UINT32 cat = INS_Category(ins);
	//conditional indirect branch
	if(cat == XED_CATEGORY_COND_BR && INS_IsIndirectBranchOrCall(ins)) 
            {
                INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR, IARG_BOOL, TRUE,  IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_INDIRECT,IARG_THREAD_ID, IARG_END);
                INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) collect, IARG_INST_PTR,IARG_BOOL, FALSE, IARG_FALLTHROUGH_ADDR, IARG_UINT32, IS_BR_INDIRECT,IARG_THREAD_ID, IARG_END);  
            }
       //  conditional direct branches.
	    else if(cat == XED_CATEGORY_COND_BR)
        	{
		  INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR, IARG_BOOL, TRUE, IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_CONDITIONAL, IARG_THREAD_ID,IARG_END);
		  INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) collect, IARG_INST_PTR,IARG_BOOL, FALSE, IARG_FALLTHROUGH_ADDR, IARG_UINT32, IS_BR_CONDITIONAL,IARG_THREAD_ID, IARG_END);
		}
	// unconditional indirect branches.
	   else if(cat == XED_CATEGORY_UNCOND_BR && INS_IsIndirectBranchOrCall(ins))
	       {
	         INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR,IARG_BOOL, TRUE, IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_INDIRECT,IARG_THREAD_ID, IARG_END);
		}
	// unconditional direct branches.
	  else if(cat == XED_CATEGORY_UNCOND_BR || cat == XED_CATEGORY_CALL || cat == XED_CATEGORY_RET)
		{
		INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR, IARG_BOOL, TRUE,IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_CALL,IARG_THREAD_ID, IARG_END);
	        }*/
//    string category = xed_category_enum_t2str((xed_category_enum_t)INS_Category(ins));
//    string instruction = xed_iclass_enum_t2str((xed_iclass_enum_t)INS_Opcode(ins));
//    string extension = xed_extension_enum_t2str((xed_extension_enum_t)INS_Extension(ins));
//    
//    map<string, string>::iterator it;
//    it = InstAndCat.find(instruction);
//    if ( it == InstAndCat.end() ) {
//        InstAndCat[instruction] = category + " " + extension;
//    }
//	cout << ++x << " "<< INS_Disassemble(ins) << endl;
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
                            "o", "inscategory.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    OutFile.flush();
    // Write to a file since cout and cerr maybe closed by the application
   
    OutFile.setf(ios::showbase);
    //OutFile << "Instruction Category Extension" << endl;
//    map<string, string>::iterator it;
//    for (it = InstAndCat.begin(); it != InstAndCat.end(); it++) {
//        OutFile << it->first << " " << it->second << endl;
//    }
    in *temp;
    temp = head;
         cout << "processing data... " << endl;
while (temp!=NULL)
{
    if ( temp->type == IS_BR_CONDITIONAL)
         {//cout << "saw cond_br " << endl;
	  branch[temp->pc].tid = temp->tid;
	  branch[temp->pc].pc = temp->pc;
	  branch[temp->pc].total_ind_br++;
	  branch[temp->pc].fn_name_br=RTN_FindNameByAddress(temp->pc);
	  pred[temp->tid].total_br++;
          pred[temp->tid].gpred = pred[temp->tid].mypred.predict_brcond (temp->pc,temp->type );
	       //mypred->FetchHistoryUpdate (temp->tid & 0x3ffff, temp->pc ,temp->type,temp->taken & 0x7f );
		if (pred[temp->tid].gpred == temp->taken)
			{	pred[temp->tid].predict++;
				branch[temp->pc].pred_ind_br++;
			}
		else
			{	pred[temp->tid].mispredict++;
				branch[temp->pc].mispred_ind_br++;
			}
	}
    else
	{
		pred[temp->tid].total_br++;
		pred[temp->tid].predict++;
	}	
       pred[temp->tid].mypred.FetchHistoryUpdate (temp->pc, temp->type ,temp->taken,temp->target);
       pred[temp->tid].mypred.update_brcond (temp->pc, temp->type ,temp->taken,temp->target);
temp = temp ->next;
}
   cout.setf(ios::fixed, ios::floatfield);
   cout.setf(ios::showpoint);

    /*//cout << " TID :" << temp->tid  << " address :" << hex << temp -> pc << " taken: " << temp->taken << " target :" << hex << temp -> target << endl;
    OutFile << " " << temp->tid  << " " <<  temp -> pc << " " <<temp->type << " " << temp->taken << " " <<  temp -> target << endl;
    temp = temp ->next;
} 
//   cout << "total_br :" << total_br << " predict :" <<predict << " misprecdict :" << mispredict << endl;
   */
    map<INT64, ind_br>::iterator it;
   /* for (it = branch.begin(); it != branch.end(); it++) {
    OutFile << hex << it->first << "	" << it -> second.tid << "	" << dec << it ->second.total_ind_br <<"	"<<it->second.pred_ind_br <<"	" << it-> second.mispred_ind_br << "	" << it -> second.fn_name_br <<  endl;
   }*/

for (x=0;x<=MAX_THREAD;x++)
{
   double pre=(double) (pred[x].predict)/pred[x].total_br*100;
   double misp=(double) (pred[x].mispredict)/pred[x].total_br*100;
   OutFile << "total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" <<pred[x].predict << " misprecdict_" <<x<<" :" << pred[x].mispredict <<" % Predict :" << pre << " % Mispredict :" << misp << endl;
   cout << "total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" <<pred[x].predict << " misprecdict_" <<x<<" :" << pred[x].mispredict <<" % Predict :" << pre << " % Mispredict :" << misp << endl;
}
    OutFile.flush();
	OutFile.close();
}




/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool gets the category and extension of each instruction executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
mypred = new my_predictor();
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    PIN_InitSymbols();
    // Initialize the lock
    OutFile.open(KnobOutputFile.Value().c_str());
    
    InitLock(&lock);
    
    // Obtain  a key for TLS storage.
    tls_key = PIN_CreateThreadDataKey(0);

    // Register ThreadStart to be called when a thread starts.
    PIN_AddThreadStartFunction(ThreadStart, 0);
    
    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);
    
    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
