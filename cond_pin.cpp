//
//  cond_pin.cpp
//  
//
//  Created by Surya Narayanan on 20/09/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//
#include <math.h>
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
//#include "predictor_indirect.h"
using namespace std;

//map<string, string> InstAndCat;
// uop type encoding
/*
 #define IS_BR_CONDITIONAL (1 << 0)   // conditional branch
 #define IS_BR_INDIRECT    (1 << 1)   // indirect branch
 #define IS_BR_CALL        (1 << 2)   // call
*/
typedef struct inputs {
    INT64 pc;
    string br_type;
    bool taken;
    INT64 target;
    struct inputs *next;
}in;

typedef struct branch_data {
    INT64 pc;
    int total_ind_br;
    int pred_ind_br;
    int mispred_ind_br;
    string fn_name_br;
}ind_br;

//ind_br branch_t={0,0,0,0,NULL};
map<INT64,ind_br>branch;



int x,y,total_br,mispredict,predict = 0;
in *head = NULL;
static my_predictor *mypred;
/* checking less than for key value*/
struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};


VOID collect (ADDRINT add_pc, bool tk, ADDRINT target_add, UINT32 value)
	{
	if ( value == IS_BR_CONDITIONAL)
	{
	  branch[add_pc].pc = add_pc;
	  branch[add_pc].total_ind_br++;
	  branch[add_pc].fn_name_br=RTN_FindNameByAddress(add_pc);
	       total_br++;
              
	       bool gpred = mypred->predict_brcond (add_pc,value );
	       //bool gpred = mypred->predict_brindirect (add_pc,value );
		if (gpred == tk){
			predict++;
			branch[add_pc].pred_ind_br++;
			}
	       else	{
			mispredict++;
			branch[add_pc].mispred_ind_br++;
			}
	}
        else
	{
	       total_br++;
		predict++;

	}
	       mypred->FetchHistoryUpdate (add_pc & 0x3ffff, value ,tk,target_add & 0x7f );
	       mypred->update_brcond (add_pc & 0x3ffff, value , tk,target_add & 0x7f);	
	/*	in *root;
		root = new in;
		root->pc = add_pc;
		root->taken =tk;
		root->target = target_add;
		root->next=head;
		head = root;
//		cout << ++y << " address :" << hex << add_pc << " taken: " << tk << " target :" << hex << target_add << endl;
	*/
	}
// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
   UINT32 cat = INS_Category(ins); 
    //if (INS_Category(ins) == XED_CATEGORY_COND_BR) 
   if(cat == XED_CATEGORY_COND_BR) 
	{
	//INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32,IS_BR_CONDITIONAL,IARG_END);
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32,IS_BR_CONDITIONAL,IARG_END);
    }
   else if ( cat == XED_CATEGORY_UNCOND_BR)
   // else if (INS_IsIndirectBranchOrCall(ins))

	{
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32, IS_BR_INDIRECT,IARG_END);
//	INS_InsertCall(ins, IPOINT_AFTER, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32, IS_BR_INDIRECT,IARG_END);
    }
 //   else if ( INS_Category(ins) == XED_CATEGORY_CALL || INS_Category(ins) == XED_CATEGORY_RET) 
    else if (INS_IsRet(ins))
	{
//	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32, IS_BR_CALL,IARG_END);
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(collect),IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_BRANCH_TARGET_ADDR,IARG_UINT32, IS_BR_CALL,IARG_END);
    }
	//conditional indirect branch
/*	if(cat == XED_CATEGORY_COND_BR && INS_IsIndirectBranchOrCall(ins)) 
            {
                INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR, IARG_BOOL, TRUE,  IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_INDIRECT, IARG_END);
                INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) collect, IARG_INST_PTR,IARG_BOOL, FALSE, IARG_FALLTHROUGH_ADDR, IARG_UINT32, IS_BR_INDIRECT, IARG_END);  
            }
       //  conditional direct branches.
	    else if(cat == XED_CATEGORY_COND_BR)
        	{
		  INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR, IARG_BOOL, TRUE, IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_CONDITIONAL, IARG_END);
		  INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) collect, IARG_INST_PTR,IARG_BOOL, FALSE, IARG_FALLTHROUGH_ADDR, IARG_UINT32, IS_BR_CONDITIONAL, IARG_END);
		}
	// unconditional indirect branches.
	   else if(cat == XED_CATEGORY_UNCOND_BR && INS_IsIndirectBranchOrCall(ins))
	       {
	         INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR,IARG_BOOL, TRUE, IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_INDIRECT, IARG_END);
		}
	// unconditional direct branches.
	  else if(cat == XED_CATEGORY_UNCOND_BR || cat == XED_CATEGORY_CALL || cat == XED_CATEGORY_RET)
		{
		INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR) collect, IARG_INST_PTR, IARG_BOOL, TRUE,IARG_BRANCH_TARGET_ADDR, IARG_UINT32, IS_BR_CALL, IARG_END);
	        }*/
        //
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
    OutFile << "Instruction Category Extension" << endl;
    map<INT64, ind_br>::iterator it;
    for (it = branch.begin(); it != branch.end(); it++) {
       // OutFile << it->first << " " << it->second << endl;
        OutFile << it->first << "	" << hex << it -> second.pc << "	" << dec << it ->second.total_ind_br <<"	"<<it->second.pred_ind_br <<"	" << it-> second.mispred_ind_br << "	" << it -> second.fn_name_br <<  endl;
    }
//    map<string, string>::iterator it;
//    for (it = InstAndCat.begin(); it != InstAndCat.end(); it++) {
//        OutFile << it->first << " " << it->second << endl;
//    }
/*    in *temp;
    temp = head;
while (temp!=NULL)
{
    cout << " address :" << hex << temp -> pc << " taken: " << temp->taken << " target :" << hex << temp -> target << endl;
    temp = temp ->next;
} */
   cout << "total_br :" << total_br << " predict :" <<predict << " misprecdict :" << mispredict << endl;
   cout.setf(ios::fixed, ios::floatfield);
   cout.setf(ios::showpoint);
   double pre=(double) (predict)/total_br*100;
   double misp=(double) (mispredict)/total_br*100;
   cout << "% Predict :" <<  pre << "  % mispredict :" << misp << endl;  
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
    
    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);
    
    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
