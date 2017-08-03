
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "graphics.h"
#include "grfont.h"
#include "texttf.h"
#include <ctype.h>
#include <string.h>

#ifndef M_SQRT1_2
#define	M_SQRT1_2	0.70710678118654752440	// 1/sqrt(2)
#endif

// Create default font structure, this is used in raster.cc and
// rgbmap.cc, and is available for use by the application.
//
GRvecFont FT;

// Definition of a scalable vector font for text rendering

namespace {
    // (space)
    // !
    GRvecFont::Cpair EP1[] = {{4, 2}, {4, 7}};
    GRvecFont::Cpair EP2[] = {{4, 9}, {4, 10}};
    GRvecFont::Cstroke EPs[] = {{2, EP1}, {2, EP2}};
    GRvecFont::Character EP_c = {2, EPs, 4, 2};

    // "
    GRvecFont::Cpair DQ1[] = {{2, 2}, {2, 4}};
    GRvecFont::Cpair DQ2[] = {{6, 2}, {6, 4}};
    GRvecFont::Cstroke DQs[] = {{2, DQ1}, {2, DQ2}};
    GRvecFont::Character DQ_c = {2, DQs, 2, 6};

    // #
    GRvecFont::Cpair PS1[] = {{2, 2}, {2, 10}};
    GRvecFont::Cpair PS2[] = {{6, 2}, {6, 10}};
    GRvecFont::Cpair PS3[] = {{1, 4}, {7, 4}};
    GRvecFont::Cpair PS4[] = {{1, 8}, {7, 8}};
    GRvecFont::Cstroke PSs[] = {{2, PS1}, {2, PS2}, {2, PS3}, {2, PS4}};
    GRvecFont::Character PS_c = {4, PSs, 1, 8};

    // $
    GRvecFont::Cpair DS1[] = {{7, 4}, {7, 3}, {6, 2}, {2, 2}, {1, 3}, {1, 5},
        {2, 6}, {6, 6}, {7, 7}, {7, 9}, {6, 10}, {2, 10}, {1, 9}, {1, 8}};
    GRvecFont::Cpair DS2[] = {{4, 0}, {4, 2}};
    GRvecFont::Cpair DS3[] = {{4, 10}, {4, 12}};
    GRvecFont::Cstroke DSs[] = {{14, DS1}, {2, DS2}, {2, DS3}};
    GRvecFont::Character DS_c = {3, DSs, 1, 8};

    // %
    GRvecFont::Cpair PC1[] = {{1, 10}, {7, 4}};
    GRvecFont::Cpair PC2[] = {{1, 4}, {1, 5}, {2, 5}, {2, 4}, {1, 4}};
    GRvecFont::Cpair PC3[] = {{6, 9}, {6, 10}, {7, 10}, {7, 9}, {6, 9}};
    GRvecFont::Cstroke PCs[] = {{2, PC1}, {5, PC2}, {5, PC3}};
    GRvecFont::Character PC_c = {3, PCs, 1, 8};

    // &
    GRvecFont::Cpair AM1[] = {{7, 6}, {6, 7}, {5, 9}, {4, 10}, {2, 10},
        {1, 9}, {1, 7}, {2, 6}, {3, 6}, {5, 5}, {6, 4}, {6, 3}, {5, 2},
        {3, 2}, {2, 3}, {2, 4}, {3, 5}, {5, 7}, {6, 9}, {7, 10}};
    GRvecFont::Cstroke AMs = {20, AM1};
    GRvecFont::Character AM_c = {1, &AMs, 1, 8};

    // '
    GRvecFont::Cpair SQ1[] = {{3, 1}, {3, 3}, {2, 4}, {1, 4}};
    GRvecFont::Cstroke SQs = {4, SQ1};
    GRvecFont::Character SQ_c = {1, &SQs, 1, 4};

    // (
    GRvecFont::Cpair LP1[] = {{5, 2}, {3, 4}, {3, 8}, {5, 10}};
    GRvecFont::Cstroke LPs = {4, LP1};
    GRvecFont::Character LP_c = {1, &LPs, 3, 4};

    // )
    GRvecFont::Cpair RP1[] = {{3, 2}, {5, 4}, {5, 8}, {3, 10}};
    GRvecFont::Cstroke RPs = {4, RP1};
    GRvecFont::Character RP_c = {1, &RPs, 3, 4};

    // *
    GRvecFont::Cpair AS1[] = {{2, 4}, {6, 8}};
    GRvecFont::Cpair AS2[] = {{6, 4}, {2, 8}};
    GRvecFont::Cpair AS3[] = {{1, 6}, {7, 6}};
    GRvecFont::Cstroke ASs[] = {{2, AS1}, {2, AS2}, {2, AS3}};
    GRvecFont::Character AS_c = {3, ASs, 1, 8};

    // +
    GRvecFont::Cpair PL1[] = {{4, 3}, {4, 9}};
    GRvecFont::Cpair PL2[] = {{1, 6}, {7, 6}};
    GRvecFont::Cstroke PLs[] = {{2, PL1}, {2, PL2}};
    GRvecFont::Character PL_c = {2, PLs, 1, 8};

    // ,
    GRvecFont::Cpair CM1[] = {{4, 8}, {4, 10}, {3, 11}, {2, 11}};
    GRvecFont::Cstroke CMs = {4, CM1};
    GRvecFont::Character CM_c = {1, &CMs, 2, 4};

    // -
    GRvecFont::Cpair MIN1[] = {{1, 6}, {7, 6}};
    GRvecFont::Cstroke MINs = {2, MIN1};
    GRvecFont::Character MIN_c = {1, &MINs, 1, 8};

    // .
    GRvecFont::Cpair PER1[] = {{4, 9}, {4, 10}, {3, 10}, {3, 9}, {4, 9}};
    GRvecFont::Cstroke PERs = {5, PER1};
    GRvecFont::Character PER_c = {1, &PERs, 3, 3};

    // /
    GRvecFont::Cpair FS1[] = {{1, 10}, {7, 3}};
    GRvecFont::Cstroke FSs = {2, FS1};
    GRvecFont::Character FS_c = {1, &FSs, 1, 8};

    // 0
    GRvecFont::Cpair ZER1[] = {{1, 3}, {1, 9}, {2, 10}, {6, 10}, {7, 9},
        {7, 3}, {6, 2}, {2, 2}, {1, 3}};
    GRvecFont::Cstroke ZERs = {9, ZER1};
    GRvecFont::Character ZER_c = {1, &ZERs, 1, 8};

