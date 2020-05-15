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
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

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
int *gresult, ghistory, gmask;
int *lresult, lmask, selector;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  switch (bpType) {
    case GSHARE:
      gresult = calloc(1 << ghistoryBits, sizeof(int));
      ghistory = 0;
      gmask = 0;
      for(int i = 0; i < ghistoryBits; i++) {
        gmask = (gmask << 1) + 1;
      }
      break;
    case TOURNAMENT:
      selector = 0;
      gresult = calloc(1 << ghistoryBits, sizeof(int));
      ghistory = 0;
      gmask = 0;
      for(int i = 0; i < ghistoryBits; i++) {
        gmask = (gmask << 1) + 1;
      }
      lresult = calloc(1 << pcIndexBits, sizeof(int));
      lmask = 0;
      for(int i = 0; i < pcIndexBits; i++) {
        lmask = (lmask << 1) + 1;
      }
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
  int gpred, lpred;
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
      gpred = gresult[ghistory & gmask] > WN;
      lpred = lresult[pc & lmask] > WN;
      if(selector < 2 && lpred || selector > 1 && gpred) {
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
    int correctness = 0;
    case GSHARE:
      if(outcome && gresult[(pc ^ ghistory) & gmask] < ST) {
        gresult[(pc ^ ghistory) & gmask]++;
      }
      if(!outcome && gresult[(pc ^ ghistory) & gmask] > SN) {
        gresult[(pc ^ ghistory) & gmask]--;
      }
      ghistory = (ghistory << 1) + (outcome? 1 : 0);
      break;
    case TOURNAMENT:
      // update selector
      if((outcome == 0) == gresult[ghistory & gmask] < WT) {
        correctness += 1;
      }
      if((outcome == 0) == lresult[pc & lmask] < WT) {
        correctness += 2;
      }
      switch (selector) {
        case 0:
          if(correctness == 1) {selector++;}
          break;
        case 1:
          if(correctness == 1) {selector++;}
          else if(correctness == 2) {selector--;}
          break;
        case 2:
          if(correctness == 1) {selector++;}
          else if(correctness == 2) {selector--;}
          break;
        case 3:
          if(correctness == 2) {selector--;}
          break;
      }
      // update global result
      if(outcome && gresult[ghistory & gmask] < ST) {
        gresult[ghistory & gmask]++;
      }
      if(!outcome && gresult[ghistory & gmask] > SN) {
        gresult[ghistory & gmask]--;
      }
      ghistory = (ghistory << 1) + (outcome? 1 : 0);
      // update local result
      if(outcome && lresult[pc & lmask] < ST) {
        lresult[pc & lmask]++;
      }
      if(!outcome && lresult[pc & lmask] > SN) {
        lresult[pc & lmask]--;
      }
      break;
    case CUSTOM:
    default:
      break;
  }
}
