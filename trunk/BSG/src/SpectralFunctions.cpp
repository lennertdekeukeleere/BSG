#include "SpectralFunctions.h"

#include "Utilities.h"
#include "ChargeDistributions.h"
#include "Screening.h"

#include <complex>
#include <stdio.h>

// GNU Scientific Library stuff
// http://www.gnu.org/software/gsl/
#include "gsl/gsl_complex_math.h"
#include "gsl/gsl_const_mksa.h"
#include "gsl/gsl_const_num.h"
#include "gsl/gsl_sf_gamma.h"
#include "gsl/gsl_sf_result.h"
#include "gsl/gsl_sf_dilog.h"

using std::cout;
using std::endl;

double SpectralFunctions::PhaseSpace(double W, double W0, int motherSpinParity,
                                     int daughterSpinParity) {
  double result = std::sqrt(W * W - 1.) * W * std::pow(W0 - W, 2.);
  // TODO forbidden transitions
  return result;
}

double SpectralFunctions::FermiFunction(double W, int Z, double R,
                                        int betaType) {
  double gamma = std::sqrt(1. - std::pow(ALPHA * Z, 2.));
  double p = std::sqrt(W * W - 1.);
  double first = 2. * (gamma + 1.);
  // the second term will be incorporated in the fifth
  // double second = 1/std::pow(gsl_sf_gamma(2*gamma+1),2);
  double third = std::pow(2. * p * R, 2. * (gamma - 1.));
  double fourth = std::exp(betaType * M_PI * ALPHA * Z * W / p);

  // the fifth is a bit tricky
  // we use the complex gamma function from GSL
  gsl_sf_result magn;
  gsl_sf_result phase;
  gsl_sf_lngamma_complex_e(gamma, betaType * ALPHA * Z * W / p, &magn, &phase);
  // now we have what we wAt in magn.val

  // but we incorporate the second term here as well
  double fifth = std::exp(2. * (magn.val - gsl_sf_lngamma(2. * gamma + 1.)));

  double result = first * third * fourth * fifth;
  return result;
}

double SpectralFunctions::CCorrection(double W, double W0, int Z, int A,
                                      double R, int betaType, double hoFit,
                                      int decayType, double gA, double gP,
                                      double fc1, double fb, double fd,
                                      double ratioM121) {
  double AC0, AC1, ACm1, AC2;
  double VC0, VC1, VCm1, VC2;

  double F1111 = 0.757 + 0.0069 * (1 - std::exp(-hoFit / 1.008));
  double F1221 = 0.844 - 0.0182 * (1 - std::exp(-hoFit / 1.1974));
  double F1222 = 1.219 - 0.0640 * (1 - std::exp(-hoFit / 1.550));
  double F1211 = -3. / 70;

  VC0 = -std::pow(W0 * R, 2.) / 5. -
        betaType * 2. / 9. * ALPHA * Z * W0 * R * F1111 -
        std::pow(ALPHA * Z, 2.) / 3. * F1222;

  VC1 = 4. / 15. * W0 * R * R -
        betaType * 2. / 3. * ALPHA * Z * R * (F1221 - F1111 / 3.);

  VCm1 = 2. / 15. * W0 * R * R - betaType * ALPHA * Z * R / 3. * F1211;

  VC2 = -4. / 15. * R * R;

  AC0 = -1. / 3. * std::pow(ALPHA * Z, 2.) * F1222 -
        1. / 5. * (W0 * W0 - 1.) * R * R +
        betaType * 2. / 27. * ALPHA * Z * W0 * R * F1111 + 11. / 45. * R * R;

  AC1 = 4. / 9. * W0 * R * R -
        betaType * 2. / 3. * ALPHA * Z * R * (1. / 9. * F1111 + F1221);

  ACm1 = -2. / 45. * W0 * R * R + betaType * ALPHA * Z * R / 3. * F1211;

  AC2 = -4. / 9. * R * R;

  double result = 1.;

  if (decayType == FERMI) {
    result = 1. + VC0 + VC1 * W + VCm1 / W + VC2 * W * W;
  } else if (decayType == GAMOW_TELLER) {
    result = 1. + AC0 + AC1 * W + ACm1 / W + AC2 * W * W;
  }
  // TODO
  /*} else if (mixingRatio > 0) {
  }*/

  result *= CICorrection(W, W0, Z, A, R, betaType, decayType);

  if (decayType == GAMOW_TELLER) {
    double M = A * NUCLEON_MASS_KEV / ELECTRON_MASS_KEV;

    double x = std::sqrt(2.) / 3. * 10. * ratioM121 -
               gP / gA * 25. / 3. / std::pow(NUCLEON_MASS_KEV / ELECTRON_MASS_KEV * R, 2.);

    double NSC0 = -1. / 45. * R * R * x +
                  1. / 3. * W0 / M / fc1 * (-betaType * 2. * fb + fd) +
                  betaType * 2. / 5. * ALPHA * Z / M / R / fc1 *
                      (betaType * 2. * fb + fd) -
                  betaType * 2. / 35. * ALPHA * Z * W0 * R * x;

    double NSC1 = betaType * 4. / 3. * fb / M / fc1 -
                  2. / 45. * W0 * R * R * x +
                  betaType * ALPHA * Z * R * 2. / 35. * x;

    double NSCm1 = -1. / 3. / M / fc1 * (betaType * 2. * fb + fd) +
                   2. / 45. * W0 * R * R * x;

    double NSC2 = 2. / 45. * R * R * x;

    result += NSC0 + NSC1 * W + NSCm1 / W + NSC2 * W * W;
  }

  // cout << "Mixing ratio badly defined. Returning 1." << endl;
  return result;
}