    // 1
    GRvecFont::Cpair ONE1[] = {{2, 4}, {4, 2}, {4, 10}};
    GRvecFont::Cpair ONE2[] = {{3, 10}, {5, 10}};
    GRvecFont::Cstroke ONEs[] = {{3, ONE1}, {2, ONE2}};
    GRvecFont::Character ONE_c = {2, ONEs, 2, 5};

    // 2
    GRvecFont::Cpair TWO1[] = {{1, 3}, {2, 2}, {6, 2}, {7, 3}, {7, 4},
        {1, 10}, {7, 10}, {7, 9}};
    GRvecFont::Cstroke TWOs = {8, TWO1};
    GRvecFont::Character TWO_c = {1, &TWOs, 1, 8};

    // 3
    GRvecFont::Cpair THR1[] = {{1, 3}, {2, 2}, {6, 2}, {7, 3}, {7, 5},
        {6, 6}};
    GRvecFont::Cpair THR2[] = {{3, 6}, {6, 6}, {7, 7}, {7, 9}, {6, 10},
        {2, 10}, {1, 9}};
    GRvecFont::Cstroke THRs[] = {{6, THR1}, {7, THR2}};
    GRvecFont::Character THR_c = {2, THRs, 1, 8};

    // 4
    GRvecFont::Cpair FOU1[] = {{7, 7}, {1, 7}, {1, 6}, {5, 2}, {6, 2},
        {6, 10}};
    GRvecFont::Cpair FOU2[] = {{5, 10}, {7, 10}};
    GRvecFont::Cstroke FOUs[] = {{6, FOU1}, {2, FOU2}};
    GRvecFont::Character FOU_c = {2, FOUs, 1, 8};

    // 5
    GRvecFont::Cpair FIV1[] = {{7, 2}, {1, 2}, {1, 6}, {6, 6}, {7, 7},
        {7, 9}, {6, 10}, {2, 10}, {1, 9}};
    GRvecFont::Cstroke FIVs = {9, FIV1};
    GRvecFont::Character FIV_c = {1, &FIVs, 1, 8};

    // 6
    GRvecFont::Cpair SIX1[] = {{6, 2}, {3, 2}, {1, 4}, {1, 9}, {2, 10},
        {6, 10}, {7, 9}, {7, 7}, {6, 6}, {1, 6}};
    GRvecFont::Cstroke SIXs = {10, SIX1};
    GRvecFont::Character SIX_c = {1, &SIXs, 1, 8};

    // 7
    GRvecFont::Cpair SEV1[] = {{1, 3}, {1, 2}, {7, 2}, {7, 3}, {3, 7},
        {3,10}};
    GRvecFont::Cstroke SEVs = {6, SEV1};
    GRvecFont::Character SEV_c = {1, &SEVs, 1, 8};

    // 8
    GRvecFont::Cpair EIG1[] = {{1, 3}, {1, 5}, {2, 6}, {1, 7}, {1, 9},
        {2, 10}, {6, 10}, {7, 9}, {7, 7}, {6, 6}, {7, 5}, {7, 3}, {6, 2},
        {2, 2}, {1, 3}};
    GRvecFont::Cpair EIG2[] = {{2, 6}, {6, 6}};
    GRvecFont::Cstroke EIGs[] = {{15, EIG1}, {2, EIG2}};
    GRvecFont::Character EIG_c = {2, EIGs, 1, 8};

    // 9
    GRvecFont::Cpair NIN1[] = {{7, 6}, {2, 6}, {1, 5}, {1, 3}, {2, 2},
        {6, 2}, {7, 3}, {7, 8}, {5, 10}, {2, 10}};
    GRvecFont::Cstroke NINs = {10, NIN1};
    GRvecFont::Character NIN_c = {1, &NINs, 1, 8};

    // :
    GRvecFont::Cpair CO1[] = {{4,3}, {4,4}};
    GRvecFont::Cpair CO2[] = {{4,8}, {4, 9}};
    GRvecFont::Cstroke COs[] = {{2, CO1}, {2, CO2}};
    GRvecFont::Character CO_c = {2, COs, 4, 2};

    // ;
    GRvecFont::Cpair SC1[] = {{4, 3}, {4, 4}};
    GRvecFont::Cpair SC2[] = {{4, 8}, {4, 9}, {3, 10}, {2, 10}};
    GRvecFont::Cstroke SCs[] = {{2, SC1}, {4, SC2}};
    GRvecFont::Character SC_c = {2, SCs, 2, 4};

    // <
    GRvecFont::Cpair LT1[] = {{6, 2}, {2, 6}, {6, 10}};
    GRvecFont::Cstroke LTs = {3, LT1};
    GRvecFont::Character LT_c = {1, &LTs, 2, 6};

    // =
    GRvecFont::Cpair EQ1[] = {{1, 5}, {6, 5}};
    GRvecFont::Cpair EQ2[] = {{1, 8}, {6, 8}};
    GRvecFont::Cstroke EQs[] = {{2, EQ1}, {2, EQ2}};
    GRvecFont::Character EQ_c = {2, EQs, 1, 7};

    // >
    GRvecFont::Cpair GT1[] = {{2, 2}, {6,6}, {2, 10}};
    GRvecFont::Cstroke GTs = {3, GT1};
    GRvecFont::Character GT_c = {1, &GTs, 2, 6};

    // ?
    GRvecFont::Cpair QM1[] = {{1,4}, {1, 3}, {2, 2}, {6, 2}, {7, 3},
        {7, 4}, {6, 5}, {5, 5}, {4, 6}, {4, 7}};
    GRvecFont::Cpair QM2[] = {{4, 9}, {4, 10}};
    GRvecFont::Cstroke QMs[] = {{10, QM1}, {2, QM2}};
    GRvecFont::Character QM_c = {2, QMs, 1, 8};

    // @
    GRvecFont::Cpair AT1[] = {{7, 5}, {4, 5}, {4, 8}, {6, 8}, {7, 7},
        {7, 3}, {6, 2}, {2, 2}, {1, 3}, {1, 9}, {2, 10}, {6, 10}};
    GRvecFont::Cstroke ATs = {12, AT1};
    GRvecFont::Character AT_c = {1, &ATs, 1, 8};

    // A
    GRvecFont::Cpair A1[] = {{1, 10}, {1, 4}, {3, 2}, {5, 2}, {7, 4},
        {7, 10}};
    GRvecFont::Cpair A2[] = {{1, 6}, {7, 6}};
    GRvecFont::Cstroke As[] = {{6, A1}, {2, A2}};
    GRvecFont::Character A_c = {2, As, 1, 8};

