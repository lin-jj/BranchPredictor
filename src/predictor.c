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

int32_t *gresult, ghistory, gmask;
int32_t *lhistoryTable, pcmask, *lresult, lmask, *selector;
uint64_t h = 0, mask40 = 0xFFFFFFFFFF, mask10 = 0x3FF, mask8 = 0xFF;
int8_t bank0[4096] = {6};
int16_t bank[4][1024] = {0x600}, idx, tag, idxs[4], tags[4]; 

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
      srand(5432);
      pcmask = (1 << 12) - 1;
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
      idx = (pc ^ h ^ h >> 10 ^ h >> 20 ^ h >> 30 ^ h >> 40) & mask10;
      tag = (pc ^ h ^ h >> 8 ^ h >> 16 ^ h >> 24 ^ h >> 32) & mask8;
      for(int x = 3; x >= 0; x--) {
        if(tag == (bank[x][idx] >> 1 & mask8))
          return bank[x][idx] >> 11;
        idx ^= (h >> (x + 1) * 10) & mask10;
        tag ^= (h >> (x + 1) * 8) & mask8;
      }
      return bank0[pc & pcmask] >> 3;
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
  int gpred, lpred, lhistory, correctness = 0, x;
  switch (bpType) {
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
      lpred = lresult[lhistory] > WN;
      gpred = gresult[ghistory] > WN;
      if(lpred != gpred) {
        if(gpred == outcome) {
          if(selector[ghistory] < 3) {
            selector[ghistory]++;
          }
        }
        else {
          if(selector[ghistory] > 0) {
            selector[ghistory]--;
          }
        }
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
      // update 3-bit counter
      idxs[3] = (pc ^ h ^ h >> 10 ^ h >> 20 ^ h >> 30 ^ h >> 40) & mask10;
      tags[3] = (pc ^ h ^ h >> 8 ^ h >> 16 ^ h >> 24 ^ h >> 32) & mask8;
      idxs[2] = (pc ^ h ^ h >> 10 ^ h >> 20 ^ h >> 30) & mask10;
      tags[2] = (pc ^ h ^ h >> 8 ^ h >> 16 ^ h >> 24) & mask8;
      idxs[1] = (pc ^ h ^ h >> 10 ^ h >> 20) & mask10;
      tags[1] = (pc ^ h ^ h >> 8 ^ h >> 16) & mask8;
      idxs[0] = (pc ^ h ^ h >> 10) & mask10;
      tags[0] = (pc ^ h ^ h >> 8) & mask8;
      int flag = 0;
      for(x = 3; x >= 0; x--) {
        if(tags[x] == (bank[x][idxs[x]] >> 1 & mask8)) {
          lpred = bank[x][idxs[x]] >> 9;
          if(outcome == TAKEN && lpred < 7)
            bank[x][idxs[x]] += 0x200;
          else if(outcome == NOTTAKEN && lpred > 0)
            bank[x][idxs[x]] -= 0x200;
          flag = 1;
          break;
        }
      }
      if(flag == 0) {
        lpred = bank0[pc & pcmask] >> 1;
        if(outcome == TAKEN && lpred < 7)
          bank0[pc & pcmask] += 2;
        else if(outcome == NOTTAKEN && lpred > 0)
          bank0[pc & pcmask] -= 2;
      }
      // allocate new entries
      flag = 0;
      if(x < 3 && outcome != lpred > 3) {
        for(int i = 3; i > x; i--){
          if(bank[i][idxs[i]] % 2 == 0) {
            bank[i][idxs[i]] = outcome?0x800:0x600 + tags[i] * 2;
            flag = 1;
          }
        }
        if(flag == 0) {
          int r = rand() % 4;
          bank[r][idxs[r]] = outcome?0x800:0x600 + tags[r] * 2;
        }
      }
      // update u and m
      if(lpred != bank0[pc & pcmask] >> 1) {
        if(outcome == lpred > 3) {
          if(x >= 0)
            bank[x][idxs[x]] |= 0x0001;
          bank0[pc & pcmask] |= 0x0001;
        }
        else {
          if(x >= 0)
            bank[x][idxs[x]] &= 0xFFFE;
          bank0[pc & pcmask] &= 0xFFFE;
        }
      }
      h = (h << 1) + (outcome == TAKEN);
    default:
      break;
  }
}