double SpectralFunctions::CICorrection(double W, double W0, int Z, int A,
                                       double R, int betaType, int decayType) {
  double AC0, AC1, AC2, ACm1;
  double VC0, VC1, VC2, VCm1;

  double rms = std::sqrt(3. / 5.) * R;
  double nu = 0.;

  int nN, lN, nZ, lZ;
  std::vector<int> occNumbersN =
      utilities::GetOccupationNumbers(A - (Z - betaType));
  nN = occNumbersN[occNumbersN.size() - 1 - 3];
  lN = occNumbersN[occNumbersN.size() - 1 - 2];
  std::vector<int> occNumbersZ = utilities::GetOccupationNumbers(Z - betaType);
  nZ = occNumbersZ[occNumbersZ.size() - 1 - 3];
  lZ = occNumbersZ[occNumbersZ.size() - 1 - 2];

  // cout << "nZ: " << nZ << " lZ: " << lZ << " p: " <<
  // occNumbersZ[occNumbersZ.size() - 1] << endl;

  double w = (4 * nZ + 2 * lZ - 1) / 5.;
  double V0 = betaType * 3 * ALPHA * Z / 2. / R;
  double e = (sqr(W0 - W) + sqr(W + V0) - 1) / 6.;

  double Ap = 1.;
  double sum = 0.;
  for (int j = 0; j < occNumbersZ.size(); j += 4) {
    if (occNumbersZ[j + 1] == 0) {
      sum += occNumbersZ[j + 3];
    }
  }
  Ap = (2. * (Z - betaType) / sum - 2.) / 3.;

  // cout << "Ap: " << Ap << endl;

  return 1 - 8. / 5. * w * e * R * R / (5. * Ap + 2);

  double weakR2;
  if (A > 90) {
    nu = ChargeDistributions::CalcChargeIndepNu(rms, Z - betaType,
                                                A - (Z - betaType));
    weakR2 = ChargeDistributions::WeakIntegratedRMS(nu, nN, lN, nu, nZ, lZ);
  } else {
    nu = ChargeDistributions::CalcNu(rms, Z - betaType);
    if (betaType == BETA_MINUS) {
      weakR2 = std::pow(ChargeDistributions::GetRMSHO(nN, lN, nu), 2);
    } else {
      weakR2 = std::pow(ChargeDistributions::GetRMSHO(nZ, lZ, nu), 2);
    }
  }

  /*cout << "Nu: " << nu << endl;
  cout << "nN: " << nN << " lN: " << lN << endl;
  cout << "nZ: " << nZ << " lZ: " << lZ << endl;*/

  double delta = (weakR2 - 3. / 5. * R * R) / 6.;

  // return (1-e*weakR2)/(1-e*3./5.*R*R);
  // return 1-e*(weakR2-R*R*3/5.);

  // cout << "rmsW: " << weakR2 << " rmsCh " << 3./5.*R*R << " Delta: " << delta
  // << endl;

  AC0 = ALPHA * Z / R * (betaType * 4. / 3. * W0 - 9 / 2. * ALPHA * Z / R +
                         4 * ALPHA * Z / R * W0 * W0 * delta);
  AC1 = 40 / 9. * W0 +
        ALPHA * Z / R * (betaType * 16. / 3. * W0 * W0 * delta -
                         betaType * 22. / 3. - 22 * ALPHA * Z / R * W0 * delta);
  AC2 = -40 / 9. - betaType * 24 * ALPHA * Z / R * W0 * delta +
        27 * std::pow(ALPHA * Z / R, 2) * delta + 44. / 9. * W0 * W0 * delta;
  ACm1 = -4 / 9. * W0;

  /*cout << "W: " << W << " W0: " << W0 << endl;
  cout << "AC0: " << AC0 << " AC1: " << AC1 << " AC2: " << AC2 << " ACm1: " <<
  ACm1 << endl;*/

  VC0 = ALPHA * Z / R * (-betaType * 4 * W0 - 9. / 2. * ALPHA * Z / R +
                         4 * ALPHA * Z / R * W0 * W0 * delta);
  VC1 = 8. / 3. * W0 +
        ALPHA * Z / R * (-betaType * 16. / 3. * W0 * W0 * delta - betaType * 2 -
                         2. * ALPHA * Z / R * W0 * delta);
  VC2 = -8 / 3. + betaType * 8. / 3. * ALPHA * Z / R * W0 * delta +
        7. * std::pow(ALPHA * Z / R, 2) * delta;
  VCm1 = 4. / 3. * W0;

  double r2 = 0.75 * R + weakR2 / 2. / R;
  double r2r = 0.7 * weakR2 / R;

  /*VC0 = -ALPHA*Z*2./9.*W0*(r2r/2.-0.25*R);
  VC1 = ALPHA*Z*(4./3.*r2+4./9.*r2r-14./9.*R);
  VCm1 = ALPHA*Z*(2*r2-r2r-0.5*R);
  VC2 = 0.;*/

  if (decayType == FERMI) {
    return 1 + delta * (VC0 + VC1 * W + VC2 * W * W + VCm1 / W);
  } else if (decayType == GAMOW_TELLER) {
    return 1 + delta * (AC0 + AC1 * W + AC2 * W * W + ACm1 / W);
  } /*else if (mixingRatio > 0) {
    return 1 +
           delta * (1. / (1 + std::pow(mixingRatio, 2)) *
                        (VC0 + VC1 * W + VC2 * W * W + VCm1 / W) +
                    1 / (1 + 1. / std::pow(mixingRatio, 2)) *
                        (AC0 + AC1 * W + AC2 * W * W + ACm1 / W));
  }
  cout << "Mixing ratio is badly defined. Returning 1." << endl;*/

  return 1.;
}