    // B
    GRvecFont::Cpair B1[] = {{1, 10}, {6, 10}, {7, 9}, {7, 7}, {6, 6},
        {7, 5}, {7, 3}, {6, 2}, {1, 2}};
    GRvecFont::Cpair B2[] = {{2, 10}, {2, 2}};
    GRvecFont::Cpair B3[] = {{2, 6}, {6, 6}};
    GRvecFont::Cstroke Bs[] = {{9, B1}, {2, B2}, {2, B3}};
    GRvecFont::Character B_c = {3, Bs, 1, 8};

    // C
    GRvecFont::Cpair C1[] = {{7, 8}, {5, 10}, {3, 10}, {1, 8}, {1, 4},
        {3, 2}, {5, 2}, {7,4}};
    GRvecFont::Cstroke Cs = {8, C1};
    GRvecFont::Character C_c = {1, &Cs, 1, 8};

    // D
    GRvecFont::Cpair D1[] = {{1, 10}, {5, 10}, {7, 8}, {7, 4}, {5, 2},
        {1, 2}};
    GRvecFont::Cpair D2[] = {{2, 10}, {2, 2}};
    GRvecFont::Cstroke Ds[] = {{6, D1}, {2, D2}};
    GRvecFont::Character D_c = {2, Ds, 1, 8};

    // E
    GRvecFont::Cpair E1[] = {{7, 9}, {7, 10}, {1, 10}};
    GRvecFont::Cpair E2[] = {{7, 3}, {7, 2}, {1, 2}};
    GRvecFont::Cpair E3[] = {{2, 10}, {2, 2}};
    GRvecFont::Cpair E4[] = {{2, 6}, {5, 6}};
    GRvecFont::Cstroke Es[] = {{3, E1}, {3, E2}, {2, E3}, {2, E4}};
    GRvecFont::Character E_c = {4, Es, 1, 8};

    // F
    GRvecFont::Cpair F1[] = {{7, 3}, {7, 2}, {1, 2}};
    GRvecFont::Cpair F2[] = {{2, 10}, {2, 2}};
    GRvecFont::Cpair F3[] = {{2, 6}, {5, 6}};
    GRvecFont::Cstroke Fs[] = {{3, F1}, {2, F2}, {2, F3}};
    GRvecFont::Character F_c = {3, Fs, 1, 8};

    // G
    GRvecFont::Cpair G1[] = {{4, 7}, {7, 7}, {7, 8}, {5, 10}, {3, 10}, {1, 8},
        {1, 4}, {3, 2}, {6, 2}, {7, 4}};
    GRvecFont::Cstroke Gs = {10, G1};
    GRvecFont::Character G_c = {1, &Gs, 1, 8};

    // H
    GRvecFont::Cpair H1[] = {{1, 10}, {1, 2}};
    GRvecFont::Cpair H2[] = {{7, 10}, {7, 2}};
    GRvecFont::Cpair H3[] = {{1, 6}, {7, 6}};
    GRvecFont::Cstroke Hs[] = {{2, H1}, {2, H2}, {2, H3}};
    GRvecFont::Character H_c = {3, Hs, 1, 8};

    // I
    GRvecFont::Cpair I1[] = {{4, 2}, {4, 10}};
    GRvecFont::Cpair I2[] = {{3, 2}, {5, 2}};
    GRvecFont::Cpair I3[] = {{3, 10}, {5, 10}};
    GRvecFont::Cstroke Is[] = {{2, I1}, {2, I2}, {2, I3}};
    GRvecFont::Character I_c = {3, Is, 3, 4};

    // J
    GRvecFont::Cpair J1[] = {{1, 8}, {1, 9}, {2, 10}, {5, 10}, {6, 9}, {6, 2}};
    GRvecFont::Cpair J2[] = {{5, 2}, {7, 2}};
    GRvecFont::Cstroke Js[] = {{6, J1}, {2, J2}};
    GRvecFont::Character J_c = {2, Js, 1, 8};

    // K
    GRvecFont::Cpair K1[] = {{1, 10}, {2, 10}, {2, 2}, {1, 2}};
    GRvecFont::Cpair K2[] = {{7, 2}, {2, 7}};
    GRvecFont::Cpair K3[] = {{3, 6}, {7, 10}};
    GRvecFont::Cstroke Ks[] = {{4, K1}, {2, K2}, {2, K3}};
    GRvecFont::Character K_c = {3, Ks, 1, 8};

    // L
    GRvecFont::Cpair L1[] = {{7, 9}, {7, 10}, {1, 10}};
    GRvecFont::Cpair L2[] = {{2, 10}, {2, 2}};
    GRvecFont::Cpair L3[] = {{1, 2}, {3,2}};
    GRvecFont::Cstroke Ls[] = {{3, L1}, {2, L2}, {2, L3}};
    GRvecFont::Character L_c = {3, Ls, 1, 8};

    // M
    GRvecFont::Cpair M1[] = {{1, 10}, {1, 2}, {4, 5}, {7, 2}, {7, 10}};
    GRvecFont::Cstroke Ms = {5, M1};
    GRvecFont::Character M_c = {1, &Ms, 1, 8};

    // N
    GRvecFont::Cpair N1[] = {{1, 10}, {1, 2}, {7, 10}, {7, 2}};
    GRvecFont::Cstroke Ns = {4, N1};
    GRvecFont::Character N_c = {1, &Ns, 1, 8};

    // O
    GRvecFont::Cpair O1[] = {{1, 4}, {1, 8}, {3, 10}, {5, 10}, {7, 8}, {7, 4},
        {5, 2}, {3, 2}, {1, 4}};
    GRvecFont::Cstroke Os = {9, O1};
    GRvecFont::Character O_c = {1, &Os, 1, 8};

    // P
    GRvecFont::Cpair P1[] = {{1, 2}, {6, 2}, {7, 3}, {7, 5}, {6, 6}, {2, 6}};
    GRvecFont::Cpair P2[] = {{2, 10}, {2, 2}};
    GRvecFont::Cpair P3[] = {{1, 10}, {3, 10}};
    GRvecFont::Cstroke Ps[] = {{6, P1}, {2, P2}, {2, P3}};
    GRvecFont::Character P_c = {3, Ps, 1, 8};

    // Q
    GRvecFont::Cpair Q1[] = {{1, 3}, {1, 9}, {2, 10}, {6, 10}, {7, 9}, {7, 3},
        {6, 2}, {2, 2}, {1, 3}};
    GRvecFont::Cpair Q2[] = {{4, 8}, {6, 10}, {6, 11}};
    GRvecFont::Cstroke Qs[] = {{9, Q1}, {3, Q2}};
    GRvecFont::Character Q_c = {2, Qs, 1, 8};

