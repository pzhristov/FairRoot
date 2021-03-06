/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * PixelFindHits.cxx
 *
 *  Created on: 18.02.2016
 *      Author: R. Karabowicz
 */

#include "PixelFindHits.h"

// Includes from base
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"
#include "FairLink.h"
#include "FairLogger.h"

// Includes from ROOT
#include "TClonesArray.h"
#include "TObjArray.h"
#include "TMath.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TGeoBBox.h"

#include "PixelDigi.h"
#include "PixelHit.h"
#include "PixelDigiPar.h"

#include <map>

 // 
#include "FairParRootFileIo.h"
#include "FairParAsciiFileIo.h"
#include "FairGeoParSet.h"

using std::pair;
using std::map;



// -----   Default constructor   ------------------------------------------
PixelFindHits::PixelFindHits()
  : PixelFindHits("Pixel Hit Finder", 0)
{
}
// -------------------------------------------------------------------------



// -----   Standard constructor   ------------------------------------------
PixelFindHits::PixelFindHits(Int_t iVerbose) 
  : PixelFindHits("Pixel Hit Finder", iVerbose)
{
}
// -------------------------------------------------------------------------



// -----   Constructor with name   -----------------------------------------
PixelFindHits::PixelFindHits(const char* name, Int_t iVerbose) 
  : FairTask(name, iVerbose)
  , fDigiPar(NULL)
  , fGeoParSet(NULL)
  , fDigis(NULL)
  , fHits(NULL)
  , fNDigis(0)
  , fNHits(0)
  , fTNofEvents(0)
  , fTNofDigis(0) 
  , fTNofHits(0)
  , fFeCols(0)
  , fFeRows(0)
  , fMaxFEperCol(0)
  , fPitchX(0.)
  , fPitchY(0.)
{
  LOG(INFO) << "Created PixelFindHits." << FairLogger::endl;
  Reset();
}
// -------------------------------------------------------------------------



// -----   Destructor   ----------------------------------------------------
PixelFindHits::~PixelFindHits() { 
  Reset();
  delete fDigiPar;
  if ( fHits ) {
    fHits->Delete();
    delete fHits;
  }
}
// -------------------------------------------------------------------------

// -----   Public method Exec   --------------------------------------------
void PixelFindHits::Exec(Option_t* /*opt*/) {
  Reset();

  LOG(DEBUG) << "PixelFindHits::Exec() EVENT " << fTNofEvents << FairLogger::endl;

  fTNofEvents++;

  fNDigis = fDigis->GetEntriesFast();
  fTNofDigis+= fNDigis;

  for ( Int_t iDigi = 0 ; iDigi < fNDigis ; iDigi++ ) {
    PixelDigi* currentDigi = static_cast<PixelDigi*>(fDigis->At(iDigi));

    Int_t detId = currentDigi->GetDetectorID();    
    TString nodeName = Form("/cave/Pixel%d_%d",detId/256,detId%256);
    
    gGeoManager->cd(nodeName.Data());
    TGeoNode* curNode = gGeoManager->GetCurrentNode();

//    TGeoMatrix* matrix = curNode->GetMatrix();

    TGeoVolume* actVolume = gGeoManager->GetCurrentVolume();
    TGeoBBox* actBox = static_cast<TGeoBBox*>(actVolume->GetShape());

    Int_t feId = currentDigi->GetFeID();
    Int_t col  = currentDigi->GetCol();
    Int_t row  = currentDigi->GetRow();

    Double_t locPosCalc[3];
    locPosCalc[0] = ( ((feId-1)/fMaxFEperCol)*fFeCols + col + 0.5 )*fPitchX;
    locPosCalc[1] = ( ((feId-1)%fMaxFEperCol)*fFeRows + row + 0.5 )*fPitchY;
    locPosCalc[2] = 0.;
    
    locPosCalc[0] -= actBox->GetDX();
    locPosCalc[1] -= actBox->GetDY();

    Double_t globPos[3];

    curNode->LocalToMaster(locPosCalc,globPos);

    LOG(DEBUG) << "HIT   ON " << detId << " POSITION:  " << locPosCalc[0] << " / " << locPosCalc[1] << FairLogger::endl;
    LOG(DEBUG) << "GLOB HIT " << detId << " POSITION:  " << globPos[0] << " / " << globPos[1] << " / " << globPos[2] << FairLogger::endl;

    TVector3 pos   (globPos[0],globPos[1],globPos[2]);
    TVector3 posErr(fPitchX/TMath::Sqrt(12.),fPitchY/TMath::Sqrt(12.),actBox->GetDZ());

    new ((*fHits)[fNHits]) PixelHit(detId,currentDigi->GetIndex(),pos,posErr);

    fNHits++;
  }

  fTNofHits += fNHits;
}
// -------------------------------------------------------------------------