double SpectralFunctions::RelativisticCorrection(double W, double W0, int Z,
                                                 int A, double R, int betaType,
                                                 int decayType) {
  if (decayType == FERMI) {
    double Wb = W + betaType * 3 * ALPHA * Z / (2. * R);
    double pb = std::sqrt(Wb * Wb - 1.);
    double H2 = -std::pow(pb * R, 2.) / 6.;
    double D1 = Wb * R / 3.;
    double D3 =
        -Wb * R * std::pow(pb * R, 2.) / 30 - betaType * ALPHA * Z / 10.;
    double d1 = R / 3.;
    double d3 = -R * std::pow(pb * R, 2.) / 30.;
    double N1 = (W0 - W) * R / 3.;
    double N2 = -std::pow((W0 - W) * R, 2.) / 6.;
    double N3 = std::pow((W0 - W) * R, 3.) / 30;

    double gamma = std::sqrt(1. - std::pow(ALPHA * Z, 2.));

    double Vf2, Vf3;
    double Af2, Af3;

    Vf2 = -2. * (D1 + N1) + 2 * gamma / W * d1;
    Vf3 = -2. * (D3 + N1 * H2 - N2 * D1 - N3) + 2 * gamma / W * (d3 - N2 * d1);

    Af2 = 2. / std::sqrt(3) * (D1 + N1) - 2. / std::sqrt(3) * gamma / W * d1;
    Af3 = 2. * std::sqrt(2. / 3.) * (D1 - N1) -
          2. * std::sqrt(2. / 3.) * gamma / W * d1;

    double mismatch = W0 - betaType * 2.5 + betaType * 6. / 5. * ALPHA * Z / R;

    return 1. - 3. / 10 * R * mismatch * Vf2 - 3. / 28. * R * mismatch * Vf3;
  } else {
    return 1.;
  }
}