    // R
    GRvecFont::Cpair R1[] = {{7, 10}, {7, 7}, {6, 6}, {7, 5}, {7, 3}, {6, 2},
        {1, 2}};
    GRvecFont::Cpair R2[] = {{2, 10}, {2, 2}};
    GRvecFont::Cpair R3[] = {{2, 6}, {6, 6}};
    GRvecFont::Cpair R4[] = {{1, 10}, {3, 10}};
    GRvecFont::Cstroke Rs[] = {{7, R1}, {2, R2}, {2, R3}, {2, R4}};
    GRvecFont::Character R_c = {4, Rs, 1, 8};

    // S
    GRvecFont::Cpair S1[] = {{7, 4}, {7, 3}, {6, 2}, {2, 2}, {1, 3}, {1, 4},
        {3, 6}, {5, 6}, {7, 8}, {7, 9}, {6, 10}, {2, 10}, {1, 9}, {1, 8}};
    GRvecFont::Cstroke Ss = {14, S1};
    GRvecFont::Character S_c = {1, &Ss, 1, 8};

    // T
    GRvecFont::Cpair T1[] = {{1, 3}, {1, 2}, {7, 2}, {7, 3}};
    GRvecFont::Cpair T2[] = {{4, 10}, {4, 2}};
    GRvecFont::Cpair T3[] = {{3, 10}, {5, 10}};
    GRvecFont::Cstroke Ts[] = {{4, T1}, {2, T2}, {2, T3}};
    GRvecFont::Character T_c = {3, Ts, 1, 8};

    // U
    GRvecFont::Cpair U1[] = {{1, 2}, {1, 9}, {2, 10}, {6, 10}, {7, 9}, {7, 2}};
    GRvecFont::Cstroke Us = {6, U1};
    GRvecFont::Character U_c = {1, &Us, 1, 8};

    // V
    GRvecFont::Cpair V1[] = {{1, 2}, {1, 7}, {4, 10}, {7, 7}, {7, 2}};
    GRvecFont::Cstroke Vs = {5, V1};
    GRvecFont::Character V_c = {1, &Vs, 1, 8};

    // W
    GRvecFont::Cpair W1[] = {{1, 2}, {1, 10}, {4, 7}, {7, 10}, {7, 2}};
    GRvecFont::Cstroke Ws = {5, W1};
    GRvecFont::Character W_c = {1, &Ws, 1, 8};

    // X
    GRvecFont::Cpair X1[] = {{1, 10}, {7, 2}};
    GRvecFont::Cpair X2[] = {{1, 2}, {7, 10}};
    GRvecFont::Cstroke Xs[] = {{2, X1}, {2, X2}};
    GRvecFont::Character X_c = {2, Xs, 1, 8};

    // Y
    GRvecFont::Cpair Y1[] = {{1, 2}, {4, 7}, {7, 2}};
    GRvecFont::Cpair Y2[] = {{4, 7}, {4, 10}};
    GRvecFont::Cpair Y3[] = {{3, 10}, {5, 10}};
    GRvecFont::Cstroke Ys[] = {{3, Y1}, {2, Y2}, {2, Y3}};
    GRvecFont::Character Y_c = {3, Ys, 1, 8};

    // Z
    GRvecFont::Cpair Z1[] = {{1, 3}, {1, 2}, {7, 2}, {1, 10}, {7, 10}, {7, 9}};
    GRvecFont::Cstroke Zs = {6, Z1};
    GRvecFont::Character Z_c = {1, &Zs, 1, 8};

    // [
    GRvecFont::Cpair LB1[] = {{5, 2}, {3, 2}, {3, 10}, {5, 10}};
    GRvecFont::Cstroke LBs = {4, LB1};
    GRvecFont::Character LB_c = {1, &LBs, 3, 4};

    // backslash
    GRvecFont::Cpair BS1[] = {{1, 3}, {7, 10}};
    GRvecFont::Cstroke BSs = {2, BS1};
    GRvecFont::Character BS_c = {1, &BSs, 1, 8};

    // ]
    GRvecFont::Cpair RB1[] = {{2, 2}, {4, 2}, {4, 10}, {2, 10}};
    GRvecFont::Cstroke RBs = {4, RB1};
    GRvecFont::Character RB_c = {1, &RBs, 2, 4};

    // ^
    GRvecFont::Cpair CF1[] = {{1, 3}, {4, 0}, {7, 3}};
    GRvecFont::Cstroke CFs = {3, CF1};
    GRvecFont::Character CF_c = {1, &CFs, 1, 8};

    // _
    GRvecFont::Cpair US1[] = {{1, 12}, {7, 12}};
    GRvecFont::Cstroke USs = {2, US1};
    GRvecFont::Character US_c = {1, &USs, 1, 8};

    // `
    GRvecFont::Cpair GA1[] = {{3, 0}, {3, 1}, {4, 2}, {5, 2}};
    GRvecFont::Cstroke GAs = {4, GA1};
    GRvecFont::Character GA_c = {1, &GAs, 3, 4};

    // a
    GRvecFont::Cpair a1[] = {{2, 5}, {5, 5}, {6, 6}, {6, 10}, {7, 10}};
    GRvecFont::Cpair a2[] = {{6, 7}, {2, 7}, {1, 8}, {1, 9}, {2, 10}, {5, 10},
        {6, 9}};
    GRvecFont::Cstroke as[] = {{5, a1}, {7, a2}};
    GRvecFont::Character a_c = {2, as, 1, 8};

    // b
    GRvecFont::Cpair b1[] = {{1, 2}, {2, 2}, {2, 10}, {1, 10}};
    GRvecFont::Cpair b2[] = {{2, 6}, {3, 5}, {6, 5}, {7, 6}, {7, 9}, {6, 10},
        {3, 10}, {2, 9}};
    GRvecFont::Cstroke bs[] = {{4, b1}, {8, b2}};
    GRvecFont::Character b_c = {2, bs, 1, 8};

    // c
    GRvecFont::Cpair c1[] = {{7, 6}, {6, 5}, {2, 5}, {1, 6}, {1, 9}, {2, 10},
        {6, 10}, {7, 9}};
    GRvecFont::Cstroke cs = {8, c1};
    GRvecFont::Character c_c = {1, &cs, 1, 8};

