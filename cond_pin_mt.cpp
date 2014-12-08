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
    // Write to a file since cout and cerr maybe closed by the application
    ofstream OutFile;
    OutFile.open(KnobOutputFile.Value().c_str());
    OutFile.setf(ios::showbase);
    //OutFile << "Instruction Category Extension" << endl;
//    map<string, string>::iterator it;
//    for (it = InstAndCat.begin(); it != InstAndCat.end(); it++) {
//        OutFile << it->first << " " << it->second << endl;
//    }
    in *temp;
    temp = head;
while (temp!=NULL)
{
    //cout << " TID :" << temp->tid  << " address :" << hex << temp -> pc << " taken: " << temp->taken << " target :" << hex << temp -> target << endl;
    OutFile << " " << temp->tid  << " " <<  temp -> pc << " " <<temp->type << " " << temp->taken << " " <<  temp -> target << endl;
    temp = temp ->next;
} 
//   cout << "total_br :" << total_br << " predict :" <<predict << " misprecdict :" << mispredict << endl;
   
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
    // Initialize the lock
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