double SpectralFunctions::DeformationCorrection(double W, double W0, int Z,
                                                double R, double beta2,
                                                int betaType) {
  double bOverA = ChargeDistributions::CalcBoverA(beta2);
  double a, b;

  a = R * std::sqrt(3. / (bOverA * bOverA + 2.));
  b = bOverA * a;

  // cout << "b/a, a, b" << bOverA << " " << a << " " << b <<endl;

  double V0s = 3. * ALPHA * Z / 2. / R;
  double V0d = 0.;

  if (beta2 > 0) {
    V0d = 3. * ALPHA * Z / 2. / a / std::sqrt(bOverA * bOverA - 1.) *
          std::log(std::sqrt(bOverA * bOverA - 1) + bOverA);
  } else if (beta2 < 0) {
    V0d = 3. * ALPHA * Z / 2. / a / std::sqrt(1. - bOverA * bOverA) *
          asin(std::sqrt(1 - bOverA * bOverA));
  }
  double etaS =
      1. / 6. * (std::pow(W0 - W, 2.) + std::pow(W + betaType * V0s, 2.) - 1.);
  double etaD =
      1. / 6. * (std::pow(W0 - W, 2.) + std::pow(W + betaType * V0d, 2.) - 1.);
  double DC0 = (1 - etaD * 3. / 5. * R * R) / (1 - etaS * 3. / 5. * R * R);

  // cout << "DC0 " << DC0 << endl;

  int steps = 1E2;
  double grid[steps];
  double integrAd[steps];

  double gamma = std::sqrt(1. - std::pow(ALPHA * Z, 2.));

  double prefact = 4. / 3. * M_PI * std::pow(R, 2. * (1 - gamma));

  // TODO
  /*for (int i = 0; i < steps; i++) {
    grid[i] = a + (i + 1.) / (double)steps * (b - a);
    integrAd[i] = std::pow(grid[i], 3.) * Z *
                   abs(ChargeDistributions::GetDerivDeformedChargeDist(grid[i],
  a, b)) *
                   L0Correction(W, grid[i]) * std::pow(grid[i], 2. * (gamma -
  1));
    cout << grid[i] << "\t" << integrAd[i] << endl;
  }

  double newL0 = prefact * utilities::Trapezoid(grid, integrAd, steps);

  // cout << "newL0 " << newL0 << endl;
  double DFS = newL0 / L0Correction(W, R);*/

  double DFS = 1.;

  return DC0 * DFS;
}