    // d
    GRvecFont::Cpair d1[] = {{5, 2}, {6, 2}, {6, 10}, {7, 10}};
    GRvecFont::Cpair d2[] = {{6, 6}, {5, 5}, {2, 5}, {1, 6}, {1, 9}, {2, 10},
        {5, 10}, {6, 9}};
    GRvecFont::Cstroke ds[] = {{4, d1}, {8, d2}};
    GRvecFont::Character d_c = {2, ds, 1, 8};

    // e
    GRvecFont::Cpair e1[] = {{1, 7}, {7, 7}, {7, 6}, {6, 5}, {2, 5}, {1, 6},
        {1, 9}, {2, 10}, {6, 10}, {7, 9}};
    GRvecFont::Cstroke es = {10, e1};
    GRvecFont::Character e_c = {1, &es, 1, 8};

    // f
    GRvecFont::Cpair f1[] = {{6, 4}, {6, 3}, {5, 2}, {3, 2}, {2, 3}, {2, 10}};
    GRvecFont::Cpair f2[] = {{1, 6}, {4, 6}};
    GRvecFont::Cpair f3[] = {{1, 10}, {3, 10}};
    GRvecFont::Cstroke fs[] = {{6, f1}, {2, f2}, {2, f3}};
    GRvecFont::Character f_c = {3, fs, 1, 7};

    // g
    GRvecFont::Cpair g1[] = {{7, 5}, {6, 5}, {6, 11}, {5, 12}, {2, 12},
        {1, 11}};
    GRvecFont::Cpair g2[] = {{6, 6}, {5, 5}, {2, 5}, {1, 6}, {1, 8}, {2, 9},
        {6, 9}};
    GRvecFont::Cstroke gs[] = {{6, g1}, {7, g2}};
    GRvecFont::Character g_c = {2, gs, 1, 8};

    // h
    GRvecFont::Cpair h1[] = {{1, 2}, {2, 2}, {2, 10}, {1, 10}};
    GRvecFont::Cpair h2[] = {{2, 6}, {3, 5}, {6, 5}, {7, 6}, {7, 10}};
    GRvecFont::Cstroke hs[] = {{4, h1}, {5, h2}};
    GRvecFont::Character h_c = {2, hs, 1, 8};

    // i
    GRvecFont::Cpair i1[] = {{3, 5}, {4, 5}, {4, 10}};
    GRvecFont::Cpair i2[] = {{4, 2}, {4, 3}};
    GRvecFont::Cpair i3[] = {{3, 10}, {5, 10}};
    GRvecFont::Cstroke is[] = {{3, i1}, {2, i2}, {2, i3}};
    GRvecFont::Character i_c = {3, is, 3, 4};

    // j
    GRvecFont::Cpair j1[] = {{5, 5}, {5, 11}, {4, 12}, {2, 12}, {1, 11},
        {1, 10}};
    GRvecFont::Cpair j2[] = {{4, 5}, {6, 5}};
    GRvecFont::Cpair j3[] = {{5, 2}, {5, 3}};
    GRvecFont::Cstroke js[] = {{6, j1}, {2, j2}, {2, j3}};
    GRvecFont::Character j_c = {3, js, 1, 7};

    // k
    GRvecFont::Cpair k1[] = {{1, 2}, {2, 2}, {2, 10}, {1, 10}};
    GRvecFont::Cpair k2[] = {{7, 4}, {2, 9}};
    GRvecFont::Cpair k3[] = {{4, 7}, {7, 10}};
    GRvecFont::Cstroke ks[] = {{4, k1}, {2, k2}, {2, k3}};
    GRvecFont::Character k_c = {3, ks, 1, 8};

    // l
    GRvecFont::Cpair l1[] = {{3, 2}, {4, 2}, {4, 10}};
    GRvecFont::Cpair l2[] = {{3, 10}, {5, 10}};
    GRvecFont::Cstroke ls[] = {{3, l1}, {2, l2}};
    GRvecFont::Character l_c = {2, ls, 3, 4};

    // m
    GRvecFont::Cpair m1[] = {{1, 10}, {1, 5}, {3, 5}, {4, 6}, {5, 5}, {6, 5},
        {7, 6}, {7, 10}};
    GRvecFont::Cpair m2[] = {{4, 6}, {4, 10}};
    GRvecFont::Cstroke ms[] = {{8, m1}, {2, m2}};
    GRvecFont::Character m_c = {2, ms, 1, 8};

    // n
    GRvecFont::Cpair n1[] = {{1, 5}, {2, 5}, {2, 10}};
    GRvecFont::Cpair n2[] = {{2, 6}, {3, 5}, {6, 5}, {7, 6}, {7, 10}};
    GRvecFont::Cstroke ns[] = {{3, n1}, {5, n2}};
    GRvecFont::Character n_c = {2, ns, 1, 8};

    // o
    GRvecFont::Cpair o1[] = {{1, 6}, {1, 9}, {2, 10}, {6, 10}, {7, 9}, {7, 6},
        {6, 5}, {2, 5}, {1, 6}};
    GRvecFont::Cstroke os = {9, o1};
    GRvecFont::Character o_c = {1, &os, 1, 8};

    // p
    GRvecFont::Cpair p1[] = {{1, 5}, {2, 5}, {2, 12}};
    GRvecFont::Cpair p2[] = {{2, 6}, {3, 5}, {6, 5}, {7, 6}, {7, 8}, {6, 9},
        {2, 9}};
    GRvecFont::Cpair p3[] = {{1, 12}, {3, 12}};
    GRvecFont::Cstroke ps[] = {{3, p1}, {7, p2}, {2, p3}};
    GRvecFont::Character p_c = {3, ps, 1, 8};

    // q
    GRvecFont::Cpair q1[] = {{7, 5}, {6, 5}, {6, 12}};
    GRvecFont::Cpair q2[] = {{6, 6}, {5, 5}, {2, 5}, {1, 6}, {1, 8}, {2, 9},
        {6, 9}};
    GRvecFont::Cpair q3[] = {{5, 12}, {7, 12}};
    GRvecFont::Cstroke qs[] = {{3, q1}, {7, q2}, {2, q3}};
    GRvecFont::Character q_c = {3, qs, 1, 8};

    // r
    GRvecFont::Cpair r1[] = {{1, 5}, {2, 5}, {2, 10}};
    GRvecFont::Cpair r2[] = {{2, 6}, {3, 5}, {6, 5}, {7, 6}, {7, 7}};
    GRvecFont::Cpair r3[] = {{1, 10}, {3, 10}};
    GRvecFont::Cstroke rs[] = {{3, r1}, {5, r2}, {2, r3}};
    GRvecFont::Character r_c = {3, rs, 1, 8};

