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
#include "all_pred.h"
#include "predictor.h"
#include "tour.h"
using namespace std;

//map<string, string> InstAndCat;
// uop type encoding
/*
 #define IS_BR_CONDITIONAL (1 << 0)   // conditional branch
 #define IS_BR_INDIRECT    (1 << 1)   // indirect branch
 #define IS_BR_CALL        (1 << 2)   // call
*/

#define MAX_THREAD 4
//structure to compute result
ofstream OutFile;

struct br_pre
{
INT64 total_br;
INT64 mispredict;
INT64 tmispredict;
INT64 predict;
INT64 tpredict;
bool gpred;
bool tpred;
my_predictor mypred;
my_predictor_1 mypred_1;
//my_predictor_2 mypred_2;
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
	{
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


VOID docount(THREADID threadid) 
{ 
//icount++;
//cout << "in docount" << endl;
thread_data_t* tdata = get_tls(threadid); 
//tdata[tid]->_count++;
tdata->_count++;
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
 INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount,IARG_THREAD_ID, IARG_END); 
    
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
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
                            "o", "inscategory.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    cout << "processing data... " << endl;
    OutFile.flush();
    // Write to a file since cout and cerr maybe closed by the application
   char br_taken;
   int bt,bnt=0;
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
    if ( temp->type == IS_BR_CONDITIONAL)
         {//cout << "saw cond_br " << endl;
	  branch[temp->pc].tid = temp->tid;
	  branch[temp->pc].pc = temp->pc;
	  branch[temp->pc].total_ind_br++;
	  branch[temp->pc].fn_name_br=RTN_FindNameByAddress(temp->pc);
	  pred[temp->tid].total_br++;
	if (temp->taken == 0)
		{ //cout << "not taken" << endl;
		  br_taken='N';
                  bnt++;
		}	
	else {
		br_taken='T';
		bt++;
	//	cout << "taken" << endl;
	     }
	  // One bit predictor	  
//	cout << " 1 bit " << endl;
          pred[temp->tid].mypred_1.bimodal1bit1024 += pred[temp->tid].mypred_1.predictor1bit(pred[temp->tid].mypred_1.table1024, temp->pc % 1024, br_taken);
          

	//Two bit predictor
//	cout << " 2 bit " << endl;
	pred[temp->tid].mypred_1.bimodal2bit1024 += pred[temp->tid].mypred_1.predictor2bit(pred[temp->tid].mypred_1.table1024, temp->pc % BR_SIZE, br_taken);
        
        int index = temp->pc % BR_SIZE;
	//global vs local predictor
	/*pred[temp->tid].tpred = pred[temp->tid].mypred_2.make_prediction (temp->pc);
		
		if (pred[temp->tid].tpred == temp->taken)
			{	pred[temp->tid].tpredict++;
			}
		else
			{	pred[temp->tid].tmispredict++;
			}
*/
	//tournament predictor
//	cout << " tour " << endl;
		pred[temp->tid].mypred_1.tCounter += pred[temp->tid].mypred_1.tournamentPred(pred[temp->tid].mypred_1.gtable[8][index], pred[temp->tid].mypred_1.table1024[index] % 4, pred[temp->tid].mypred_1.gtable[8][index ^ pred[temp->tid].mypred_1.globalReg & (BR_SIZE-1)] % 4, br_taken);

	// Gshare Predictor (2 - 10 bits global register)
        for (int i = 2; i <= 10; i++) {
            int cutOff = 1 << i; //Use to obtain bits from global register
            pred[temp->tid].mypred_1.gCounter[i - 2] += pred[temp->tid].mypred_1.predictor2bit(pred[temp->tid].mypred_1.gtable[i - 2], (index) ^ (pred[temp->tid].mypred_1.globalReg % cutOff), br_taken);
        }
        pred[temp->tid].mypred_1.globalReg = pred[temp->tid].mypred_1.globalReg << 1;
        if (br_taken == 'T') {
            pred[temp->tid].mypred_1.alwaysT++;
            pred[temp->tid].mypred_1.globalReg++;
        } else {
            pred[temp->tid].mypred_1.alwaysNT++;
        }
        pred[temp->tid].mypred_1.globalReg &= (BR_SIZE-1); //keep global register to 10 bits
	
	
	//TAGE 
//	cout << " TAGE " << endl;
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
	//	pred[temp->tid].total_br++;
	//	pred[temp->tid].predict++;
	}	
       pred[temp->tid].mypred.FetchHistoryUpdate (temp->pc, temp->type ,temp->taken,temp->target);
       pred[temp->tid].mypred.update_brcond (temp->pc, temp->type ,temp->taken,temp->target);
      // }
temp = temp ->next;
}
   cout.setf(ios::fixed, ios::floatfield);
   cout.setf(ios::showpoint);

    /*//cout << " TID :" << temp->tid  << " address :" << hex << temp -> pc << " taken: " << temp->taken << " target :" << hex << temp -> target << endl;
    OutFile << " " << temp->tid  << " " <<  temp -> pc << " " <<temp->type << " " << temp->taken << " " <<  temp -> target << endl;
    temp = temp ->next;
//   cout << "total_br :" << total_br << " predict :" <<predict << " misprecdict :" << mispredict << endl;
   */
    map<INT64, ind_br>::iterator it;
   /* for (it = branch.begin(); it != branch.end(); it++) {
    OutFile << hex << it->first << "	" << it -> second.tid << "	" << dec << it ->second.total_ind_br <<"	"<<it->second.pred_ind_br <<"	" << it-> second.mispred_ind_br << "	" << it -> second.fn_name_br <<  endl;
   }*/

//for (x=0;x<=MAX_THREAD;x++)
   cout << "done" << endl;
    OutFile << " I_count  br_count  1bit  2bit  tour  TAGE " << endl; 
for (x=0;x<=2;x++)
{
    thread_data_t* tdata = get_tls(x);
   //double pre=(double) (pred[x].predict)/pred[x].total_br*100;
   //double misp=(double) (pred[x].mispredict)/pred[x].total_br*100;
  // OutFile << "1-bit Instructions " << tdata->_count << "  total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" <<pred[x].mypred_1.bimodal1bit1024 << endl;
  // OutFile << "2-bit Instructions " << tdata->_count << "  total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" <<pred[x].mypred_1.bimodal2bit1024 << endl;
  // OutFile << "global Instructions " << tdata->_count << "  total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" <<pred[x].tpredict << endl;
  // OutFile << "TOUR Instructions " << tdata->_count << "  total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" << pred[x].mypred_1.tCounter << endl;
  // OutFile << "TAGE Instructions " << tdata->_count << "  total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" <<pred[x].predict << endl;
   //cout << "total_br_" <<x<<" :" << pred[x].total_br << " predict_" <<x<<" :" <<pred[x].predict << " misprecdict_" <<x<<" :" << pred[x].mispredict <<" % Predict :" << pre << " % Mispredict :" << misp << endl;
  
   OutFile  <<  tdata->_count << "  "  << pred[x].total_br << "  " << pred[x].mypred_1.bimodal1bit1024 << "  " << pred[x].mypred_1.bimodal2bit1024 << "  " << pred[x].mypred_1.tCounter << "  " << pred[x].predict << endl;
   // cout << "br_count " << pred[x].mypred_1.iCount << endl;
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
