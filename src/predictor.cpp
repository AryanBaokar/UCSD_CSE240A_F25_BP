//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "ARYAN_BAOKAR";
const char *studentID = "A69040621";
const char *email = "abaokar@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 15; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

// Alpha 21264 Tournament Predictor Config
uint8_t a21264_localHistoryBits = 10;   // Number of bits for local history of each branch
uint8_t a21264_globalHistoryBits  = 12; // Number of bits for global history

// Custom Predictor Config
uint8_t custom_localHistoryBits = 12;   // Number of bits for local history of each branch
uint8_t custom_globalHistoryBits  = 12; // Number of bits for global history

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// Alpha 21264 Tournament predictor
uint16_t a21264_globalHistory;   // Actual history of global
uint16_t* a21264_localHistory;   // Actual history of each branch, use pc to index this
uint8_t* a21264_ght;             // Table containing predictions for global history
uint8_t* a21264_localPredTable;  // Prediction to be made for each type of local history
uint8_t* a21264_chooser;         // Chooser function

// Custom predictor
uint16_t  custom_globalHistory;   // Actual history of global
uint16_t* custom_localHistory;   // Actual history of each branch, use pc to index this
uint8_t*  custom_ght;             // Table containing predictions for global history
uint8_t*  custom_localPredTable;  // Prediction to be made for each type of local history
uint8_t*  custom_chooser;         // Chooser function

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}

/***********************************************************************************************************************************/
// Alpha 21264 Tournament Predictor functions
void a21264_init()
{
  uint16_t a21264_ghtEntries  = 1 << a21264_globalHistoryBits;
  a21264_ght = (uint8_t *)malloc(a21264_ghtEntries * sizeof(uint8_t));

  for(int i = 0; i < a21264_ghtEntries; i++)
  {
    a21264_ght[i] = WN;
  }

  a21264_globalHistory = 0;

  uint16_t a21264_lhtEntries = 1 << a21264_localHistoryBits;
  a21264_localHistory = (uint16_t *)malloc(a21264_lhtEntries * sizeof(uint16_t));
  a21264_localPredTable = (uint8_t *)malloc(a21264_lhtEntries * sizeof(uint8_t));

  for(int i = 0; i < a21264_lhtEntries; i++)
  {
    a21264_localPredTable[i] = VWNT;
  }

  for(int i = 0; i < a21264_lhtEntries; i++)
  {
    a21264_localHistory[i] = 0xAAAA;
  }

  a21264_chooser = (uint8_t *)malloc(a21264_ghtEntries * sizeof(uint8_t));

  for(int i = 0; i < a21264_ghtEntries; i++)
  {
    a21264_chooser[i] = 1;
  }

}

uint8_t a21264_predictGlobal()
{
  uint16_t a21264_ghtEntries  = 1 << a21264_globalHistoryBits;
  uint16_t index = a21264_globalHistory & (a21264_ghtEntries - 1);
  
  switch (a21264_ght[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in A21264 BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t a21264_predictLocal(uint32_t pc)
{
  uint32_t a21264_lhtentries = 1 << a21264_localHistoryBits;
  uint32_t pc_lower_bits = (pc & (a21264_lhtentries - 1));
  uint16_t history = a21264_localHistory[pc_lower_bits];
  uint32_t index = (history & (a21264_lhtentries - 1));

  switch(a21264_localPredTable[index])
  {
    case VSNT:
      return NOTTAKEN;
    case SNT:
      return NOTTAKEN;
    case WNT:
      return NOTTAKEN;
    case VWNT:
      return NOTTAKEN;
    case VWT:
      return TAKEN;
    case WTT:
      return TAKEN;
    case STT:
      return TAKEN;
    case WST:
      return TAKEN;
    default:
      return NOTTAKEN;
  }
}

uint8_t a21264_predict(uint32_t pc)
{
  uint8_t globalPred = a21264_predictGlobal();
  uint8_t localPred = a21264_predictLocal(pc);

  uint16_t a21264_ghtEntries  = 1 << a21264_globalHistoryBits;
  uint16_t index = a21264_globalHistory & (a21264_ghtEntries - 1);

  uint8_t finalPred;

  if(a21264_chooser[index] >= 2)
  {
    finalPred = globalPred;
  }
  else
  {
    finalPred = localPred;
  }

  return finalPred;
}

uint8_t a21264_trainGlobal(uint8_t outcome)
{
  // Training Global Prediction
  uint16_t a21264_ghtEntries  = 1 << a21264_globalHistoryBits;
  uint16_t index = a21264_globalHistory & (a21264_ghtEntries - 1);
  uint8_t retVal;

  // Update state of entry in bht based on outcome
  switch (a21264_ght[index])
  {
  case WN:
  {
    a21264_ght[index] = (outcome == TAKEN) ? WT : SN;
    retVal = NOTTAKEN;
    break;
  }
  case SN:
  {
    a21264_ght[index] = (outcome == TAKEN) ? WN : SN;
    retVal = NOTTAKEN;
    break;
  }
  case WT:
  {
    a21264_ght[index] = (outcome == TAKEN) ? ST : WN;
    retVal = TAKEN;
    break;
  }
  case ST:
  {
    a21264_ght[index] = (outcome == TAKEN) ? ST : WT;
    retVal = TAKEN;
    break;
  }
  default:
    printf("Warning: Undefined state of entry in A21264 GHT!\n");
    break;
  }

  a21264_globalHistory = ((a21264_globalHistory << 1) | outcome);
  return retVal;
}

uint8_t a21264_trainLocal(uint32_t pc , uint8_t outcome)
{
  uint32_t a21264_lhtentries = 1 << a21264_localHistoryBits;
  uint32_t pc_lower_bits = (pc & (a21264_lhtentries - 1));
  uint32_t history = a21264_localHistory[pc_lower_bits];
  uint32_t index = (history & (a21264_lhtentries - 1));

  uint8_t retVal;

  switch (a21264_localPredTable[index])
  {
  case VSNT:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ? SNT : VSNT;
    retVal = NOTTAKEN;
    break;
  } 
  case SNT:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ? WNT : VSNT;
    retVal = NOTTAKEN;
    break;
  }
  case WNT:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ?  VWNT : SNT;
    retVal = NOTTAKEN;
    break;
  }
  case VWNT:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ? VWT : WNT;
    retVal = NOTTAKEN;
    break;
  }
  case VWT:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ? WTT : VWNT;
    retVal = TAKEN;
    break;
  }
  case WTT:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ? STT : VWT;
    retVal = TAKEN;
    break;
  }
  case STT:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ? WST : WTT;
    retVal = TAKEN;
    break;
  }
  case WST:
  {
    a21264_localPredTable[index] = (outcome == TAKEN) ? WST : STT;
    retVal = TAKEN;
    break;
  }
    
  default:
    printf("Warning: Undefined state of entry in A21264 LHT!\n");
    break;
  }

  a21264_localHistory[pc_lower_bits] = ((a21264_localHistory[pc_lower_bits] << 1) | outcome);

  return retVal;
}