    // s
    GRvecFont::Cpair s1[] = {{7, 6}, {6, 5}, {2, 5}, {1, 6}, {2, 7}, {6, 8},
        {7, 9}, {6, 10}, {2, 10}, {1, 9}};
    GRvecFont::Cstroke ss = {10, s1};
    GRvecFont::Character s_c = {1, &ss, 1, 8};

    // t
    GRvecFont::Cpair t1[] = {{4, 2}, {4, 9}, {5, 10}, {6, 10}, {7, 9}};
    GRvecFont::Cpair t2[] = {{2, 5}, {6, 5}};
    GRvecFont::Cstroke ts[] = {{5, t1}, {2, t2}};
    GRvecFont::Character t_c = {2, ts, 2, 7};

    // u
    GRvecFont::Cpair u1[] = {{1, 5}, {1, 9}, {2, 10}, {5, 10}, {6, 9}};
    GRvecFont::Cpair u2[] = {{6, 5}, {6, 10}, {7, 10}};
    GRvecFont::Cstroke us[] = {{5, u1}, {3, u2}};
    GRvecFont::Character u_c = {2, us, 1, 8};

    // v
    GRvecFont::Cpair v1[] = {{1, 5}, {1, 7}, {4, 10}, {7, 7}, {7, 5}};
    GRvecFont::Cstroke vs = {5, v1};
    GRvecFont::Character v_c = {1, &vs, 1, 8};

    // w
    GRvecFont::Cpair w1[] = {{1, 5}, {1, 9}, {2, 10}, {3, 10}, {4, 9}, {5, 10},
        {6, 10}, {7, 9}, {7, 5}};
    GRvecFont::Cpair w2[] = {{4, 7}, {4, 9}};
    GRvecFont::Cstroke ws[] = {{9, w1}, {2, w2}};
    GRvecFont::Character w_c = {2, ws, 1, 8};

    // x
    GRvecFont::Cpair x1[] = {{1, 5}, {6, 10}};
    GRvecFont::Cpair x2[] = {{1, 10}, {6, 5}};
    GRvecFont::Cstroke xs[] = {{2, x1}, {2, x2}};
    GRvecFont::Character x_c = {2, xs, 1, 7};

    // y
    GRvecFont::Cpair y1[] = {{1, 5}, {1, 8}, {2, 9}, {6, 9}};
    GRvecFont::Cpair y2[] = {{6, 5}, {6, 10}, {4, 12}, {1, 12}};
    GRvecFont::Cstroke ys[] =  {{4, y1}, {4, y2}};
    GRvecFont::Character y_c = {2, ys, 1, 7};

    // z
    GRvecFont::Cpair z1[] = {{1, 6}, {1, 5}, {6, 5}, {1, 10}, {6, 10}, {6, 9}};
    GRvecFont::Cstroke zs = {6, z1};
    GRvecFont::Character z_c = {1, &zs, 1, 7};

    // {
    GRvecFont::Cpair LCB1[] = {{6, 2}, {4, 2}, {3, 3}, {3, 5}, {2, 6}, {3, 7},
        {3, 9}, {4, 10}, {6, 10}};
    GRvecFont::Cstroke LCBs = {9, LCB1};
    GRvecFont::Character LCB_c = {1, &LCBs, 2, 6};

    // |
    GRvecFont::Cpair BAR1[] = {{4, 2}, {4, 10}};
    GRvecFont::Cstroke BARs = {2, BAR1};
    GRvecFont::Character BAR_c = {1, &BARs, 4, 2};

    // }
    GRvecFont::Cpair RCB1[] = {{1, 2}, {3, 2}, {4, 3}, {4, 5}, {5, 6}, {4, 7},
        {4, 9}, {3, 10}, {1, 10}};
    GRvecFont::Cstroke RCBs = {9, RCB1};
    GRvecFont::Character RCB_c = {1, &RCBs, 1, 6};

    // ~
    GRvecFont::Cpair TIL1[] = {{1, 3}, {2, 2}, {3, 2}, {5, 3}, {6, 3}, {7, 2}};
    GRvecFont::Cstroke TILs = {6, TIL1};
    GRvecFont::Character TIL_c = {1, &TILs, 1, 8};

    GRvecFont::Character *GRcharset[] =
    {
        &EP_c, &DQ_c, &PS_c, &DS_c, &PC_c, &AM_c, &SQ_c, &LP_c, &RP_c, &AS_c,
        &PL_c, &CM_c, &MIN_c, &PER_c, &FS_c,
        &ZER_c, &ONE_c, &TWO_c, &THR_c, &FOU_c,
        &FIV_c, &SIX_c, &SEV_c, &EIG_c, &NIN_c,
        &CO_c, &SC_c, &LT_c, &EQ_c, &GT_c, &QM_c, &AT_c,
        &A_c, &B_c, &C_c, &D_c, &E_c, &F_c, &G_c, &H_c,
        &I_c, &J_c, &K_c, &L_c, &M_c, &N_c, &O_c, &P_c,
        &Q_c, &R_c, &S_c, &T_c, &U_c, &V_c, &W_c, &X_c, &Y_c, &Z_c,
        &LB_c, &BS_c, &RB_c, &CF_c, &US_c, &GA_c,
        &a_c, &b_c, &c_c, &d_c, &e_c, &f_c, &g_c, &h_c,
        &i_c, &j_c, &k_c, &l_c, &m_c, &n_c, &o_c, &p_c,
        &q_c, &r_c, &s_c, &t_c, &u_c, &v_c, &w_c, &x_c, &y_c, &z_c,
        &LCB_c, &BAR_c, &RCB_c, &TIL_c
    };
}


GRvecFont::GRvecFont()
{
    vf_cellWidth = 8;
    vf_cellHeight = 14;
    vf_startChar = '!';
    vf_endChar = '~';
    vf_charset = new Character*[sizeof(GRcharset)/sizeof(Character*)];
    memcpy(vf_charset, GRcharset, sizeof(GRcharset));
}


// Return the size of the string in native pixels.  If text is 0,
// pass back the maximum size of a character.
//
void
GRvecFont::textExtent(const char *text, int *width, int *height,
    int *lines, int maxlines) const
{
    int xcount = 1, ycount = 1;
    if (text) {
        int count = 1;
        for (const char *s = text; *s; s++) {
            int i = charWidth(*s);
            count += i;
            if (!i) {
                if (count > xcount)
                    xcount = count;
                count = 1;
                if (*(s+1)) {
                    if (ycount == maxlines)
                        break;
                    ycount++;
                }
            }
        }
        if (count > xcount)
            xcount = count;
    }
    else
        xcount = vf_cellWidth;
    if (width)
        *width = xcount;
    if (height)
        *height = ycount*vf_cellHeight;
    if (lines)
        *lines = ycount;
}


