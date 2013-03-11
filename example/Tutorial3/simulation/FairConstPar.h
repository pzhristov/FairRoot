/** FairConstPar.h
 ** @author M.Al-Turany
 ** @since 30.01.2007
 ** @version 1.0
 **
 ** Parameter set for the region between Solenoid and dipole. For the runtime database.
 **/


#ifndef FairConstPAR_H
#define FairConstPAR_H 1
#include "FairMapPar.h"
class FairParamList;

class FairConstPar : public FairMapPar
{

  public:


    /** Standard constructor  **/
    FairConstPar(const char* name, const char* title, const char* context);

    /** default constructor  **/
    FairConstPar();

    /** Destructor **/
    ~FairConstPar();

    void putParams(FairParamList* list);


    /** Get parameters **/
    Bool_t getParams(FairParamList* list);


    /** Set parameters from FairField  **/
    void SetParameters(FairField* field);


    Double_t GetBx()        const { return fBx; }
    Double_t GetBy()        const { return fBy; }
    Double_t GetBz()        const { return fBz; }

  protected:

    /** Field values in [kG] **/
    Double_t fBx, fBy, fBz;

    ClassDef(FairConstPar,1);

};


#endif