void a21264_train(uint32_t pc, uint8_t outcome)
{
  uint16_t a21264_ghtEntries  = 1 << a21264_globalHistoryBits;
  uint16_t index = a21264_globalHistory & (a21264_ghtEntries - 1);

  uint8_t globTrained = a21264_trainGlobal(outcome);
  uint8_t localTrained = a21264_trainLocal(pc, outcome);

  if(globTrained == localTrained)
  {
    // Do nothing
  }
  else if(globTrained == outcome)
  {
    if(a21264_chooser[index] != 3)
    {
      a21264_chooser[index]++;
    }
  }
  else if(localTrained == outcome)
  {
    if(a21264_chooser[index] != 0)
    {
      a21264_chooser[index]--;
    }
  }
}

void a21264_free()
{
  free(a21264_ght);
  free(a21264_localHistory);
  free(a21264_localPredTable);
  free(a21264_chooser);
}

void custom_init()
{
  uint16_t custom_ghtEntries  = 1 << custom_globalHistoryBits;
  custom_ght = (uint8_t *)malloc(custom_ghtEntries * sizeof(uint8_t));

  for(int i = 0; i < custom_ghtEntries; i++)
  {
    custom_ght[i] = WN;
  }

  custom_globalHistory = 0;

  uint16_t custom_lhtEntries = 1 << custom_localHistoryBits;
  custom_localHistory = (uint16_t *)malloc(custom_lhtEntries * sizeof(uint16_t));
  custom_localPredTable = (uint8_t *)malloc(custom_lhtEntries * sizeof(uint8_t));

  for(int i = 0; i < custom_lhtEntries; i++)
  {
    custom_localPredTable[i] = VWNT;
  }

  for(int i = 0; i < custom_lhtEntries; i++)
  {
    custom_localHistory[i] = 0xAAAA;
  }

  custom_chooser = (uint8_t *)malloc(custom_ghtEntries * sizeof(uint8_t));

  for(int i = 0; i < custom_ghtEntries; i++)
  {
    custom_chooser[i] = 1;
  }

}

