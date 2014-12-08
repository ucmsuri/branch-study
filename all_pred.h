
#include <fstream>
#include <iostream>

// uop type encoding
 #define IS_BR_CONDITIONAL (1 << 0)   // conditional branch
 #define IS_BR_INDIRECT    (1 << 1)   // indirect branch
 #define IS_BR_CALL        (1 << 2)   // call
 #define BR_SIZE 8192
 //#define BR_SIZE 4

class my_predictor_1
{

//private:
public:
	// Prediction table for 1 bit and 2 bit Bimodal Predictor
	// Third least significant bit is for 1 bit Bimodal Predictor
	// First and second least significant bit for 2 bit Bimodal Predictor
    int *table1024; 
    int globalReg;
    int **gtable;
    int alwaysT;
    int alwaysNT;
    int bimodal1bit1024;
    int bimodal2bit1024;
    int gCounter[9];
    int tCounter;    
    int index;
    int iCount;
    char branchTaken;
	my_predictor_1(void)
	{
    	table1024 = new int[BR_SIZE];


	// Gshare Predictor
	// Use third and fourth least significant bits of gtable[8] for tournament predictor
    	globalReg = 0;
    	gtable = new int*[9];		// Prediction table for 2 to 10 bit global register
    for (int i = 0; i < 9; i++) {
        gtable[i] = new int[BR_SIZE];
    }  


	// Initialize prediction table
	// 1 bit Bimodal Prediction Table set to Taken (1)
	// 2 bit Bimodal Prediction Table set to Strongly Taken (11)
	// Gshare Prediction Table also set to Strongly Taken (11)
	// Tournament Prediction Table set to Prefer Gshare (00)
    for (int i = 0; i < BR_SIZE; i++) {
        table1024[i] = 7;
        gtable[0][i] = gtable[1][i] = gtable[2][i] = gtable[3][i] = gtable[4][i] = gtable[5][i] = gtable[6][i] = gtable[7][i] = gtable[8][i] = 3;//gtable[9][i] = gtable[10][i] = gtable[11][i] = gtable[12][i]= 3;
    }
	// Create Counters
    	alwaysT = 0;
    	alwaysNT = 0;
        bimodal1bit1024 = 0;
        bimodal2bit1024 = 0;
    	gCounter[9] = {0};
    	tCounter = 0;
    	iCount = 0;
        




       // int index = address % 1024;
		// Tournament Predictor
       // tCounter += tournamentPred(gtable[8][index], table1024[index] % 4, gtable[8][index ^ globalReg & 1023] % 4, branchTaken);

		// Gshare Predictor (2 - 10 bits global register)
      //  for (int i = 2; i <= 10; i++) {
      //      int cutOff = 1 << i; //Use to obtain bits from global register
      //      gCounter[i - 2] += predictor2bit(gtable[i - 2], (index) ^ (globalReg % cutOff), branchTaken);
      //  }

//        globalreg = globalreg << 1;
//        if (branchtaken == 't') {
//            alwayst++;
//            globalreg++;
//        } else {
//            alwaysnt++;
//        }
//        globalreg &= (br_size-1); //keep global register to 10 bits

		//1 bit Bimodal Predictor with various table size
    
       // bimodal1bit1024 += predictor1bit(table1024, address % 1024, branchTaken);
		//2 bit Bimodal Predictor with various table size
       // bimodal2bit1024 += predictor2bit(table1024, address % 1024, branchTaken);
        iCount++;	//Instruction Count
}
/**
 * Predictor using 1 bit saturating counters
 * Update the entry in table base on the actual branch direction
 * (1 - Taken, 0 - Non-Taken)
 *
 * @param table prediction table
 * @param index locate the entry in prediction table
 * @param taken actual branch direction
 * @return whether entry in the table matches actual branch direction
 */
bool predictor1bit(int * table, int index, char taken) {
    bool correct = 0;
    if (taken == 'T') {
        if (table[index] >= 4) {
            correct = 1;
        } else {
            table[index] += 4;
        }
    } else {
        if (table[index] < 4) {
            correct = 1;
        } else {
            table[index] -= 4;
        }
    }
    return correct;
}

/**
 * Predictor using 2 bit saturating counters
 * Update the entry in table base on the actual branch direction
 * (11 - Strongly Taken, 10 - Weakly Taken, 01 - Weakly Non-Taken, 00 - Strongly Non-Taken)
 *
 * @param table prediction table
 * @param index locate the entry in prediction table
 * @param taken actual branch direction
 * @return whether entry in the table matches actual branch direction
 */
bool predictor2bit(int * table, int index, char taken) {
    bool correct = 0;
    int state = table[index] % 4;
    if (taken == 'T') {
        switch (state) {
            case 0:
            case 1:
                table[index]++;
                break;
            case 2:
                table[index]++;
            case 3:
                correct = 1;
                break;
        }
    } else {
        switch (state) {
            case 0:
                correct = 1;
                break;
            case 1:
                correct = 1;
            case 2:
            case 3:
                table[index]--;
                break;
        }
    }
    return correct;
}

/**
 * Tournament Predictor
 * Update the entry in selector base on the actual branch direction
 * (11 - Strongly Prefer 2 bits Bimodal, 10 - Weakly Prefer 2 bits Bimodal, 
 *  01 - Weakly Prefer Gshare, 00 - Strongly Prefer Gshare)
 *
 * @param entry entry of the selector
 * @param bPred prediction made by 2 bits Bimodal
 * @param gPred prediction made by Gshare
 * @return whether entry in the prefered table matches actual branch direction
 */
bool tournamentPred(int &entry, int bPred, int gPred, char taken) {
    bool correct = 0;
    int state = entry & 12;
    if (taken == 'T') {
        switch (state) {
            case 0:
            case 4:
                if (gPred >= 2) {
                    if (bPred <= 1) {
                        entry = entry % 4;
                    }
                    correct = 1;
                } else if (bPred > 1) {
                    entry += 4;
                }
                break;
            case 8:
            case 12:
                if (bPred >= 2) {
                    if (gPred <= 1) {
                        entry = 12 + entry % 4;
                    }
                    correct = 1;
                } else if (gPred >= 2) {
                    entry -= 4;
                }
                break;
        } 
    } else {
        switch (state) {
            case 0:
            case 4:
                if (gPred <= 1) {
                    if (bPred >= 2) {
                        entry = entry % 4;
                    }
                    correct = 1;
                } else if (bPred <= 1) {
                    entry += 4;
                }
                break;
            case 8:
            case 12:
                if (bPred <= 1) {
                    if (gPred >= 2) {
                        entry = 12 + entry % 4;
                    }
                    correct = 1;
                } else if (bPred > 1 && gPred <= 1) {
                    entry -= 4;
                }
                break;
        }
    }
    return correct;
}

~my_predictor_1()
{
	delete [] table1024;
    for (int i = 0; i < 12; i++) {
        delete [] gtable[i];
    }  
	delete [] gtable;
}
};