// This returns the x offset of the start of the line.
//
int
GRvecFont::xoffset(const char *string, int xform, int pix_width,
    int lwid) const
{
    int xos = pix_width;
    if (xform & TXTF_HJC) {
        int xw = lineExtent(string);
        xos += ((lwid - xw)*pix_width)/2;
    }
    else if (xform & TXTF_HJR) {
        int xw = lineExtent(string);
        xos += (lwid - xw)*pix_width;
    }
    return (xos);
}


namespace {
    void tf_point(int[][3], int*, int*);
    void tf_set_matrix(int[][3], int, int, int);
}

// Render the string.  This assumes a coordinate system where the origin is
// the lower left corner.  The text is scaled to fit inside a box specified
// with width/height.  Watch out for integer scaling problems.
//
void
GRvecFont::renderText(GRdraw *context, const char *text, int x, int y,
    int xform, int width, int height, int maxlines) const
{
    if (!text || !*text)
        return;

    int lwid, lhei, numlines;
    textExtent(text, &lwid, &lhei, &numlines, maxlines);

    int Matrix[3][3];
    width /= lwid;
    height /= lhei;
    if (!width || !height)
        return;
    tf_set_matrix(Matrix, xform, width*lwid, height*lhei);

    int xos = xoffset(text, xform, width, lwid);
    int yos = numlines > 1 ? (numlines-1)*height*vf_cellHeight : 0;

    int lcnt = 0;
    for (const char *s = text; *s; s++) {
        Character *cp = entry(*s);
        if (cp) {
            for (int i = 0; i < cp->numstroke; i++) {
                Cstroke *stroke = &cp->stroke[i];
                int x0 = width*(stroke->cp[0].x - cp->ofst) + xos;
                int y0 = height*(vf_cellHeight - 1 - stroke->cp[0].y) + yos;

                tf_point(Matrix, &x0, &y0);

                for (int j = 1; j < stroke->numpts; j++) {
                    int tx1 = width*(stroke->cp[j].x - cp->ofst) + xos;
                    int ty1 = height*(vf_cellHeight - 1 - stroke->cp[j].y) +
                        yos;

                    tf_point(Matrix, &tx1, &ty1);
                    context->Line(x0 + x, y0 + y, tx1 + x, ty1 + y);

                    x0 = tx1;
                    y0 = ty1;
                }
            }
            xos += width*cp->width;
        }
        else if (*s == '\n') {
            xos = xoffset(s+1, xform, width, lwid);
            yos -= height*vf_cellHeight;
            if (++lcnt == maxlines)
                break;
        }
        else
            xos += width*vf_cellWidth;
    }
}


namespace {
    // Transform the point
    //
    void
    tf_point(int Matrix[][3], int *x, int *y)
    {
        bool ortho = (!Matrix[0][0] || !Matrix[0][1]);
        int i = *x*Matrix[0][0] + *y*Matrix[1][0];
        int j = *x*Matrix[0][1] + *y*Matrix[1][1];
        if (!ortho) {
            i = (int)(i * M_SQRT1_2);
            j = (int)(j * M_SQRT1_2);
        }
        *x = i + Matrix[2][0];
        *y = j + Matrix[2][1];
    }


    // Set up the transformation matrix for xform
    //
    void
    tf_set_matrix(int Matrix[][3], int xform, int wid, int hei)
    {
        Matrix[0][0] = 1;
        Matrix[0][1] = 0;
        Matrix[0][2] = 0;
        Matrix[1][0] = 0;
        Matrix[1][1] = 1;
        Matrix[1][2] = 0;
        Matrix[2][0] = 0;
        Matrix[2][1] = 0;
        Matrix[2][2] = 1;

        if (xform & TXTF_HJC)
            Matrix[2][0] += -wid/2;
        else if (xform & TXTF_HJR)
            Matrix[2][0] += -wid;
        if (xform & TXTF_VJC)
            Matrix[2][1] += -hei/2;
        else if (xform & TXTF_VJT)
            Matrix[2][1] += -hei;
        int shift = xform & TXTF_45;
        int mx = xform & TXTF_MX;
        int my = xform & TXTF_MY;
        xform &= TXTF_ROT;
        int i, j;
        if (!shift) {
            if (xform == 1) {
                // Rotate ccw by 90 degrees (0, 1)
                i = Matrix[0][0];
                Matrix[0][0] = -Matrix[0][1];
                Matrix[0][1] = i;
                i = Matrix[1][0];
                Matrix[1][0] = -Matrix[1][1];
                Matrix[1][1] = i;
                i = Matrix[2][0];
                Matrix[2][0] = -Matrix[2][1];
                Matrix[2][1] = i;
            }
            else if (xform == 2) {
                // Rotate ccw by 180 degrees (-1, 0)
                Matrix[0][0] = -Matrix[0][0];
                Matrix[0][1] = -Matrix[0][1];
                Matrix[1][0] = -Matrix[1][0];
                Matrix[1][1] = -Matrix[1][1];
                Matrix[2][0] = -Matrix[2][0];
                Matrix[2][1] = -Matrix[2][1];
            }
            else if (xform == 3) {
                // Rotate ccw by 270 degrees (0, -1)
                i = Matrix[0][0];
                Matrix[0][0] = Matrix[0][1];
                Matrix[0][1] = -i;
                i = Matrix[1][0];
                Matrix[1][0] = Matrix[1][1];
                Matrix[1][1] = -i;
                i = Matrix[2][0];
                Matrix[2][0] = Matrix[2][1];
                Matrix[2][1] = -i;
            }
        }
        else {
            if (xform == 0) {
                // Rotate ccw by 45 degrees (1, 1)
                i = Matrix[0][0] - Matrix[0][1];
                j = Matrix[0][1] + Matrix[0][0];
                Matrix[0][0] = i;
                Matrix[0][1] = j;
                i = Matrix[1][0] - Matrix[1][1];
                j = Matrix[1][1] + Matrix[1][0];
                Matrix[1][0] = i;
                Matrix[1][1] = j;
                i = Matrix[2][0] - Matrix[2][1];
                j = Matrix[2][1] + Matrix[2][0];
                Matrix[2][0] = (int)(i*M_SQRT1_2);
                Matrix[2][1] = (int)(j*M_SQRT1_2);
            }
            else if (xform == 1) {
                // Rotate ccw by 135 degrees (-1, 1)
                i = -Matrix[0][0] - Matrix[0][1];
                j = -Matrix[0][1] + Matrix[0][0];
                Matrix[0][0] = i;
                Matrix[0][1] = j;
                i = -Matrix[1][0] - Matrix[1][1];
                j = -Matrix[1][1] + Matrix[1][0];
                Matrix[1][0] = i;
                Matrix[1][1] = j;
                i = -Matrix[2][0] - Matrix[2][1];
                j = -Matrix[2][1] + Matrix[2][0];
                Matrix[2][0] = (int)(i*M_SQRT1_2);
                Matrix[2][1] = (int)(j*M_SQRT1_2);
            }
            else if (xform == 2) {
                // Rotate ccw by 225 degrees (-1, -1)
                i = -Matrix[0][0] + Matrix[0][1];
                j = -Matrix[0][1] - Matrix[0][0];
                Matrix[0][0] = i;
                Matrix[0][1] = j;
                i = -Matrix[1][0] + Matrix[1][1];
                j = -Matrix[1][1] - Matrix[1][0];
                Matrix[1][0] = i;
                Matrix[1][1] = j;
                i = -Matrix[2][0] + Matrix[2][1];
                j = -Matrix[2][1] - Matrix[2][0];
                Matrix[2][0] = (int)(i*M_SQRT1_2);
                Matrix[2][1] = (int)(j*M_SQRT1_2);
            }
            else {
                // Rotate ccw by 315 degrees (1, -1)
                i = Matrix[0][0] + Matrix[0][1];
                j = Matrix[0][1] - Matrix[0][0];
                Matrix[0][0] = i;
                Matrix[0][1] = j;
                i = Matrix[1][0] + Matrix[1][1];
                j = Matrix[1][1] - Matrix[1][0];
                Matrix[1][0] = i;
                Matrix[1][1] = j;
                i = Matrix[2][0] + Matrix[2][1];
                j = Matrix[2][1] - Matrix[2][0];
                Matrix[2][0] = (int)(i*M_SQRT1_2);
                Matrix[2][1] = (int)(j*M_SQRT1_2);
            }
        }
        // reflections go last
        if (my) {
            Matrix[0][1] = -Matrix[0][1];
            Matrix[1][1] = -Matrix[1][1];
            Matrix[2][1] = -Matrix[2][1];
        }
        if (mx) {
            Matrix[0][0] = -Matrix[0][0];
            Matrix[1][0] = -Matrix[1][0];
            Matrix[2][0] = -Matrix[2][0];
        }
    }
}