// -----   Private method SetParContainers   -------------------------------
void PixelFindHits::SetParContainers() {
  
  // Get run and runtime database
  FairRun* run = FairRun::Instance();
  if ( ! run ) LOG(FATAL) << "No analysis run" << FairLogger::endl;

  FairRuntimeDb* db = run->GetRuntimeDb();
  if ( ! db ) LOG(FATAL) << "No runtime database" << FairLogger::endl;

  // Get GEM digitisation parameter container
  fDigiPar = static_cast<PixelDigiPar*>(db->getContainer("PixelDigiParameters"));

}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
void PixelFindHits::GetParList(TList* tempList) {
  fDigiPar = new PixelDigiPar("PixelDigiParameters");
  tempList->Add(fDigiPar);
  
  return;
}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
void   PixelFindHits::InitMQ(TList* tempList) {
  LOG(INFO) << "********************************************** PixelFindHits::InitMQ()" << FairLogger::endl;
  fDigiPar = (PixelDigiPar*)tempList->FindObject("PixelDigiParameters");

  fFeCols = fDigiPar->GetFECols();
  fFeRows = fDigiPar->GetFERows();
  fMaxFEperCol = fDigiPar->GetMaxFEperCol();
  fPitchX = fDigiPar->GetXPitch();
  fPitchY = fDigiPar->GetYPitch();

  LOG(INFO) << ">> fFeCols      = " << fFeCols << FairLogger::endl;
  LOG(INFO) << ">> fFeRows      = " << fFeRows << FairLogger::endl;
  LOG(INFO) << ">> fMaxFEperCol = " << fMaxFEperCol << FairLogger::endl;
  LOG(INFO) << ">> fPitchX      = " << fPitchX << FairLogger::endl;
  LOG(INFO) << ">> fPitchY      = " << fPitchY << FairLogger::endl;

  fHits = new TClonesArray("PixelHit",10000);

  return;
}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
void   PixelFindHits::ExecMQ(TList* inputList,TList* outputList) {
  //  LOG(INFO) << "********************************************** PixelFindHits::ExecMQ(" << inputList->GetName() << "," << outputList->GetName() << "), Event " << fTNofEvents << FairLogger::endl;
  //  LOG(INFO) << "********************************************** PixelFindHits::ExecMQ(), Event " << fTNofEvents << FairLogger::endl;
  //  LOG(INFO) << "h" << FairLogger::flush;
  fDigis = (TClonesArray*) inputList->FindObject("PixelDigis");
  outputList->Add(fHits);
  Exec("");
  return;
}
// -------------------------------------------------------------------------

// -----   Private method Init   -------------------------------------------
InitStatus PixelFindHits::Init() {

  // Get input array 
  FairRootManager* ioman = FairRootManager::Instance();

  if ( ! ioman ) LOG(FATAL) << "No FairRootManager" << FairLogger::endl;
  fDigis = static_cast<TClonesArray*>(ioman->GetObject("PixelDigis"));

  if ( !fDigis ) 
    LOG(WARNING) << "PixelFindHits::Init() No input PixelDigis array!" << FairLogger::endl;

  // Register output array PixelHit
  fHits = new TClonesArray("PixelHit",10000);
  ioman->Register("PixelHits", "Pixel", fHits, kTRUE);

  LOG(INFO) << "-I- " << fName.Data() << "::Init(). Initialization succesfull." << FairLogger::endl;

  fFeCols = fDigiPar->GetFECols();
  fFeRows = fDigiPar->GetFERows();
  fMaxFEperCol = fDigiPar->GetMaxFEperCol();
  fPitchX = fDigiPar->GetXPitch();
  fPitchY = fDigiPar->GetYPitch();

  LOG(INFO) << "PixelFindHits::SetParContainers() Pixel detector with pitch size " << fPitchX << "cm x" << fPitchY << "cm" << FairLogger::endl;
  

  return kSUCCESS;

}
// -------------------------------------------------------------------------



// -----   Private method ReInit   -----------------------------------------
InitStatus PixelFindHits::ReInit() {

  return kSUCCESS;

}
// -------------------------------------------------------------------------



// -----   Private method Reset   ------------------------------------------
void PixelFindHits::Reset() {
  fNDigis = fNHits = 0;
  if ( fHits ) fHits->Clear();
}
// -------------------------------------------------------------------------

// -----   Public method Finish   ------------------------------------------
void PixelFindHits::Finish() {
  if ( fHits ) fHits->Delete();

  LOG(INFO) << "-------------------- " << fName.Data() << " : Summary ------------------------" << FairLogger::endl;
  LOG(INFO) << " Events:        " << fTNofEvents << FairLogger::endl;
  LOG(INFO) << " Digis:         " << fTNofDigis  << "    ( " << static_cast<Double_t>(fTNofDigis) /(static_cast<Double_t>(fTNofEvents)) << " per event )" << FairLogger::endl;
  LOG(INFO) << " Hits:          " << fTNofHits   << "    ( " << static_cast<Double_t>(fTNofHits  )/(static_cast<Double_t>(fTNofEvents)) << " per event )" << FairLogger::endl;
  LOG(INFO) << "---------------------------------------------------------------------" << FairLogger::endl; 
}
// -------------------------------------------------------------------------

ClassImp(PixelFindHits)

