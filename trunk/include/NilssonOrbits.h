#ifndef NILSSON_ORBITS
#define NILSSON_ORBITS

#include "Utilities.h"
#include "Constants.h"

#include <iostream>

#include "gsl/gsl_eigen.h"
#include "gsl/gsl_matrix.h"
#include "gsl/gsl_vector.h"

#define NDIM1 56
#define NDIM3 35
#define NDIM4 40

namespace nilsson {

using std::cout;
using std::endl;

// Here dO stands for double Omega
// s is +-1 depending of whether it is j=l+-1/2
struct WFComp {
  double C;
  int l, s, dO;
};

struct NuclearState {
  double O, K;
  int parity;
  std::vector<WFComp> states;
};

inline double V(int n, int l, double x) {
  int m = (n - l) / 2;
  double V = 1.0;
  double F = 1.0;
  int mK = 1;
  int mM = 1;
  if (m > 0) {
    for (int k = 1; k <= m; k++) {
      mK *= m + 1 - k;
      F *= -2. * x / k;
      mM *= 2 * n - 4 * m + 1 + 2 * k;
      V += F * mK / mM;
    }
  }
  return V;
}

inline double VNORM(int n, int l) {
  int minus = (n - l) / 2;
  double eMinus = 1 + 4 * (minus / 2) - 2 * minus;
  double vNorm = 2. / std::pow(M_PI, 0.25) * eMinus;
  int k = (n + 1) / 2 - n / 2 + 1;
  if (k > 1) {
    vNorm *= std::sqrt(2. / 3.);
  }
  if (n > 1) {
    for (int j = k; j <= n; j += 2) {
      vNorm *= std::sqrt((j + k + 1.) / (j - k + 2.));
    }
  }
  if (l > 1) {
    for (int j = k; j <= l; j += 2) {
      vNorm *= 2. * std::sqrt((j + n + 2.) * (n - j + 1.)) /
               ((2. * j + 3.) * (2. * j + 1.));
    }
  }
  return vNorm;
}

inline void WoodsSaxon(double V0, double R, double A0, double V0S, double A,
                       double Z, int nMax, double SW[2][84], double SDW[462],
                       bool report = false) {
  double FINT[3] = {};
  double S[3][4] = {};
  int II = 0;
  int JJ = 0;
  int NDX = 300;
  int N = NDX / 4 - 2;
  double AO = A0;
  // Compton wavelength of the pion=  1.4133 fm
  double pionComp = hbar / pionMasskeV / speedOfLight;
  pionComp = 1.4133;
  double VOS = V0S * pionComp * pionComp;
  double TWONU = 0.154666 * std::sqrt(V0) / R;
  double T = TWONU * 20.899 * A / (A - 1.);
  // e^2 = 1.439978 MeV*fm
  double ZZ = Z * 1.439978;
  double ZCONST = 1.5 * ZZ / R;
  double ZIN = 0.5 * ZZ / std::pow(R, 3.);
  double DX = (R + 8. * AO) / NDX;
  double DFO = std::exp(DX / AO);
  int nMin = nMax % 2;

  if (report) {
    cout << "Radial integrals for Woods-Saxon potential" << endl;
  }
  for (int NI = nMin; NI <= nMax; NI += 2) {
    for (int NJ = nMin; NJ <= NI; NJ += 2) {
      for (int LI = nMin; LI <= NI; LI += 2) {
        for (int LJ = nMin; LJ <= NJ; LJ += 2) {
          int KK = 0;
          if (LI != LJ) {
            KK = 2;
          }
          double r = DX;
          double FO = DFO / std::exp(R / AO);
          for (int i = 0; i < 3; i++) {
            FINT[i] = 0.0;
          }
          int M = -N - 2;
          while (true) {
            double r2 = r * r * TWONU;
            double PSI2 = V(NI, LI, r2) * V(NJ, LJ, r2) *
                          std::pow(r2, (LI + LJ) / 2) * r * r / std::exp(r2);
            double F = 1. / (1. + FO);
            double DFDR = -FO * F * F / AO;
            for (int i = 0; i < 4; i++) {
              if (r <= R) {
                S[0][i] = -PSI2 * (V0 * F + T * r2 + ZIN * r * r - ZCONST);
              } else {
                S[0][i] = -PSI2 * (V0 * F + T * r2 - ZZ / r);
              }
              S[1][i] = PSI2 * DFDR / r;
              S[2][i] = PSI2 * DFDR;
              FO *= DFO;
              r += DX;
              for (int j = KK; j < 3; j++) {
                FINT[j] += S[j][i];
              }
            }
            M += 2;
            if (M / N < 0) {
              for (int i = 1; i < 4; i++) {
                for (int l = i; l < 4; l++) {
                  int k = 3 + i - l;
                  for (int j = KK; j < 3; j++) {
                    S[j][k] -= S[j][k - 1];
                  }
                }
              }
              for (int j = KK; j < 3; j++) {
                FINT[j] += -S[j][0] / 2. + S[j][1] / 12. - S[j][2] / 24. +
                           19. * S[j][3] / 720.;
              }
              if (DX <= 0) {
                break;
              }
            } else if (M / N > 0) {
              r += 4. * DX;
              DX = -DX;
              FO *= std::pow(DFO, 4.);
              DFO = 1. / DFO;
              M = -N - 2;
            }
          }
          DX = -DX;
          DFO = 1. / DFO;
          double X = VNORM(NI, LI) * VNORM(NJ, LJ);
          if (KK != 2) {
            SW[0][II] = FINT[0] * X * DX * std::pow(TWONU, 1.5);
            if (NI == NJ) {
              // Harmonic oscillator energy
              SW[0][II] += 2. * T * (NI + 1.5);
            }
            SW[1][II] = FINT[1] * X * DX * std::pow(TWONU, 1.5) * VOS;
            II++;
          }
          SDW[JJ] = FINT[2] * X * DX * std::pow(TWONU, 1.5) * V0 * R;
          JJ++;
          if (report) {
            if (KK == 0) {
              cout << "< " << NI << "," << LI << " | " << NJ << "," << LJ
                   << " >";
              cout << "    " << II << " " << JJ << " =    " << SW[0][II - 1]
                   << "\t " << SW[1][II - 1] << "\t " << SDW[JJ - 1];
              cout << endl;
            } else if (KK == 2) {
              cout << "< " << NI << "," << LI << " | " << NJ << "," << LJ
                   << " >";
              cout << "      " << JJ << " = \t\t\t\t\t" << SDW[JJ - 1];
              cout << endl;
            }
          }
        }
      }
    }
  }
}

// Calculate eigenvalues & vectors for a real symmetric FORTRAN matrix A, only
// upper half of A is used
inline void Eigen(double* A, int dim, double* eVecs, std::vector<double>& eVals,
                  bool onlyUpper = true) {
  gsl_matrix* aNew = gsl_matrix_alloc(dim, dim);
  // Loop over upper half of matrix
  for (int j = 0; j < dim; j++) {
    for (int i = 0; i <= j; i++) {
      // FORTRAN matrices are stored column-major
      int index = dim * j + i;
      if (onlyUpper) {
        index = j * (j + 1) / 2 + i;
      }
      gsl_matrix_set(aNew, i, j, A[index]);
      if (i != j) {
        gsl_matrix_set(aNew, j, i, A[index]);
      }
    }
  }
  gsl_matrix* eVec = gsl_matrix_calloc(dim, dim);
  gsl_vector* eVal = gsl_vector_calloc(dim);
  int size = 4 * dim;
  gsl_eigen_symmv_workspace* w = gsl_eigen_symmv_alloc(size);
  gsl_eigen_symmv(aNew, eVal, eVec, w);
  gsl_eigen_symmv_free(w);
  //Sort eigenvalues and eigenvalues according to descending eigenvalue
  for (int i = 0; i < dim; i++) {
    for (int j = i; j < dim; j++) {
      if (gsl_vector_get(eVal, j) > gsl_vector_get(eVal, i)) {
        double x = gsl_vector_get(eVal, i);
        gsl_vector_set(eVal, i, gsl_vector_get(eVal, j));
        gsl_vector_set(eVal, j, x);
        for (int k = 0; k < dim; k++) {
          x = gsl_matrix_get(eVec, k, i);
          gsl_matrix_set(eVec, k, i, gsl_matrix_get(eVec, k, j));
          gsl_matrix_set(eVec, k, j, x);
        }
        i = 0;
        j = 0;
      }
    }
  }
  for (int i = 0; i < dim; i++) {
    eVals.push_back(gsl_vector_get(eVal, i));
  }
  gsl_vector_free(eVal);
  for (int j = 0; j < dim; j++) {
    for (int i = 0; i < dim; i++) {
      // FORTRAN matrices are stored column-major
      eVecs[dim * j + i] = gsl_matrix_get(eVec, i, j);
    }
  }
  gsl_matrix_free(eVec);
}

/*inline void Eigen(double* A, int rank, double* R) {
  // Turn R into identity matrix of dim rank
  for (int j = 0; j < rank; j++) {
    for (int i = 0; i < rank; i++) {
      R[rank * j + i] = 0.0;
      if (i == j) R[rank * j + i] = 1.0;
    }
  }
  double aNorm = 0.0;
  for (int i = 1; i <= rank; i++) {
    for (int j = i; j <= rank; j++) {
      if (i != j) {
        int IA = i + j * (j - 1) / 2;
        aNorm += A[IA - 1] * A[IA - 1];
      }
    }
  }
  cout << "aNorm: " << aNorm << endl;
  if (aNorm > 0.) {
    aNorm = 1.414 * std::sqrt(aNorm);

    int IND = 0;
    double THR = aNorm / rank;
    int L = 1;
    int M = L + 1;

    while (true) {
      int MQ = M * (M - 1) / 2;
      int LQ = L * (L - 1) / 2;
      int LM = L + MQ;

      if (std::abs(A[LM - 1]) >= THR) {
        IND = 1;
        int LL = L + LQ;
        int MM = M + MQ;
        double X = 0.5 * (A[LL - 1] - A[MM - 1]);
        double Y = -A[LM - 1] / std::sqrt(A[LM - 1] * A[LM - 1] + X * X);
        if (X < 0) {
          Y = -Y;
        }
        double SINX = Y / std::sqrt(2.0 * (1.0 + std::sqrt(1.0 - Y * Y)));
        double SINX2 = SINX * SINX;
        double COSX = std::sqrt(1.0 - SINX2);
        double COSX2 = COSX * COSX;
        double SINCS = SINX * COSX;

        int ILQ = rank * (L - 1);
        int IMQ = rank * (M - 1);
        for (int I = 1; I <= rank; I++) {
          int IQ = I * (I - 1) / 2;
          if (I != L) {
            if (I != M) {
              int IM = 0;
              if (I < M) {
                IM = I + MQ;
              } else if (I > M) {
                IM = M + IQ;
              }
              int IL = 0;
              if (I < L) {
                IL = I + LQ;
              } else {
                IL = L + IQ;
              }
              X = A[IL - 1] * COSX - A[IM - 1] * SINX;
              A[IM - 1] = A[IL - 1] * SINX + A[IM - 1] * COSX;
              A[IL - 1] = X;
            }
          }
          int ILR = ILQ + 1;
          int IMR = IMQ + 1;
          X = R[ILR - 1] * COSX - R[IMR - 1] * SINX;
          R[IMR - 1] = R[ILR - 1] * SINX + R[IMR - 1] * COSX;
          R[ILR - 1] = X;
          X = 2.0 * A[LM - 1] * SINCS;
          Y = A[LL - 1] * COSX2 + A[MM - 1] * SINX2 - X;
          X = A[LL - 1] * SINX2 + A[MM - 1] * COSX2 + X;
          A[LM - 1] =
              (A[LL - 1] - A[MM - 1]) * SINCS + A[LM - 1] * (COSX2 - SINX2);
          A[LL - 1] = Y;
          A[MM - 1] = X;
        }
      }
      if (M == rank && L == (rank - 1) && IND != 1) {
        break;
      }
      if (M != rank) {
        M++;
      } else {
        if (L != (rank-1)) {
          L++;
          M++;
        }
        else if (IND == 1) {
          IND = 0;
          L = 1;
          M = L + 1;
        }
      }
    }
  }
  int IQ = -rank;
  for (int I = 1; I <= rank; I++) {
    IQ += rank;
    int LL = I + I * (I - 1) / 2;
    int JQ = rank * (I - 2);
    for (int J = I; J <= rank; J++) {
      JQ += rank;
      int MM = J + J * (J - 1) / 2;
      if (A[LL - 1] < A[MM - 1]) {
        double X = A[LL - 1];
        A[LL - 1] = A[MM - 1];
        A[MM - 1] = X;
        for (int K = 1; K <= rank; K++) {
          int ILR = IQ + K;
          int IMR = JQ + K;
          X = R[ILR - 1];
          R[ILR - 1] = R[IMR - 1];
          R[IMR - 1] = X;
        }
      }
    }
  }
  for (int i = 0; i < rank; i++) {
    for (int j = 0; j < rank; j++) {
      cout << R[i * rank + j] << " ";
    }
    cout << endl;
  }
}*/

inline void Calculate(double spin, double beta2, double beta4, double V0,
                      double R, double A0, double V0S, double A, double Z,
                      int nMax, bool report = false) {
  double SW[2][84] = {};
  double SDW[462] = {};
  int N[NDIM1];
  int L[NDIM1];
  // Matrix elements of Hamiltonian in chosen basis, real & symmetric
  double hamM[NDIM4 * (NDIM4 + 1) / 2];
  double eVecs[NDIM4 * NDIM4] = {};
  double defExpCoef[NDIM4][NDIM1];
  double B[NDIM4 * (NDIM4 + 1) / 2];
  double D[NDIM4 * (NDIM4 + 1) / 2];
  int LA[NDIM1];
  int IX2[NDIM1];
  int JX2[NDIM1];
  double sphExpCoef[NDIM3][NDIM1] = {};
  double eValsWS[NDIM3];
  int LK[NDIM3];
  int LKK[NDIM3];
  int KN[7][NDIM3];
  int K1[7];
  double eValsDWS[NDIM4];
  int K2[NDIM4];
  int K3[NDIM4];
  int K4[NDIM4];

  WoodsSaxon(V0, R, A0, V0S, A, Z, nMax, SW, SDW, report);

  int II = 0;
  int K = 0;

  int nMin = nMax % 2 + 1;
  int nMaxP1 = nMax + 1;
  // Diagonalize each L-value separately
  for (int LI = nMin; LI <= nMaxP1; LI += 2) {
    int NIM = II;
    int NI = II + 1;
    int NK = K + 1;

    // Set up quantum numbers of the harmonic oscillator basis
    for (int NN = LI; NN <= nMaxP1; NN += 2) {
      for (int I = 1; I <= 2; I++) {
        N[II] = NN - 1;
        L[II] = LI - 1;
        LA[II] = 2 - I;
        IX2[II] = 2 * I - 3;
        JX2[II] = 2 * L[II] + IX2[II];
        if (L[II] >= LA[II]) {
          II++;
        }
      }
    }
    if (II == NIM) {
      break;
    }

    // Set up matrix
    int KK = 0;
    for (int i = NI; i <= II; i++) {
      for (int j = NI; j <= i; j++) {
        hamM[KK] = 0.0;
        if (JX2[j - 1] == JX2[i - 1]) {
          int NN =
              (N[i - 1] / 2) * (N[i - 1] / 2 + 1) * (N[i - 1] / 2 + 2) / 6 +
              (N[j - 1] / 2) * (N[j - 1] / 2 + 1) / 2 + L[i - 1] / 2;
          hamM[KK] =
              SW[0][NN] + SW[1][NN] * IX2[j - 1] * (L[j - 1] + LA[j - 1]);
        }
        KK++;
      }
    }
    std::vector<double> eVals;
    Eigen(hamM, II - NIM, eVecs, eVals);

    int index = 0.0;
    for (int i = NI; i <= II; i++) {
      double eigenValue = eVals[index];
      if (eigenValue <= 10.0) {
        if (K >= NDIM3 - 1) {
          cout << "Dimensioned space inadequate" << endl;
          return;
        }
        eValsWS[K] = eigenValue;
        LK[K] = L[i - 1];
        K3[K] = JX2[i - 1];
        for (int J = NI; J <= II; J++) {
          int N0 = (II - NIM) * (i - NI) + J - NIM;
          sphExpCoef[K][J - 1] = eVecs[N0 - 1];
        }
        K++;
      }
      index++;
    }
    // II = size of harmonic oscillator basis
    // K = size of Woods-Saxon basis for deformed state diagonalization
    if (NK == K + 1) {
      II = NIM;
    }
  }
  if (K == 0) {
    return;
  }

  if (report) {
    cout << "Spherical Woods-Saxon expansion in Harmonic Oscillator basis."
         << endl;
    for (int KKK = 1; KKK <= K; KKK += 12) {
      int KKKK = std::min(K, KKK + 11);
      cout << "Energy (MeV): ";
      for (int i = KKK; i <= KKKK; i++) {
        cout << eValsWS[i - 1] << " ";
      }
      cout << endl;
      cout << "Spin: \t\t";
      for (int i = KKK; i <= KKKK; i++) {
        cout << K3[i - 1] << "/2 \t";
      }
      cout << endl;
      cout << "Coefficients: " << endl;
      for (int J = 1; J <= II; J++) {
        cout << "| " << N[J - 1] << " " << JX2[J - 1] << "/2 > ";
        for (int L = KKK; L <= KKKK; L++) {
          cout << sphExpCoef[L - 1][J - 1] << "\t\t ";
        }
        cout << endl;
      }
    }
  }

  int ISX2 = std::abs(2 * spin);
  int ISPIN = (ISX2 + 1) / 2;

  int KK = 0;
  int KKKK = 0;
  int IOM = 1;
  for (int IIOM = 1; IIOM <= ISPIN; IIOM++) {
    IOM = -IOM - 2 * (int)(std::pow(-1., IIOM));
    int IIIOM = 4 * (std::abs(IOM) / 4) + 2 - nMin;
    int KKK = 0;
    // Loop over all spherical states with E < 10.0 MeV
    for (int I = 0; I < K; I++) {
      LKK[KKK] = LK[I];
      KN[IIOM - 1][KKK] = I + 1;
      double X = 0.0;
      // Loop over the full spherical basis
      for (int J = 0; J < II; J++) {
        if (L[J] == LKK[KKK]) {
          double cg = utilities::ClebschGordan(2 * L[J], 1, JX2[J],
                                               2 * (LA[J] + IIOM - 1), IX2[J],
                                               2 * (LA[J] + IIOM - 1) + IX2[J]);
          double X1 = sphExpCoef[I][J] * cg;
          cg = utilities::ClebschGordan(2 * L[J], 1, JX2[J] - 2 * IX2[J],
                                        2 * (LA[J] + IIOM - 1), IX2[J],
                                        2 * (LA[J] + IIOM - 1) + IX2[J]);
          if (JX2[J] > IIIOM) {
            X1 += sphExpCoef[I][J - IX2[J]] * cg;
          }
          defExpCoef[KKK][J] = X1;
          X += X1 * X1;
        }
      }
      if (X < 0.1) {
        KKK--;
      }
      KKK++;
    }
    KKKK += KKK;
    if (KKKK > NDIM4) {
      return;
    }
    K1[IIOM - 1] = KKK;
    if (KKK == 0) {
      break;
    }
    for (int I = 0; I < KKK; I++) {
      for (int J = 0; J <= I; J++) {
        B[KK] = 0.0;
        D[KK] = 0.0;
        for (int N1 = 0; N1 < II; N1++) {
          for (int N2 = 0; N2 < II; N2++) {
            if (!(L[N1] != LKK[I] || L[N2] != LKK[J] ||
                  (L[N1] - L[N2]) / 5 != 0 || LA[N1] != LA[N2])) {
              int NI = N[N1] / 2 + 1;
              int NJ = N[N2] / 2 + 1;
              int LI = L[N1] / 2 + 1;
              int LJ = L[N2] / 2 + 1;
              if (NI < NJ) {
                int NN = NI;
                NI = NJ;
                NJ = NN;
                NN = LI;
                LI = LJ;
                LJ = NN;
              }
              int NNP = NI * (NI - 1);
              NNP = (3 * NNP * NNP + 2 * NNP * (2 * NI - 1)) / 24 +
                    NI * NJ * (NJ - 1) / 2 + (LI - 1) * NJ + LJ;
              double X = defExpCoef[I][N1] * SDW[NNP - 1] * defExpCoef[J][N2];
              int LP = L[N1];
              int LAP = LA[N1] + IIOM - 1;
              int LL = L[N2];
              int LLA = LA[N2] + IIOM - 1;
              B[KK] +=
                  X * utilities::SphericalHarmonicME(LP, LAP, 2, 0, LL, LLA);
              D[KK] +=
                  X * utilities::SphericalHarmonicME(LP, LAP, 4, 0, LL, LAP);
            }
          }
        }
        KK++;
      }
    }
  }

  K = 0;
  KK = 0;
  IOM = 1;
  for (int IIOM = 1; IIOM <= ISPIN; IIOM++) {
    IOM = -IOM - 2 * (int)(std::pow(-1., IIOM));
    int KKK = K1[IIOM - 1];
    if (KKK != 0) {
      int NK = 0;
      for (int I = 1; I <= KKK; I++) {
        for (int J = 1; J <= I; J++) {
          NK++;
          hamM[NK - 1] = beta2 * B[KK] + beta4 * D[KK];
          KK++;
        }
        int N0 = KN[IIOM - 1][I - 1];
        hamM[NK - 1] += eValsWS[N0 - 1];
      }
      std::vector<double> eVals;
      Eigen(hamM, KKK, eVecs, eVals);
      int N0 = 0;
      for (int MU = 1; MU <= KKK; MU++) {
        N0 += MU;
        K2[K] = IOM;
        K3[K] = std::abs(IOM);
        K4[K] = KKK - MU + 1;
        eValsDWS[K] = eVals[MU - 1];
        for (int J = 0; J < II; J++) {
          defExpCoef[K][J] = 0.0;
          NK = KKK * (MU - 1);
          for (int NU = 0; NU < KKK; NU++) {
            int I = KN[IIOM - 1][NU];
            NK++;
            defExpCoef[K][J] += eVecs[NK - 1] * sphExpCoef[I - 1][J];
          }
        }
        K++;
      }
    }
  }
  if (report) {
    cout << "Deformed states: No band mixing   Beta2: " << beta2
         << " Beta4: " << beta4 << endl;
    for (int KKK = 1; KKK <= K; KKK += 12) {
      int KKKK = std::min(K, KKK + 11);
      cout << "Energy (MeV): ";
      for (int i = KKK; i <= KKKK; i++) {
        cout << eValsDWS[i - 1] << " ";
      }
      cout << endl;
      cout << "*OMEGA |MU> ";
      for (int i = KKK; i <= KKKK; i++) {
        cout << K3[i - 1] << "/2|" << K4[i - 1] << "> \t";
      }
      cout << endl;
      for (int j = 1; j <= II; j++) {
        cout << "| " << N[j - 1] << ", " << JX2[j - 1] << "/2 > ";
        for (int l = KKK; l <= KKKK; l++) {
          cout << defExpCoef[l - 1][j - 1] << "\t\t";
        }
        cout << endl;
      }
    }
  }
}

inline NuclearState CalculateDeformedState(int Z, int A, double beta2,
                                           double beta4) {
  double O;
  double K;
  double parity;

  // TODO

  // special case for 19Ne
  /*struct WFComp s12 = {0.528947, 0, 1, 1};
  struct WFComp d32 = {-0.297834, 2, -1, 1};
  struct WFComp d52 = {0.794676, 2, 1, 1};

  std::vector<WFComp> states = {s12, d32, d52};*/

  // special case for 33Cl
  WFComp d32 = {0.913656, 2, -1, 3};
  WFComp d52 = {-0.406489, 2, 1, 3};

  std::vector<WFComp> states = {d32, d52};

  O = 3. / 2.;
  K = 3. / 2.;
  parity = 1;

  NuclearState nuclearState = {O, K, parity, states};

  return nuclearState;
}
}
#endif