//
// Parser for character tables.  An entry for a character has the form
// (example for '!'):
//
// character !
// path 4,2 4,7
// path 4,9 4,10
// width 2
// offset 4
// # anything
//
// only the first char for each keyword is needed.  Any non-digit serves
// as a delimiter for the paths.
//

#define MAX_STROKES 6

namespace ginterf
{
    struct sCpair : public GRvecFont::Cpair
    {
        sCpair() { x = y = 0; }
        sCpair(char**);
    };

    struct sCstroke : public GRvecFont::Cstroke
    {
        sCstroke() { cp = 0; numpts = 0; }
        sCstroke(char*);
        void print(FILE*);
    };

    struct sCharacter : public GRvecFont::Character
    {
        sCharacter(GRvecFont::Cstroke *s, int ns, int w, int o);
        void print(FILE*);
    };
}


sCpair::sCpair(char **s)
{
    char *t = *s;
    while (*s && !isdigit(*t))
        t++;
    if (*t) {
        x = atoi(t);
        while (isdigit(*t))
            t++;
        while (*s && !isdigit(*t))
            t++;
        if (*t) {
            y = atoi(t);
            while (isdigit(*t))
                t++;
        }
    }
    *s = t;
}


sCstroke::sCstroke(char *s)
{
    char *t = s;
    int n = 0;
    for (;;) {
        while (*t && !isdigit(*t))
            t++;
        if (isdigit(*t)) {
            n++;;
            while (isdigit(*t))
                t++;
        }
        else
            break;
    }
    if (n & 1)
        n++;
    else if (!n) {
        cp = new sCpair[1];
        numpts = 1;
    }
    else {
        n >>= 1;
        cp = new GRvecFont::Cpair[n];
        numpts = n;
        for (n = 0; n < numpts; n++)
            cp[n] = sCpair(&s);
    }
}


void
sCstroke::print(FILE *fp)
{
    fprintf(fp, "path ");
    for (int i = 0; i < numpts; i++)
        fprintf(fp, " %d,%d", cp[i].x, cp[i].y);
    fprintf(fp, "\n");
}


sCharacter::sCharacter(GRvecFont::Cstroke *s, int nst, int w, int o)
{
    stroke = new sCstroke[nst];
    for (int i = 0; i < nst; i++)
        stroke[i] = s[i];
    numstroke = nst;
    width = w;
    ofst = o;
}


void
sCharacter::print(FILE *fp)
{
    for (int i = 0; i < numstroke; i++)
        (*(sCstroke*)&stroke[i]).print(fp);
    fprintf(fp, "width %d\n", width);
    fprintf(fp, "offset %d\n", ofst);
}


void
GRvecFont::parseChars(FILE *fp)
{
    int c = 0, width = 0, offset = 0;
    char *s;
    int strcnt = 0;
    sCstroke strokes[MAX_STROKES];
    char buf[256];
    while ((s = fgets(buf, 256, fp)) != 0) {
        while (isspace(*s))
            s++;
        char *tok = s;
        while (*s && !isspace(*s))
            s++;
        while (isspace(*s))
            s++;
        if (isupper(*tok))
            *tok = tolower(*tok);
        switch (*tok) {
        case 'c':
            if (c)
                vf_charset[c - vf_startChar] =
                    new sCharacter(strokes, strcnt, width, offset);
            strcnt = 0;
            width = 1;
            offset = 0;
            c = *s;
            if (c < vf_startChar || c > vf_endChar)
                c = 0;
            break;
        case 'p':
            if (strcnt < MAX_STROKES)
                strokes[strcnt++] = sCstroke(s);
            break;
        case 'w':
            width = atoi(s);
            break;
        case 'o':
            offset = atoi(s);
            break;
        }
    }
    if (c)
        vf_charset[c - vf_startChar] =
            new sCharacter(strokes, strcnt, width, offset);
}


void
GRvecFont::dumpFont(FILE *fp) const
{
    if (!fp)
        return;
    for (int i = 0; i < vf_endChar - vf_startChar; i++) {
        fprintf(fp, "character %c\n", i + vf_startChar);
        static_cast<sCharacter*>(vf_charset[i])->print(fp);
        fprintf(fp, "\n");
    }
}