double SpectralFunctions::L0Correction(double W, int Z, double r, int betaType,
                                       double aPos[], double aNeg[]) {

  double gamma = std::sqrt(1. - std::pow(ALPHA * Z, 2.));
  double sum = 0;
  double common = 0;
  double specific = 0;
  for (int i = 1; i < 7; i++) {
    if (betaType == BETA_PLUS)
      sum += aPos[i] * std::pow(W * r, i - 1);
    else
      sum += aNeg[i] * std::pow(W * r, i - 1);
  }
  common = 1. + 13. / 60. * std::pow(ALPHA * Z, 2) -
            betaType * W * r * ALPHA * Z * (41. - 26. * gamma) / 15. /
                (2. * gamma - 1) -
            betaType * ALPHA * Z * r * gamma * (17. - 2. * gamma) / 30. / W /
                (2. * gamma - 1) +
            sum;
  if (betaType == BETA_PLUS)
    specific = aPos[0] * r / W + 0.22 * (r - 0.0164) * std::pow(ALPHA * Z, 4.5);
  else
    specific = aNeg[0] * r / W + 0.41 * (r - 0.0164) * std::pow(ALPHA * Z, 4.5);
  return (common + specific) * 2. / (1. + gamma);
}

double SpectralFunctions::UCorrection(double W, int Z, int betaType) {
  double a0 = -5.6E-5 - betaType * 4.94E-5 * Z + 6.23E-8 * std::pow(Z, 2);
  double a1 = 5.17E-6 + betaType * 2.517E-6 * Z + 2.00E-8 * std::pow(Z, 2);
  double a2 = -9.17e-8 + betaType * 5.53E-9 * Z + 1.25E-10 * std::pow(Z, 2);

  double p = std::sqrt(W * W - 1);

  return 1 + a0 + a1 * p + a2 * p * p;
}

double SpectralFunctions::QCorrection(double W, double W0, int Z, int A,
                                      int betaType, int decayType,
                                      double mixingRatio) {
  double a = 0;

  if (decayType == FERMI)
    a = 1.;
  else if (decayType == GAMOW_TELLER)
    a = -1. / 3.;
  else if (mixingRatio > 0)
    a = (1 - std::pow(mixingRatio, 2.) / 3) / (1 + std::pow(mixingRatio, 2));

  double M = A * (PROTON_MASS_KEV + NEUTRON_MASS_KEV) / 2. / ELECTRON_MASS_KEV;

  double p = std::sqrt(W * W - 1);

  return 1 - betaType * M_PI * ALPHA * Z / M / p * (1 + a * (W0 - W) / 3 / M);
}

double SpectralFunctions::RadiativeCorrection(double W, double W0, int Z,
                                              double R, int betaType, double gA,
                                              double gM) {
  // 1st order, based on the 5th Wilkinson article
  double beta = std::sqrt(1.0 - 1.0 / W / W);

  double g = 3 * std::log(PROTON_MASS_KEV / ELECTRON_MASS_KEV) - 0.75 +
             4 * (std::atanh(beta) / beta - 1) *
                 ((W0 - W) / 3 / W - 1.5 + std::log(2 * (W0 - W)));
  g += 4.0 / beta * Spence(2 * beta / (1 + beta)) +
       std::atanh(beta) / beta *
           (2 * (1 + beta * beta) + (W0 - W) * (W0 - W) / 6 / W / W -
            4 * std::atanh(beta));

  double O1corr = ALPHA / 2 / M_PI *
                  (g - 3 * std::log(PROTON_MASS_KEV / ELECTRON_MASS_KEV / 2 / W0));

  double L =
      1.026725 * std::pow(1 - 2 * ALPHA / 3 / M_PI * std::log(2 * W0), 9 / 4.);

  // 2nd order
  double d1f, d2, d3, d14;
  double lambda = std::sqrt(10) / R;
  double lambdaOverM =
      lambda / PROTON_MASS_KEV * ELECTRON_MASS_KEV;  // this is dimensionless

  d14 = std::log(PROTON_MASS_KEV / ELECTRON_MASS_KEV) - 5. / 3. * std::log(2 * W) +
        43. / 18.;

  d1f = std::log(lambdaOverM) - EULER_MASCHERONI_CONSTANT + 4. / 3. -
        std::log(std::sqrt(10.0)) -
        3.0 / M_PI / std::sqrt(10.0) *
            (0.5 + EULER_MASCHERONI_CONSTANT + std::log(std::sqrt(10) / lambdaOverM));

  d2 = 3.0 / 2.0 / M_PI / std::sqrt(10.0) * lambdaOverM *
       (1 - M_PI / 2 / std::sqrt(10) * lambdaOverM);

  d3 = 3.0 * gA * gM / M_PI / std::sqrt(10.0) * lambdaOverM *
       (EULER_MASCHERONI_CONSTANT - 1 + std::log(std::sqrt(10) / lambdaOverM) +
        M_PI / 4 / std::sqrt(10) * lambdaOverM);

  double O2corr = betaType * ALPHA * ALPHA * Z * (d14 + d1f + d2 + d3);

  // 3rd order
  double a = 0.5697;
  double b = 4 / 3 / M_PI * (11 / 4. - EULER_MASCHERONI_CONSTANT - M_PI * M_PI / 6);
  double f = std::log(2 * W0) - 5. / 6.;
  double g2 = 0.5 * (std::pow(log(R), 2.) - std::pow(log(2 * W), 2.)) +
              5. / 3. * std::log(2 * R * W);

  double O3corr = std::pow(ALPHA, 3) * std::pow(Z, 2) *
                  (a * std::log(lambda / W) + b * f + 4 * M_PI / 3. * g2 -
                   0.649 * std::log(2 * W0));

  return (1 + O1corr) * (L + O2corr + O3corr);
}

