//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Junjie Lin";
const char *studentID   = "A53301275";
const char *email       = "j5lin@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

int *gresult, ghistory, gmask;
int *lhistoryTable, pcmask, *lresult, lmask, *selector;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  switch (bpType) {
    case TOURNAMENT:
      selector = malloc((1 << ghistoryBits) * sizeof(int));
      for(int i = 0; i < (1 << ghistoryBits); i++) {
        selector[i] = 2;
      }
      lhistoryTable = calloc(1 << pcIndexBits, sizeof(int));
      pcmask = (1 << pcIndexBits) - 1;
      lresult = calloc(1 << lhistoryBits, sizeof(int));
      lmask = (1 << lhistoryBits) - 1;
    case GSHARE:
      ghistory = 0;
      gresult = calloc(1 << ghistoryBits, sizeof(int));
      gmask = (1 << ghistoryBits) - 1;
      break;
    case CUSTOM:
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType
  int gpred, lpred, lhistory;
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      if(gresult[(pc ^ ghistory) & gmask] > WN) {
        return TAKEN;
      }
      else {
        return NOTTAKEN;
      }
    case TOURNAMENT:
      lhistory = lhistoryTable[pc & pcmask];
      lpred = lresult[lhistory] > WN;
      gpred = gresult[ghistory] > WN;
      if(selector[ghistory] < 2 && lpred || selector[ghistory] > 1 && gpred) {
        return TAKEN;
      }
      else {
        return NOTTAKEN;
      }
    case CUSTOM:
    default:
      break;
  }
  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  switch (bpType) {
    int lhistory, correctness = 0;
    case GSHARE:
      if(outcome == TAKEN && gresult[(pc ^ ghistory) & gmask] < ST) {
        gresult[(pc ^ ghistory) & gmask]++;
      }
      if(outcome == NOTTAKEN && gresult[(pc ^ ghistory) & gmask] > SN) {
        gresult[(pc ^ ghistory) & gmask]--;
      }
      ghistory = ((ghistory << 1) + (outcome == TAKEN)) & gmask;
      break;
    case TOURNAMENT:
      lhistory = lhistoryTable[pc & pcmask];
      // update selector
      if((outcome == TAKEN) == (lresult[lhistory] > WN)) {
        correctness += 2;
      }
      if((outcome == TAKEN) == (gresult[ghistory] > WN)) {
        correctness += 1;
      }
      if(correctness == 1 && selector[ghistory] < 3) {
        selector[ghistory]++;
      }
      else if(correctness == 2 && selector[ghistory] > 0) {
        selector[ghistory]--;
      }
      // update global result
      if(outcome == TAKEN && gresult[ghistory] < ST) {
        gresult[ghistory]++;
      }
      if(outcome == NOTTAKEN && gresult[ghistory] > SN) {
        gresult[ghistory]--;
      }
      ghistory = ((ghistory << 1) + (outcome == TAKEN)) & gmask;
      // update local result
      if(outcome == TAKEN && lresult[lhistory] < ST) {
        lresult[lhistory]++;
      }
      if(outcome == NOTTAKEN && lresult[lhistory] > SN) {
        lresult[lhistory]--;
      }
      lhistoryTable[pc & pcmask] = ((lhistoryTable[pc & pcmask] << 1) + (outcome == TAKEN)) & lmask;
      break;
    case CUSTOM:
    default:
      break;
  }
}
