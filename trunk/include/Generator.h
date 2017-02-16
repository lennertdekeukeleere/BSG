#ifndef GENERATOR
#define GENERATOR

#include <string>
#include <iostream>
#include <vector>

class Generator {
 private:
  /// type of beta decay, negative or positive
  enum BetaType { BETA_PLUS = -1, BETA_MINUS = 1 };
  enum DecayType { FERMI, GAMOW_TELLER, MIXED };
  double aNeg[7];
  double aPos[7];
  double exPars[9];
  double W0;     ///< total endpoint energy in electron mass
  double R;   ///< nuclear radius in natural units
  double A;  ///< Mass number \f$ A \f$
  double Z;  ///< number of protons of the daughter nucleus \f$ Z \f$
  double mixingRatio;
  double hoFit;
  double daughterBeta2;
  double daughterBeta4;
  double motherBeta2;
  double motherBeta4;
  int motherSpinParity;
  int daughterSpinParity;
  BetaType fBetaType;  ///< type of beta decay, negative or positive
  DecayType fDecayType;

  /// recoil correction form factors
  double fb, fc1, fd;
  double gA, gP, gM;

  double CalculateWeakMagnetism();
  double CalculateInducedTensor();
  double CalculateInducedPseudoscalar();
  void CalculateMatrixElements();

  void InitializeL0Constants();
  void LoadExchangeParameters();

 public:
  Generator();
  /// nothing is created in the heap, therefore we have an empty destructor
  ~Generator(){};

  void CalculateSpectrum();

};

#endif  // SPECTRUM_H