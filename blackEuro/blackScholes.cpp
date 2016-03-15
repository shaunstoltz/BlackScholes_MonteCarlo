/*----------------------------------------------------------------------------
*
* Author:   Liang Ma (liang-ma@polito.it)
*
* NUM_RNGS is the number of RNG objects implemented
*  NUM_SIMGROUPS 	is the number simulation groups (it must be an even number)
* NUM_STEPS	is the number of time steps/partitions, which is 1 for the European option.
*
* init_array(), a static member function of RNG is defined here due to the dimension of the array.
*
* The algorithm in sampleSIM() is optimized from the mathematical and FPGA
* implementation points of view for the European option.
*
*----------------------------------------------------------------------------
*/
#include "../common/blackScholes.h"

const int blackScholes::NUM_RNGS=8;
const int blackScholes::NUM_SIMS=512;
const int blackScholes::NUM_SIMGROUPS=4;
const int blackScholes::NUM_STEPS=1;

blackScholes::blackScholes(stockData data):data(data)
{
}

void blackScholes::sampleSIM(RNG* mt_rng, data_t* pCall, data_t *pPut)
{
	const data_t ratio1=data.strikePrice*expf(-data.freeRate*data.timeT);
	const data_t ratio2=(data.freeRate-0.5f*data.volatility*data.volatility)*data.timeT;
	const data_t ratio3=data.volatility*data.volatility*data.timeT'
	const data_t ratio4=ratio2-logf(data.strikePrice/data.initPrice);
	data_t sumCall=0,sumPut=0;
	data_t pCall1[NUM_RNGS][NUM_SIMS],pCall2[NUM_RNGS][NUM_SIMS];
#pragma HLS ARRAY_PARTITION variable=pCall1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=pCall2 complete dim=1
	data_t pPut1[NUM_RNGS][NUM_SIMS],pPut2[NUM_RNGS][NUM_SIMS];
#pragma HLS ARRAY_PARTITION variable=pPut1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=pPut2 complete dim=1

	data_t num1[NUM_RNGS][NUM_SIMS],num2[NUM_RNGS][NUM_SIMS];
#pragma HLS ARRAY_PARTITION variable=num2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=num1 complete dim=1


	loop_init:for(int k=0;k<NUM_SIMS;k++) {
#pragma HLS PIPELINE
		loop_r:for(int i =0;i<NUM_RNGS;i++) {
#pragma HLS UNROLL
			pCall1[i][k]=0;
			pCall2[i][k]=0;
			pPut1[i][k]=0;
			pPut2[i][k]=0;
		}
	}
	loop_main:for(int j=0;j<NUM_SIMGROUPS;j+=2) {
		loop_share:for(uint k=0;k<NUM_SIMS;k++) {
#pragma HLS PIPELINE
			loop_parallel:for(int i=0;i<NUM_RNGS;i++) {
#pragma HLS UNROLL
				mt_rng[i].BOX_MULLER(&num1[i][k],&num2[i][k],ratio4,ratio3);
				if(num1[i][k]>0)
					pCall1[i][k]+=expf(num1[i][k])-1;
				else
					pPut1[i][k]+=1-expf(num1[i][k]);
				if(num2[i][k]>0)
					pCall2[i][k]+=expf(num2[i][k])-1;
				else
					pPut2[i][k]+=1-expf(num2[i][k]);
			}
		}
	}

	loop_sumCall:for(int k=0;k<NUM_SIMS;k++) {
		loop_sr:for(int i =0;i<NUM_RNGS;i++) {
#pragma HLS UNROLL
			sumCall+=pCall1[i][k]+pCall2[i][k];
			sumPut+=pPut1[i][k]+pPut2[i][k];
		}
	}
	*pCall=ratio1*sumCall/NUM_RNGS/NUM_SIMGROUPS/NUM_SIMS;
	*pPut =ratio1*sumPut/NUM_RNGS/NUM_SIMGROUPS/NUM_SIMS;
}

void blackScholes::simulation(data_t *pCall, data_t *pPut)
{

	RNG mt_rng[NUM_RNGS];
#pragma HLS ARRAY_PARTITION variable=mt_rng complete dim=1

	uint seeds[NUM_RNGS];

	loop_seed:for(int i=0;i<NUM_RNGS;i++) {
#pragma HLS UNROLL
		seeds[i]=i;
	}
	RNG::init_array(mt_rng,seeds,NUM_RNGS);
	sampleSIM(mt_rng,pCall,pPut);

}