uint8_t custom_predictGlobal()
{
  uint16_t custom_ghtEntries  = 1 << custom_globalHistoryBits;
  uint16_t index = custom_globalHistory & (custom_ghtEntries - 1);
  
  switch (custom_ght[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in custom BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t custom_predictLocal(uint32_t pc)
{
  uint32_t custom_lhtentries = 1 << custom_localHistoryBits;
  uint32_t pc_lower_bits = (pc & (custom_lhtentries - 1));
  uint16_t history = custom_localHistory[pc_lower_bits];
  uint32_t index = (history & (custom_lhtentries - 1));

  switch(custom_localPredTable[index])
  {
    case VSNT:
      return NOTTAKEN;
    case SNT:
      return NOTTAKEN;
    case WNT:
      return NOTTAKEN;
    case VWNT:
      return NOTTAKEN;
    case VWT:
      return TAKEN;
    case WTT:
      return TAKEN;
    case STT:
      return TAKEN;
    case WST:
      return TAKEN;
    default:
      return NOTTAKEN;
  }
}

uint8_t custom_predict(uint32_t pc)
{
  uint8_t globalPred = custom_predictGlobal();
  uint8_t localPred = custom_predictLocal(pc);

  uint16_t custom_ghtEntries  = 1 << custom_globalHistoryBits;
  uint16_t index = custom_globalHistory & (custom_ghtEntries - 1);

  uint8_t finalPred;

  if(custom_chooser[index] >= 2)
  {
    finalPred = globalPred;
  }
  else
  {
    finalPred = localPred;
  }

  return finalPred;
}

uint8_t custom_trainGlobal(uint8_t outcome)
{
  // Training Global Prediction
  uint16_t custom_ghtEntries  = 1 << custom_globalHistoryBits;
  uint16_t index = custom_globalHistory & (custom_ghtEntries - 1);
  uint8_t retVal;

  // Update state of entry in bht based on outcome
  switch (custom_ght[index])
  {
  case WN:
  {
    custom_ght[index] = (outcome == TAKEN) ? WT : SN;
    retVal = NOTTAKEN;
    break;
  }
  case SN:
  {
    custom_ght[index] = (outcome == TAKEN) ? WN : SN;
    retVal = NOTTAKEN;
    break;
  }
  case WT:
  {
    custom_ght[index] = (outcome == TAKEN) ? ST : WN;
    retVal = TAKEN;
    break;
  }
  case ST:
  {
    custom_ght[index] = (outcome == TAKEN) ? ST : WT;
    retVal = TAKEN;
    break;
  }
  default:
    printf("Warning: Undefined state of entry in custom GHT!\n");
    break;
  }

  custom_globalHistory = ((custom_globalHistory << 1) | outcome);
  return retVal;
}

uint8_t custom_trainLocal(uint32_t pc , uint8_t outcome)
{
  uint32_t custom_lhtentries = 1 << custom_localHistoryBits;
  uint32_t pc_lower_bits = (pc & (custom_lhtentries - 1));
  uint32_t history = custom_localHistory[pc_lower_bits];
  uint32_t index = (history & (custom_lhtentries - 1));

  uint8_t retVal;

  switch (custom_localPredTable[index])
  {
  case VSNT:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ? SNT : VSNT;
    retVal = NOTTAKEN;
    break;
  } 
  case SNT:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ? WNT : VSNT;
    retVal = NOTTAKEN;
    break;
  }
  case WNT:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ?  VWNT : SNT;
    retVal = NOTTAKEN;
    break;
  }
  case VWNT:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ? VWT : WNT;
    retVal = NOTTAKEN;
    break;
  }
  case VWT:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ? WTT : VWNT;
    retVal = TAKEN;
    break;
  }
  case WTT:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ? STT : VWT;
    retVal = TAKEN;
    break;
  }
  case STT:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ? WST : WTT;
    retVal = TAKEN;
    break;
  }
  case WST:
  {
    custom_localPredTable[index] = (outcome == TAKEN) ? WST : STT;
    retVal = TAKEN;
    break;
  }
    
  default:
    printf("Warning: Undefined state of entry in custom LHT!\n");
    break;
  }

  custom_localHistory[pc_lower_bits] = ((custom_localHistory[pc_lower_bits] << 1) | outcome);

  return retVal;
}

void custom_train(uint32_t pc, uint8_t outcome)
{
  uint16_t custom_ghtEntries  = 1 << custom_globalHistoryBits;
  uint16_t index = custom_globalHistory & (custom_ghtEntries - 1);

  //uint8_t globTrained = custom_trainGlobal(outcome);
  uint8_t globTrained = custom_trainGlobal(outcome);
  uint8_t localTrained = custom_trainLocal(pc, outcome);

  if(globTrained == localTrained)
  {
    // Do nothing
  }
  else if(globTrained == outcome)
  {
    if(custom_chooser[index] != 3)
    {
      custom_chooser[index]++;
    }
  }
  else if(localTrained == outcome)
  {
    if(custom_chooser[index] != 0)
    {
      custom_chooser[index]--;
    }
  }
}

void custom_free()
{
  free(custom_ght);
  free(custom_localHistory);
  free(custom_localPredTable);
  free(custom_chooser);
}



void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    a21264_init();
    break;
  case CUSTOM:
    custom_init();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return a21264_predict(pc);
  case CUSTOM:
    return custom_predict(pc);
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

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return a21264_train(pc, outcome);
    case CUSTOM:
      return custom_train(pc, outcome);
    default:
      break;
    }
  }
}
