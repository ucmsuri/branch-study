// Combining Predictors (gshare + bimodal)
// cost = 	gShare	+ bimodal	+ choice
//		2^13x2 	+ 2^12x2 	+ 2^12x2
//		16384 	+ 8192	  	+ 8192
//		32768	< 33024(limit)

/*//gShare
#define gs_PHT_SIZE 	8192	//2^13
#define gs_PHT_MASK		0x1FFF  //13 bit address mask

//bimodal
#define bi_PHT_SIZE		4096 	//2^12
#define bi_PHT_MASK 	0xFFF   //13 bit address mask

//choice
#define choice_SIZE		4096
#define choice_MASK		0xFFF	
*/

//gShare
#define gs_PHT_SIZE 	16384	//2^13
#define gs_PHT_MASK	0x3FFF  //13 bit address mask

//bimodal
#define bi_PHT_SIZE	8192 	//2^12
#define bi_PHT_MASK 	0x1FFF   //13 bit address mask

//choice
#define choice_SIZE	8192
#define choice_MASK	0x1FFF	


class my_predictor_2
{
public:
int gs_PHT[gs_PHT_SIZE];
int gs_HISTORY;					//the history shift register
int gs_index;
int bi_PHT[bi_PHT_SIZE];
int bi_index;
int CHOICE[choice_SIZE];

bool gsPredict;			//prediction from gShare
bool biPredict;			//prediction from bimodal

my_predictor_2(void)
{
bool gsPredict = false;			//prediction from gShare
bool biPredict = false;			//prediction from bimodal
init_predictor();
}

void init_predictor ()
{
	//all values initiated to 0
	//init gShare
	for(int i = 0; i < gs_PHT_SIZE; i++)
		gs_PHT[i] = 3;
	gs_HISTORY = 0;

	//init bimodal and gshare
	for(int i = 0; i < bi_PHT_SIZE; i++){
		bi_PHT[i] = 3;
		CHOICE[i] = 3;
	}
}

bool make_prediction (unsigned int pc)
{
	gsPredict = false;
	biPredict = false;

	//gShare
	gs_index = gs_HISTORY ^ (pc & gs_PHT_MASK);
	if(gs_PHT[gs_index] > 1)		//for values 10 and 11 return true
		gsPredict = true;

	//biModal
	bi_index = pc & bi_PHT_MASK;
	if(bi_PHT[bi_index] > 1)		//for values 10 and 11 return true
		biPredict = true;

	//CHOICE
	if(CHOICE[gs_HISTORY & choice_MASK] > 1)
		return gsPredict;
	else
		return biPredict;
}

void train_predictor (unsigned int pc, bool outcome)
{
	//train gShare
	if(outcome == true && gs_PHT[gs_index] < 3)		//saturate towards T 
		gs_PHT[gs_index]++;
	if(outcome == false && gs_PHT[gs_index] > 0)	//saturate towards NT
		gs_PHT[gs_index]--;

	//train biModal
	if(outcome == true && bi_PHT[bi_index] < 3)		//saturate towards T
		bi_PHT[bi_index]++;
	if(outcome == false && bi_PHT[bi_index] > 0)	//saturate towards NT
		bi_PHT[bi_index]--;


	//Train CHOICE only when the two predictors differ
	if(biPredict != gsPredict){
		if(gsPredict == outcome && CHOICE[gs_HISTORY & choice_MASK] < 3){
			//train chooser to choose GLOBAL
			CHOICE[gs_HISTORY & choice_MASK]++;
		} else if(biPredict == outcome && CHOICE[gs_HISTORY & choice_MASK] > 0){
			//train chooser to choose LOCAL
			CHOICE[gs_HISTORY & choice_MASK]--;
		}
	}

	//updating the HISTORY register
	gs_HISTORY = gs_HISTORY << 1;
	gs_HISTORY = gs_HISTORY | int(outcome);
	gs_HISTORY = gs_HISTORY & gs_PHT_MASK;

}
};