double SpectralFunctions::NeutrinoRadiativeCorrection(double Wv) {
  double h = 0.;
  double pv = std::sqrt(Wv * Wv - 1);
  double beta = pv / Wv;
  h += 3 * std::log(PROTON_MASS_KEV / ELECTRON_MASS_KEV) + 23 / 4. +
       8 / beta * Spence(2 * beta / (1 + beta)) +
       8 * (std::atan(beta) / beta - 1) * std::log(2 * Wv * beta) +
       4 * std::atan(beta) / beta *
           ((7 + 3 * beta * beta) / 8 - 2 * std::atan(beta));
  return 1 + ALPHA / 2 / M_PI * h;
}

double SpectralFunctions::Spence(double x) { return -gsl_sf_dilog(x); }

double SpectralFunctions::RecoilCorrection(double W, double W0, int A,
                                           int decayType, double mixingRatio) {
  double Vr0, Vr1, Vr2, Vr3;
  double Ar0, Ar1, Ar2, Ar3;
  double M = A * (PROTON_MASS_KEV + NEUTRON_MASS_KEV) / 2. /
             ELECTRON_MASS_KEV;  // in units of electron mass
  double M2 = sqr(M);

  Ar0 = -2. * W0 / 3. / M - W0 * W0 / 6. / M2 - 77. / 18. / M2;
  Ar1 = -2. / 3. / M + 7. * W0 / 9. / M2;
  Ar2 = 10. / 3. / M - 28. * W0 / 9. / M2;
  Ar3 = 88. / 9. / M2;

  Vr0 = W0 * W0 / 2. / M2 - 11. / 6. / M2;
  Vr1 = W0 / 3. / M2;
  Vr2 = 2. / M - 4. * W0 / 3. / M2;
  Vr3 = 16. / 3. / M2;

  if (decayType == FERMI) {
    return 1 + Vr0 + Vr1 / W + Vr2 * W + Vr3 * W * W;
  } else if (decayType == GAMOW_TELLER) {
    return 1 + Ar0 + Ar1 / W + Ar2 * W + Ar3 * W * W;
  } else if (mixingRatio > 0) {
    return 1 +
           1. / (1 + std::pow(mixingRatio, 2)) *
               (Vr0 + Vr1 / W + Vr2 * W + Vr3 * W * W) +
           1. / (1 + 1. / std::pow(mixingRatio, 2)) *
               (Ar0 + Ar1 / W + Ar2 * W + Ar3 * W * W);
  }
  cout << "Mixing ratio badly defined. Returning 1." << endl;
  return 1;
}

double SpectralFunctions::AtomicScreeningCorrection(double W, int Z,
                                                    int betaType) {
  std::vector<double> Aby, Bby;

  screening::PotParam(Z - 1 * betaType, Aby, Bby);

  double l = 2 * (Aby[0] * Bby[0] + Aby[1] * Bby[1] + Aby[2] * Bby[2]);

  double p = std::sqrt(W * W - 1);

  double Wt = W - betaType * 0.5 * ALPHA * (Z - betaType) * l;

  std::complex<double> pt;

  pt = 0.5 * p +
       0.5 * std::sqrt(std::complex<double>(p * p -
                                            betaType * 2 * ALPHA * Z * Wt * l));

  double y = betaType * ALPHA * Z * W / p;
  std::complex<double> yt = betaType * ALPHA * Z * Wt / pt;

  double gamma = std::sqrt(1. - pow(ALPHA * Z, 2.));

  /*cout << "V: " << W-Wt << endl;
  cout << W << " " << yt.real() << " " << yt.imag() << endl;
  cout << W << " " << pt.real() << " " << pt.imag() << endl;*/

  gsl_sf_result magn;
  gsl_sf_result phase;
  gsl_sf_lngamma_complex_e(gamma, y, &magn, &phase);

  gsl_sf_result magnT;
  gsl_sf_result phaseT;
  gsl_sf_lngamma_complex_e(gamma - yt.imag(), yt.real(), &magnT, &phaseT);
  // now we have what we wAt in magn.val

  double first = Wt / W;
  double second = std::exp(2 * (magnT.val - magn.val));

  gsl_sf_lngamma_complex_e(gamma - 2 * pt.imag() / l, 2 * pt.real() / l, &magnT,
                           &phaseT);
  gsl_sf_lngamma_complex_e(1, 2 * p / l, &magn, &phase);
  double third = std::exp(2 * (magnT.val - magn.val));
  double fourth = std::exp(-M_PI * y);
  double fifth = std::pow(2 * p / l, 2 * (1 - gamma));

  return first * second * third * fourth * fifth;
}

double SpectralFunctions::AtomicExchangeCorrection(double W, double exPars[9]) {
  double E = W - 1;

  return 1 + exPars[0] / E + exPars[1] / E / E +
         exPars[2] * std::exp(-exPars[3] * E) +
         exPars[4] * sin(std::pow(W - exPars[6], exPars[5]) + exPars[7]) /
             std::pow(W, exPars[8]);
}

double SpectralFunctions::AtomicMismatchCorrection(double W, double W0, int Z,
                                                   int A, int betaType) {
  double dBdZ2 = (44.200 * std::pow(Z - betaType, 0.41) +
                  2.3196E-7 * std::pow(Z - betaType, 4.45)) /
                 ELECTRON_MASS_KEV / 1000.;

  double K = -0.872 + 1.270 * std::pow(Z, 0.097) + 9.062E-11 * std::pow(Z, 4.5);
  double vp = std::sqrt(1 - 1 / W / W);
  double l = 1.83E-3 * K * Z / vp;
  double M = A * (PROTON_MASS_KEV + NEUTRON_MASS_KEV) / 2. / ELECTRON_MASS_KEV;
  // assume as A average the recoil velocity at half-momentum trAsfer ~
  // std::sqrt(W0^2-1)/2
  double vR = std::sqrt(1 - M * M / (M * M + (W0 * W0 - 1) / 4.));

  double psi2 = 1 + 2 * ALPHA / vp * (std::atan(1 / l) - l / 2 / (1 + l * l));

  double C0 = -ALPHA * ALPHA * Z * ALPHA / vp * l / (1 + l * l) / psi2;

  double C1 = 2 * ALPHA * ALPHA * Z * vR / vp *
              ((0.5 + l * l) / (1 + l * l) - l * std::atan(1 / l)) / psi2;

  return 1 - 2 / (W0 - W) * (0.5 * dBdZ2 + 2 * (C0 + C1));
}
