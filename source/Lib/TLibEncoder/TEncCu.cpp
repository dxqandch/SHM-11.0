/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2015, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

/** \file     TEncCu.cpp
\brief    Coding Unit (CU) encoder class
*/

#include <stdio.h>
#include "TEncTop.h"
#include "TEncCu.h"
#include "TEncAnalyze.h"
#include "TLibCommon/Debug.h"
#include <cmath>
#include <algorithm>
#include<fstream>
#include<iostream>
using namespace std;
int ne = 0, tcnt = 0, fr = 0;
int fdp[40][40], edp[40][40];
float dp[4] = { 0 }, dp1 = 0;
bool iscomplete = true;
#define depth 0
int cnt = 0;
int ctunum = 0;
int Strides[] = { 16, 8, 4, 2 };
bool flag1 = false;
vector<float>feature;
vector<double>feature2;
vector<double>sample0;
int x, y;
int r0 = -1;
//! \ingroup TLibEncoder
//! \{
// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

/**
\param    uhTotalDepth  total number of allowable depth  //�����������
\param    uiMaxWidth    largest CU width      ���CU����
\param    uiMaxHeight   largest CU height     ���CU�߶�
\param    chromaFormat  chroma format
*/
Void TEncCu::create(UChar uhTotalDepth, UInt uiMaxWidth, UInt uiMaxHeight, ChromaFormat chromaFormat)  //create internal buffers
{
	Int i;

	m_uhTotalDepth = uhTotalDepth + 1;
	m_ppcBestCU = new TComDataCU * [m_uhTotalDepth - 1];
	m_ppcTempCU = new TComDataCU * [m_uhTotalDepth - 1];

	m_ppcPredYuvBest = new TComYuv * [m_uhTotalDepth - 1];
	m_ppcResiYuvBest = new TComYuv * [m_uhTotalDepth - 1];
	m_ppcRecoYuvBest = new TComYuv * [m_uhTotalDepth - 1];
	m_ppcPredYuvTemp = new TComYuv * [m_uhTotalDepth - 1];
	m_ppcResiYuvTemp = new TComYuv * [m_uhTotalDepth - 1];
	m_ppcRecoYuvTemp = new TComYuv * [m_uhTotalDepth - 1];
	m_ppcOrigYuv = new TComYuv * [m_uhTotalDepth - 1];

	UInt uiNumPartitions;
	for (i = 0; i < m_uhTotalDepth - 1; i++)
	{
		uiNumPartitions = 1 << ((m_uhTotalDepth - i - 1) << 1);
		UInt uiWidth = uiMaxWidth >> i;
		UInt uiHeight = uiMaxHeight >> i;

		m_ppcBestCU[i] = new TComDataCU; m_ppcBestCU[i]->create(chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1));
		m_ppcTempCU[i] = new TComDataCU; m_ppcTempCU[i]->create(chromaFormat, uiNumPartitions, uiWidth, uiHeight, false, uiMaxWidth >> (m_uhTotalDepth - 1));

		m_ppcPredYuvBest[i] = new TComYuv; m_ppcPredYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
		m_ppcResiYuvBest[i] = new TComYuv; m_ppcResiYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);
		m_ppcRecoYuvBest[i] = new TComYuv; m_ppcRecoYuvBest[i]->create(uiWidth, uiHeight, chromaFormat);

		m_ppcPredYuvTemp[i] = new TComYuv; m_ppcPredYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
		m_ppcResiYuvTemp[i] = new TComYuv; m_ppcResiYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);
		m_ppcRecoYuvTemp[i] = new TComYuv; m_ppcRecoYuvTemp[i]->create(uiWidth, uiHeight, chromaFormat);

		m_ppcOrigYuv[i] = new TComYuv; m_ppcOrigYuv[i]->create(uiWidth, uiHeight, chromaFormat);
	}

	m_bEncodeDQP = false;
	m_stillToCodeChromaQpOffsetFlag = false;
	m_cuChromaQpOffsetIdxPlus1 = 0;
	m_bFastDeltaQP = false;

	// initialize partition order.��ʼ���ָ�˳��
	UInt* piTmp = &g_auiZscanToRaster[0];
	initZscanToRaster(m_uhTotalDepth, 1, 0, piTmp);
	initRasterToZscan(uiMaxWidth, uiMaxHeight, m_uhTotalDepth);

	// initialize conversion matrix from partition index to pel
	initRasterToPelXY(uiMaxWidth, uiMaxHeight, m_uhTotalDepth);
}

Void TEncCu::destroy()
{
	Int i;

	for (i = 0; i < m_uhTotalDepth - 1; i++)
	{
		if (m_ppcBestCU[i])
		{
			m_ppcBestCU[i]->destroy();      delete m_ppcBestCU[i];      m_ppcBestCU[i] = NULL;
		}
		if (m_ppcTempCU[i])
		{
			m_ppcTempCU[i]->destroy();      delete m_ppcTempCU[i];      m_ppcTempCU[i] = NULL;
		}
		if (m_ppcPredYuvBest[i])
		{
			m_ppcPredYuvBest[i]->destroy(); delete m_ppcPredYuvBest[i]; m_ppcPredYuvBest[i] = NULL;
		}
		if (m_ppcResiYuvBest[i])
		{
			m_ppcResiYuvBest[i]->destroy(); delete m_ppcResiYuvBest[i]; m_ppcResiYuvBest[i] = NULL;
		}
		if (m_ppcRecoYuvBest[i])
		{
			m_ppcRecoYuvBest[i]->destroy(); delete m_ppcRecoYuvBest[i]; m_ppcRecoYuvBest[i] = NULL;
		}
		if (m_ppcPredYuvTemp[i])
		{
			m_ppcPredYuvTemp[i]->destroy(); delete m_ppcPredYuvTemp[i]; m_ppcPredYuvTemp[i] = NULL;
		}
		if (m_ppcResiYuvTemp[i])
		{
			m_ppcResiYuvTemp[i]->destroy(); delete m_ppcResiYuvTemp[i]; m_ppcResiYuvTemp[i] = NULL;
		}
		if (m_ppcRecoYuvTemp[i])
		{
			m_ppcRecoYuvTemp[i]->destroy(); delete m_ppcRecoYuvTemp[i]; m_ppcRecoYuvTemp[i] = NULL;
		}
		if (m_ppcOrigYuv[i])
		{
			m_ppcOrigYuv[i]->destroy();     delete m_ppcOrigYuv[i];     m_ppcOrigYuv[i] = NULL;
		}
	}
	if (m_ppcBestCU)
	{
		delete[] m_ppcBestCU;
		m_ppcBestCU = NULL;
	}
	if (m_ppcTempCU)
	{
		delete[] m_ppcTempCU;
		m_ppcTempCU = NULL;
	}

	if (m_ppcPredYuvBest)
	{
		delete[] m_ppcPredYuvBest;
		m_ppcPredYuvBest = NULL;
	}
	if (m_ppcResiYuvBest)
	{
		delete[] m_ppcResiYuvBest;
		m_ppcResiYuvBest = NULL;
	}
	if (m_ppcRecoYuvBest)
	{
		delete[] m_ppcRecoYuvBest;
		m_ppcRecoYuvBest = NULL;
	}
	if (m_ppcPredYuvTemp)
	{
		delete[] m_ppcPredYuvTemp;
		m_ppcPredYuvTemp = NULL;
	}
	if (m_ppcResiYuvTemp)
	{
		delete[] m_ppcResiYuvTemp;
		m_ppcResiYuvTemp = NULL;
	}
	if (m_ppcRecoYuvTemp)
	{
		delete[] m_ppcRecoYuvTemp;
		m_ppcRecoYuvTemp = NULL;
	}
	if (m_ppcOrigYuv)
	{
		delete[] m_ppcOrigYuv;
		m_ppcOrigYuv = NULL;
	}

}

/** \param    pcEncTop      pointer of encoder class
*/
Void TEncCu::init(TEncTop* pcEncTop)
{
	m_pcEncCfg = pcEncTop;
	m_pcPredSearch = pcEncTop->getPredSearch();
	m_pcTrQuant = pcEncTop->getTrQuant();
	m_pcRdCost = pcEncTop->getRdCost();

#if SVC_EXTENSION
	m_ppcTEncTop = pcEncTop->getLayerEnc();
#endif

	m_pcEntropyCoder = pcEncTop->getEntropyCoder();
	m_pcBinCABAC = pcEncTop->getBinCABAC();

	m_pppcRDSbacCoder = pcEncTop->getRDSbacCoder();
	m_pcRDGoOnSbacCoder = pcEncTop->getRDGoOnSbacCoder();

	m_pcRateCtrl = pcEncTop->getRateCtrl();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
\param  pCtu pointer of CU data class
*/
Void TEncCu::compressCtu(TComDataCU* pCtu)
{
	iscomplete = true;
	feature.clear();
	feature2.clear();
	flag1 = false;
	// initialize CU data����ʼ��CU��Ԫ���ڱ���ǰ�����ڲ��������ͳ�ʼֵ��
	m_ppcBestCU[0]->initCtu(pCtu->getPic(), pCtu->getCtuRsAddr());       //�ú����������Pic��CTU������getCtuRsAddr()��ʾ���ø�CU�ĵ�ַ��ƫ�ƿ��Լ������ǰCU�ĵ�ַ
	m_ppcTempCU[0]->initCtu(pCtu->getPic(), pCtu->getCtuRsAddr());

#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
	m_disableILP = xCheckTileSetConstraint(pCtu);
	m_pcPredSearch->setDisableILP(m_disableILP);
#endif

	// analysis of CU
	int k = 0;
	DEBUG_STRING_NEW(sDebug)
		if (m_ppcBestCU[0]->getPic()->getLayerId() > 0)
		{
			k = (ne / m_ppcBestCU[0]->getPic()->getFrameWidthInCtus() != 0) && (ne % m_ppcBestCU[0]->getPic()->getFrameWidthInCtus() != 0) && ((ne + 1) % m_ppcBestCU[0]->getPic()->getFrameWidthInCtus() != 0);
			if (k)
			{
				if (fr > 0)
				{
					const float dd[4][4] = { {65.4777,24.2592,7.15004,3.11309},{23.4784,45.9987,17.1118,13.3924},{14.3684,35.7004,25.2922,23.9471},{13.8702,28.0005,17.961,40.9575} };
					const float ave[4] = { 32.5502,34.0932,15.5657,17.7909 };
					const float dep[4] = { 29.8235,33.5833,15.2797,21.3135 };
					float pp[4] = { 1 }, m = 1, n = 1, s = 0;
					int a[5] = { 0 };
					int i = 0, j = 0;
					if (y - 1 < 0)
					{
						edp[x][y - 1] = 0;
					}
					a[0] = fdp[x][y];                        //��ȡ��ǰCU����Ⱥͱ�֡�ο����뵥Ԫ��Ⱥ�ǰһ֡�ο����뵥Ԫ���
					a[1] = edp[x - 1][y - 1];                    //��������
					a[2] = edp[x - 1][y];                      //������
					a[3] = edp[x - 1][y + 1];                    //��������
					a[4] = edp[x][y - 1];                      //������
					ofstream ofs;
					ofs.open("depth.txt", ios::app);
					ofs << a[0] << "\t" << edp[x][y] << "\t" << a[1] << "\t" << a[2] << "\t" << a[3] << "\t" << a[4] << endl;
					for (i = 0; i < 4; i++)
					{
						m = n = 1;
						for (j = 0; j < 5; j++)
						{
							m = m * dd[i][a[j]];
							n = n * ave[a[j]];
						}
						pp[i] = m * dep[i] / n;

					}
					for (i = 0; i < 4; i++)
						s = s + pp[i];
					for (i = 0; i < 4; i++)
					{
						dp[i] = pp[i] / s;
					}
					dp1 = dp[0] * 0 + dp[1] * 1 + dp[2] * 2 + dp[3] * 3;
					feature2.push_back(dp1);
				}
			}
			if (!(k - 1))
			{
				if (fr > 0)
				{
					flag1 = true;
				}
			}
		}
	if (m_ppcBestCU[0]->getPic()->getLayerId() > 0)
	{
		if (flag1)
		{
			ofstream ofs;
			ofs.open("depth1.txt", ios::app);
			ofs << r0 << endl;
		}
	}
	xCompressCU(m_ppcBestCU[0], m_ppcTempCU[0], 0 DEBUG_STRING_PASS_INTO(sDebug));
	DEBUG_STRING_OUTPUT(std::cout, sDebug)

		/*************************���0*********************************/
		if (m_ppcBestCU[0]->getPic()->getLayerId() > 0)
		{
			int max = 0, m = -1, n = 0, d[16][16] = { 0 }, a[5] = { 0 };
			int iCount = 0;
			int iWidthInpart = MAX_CU_SIZE >> 2;                                            //16
			for (int i = 0; i < pCtu->getTotalNumPart(); i++)
			{
				if ((iCount & (iWidthInpart - 1)) == 0)
				{
					m++;
					n = 0;
				}
				d[m][n++] = pCtu->getDepth(g_auiRasterToZscan[i]);                        //��ȡ���д�С�Ŀ�
				iCount++;
			}
			int strides = Strides[depth];
			int cnt_ctus = m_ppcBestCU[0]->getPic()->getFrameWidthInCtus();		// һ����32*32CU�ĸ���
			for (int i = 0; i < 16; i = i + strides)
			{
				for (int j = 0; j < 16; j = j + strides)
				{
					max = 0;
					for (m = i; m < i + strides; m++)
					{
						for (n = j; n < j + strides; n++)
						{
							if (max < d[m][n]);
							max = d[m][n];
						}
					}
					x = ne / cnt_ctus;
					y = ne % cnt_ctus;
					edp[x][y] = max;
					r0 = max;

				}
			}
			ne++;
			if (ne % (m_ppcBestCU[0]->getPic()->getFrameWidthInCtus() * m_ppcBestCU[0]->getPic()->getFrameHeightInCtus()) == 0)   //һ֡������
			{
				for (int i = 0; i < (m_ppcBestCU[0]->getPic()->getFrameHeightInCtus()); i++)
				{
					for (int j = 0; j < (m_ppcBestCU[0]->getPic()->getFrameWidthInCtus()); j++)
					{
						fdp[i][j] = edp[i][j];
					}
				}
				ne = 0;
				fr++;
			}
		}

	/****************************************************************************************************************/
#if ADAPTIVE_QP_SELECTION
	if (m_pcEncCfg->getUseAdaptQpSelect())
	{
		if (pCtu->getSlice()->getSliceType() != I_SLICE) //IIII
		{
			xCtuCollectARLStats(pCtu);
		}
	}
#endif

#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
	xVerifyTileSetConstraint(pCtu);
#endif
}
/** \param  pCtu  pointer of CU data class
*/
Void TEncCu::encodeCtu(TComDataCU* pCtu)
{
	if (pCtu->getSlice()->getPPS()->getUseDQP())
	{
		setdQPFlag(true);
	}

	if (pCtu->getSlice()->getUseChromaQpAdj())
	{
		setCodeChromaQpAdjFlag(true);
	}

	// Encode CU data
	xEncodeCU(pCtu, 0, 0);
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
//! Derive small set of test modes for AMP encoder speed-up
#if AMP_ENC_SPEEDUP
#if AMP_MRG
Void TEncCu::deriveTestModeAMP(TComDataCU* pcBestCU, PartSize eParentPartSize, Bool& bTestAMP_Hor, Bool& bTestAMP_Ver, Bool& bTestMergeAMP_Hor, Bool& bTestMergeAMP_Ver)
#else
Void TEncCu::deriveTestModeAMP(TComDataCU* pcBestCU, PartSize eParentPartSize, Bool& bTestAMP_Hor, Bool& bTestAMP_Ver)
#endif
{
	if (pcBestCU->getPartitionSize(0) == SIZE_2NxN)
	{
		bTestAMP_Hor = true;
	}
	else if (pcBestCU->getPartitionSize(0) == SIZE_Nx2N)
	{
		bTestAMP_Ver = true;
	}
	else if (pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->getMergeFlag(0) == false && pcBestCU->isSkipped(0) == false)
	{
		bTestAMP_Hor = true;
		bTestAMP_Ver = true;
	}

#if AMP_MRG
	//! Utilizing the partition size of parent PU
	if (eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N)
	{
		bTestMergeAMP_Hor = true;
		bTestMergeAMP_Ver = true;
	}

	if (eParentPartSize == NUMBER_OF_PART_SIZES) //! if parent is intra
	{
		if (pcBestCU->getPartitionSize(0) == SIZE_2NxN)
		{
			bTestMergeAMP_Hor = true;
		}
		else if (pcBestCU->getPartitionSize(0) == SIZE_Nx2N)
		{
			bTestMergeAMP_Ver = true;
		}
	}

	if (pcBestCU->getPartitionSize(0) == SIZE_2Nx2N && pcBestCU->isSkipped(0) == false)
	{
		bTestMergeAMP_Hor = true;
		bTestMergeAMP_Ver = true;
	}

	if (pcBestCU->getWidth(0) == 64)
	{
		bTestAMP_Hor = false;
		bTestAMP_Ver = false;
	}
#else
	//! Utilizing the partition size of parent PU
	if (eParentPartSize >= SIZE_2NxnU && eParentPartSize <= SIZE_nRx2N)
	{
		bTestAMP_Hor = true;
		bTestAMP_Ver = true;
	}

	if (eParentPartSize == SIZE_2Nx2N)
	{
		bTestAMP_Hor = false;
		bTestAMP_Ver = false;
	}
#endif
}
#endif


// ====================================================================================================================
// Protected member functions
// ====================================================================================================================
/** Compress a CU block recursively with enabling sub-CTU-level delta QP
*  - for loop of QP value to compress the current CU with all possible QP
*/
#if AMP_ENC_SPEEDUP
Void TEncCu::xCompressCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, const UInt uiDepth DEBUG_STRING_FN_DECLARE(sDebug_), PartSize eParentPartSize)
#else
Void TEncCu::xCompressCU(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, const UInt uiDepth)
#endif
{
	TComPic* pcPic = rpcBestCU->getPic();
	DEBUG_STRING_NEW(sDebug)
		const TComPPS& pps = *(rpcTempCU->getSlice()->getPPS());
	const TComSPS& sps = *(rpcTempCU->getSlice()->getSPS());

	// These are only used if getFastDeltaQp() is true
	const UInt fastDeltaQPCuMaxSize = Clip3(sps.getMaxCUHeight() >> sps.getLog2DiffMaxMinCodingBlockSize(), sps.getMaxCUHeight(), 32u);

	// get Original YUV data from picture(��ͼ���л�ȡԭʼYUV��Ϣ)
	m_ppcOrigYuv[uiDepth]->copyFromPicYuv(pcPic->getPicYuvOrg(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu());


	/**************************************************************��ȡYUV��������Ϣ(����ֵ)*******************************************************************************/
	if (rpcBestCU->getPic()->getLayerId() > 0)
	{
		if (uiDepth == depth)                                              //�ж����0һ�����ǲ���������
		{
			int i, j, x1 = 0, p[64][64];
			UInt uiPartSize = rpcBestCU->getWidth(0);
			const ComponentID compID = ComponentID(0);
			const Int Width = uiPartSize >> m_ppcOrigYuv[uiDepth]->getComponentScaleX(compID);
			const Pel* pSrc0 = m_ppcOrigYuv[uiDepth]->getAddr(compID, 0, Width);
			const Int  iSrc0Stride = m_ppcOrigYuv[uiDepth]->getStride(compID);
			int cnt1 = 0;
			int cnt2 = 0;
			for (i = 0; i < Width; i++)
			{
				for (j = 0; j < Width; j++)
				{
					p[i][j] = pSrc0[j];
					if (j == Width - 1 && p[i][j] == 0)
						cnt1++;
					if (i == Width - 1 && p[i][j] == 0)
						cnt2++;
				}
				pSrc0 += iSrc0Stride;
				if (cnt1 >= Width || cnt2 >= Width)
				{
					iscomplete = false;
				}
			}
		}
		/*if (uiDepth == 0)                                      //�ж����1һ�����ǲ���������
		{
			int i, j;
			UInt uiPartSize = rpcBestCU->getWidth(0);
			const ComponentID compID = ComponentID(0);
			const Int Width = uiPartSize >> m_ppcOrigYuv[uiDepth]->getComponentScaleX(compID);
			const Pel* pSrc0 = m_ppcOrigYuv[uiDepth]->getAddr(compID, 0, Width);
			const Int  iSrc0Stride = m_ppcOrigYuv[uiDepth]->getStride(compID);
			int p[64][64];
			int cnt1 = 0;
			for (i = 0; i < Width; i++)
			{
				for (j = 0; j < Width; j++)
				{
					p[i][j] = pSrc0[j];
					if (p[i][j] == 0)
						cnt1++;
				}
				pSrc0 += iSrc0Stride;
			}
			if (cnt1 >= 64)
				iscomplete = false;
		}*/
		if (uiDepth == depth)
		{
			if (flag1)
			{
				int n = 0, n1 = 0, i, j;
				float  s = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0, s6 = 0, s7 = 0, s8 = 0, D1 = 0;
				float  subsd[12] = { 0 };
				float x1 = 0, u = 0, u1 = 0, u2 = 0, u3 = 0, u4 = 0, ave = 0, ave1 = 0, ave2 = 0, ave3 = 0, ave4 = 0, nmse1 = 0, sd1 = 0, sd2 = 0, sd3 = 0;
				float var1 = 0, var2 = 0, var3 = 0, var4 = 0, var5 = 0, var6 = 0, QP = 0, nmse = 0;
				int dx[4] = { 0 }, dy[4] = { 0 }, dtx[4] = { 0 }, dty[4] = { 0 };
				int avegd[4] = { 0 }, aveGDN[4] = { 0 };
				float aveggd[4] = { 0 };
				float m = 0, sumgd = 0;
				float subgd = 0;
				UInt uiPartSize = rpcBestCU->getWidth(0);
				const ComponentID compID = ComponentID(0);
				const Int Width = uiPartSize >> m_ppcOrigYuv[uiDepth]->getComponentScaleX(compID);
				const Pel* pSrc0 = m_ppcOrigYuv[uiDepth]->getAddr(compID, 0, Width);
				const Int  iSrc0Stride = m_ppcOrigYuv[uiDepth]->getStride(compID);
				int p[64][64], p1[32][32], p2[32][32], p3[32][32], p4[32][32], p5[32][32], p6[64][64];
				int l1[64][64];
				map<int, double> ent;
				for (i = 0; i < Width; i++)
				{
					for (j = 0; j < Width; j++)
					{
						p[i][j] = pSrc0[j];
						x1 += pSrc0[j];
						ave += pSrc0[j];
						ent[p[i][j]]++;
					}
					pSrc0 += iSrc0Stride;
				}
				for (i = 0; i < Width; i++)
				{
					for (j = 0; j < Width; j++)
					{
						p6[i][j] = p[i][j];
					}
				}
				n = Width * Width;
				u = x1 / n;
				x1 = 0;
				/*****����****/
				for (i = 0; i < Width; i++)
				{
					for (j = 0; j < Width; j++)
					{
						x1 = x1 + pow((p[i][j] - u), 2);
					}
				}
				s = x1 / n;
				if (s > 0)
				{
					s1 = log10(s);
				}
				else
					s1 = 0;
				/*****��Ϣ��****/
				double ents = 0, allent = 0;
				map<int, double>::iterator iter;
				for (iter = ent.begin(); iter != ent.end(); iter++)
				{

					iter->second /= (n * 1.0);
					ents = (-1) * (iter->second) * (log(iter->second) / log(2.0));
					allent += ents;
				}
				if (allent > 0)
				{
					s2 = log10(allent);
				}
				else
					s2 = 0;
				/****�ӿ鸴�Ӷȷ���*****/
				//�ӿ鸴�Ӷ�0
				for (i = 0; i < Width / 2; i++)
				{
					for (j = 0; j < Width / 2; j++)
					{
						p1[i][j] = p[i][j];
						ave1 += p1[i][j];
						subsd[0] += pow(p[i][j], 2);
						subsd[1] += p[i][j];
					}
				}
				n1 = (Width / 2) * (Width / 2);                                   //��ǰ��CU�Ĵ�С
				u1 = ave1 / n1;                                              //���ص�ƽ��ֵ    
				subsd[0] = subsd[0] / n1;
				subsd[1] = subsd[1] / n1;
				subsd[2] = pow(abs(subsd[0] - pow(subsd[1], 2)), 0.5);
				ave1 = 0;
				for (i = 0; i < Width / 2; i++)
				{
					for (j = 0; j < Width / 2; j++)
					{
						ave1 = ave1 + pow((p1[i][j] - u1), 2);                //����CU�����ط���
					}
				}
				var1 = ave1 / n1;                                         //��һ������  
				//�ӿ鸴�Ӷ�1
				for (i = Width / 2; i < Width; i++)
				{
					for (j = 0; j < Width / 2; j++)
					{
						p2[i][j] = p[i][j];
						ave2 += p2[i][j];
						subsd[3] += pow(p[i][j], 2);
						subsd[4] += p[i][j];
					}
				}
				n1 = (Width / 2) * (Width / 2);                                   //��ǰ��CU�Ĵ�С                
				u2 = ave2 / n1;                                              //���ص�ƽ��ֵ    
				subsd[3] = subsd[3] / n1;
				subsd[4] = subsd[4] / n1;
				subsd[5] = pow(abs(subsd[3] - pow(subsd[4], 2)), 0.5);
				ave2 = 0;
				for (i = Width / 2; i < Width; i++)
				{
					for (j = 0; j < Width / 2; j++)
					{
						ave2 = ave2 + pow((p2[i][j] - u2), 2);                //����CU�����ط���
					}
				}
				var2 = ave2 / n1;                                         //��һ������  
				//�ӿ鸴�Ӷ�2
				for (i = 0; i < Width / 2; i++)
				{
					for (j = Width / 2; j < Width; j++)
					{
						p3[i][j] = p[i][j];
						ave3 += p3[i][j];
						subsd[6] += pow(p[i][j], 2);
						subsd[7] += p[i][j];
					}
				}
				n1 = (Width / 2) * (Width / 2);                                   //��ǰ��CU�Ĵ�С
				u3 = ave3 / n1;                                              //���ص�ƽ��ֵ    
				subsd[6] = subsd[6] / n1;
				subsd[7] = subsd[7] / n1;
				subsd[8] = pow(abs(subsd[6] - pow(subsd[7], 2)), 0.5);
				ave3 = 0;
				for (i = 0; i < Width / 2; i++)
				{
					for (j = Width / 2; j < Width; j++)
					{
						ave3 = ave3 + pow((p3[i][j] - u3), 2);                //����CU�����ط���
					}
				}
				var3 = ave3 / n1;                                         //��һ������  
				//�ӿ鸴�Ӷ�3
				for (i = Width / 2; i < Width; i++)
				{
					for (j = Width / 2; j < Width; j++)
					{
						p4[i][j] = p[i][j];
						ave4 += p4[i][j];
						subsd[9] += pow(p[i][j], 2);
						subsd[10] += p[i][j];
					}
				}
				n1 = (Width / 2) * (Width / 2);                                   //��ǰ��CU�Ĵ�С
				u4 = ave4 / n1;                                              //���ص�ƽ��ֵ 
				subsd[9] = subsd[9] / n1;
				subsd[10] = subsd[10] / n1;
				subsd[11] = pow(abs(subsd[9] - pow(subsd[11], 2)), 0.5);
				D1 = subsd[0] > subsd[1] ? subsd[0] : subsd[1] > subsd[2] ? subsd[1] : subsd[2] > subsd[3] ? subsd[2] : subsd[3];
				s6 = log10(D1);
				ave4 = 0;
				for (i = Width / 2; i < Width; i++)
				{
					for (j = Width / 2; j < Width; j++)
					{
						ave4 = ave4 + pow((p4[i][j] - u4), 2);                //����CU�����ط���
					}
				}
				var4 = ave4 / n1;                                         //��һ������  
				//�ӿ鸴�Ӷ�4
				var5 = (var1 + var2 + var3 + var4) / 4;
				var6 = (pow((var1 - var5), 2) + pow((var2 - var5), 2) + pow((var3 - var5), 2) + pow((var4 - var5), 2)) / 4;
				if (var6 > 0)
				{
					s3 = log(var6) / log(10);
				}
				else
					s3 = 0;
				/*****�õ�����QP****/
				QP = m_ppcBestCU[0]->getQP(0);
				s4 = log10(QP);

				/****����NMSE(���ھ������)****/
				int sum = 0, num = 0;
				for (i = 1; i < Width - 1; i++)
				{
					for (j = 1; j < Width - 1; j++)
					{
						l1[i][j] = (p6[i - 1][j - 1] + p6[i - 1][j] + p6[i - 1][j + 1] + p6[i][j - 1] + p6[i][j + 1] + p6[i + 1][j - 1] + p6[i + 1][j] + p6[i + 1][j + 1]) / 8;
						nmse1 += pow((p6[i][j] - l1[i][j]), 2);
					}
				}
				nmse = nmse1 / ((Width - 2) * (Width - 2));                       //���ھ������
				if (nmse > 0)
				{
					s5 = log10(nmse);
				}
				else
					s5 = 0;
				nmse1 = 0;
				/****��׼��****/
				for (i = 0; i < Width - 1; i++)
				{
					for (j = 0; j < Width - 1; j++)
					{
						sd1 += pow(p[i][j], 2);
						sd2 += p[i][j];
					}
				}
				sd1 = sd1 / ((Width - 1) * (Width - 1));
				sd2 = sd2 / ((Width - 1) * (Width - 1));
				sd3 = pow(sd1 - pow(sd2, 2), 0.5);
				s7 = log10(sd3);
				/****ƽ�������ݶ�*****/
				//�Ӹ��Ӷ�0
				avegd[0] = 0;
				for (i = 1; i < (Width / 2) - 1; i++)
				{
					for (j = 1; j < (Width / 2) - 1; j++)
					{
						dx[0] = p[i][j - 1] - p[i][j + 1];
						dy[0] = p[i - 1][j] - p[i + 1][j];
						dtx[0] = p[i - 1][j - 1] - p[i + 1][j + 1];
						dty[0] = p[i - 1][j + 1] - p[i + 1][j - 1];
						aveGDN[0] = abs(dx[0]) + abs(dy[0]) + abs(dtx[0]) + abs(dty[0]);
						avegd[0] += aveGDN[0];
					}
				}
				aveggd[0] = avegd[0] / ((n1 - 2) * 1.0);
				//�ӿ�1
				avegd[1] = 0;
				for (i = (Width / 2) + 1; i < Width - 1; i++)
				{
					for (j = 1; j < (Width / 2) - 1; j++)
					{

						dx[1] = p[i][j - 1] - p[i][j + 1];
						dy[1] = p[i - 1][j] - p[i + 1][j];
						dtx[1] = p[i - 1][j - 1] - p[i + 1][j + 1];
						dty[1] = p[i - 1][j + 1] - p[i + 1][j - 1];
						aveGDN[1] = abs(dx[1]) + abs(dy[1]) + abs(dtx[1]) + abs(dty[1]);
						avegd[1] += aveGDN[1];
					}
				}
				aveggd[1] = avegd[1] / (n1 - 2);
				//�ӿ鸴�Ӷ�2
				avegd[2] = 0;
				for (i = 1; i < (Width / 2) - 1; i++)
				{
					for (j = (Width / 2) + 1; j < Width - 1; j++)
					{

						dx[2] = p[i][j - 1] - p[i][j + 1];
						dy[2] = p[i - 1][j] - p[i + 1][j];
						dtx[2] = p[i - 1][j - 1] - p[i + 1][j + 1];
						dty[2] = p[i - 1][j + 1] - p[i + 1][j - 1];
						aveGDN[2] = abs(dx[2]) + abs(dy[2]) + abs(dtx[2]) + abs(dty[2]);
						avegd[2] += aveGDN[2];
					}
				}
				aveggd[2] = avegd[2] / (n1 - 2);
				avegd[3] = 0;
				for (i = (Width / 2) + 1; i < Width - 1; i++)
				{
					for (j = (Width / 2) + 1; j < (Width - 1); j++)
					{

						dx[3] = p[i][j - 1] - p[i][j + 1];
						dy[3] = p[i - 1][j] - p[i + 1][j];
						dtx[3] = p[i - 1][j - 1] - p[i + 1][j + 1];
						dty[3] = p[i - 1][j + 1] - p[i + 1][j - 1];
						aveGDN[3] = abs(dx[3]) + abs(dy[3]) + abs(dtx[3]) + abs(dty[3]);
						avegd[3] += aveGDN[3];
					}
				}
				aveggd[3] = avegd[3] / (n1 - 2);
				m = (aveggd[0] + aveggd[1] + aveggd[2] + aveggd[3]) / 4.0;
				for (int i = 0; i < 4; i++) {
					sumgd += pow((aveggd[i] - m), 2);
				}
				subgd = sumgd / 4.0;
				if (subgd > 0)
				{
					s8 = log10(subgd);
				}
				else
					s8 = 0;
				//��ӡ
				feature2.push_back(s1);
				feature2.push_back(s2);
				feature2.push_back(s3);
				feature2.push_back(s4);
				feature2.push_back(s5);
				/*feature2.push_back(s6);
				feature2.push_back(s7);*/
				feature2.push_back(s8);
				sample0 = feature2;
				ofstream ofs1;
				ofs1.open("features.txt", ios::app);
				ofs1 << setiosflags(ios::fixed) << setprecision(3);
				for (i = 0; i < feature2.size(); i++)
				{
					ofs1 << feature2[i] << "\t";
				}
				ofs1 << endl;
				feature2.clear();
			}
		}
	}

	/*****************************************************************************************************************/

	// variable for Cbf fast mode PU decision
	Bool    doNotBlockPu = true;
	Bool    earlyDetectionSkipMode = false;

	const UInt uiLPelX = rpcBestCU->getCUPelX();
	const UInt uiRPelX = uiLPelX + rpcBestCU->getWidth(0) - 1;
	const UInt uiTPelY = rpcBestCU->getCUPelY();
	const UInt uiBPelY = uiTPelY + rpcBestCU->getHeight(0) - 1;
	const UInt uiWidth = rpcBestCU->getWidth(0);
	//// ���뵱ǰCU����ȣ�����Ե�ǰCU��QP��������Ƕ�ÿ��CU����Ӧ�ĸı�QP����ֱ����֮ǰslice�����QP
	Int iBaseQP = xComputeQP(rpcBestCU, uiDepth);   //xComputeQP�ó���������
	Int iMinQP;   //��С����������
	Int iMaxQP;   //������������
	Bool isAddLowestQP = false;

	const UInt numberValidComponents = rpcBestCU->getPic()->getNumberValidComponents();

	if (uiDepth <= pps.getMaxCuDQPDepth())    //Ϊ�˻�����������ķ�Χ��iMinQP��iMaxQP��
	{
		Int idQP = m_pcEncCfg->getMaxDeltaQP();  //�������������QP
		iMinQP = Clip3(-sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP - idQP);
		iMaxQP = Clip3(-sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP + idQP);
	}
	else
	{
		iMinQP = rpcTempCU->getQP(0);
		iMaxQP = rpcTempCU->getQP(0);
	}

	if (m_pcEncCfg->getUseRateCtrl())
	{
		iMinQP = m_pcRateCtrl->getRCQP();
		iMaxQP = m_pcRateCtrl->getRCQP();
	}

	// transquant-bypass (TQB) processing loop variable initialisation ---

	const Int lowestQP = iMinQP; // For TQB, use this QP which is the lowest non TQB QP tested (rather than QP'=0) - that way delta QPs are smaller, and TQB can be tested at all CU levels.

	if ((pps.getTransquantBypassEnableFlag()))
	{
		isAddLowestQP = true; // mark that the first iteration is to cost TQB mode.
		iMinQP = iMinQP - 1;  // increase loop variable range by 1, to allow testing of TQB mode along with other QPs
		if (m_pcEncCfg->getCUTransquantBypassFlagForceValue())
		{
			iMaxQP = iMinQP;
		}
	}

	TComSlice* pcSlice = rpcTempCU->getPic()->getSlice(rpcTempCU->getPic()->getCurrSliceIdx());
	const Bool bBoundary = !(uiRPelX < sps.getPicWidthInLumaSamples() && uiBPelY < sps.getPicHeightInLumaSamples());

	if (!bBoundary)
	{
#if HIGHER_LAYER_IRAP_SKIP_FLAG
		if (m_pcEncCfg->getSkipPictureAtArcSwitch() && m_pcEncCfg->getAdaptiveResolutionChange() > 0 && pcSlice->getLayerId() == 1 && pcSlice->getPOC() == m_pcEncCfg->getAdaptiveResolutionChange())
		{
			Int iQP = iBaseQP;
			const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP);

			if (bIsLosslessMode)
			{
				iQP = lowestQP;
			}

			rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

			xCheckRDCostMerge2Nx2N(rpcBestCU, rpcTempCU, &earlyDetectionSkipMode, true);
		}
		else
		{
#endif

#if ENCODER_FAST_MODE
			Bool testInter = true;
			if (rpcBestCU->getPic()->getLayerId() > 0)
			{
				if (pcSlice->getSliceType() == P_SLICE && pcSlice->getNumRefIdx(REF_PIC_LIST_0) == pcSlice->getActiveNumILRRefIdx())
				{
					testInter = false;
				}
				if (pcSlice->getSliceType() == B_SLICE && pcSlice->getNumRefIdx(REF_PIC_LIST_0) == pcSlice->getActiveNumILRRefIdx() && pcSlice->getNumRefIdx(REF_PIC_LIST_1) == pcSlice->getActiveNumILRRefIdx())
				{
					testInter = false;
				}
			}
#endif
			for (Int iQP = iMinQP; iQP <= iMaxQP; iQP++)
			{
				const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP);

				if (bIsLosslessMode)
				{
					iQP = lowestQP;
				}

				m_cuChromaQpOffsetIdxPlus1 = 0;
				if (pcSlice->getUseChromaQpAdj())
				{
					/* Pre-estimation of chroma QP based on input block activity may be performed
					* here, using for example m_ppcOrigYuv[uiDepth] */
					/* To exercise the current code, the index used for adjustment is based on
					* block position
					*/
					Int lgMinCuSize = sps.getLog2MinCodingBlockSize() +
						std::max<Int>(0, sps.getLog2DiffMaxMinCodingBlockSize() - Int(pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth()));
					m_cuChromaQpOffsetIdxPlus1 = ((uiLPelX >> lgMinCuSize) + (uiTPelY >> lgMinCuSize)) % (pps.getPpsRangeExtension().getChromaQpOffsetListLen() + 1);
				}

				rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
				/*******************************************֡��****************************************/
				// do inter modes, SKIP and 2Nx2N
#if ENCODER_FAST_MODE == 1
				if (rpcBestCU->getSlice()->getSliceType() != I_SLICE && testInter)
#else
				if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)
#endif
				{
					// 2Nx2N
					if (m_pcEncCfg->getUseEarlySkipDetection())
					{
						xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug));
						rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);//by Competition for inter_2Nx2N
					}
					// SKIP
					xCheckRDCostMerge2Nx2N(rpcBestCU, rpcTempCU DEBUG_STRING_PASS_INTO(sDebug), &earlyDetectionSkipMode);//by Merge for inter_2Nx2N
					rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

#if ENCODER_FAST_MODE == 2
					if (testInter)
					{
#endif
						if (!m_pcEncCfg->getUseEarlySkipDetection())
						{
							// 2Nx2N, NxN
							xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug));
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
							if (m_pcEncCfg->getUseCbfFastMode())
							{
								doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
							}
						}
#if ENCODER_FAST_MODE == 2
					}
#endif
				}

				if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
				{
					iQP = iMinQP;
				}
			}

			if (!earlyDetectionSkipMode)
			{
				for (Int iQP = iMinQP; iQP <= iMaxQP; iQP++)
				{
					const Bool bIsLosslessMode = isAddLowestQP && (iQP == iMinQP); // If lossless, then iQP is irrelevant for subsequent modules.

					if (bIsLosslessMode)
					{
						iQP = lowestQP;
					}

					rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

					// do inter modes, NxN, 2NxN, and Nx2N
#if ENCODER_FAST_MODE
					if (rpcBestCU->getSlice()->getSliceType() != I_SLICE && testInter)
#else
					if (rpcBestCU->getSlice()->getSliceType() != I_SLICE)
#endif
					{
						// 2Nx2N, NxN

						if (!((rpcBestCU->getWidth(0) == 8) && (rpcBestCU->getHeight(0) == 8)))
						{
							if (uiDepth == sps.getLog2DiffMaxMinCodingBlockSize() && doNotBlockPu)
							{
								xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug));
								rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
							}
						}

						if (doNotBlockPu)
						{
							xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_Nx2N DEBUG_STRING_PASS_INTO(sDebug));
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
							if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_Nx2N)
							{
								doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
							}
						}
						if (doNotBlockPu)
						{
							xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxN DEBUG_STRING_PASS_INTO(sDebug));
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
							if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxN)
							{
								doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
							}
						}

						//! Try AMP (SIZE_2NxnU, SIZE_2NxnD, SIZE_nLx2N, SIZE_nRx2N)
						if (sps.getUseAMP() && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize())
						{
#if AMP_ENC_SPEEDUP
							Bool bTestAMP_Hor = false, bTestAMP_Ver = false;

#if AMP_MRG
							Bool bTestMergeAMP_Hor = false, bTestMergeAMP_Ver = false;

							deriveTestModeAMP(rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver, bTestMergeAMP_Hor, bTestMergeAMP_Ver);
#else
							deriveTestModeAMP(rpcBestCU, eParentPartSize, bTestAMP_Hor, bTestAMP_Ver);
#endif

							//! Do horizontal AMP
							if (bTestAMP_Hor)
							{
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug));
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
									if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU)
									{
										doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
									}
								}
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug));
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
									if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD)
									{
										doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
									}
								}
							}
#if AMP_MRG
							else if (bTestMergeAMP_Hor)
							{
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnU DEBUG_STRING_PASS_INTO(sDebug), true);
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
									if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnU)
									{
										doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
									}
								}
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnD DEBUG_STRING_PASS_INTO(sDebug), true);
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
									if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_2NxnD)
									{
										doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
									}
								}
							}
#endif

							//! Do horizontal AMP
							if (bTestAMP_Ver)
							{
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug));
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
									if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N)
									{
										doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
									}
								}
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug));
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
								}
							}
#if AMP_MRG
							else if (bTestMergeAMP_Ver)
							{
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nLx2N DEBUG_STRING_PASS_INTO(sDebug), true);
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
									if (m_pcEncCfg->getUseCbfFastMode() && rpcBestCU->getPartitionSize(0) == SIZE_nLx2N)
									{
										doNotBlockPu = rpcBestCU->getQtRootCbf(0) != 0;
									}
								}
								if (doNotBlockPu)
								{
									xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nRx2N DEBUG_STRING_PASS_INTO(sDebug), true);
									rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
								}
							}
#endif

#else
							xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnU);
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
							xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_2NxnD);
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
							xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nLx2N);
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

							xCheckRDCostInter(rpcBestCU, rpcTempCU, SIZE_nRx2N);
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

#endif
						}
					}
					/******************************************************֡��************************************************************/
					// do normal intra modes
					// speedup for inter frames
					Double intraCost = 0.0;

					if ((rpcBestCU->getSlice()->getSliceType() == I_SLICE) ||
#if ENCODER_FAST_MODE
						rpcBestCU->getPredictionMode(0) == NUMBER_OF_PREDICTION_MODES ||  // if there is no valid inter prediction
						!testInter ||
#endif
						((!m_pcEncCfg->getDisableIntraPUsInInterSlices()) && (
							(rpcBestCU->getCbf(0, COMPONENT_Y) != 0) ||
							((rpcBestCU->getCbf(0, COMPONENT_Cb) != 0) && (numberValidComponents > COMPONENT_Cb)) ||
							((rpcBestCU->getCbf(0, COMPONENT_Cr) != 0) && (numberValidComponents > COMPONENT_Cr))  // avoid very complex intra if it is unlikely
							)))
					{
						xCheckRDCostIntra(rpcBestCU, rpcTempCU, intraCost, SIZE_2Nx2N DEBUG_STRING_PASS_INTO(sDebug));
						rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
						if (uiDepth == sps.getLog2DiffMaxMinCodingBlockSize())
						{
							if (rpcTempCU->getWidth(0) > (1 << sps.getQuadtreeTULog2MinSize()))
							{
								Double tmpIntraCost;
								xCheckRDCostIntra(rpcBestCU, rpcTempCU, tmpIntraCost, SIZE_NxN DEBUG_STRING_PASS_INTO(sDebug));
								intraCost = std::min(intraCost, tmpIntraCost);
								rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
							}
						}
					}
					/*****************************************************************PCM***********************************************************/
					// test PCM
					if (sps.getUsePCM()
						&& rpcTempCU->getWidth(0) <= (1 << sps.getPCMLog2MaxSize())
						&& rpcTempCU->getWidth(0) >= (1 << sps.getPCMLog2MinSize()))
					{
						UInt uiRawBits = getTotalBits(rpcBestCU->getWidth(0), rpcBestCU->getHeight(0), rpcBestCU->getPic()->getChromaFormat(), sps.getBitDepths().recon);
						UInt uiBestBits = rpcBestCU->getTotalBits();
						if ((uiBestBits > uiRawBits) || (rpcBestCU->getTotalCost() > m_pcRdCost->calcRdCost(uiRawBits, 0)))
						{
							xCheckIntraPCM(rpcBestCU, rpcTempCU);
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
						}
					}
#if ENCODER_FAST_MODE
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
					if (pcPic->getLayerId() > 0 && !m_disableILP)
#else
					if (pcPic->getLayerId() > 0)
#endif
					{
						for (Int refLayer = 0; refLayer < pcSlice->getActiveNumILRRefIdx(); refLayer++)
						{
							xCheckRDCostILRUni(rpcBestCU, rpcTempCU, pcSlice->getVPS()->getRefLayerId(pcSlice->getLayerId(), pcSlice->getInterLayerPredLayerIdc(refLayer)));
							rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);
						}
					}
#endif

					if (bIsLosslessMode) // Restore loop variable if lossless mode was searched.
					{
						iQP = iMinQP;
					}
				}
			}

			if (rpcBestCU->getTotalCost() != MAX_DOUBLE)
			{
				m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
				m_pcEntropyCoder->resetBits();
				m_pcEntropyCoder->encodeSplitFlag(rpcBestCU, 0, uiDepth, true);
				rpcBestCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
				rpcBestCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
				rpcBestCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcBestCU->getTotalBits(), rpcBestCU->getTotalDistortion());
				m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);
			}

#if HIGHER_LAYER_IRAP_SKIP_FLAG
		}
#endif
	}

	// copy original YUV samples to PCM buffer
	if (rpcBestCU->getTotalCost() != MAX_DOUBLE && rpcBestCU->isLosslessCoded(0) && (rpcBestCU->getIPCMFlag(0) == false))
	{
		xFillPCMBuffer(rpcBestCU, m_ppcOrigYuv[uiDepth]);
	}

	if (uiDepth == pps.getMaxCuDQPDepth())
	{
		Int idQP = m_pcEncCfg->getMaxDeltaQP();
		iMinQP = Clip3(-sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP - idQP);
		iMaxQP = Clip3(-sps.getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQP + idQP);
	}
	else if (uiDepth < pps.getMaxCuDQPDepth())
	{
		iMinQP = iBaseQP;
		iMaxQP = iBaseQP;
	}
	else
	{
		const Int iStartQP = rpcTempCU->getQP(0);
		iMinQP = iStartQP;
		iMaxQP = iStartQP;
	}

	if (m_pcEncCfg->getUseRateCtrl())
	{
		iMinQP = m_pcRateCtrl->getRCQP();
		iMaxQP = m_pcRateCtrl->getRCQP();
	}

	if (m_pcEncCfg->getCUTransquantBypassFlagForceValue())
	{
		iMaxQP = iMinQP; // If all TUs are forced into using transquant bypass, do not loop here.
	}

	const Bool bSubBranch = bBoundary || !(m_pcEncCfg->getUseEarlyCU() && rpcBestCU->getTotalCost() != MAX_DOUBLE && rpcBestCU->isSkipped(0));

	if (bSubBranch && uiDepth < sps.getLog2DiffMaxMinCodingBlockSize() && (!getFastDeltaQp() || uiWidth > fastDeltaQPCuMaxSize || bBoundary))
	{
		// further split
		for (Int iQP = iMinQP; iQP <= iMaxQP; iQP++)
		{
			const Bool bIsLosslessMode = false; // False at this level. Next level down may set it to true.

			rpcTempCU->initEstData(uiDepth, iQP, bIsLosslessMode);

			UChar       uhNextDepth = uiDepth + 1;
			TComDataCU* pcSubBestPartCU = m_ppcBestCU[uhNextDepth];
			TComDataCU* pcSubTempPartCU = m_ppcTempCU[uhNextDepth];
			DEBUG_STRING_NEW(sTempDebug)

				for (UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++)
				{
					pcSubBestPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);           // clear sub partition datas or init.
					pcSubTempPartCU->initSubCU(rpcTempCU, uiPartUnitIdx, uhNextDepth, iQP);           // clear sub partition datas or init.

					if ((pcSubBestPartCU->getCUPelX() < sps.getPicWidthInLumaSamples()) && (pcSubBestPartCU->getCUPelY() < sps.getPicHeightInLumaSamples()))
					{
						if (0 == uiPartUnitIdx) //initialize RD with previous depth buffer
						{
							m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);
						}
						else
						{
							m_pppcRDSbacCoder[uhNextDepth][CI_CURR_BEST]->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
						}

#if AMP_ENC_SPEEDUP
						DEBUG_STRING_NEW(sChild)
							if (!(rpcBestCU->getTotalCost() != MAX_DOUBLE && rpcBestCU->isInter(0)))
							{
								xCompressCU(pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), NUMBER_OF_PART_SIZES);
							}
							else
							{

								xCompressCU(pcSubBestPartCU, pcSubTempPartCU, uhNextDepth DEBUG_STRING_PASS_INTO(sChild), rpcBestCU->getPartitionSize(0));
							}
						DEBUG_STRING_APPEND(sTempDebug, sChild)
#else
						xCompressCU(pcSubBestPartCU, pcSubTempPartCU, uhNextDepth);
#endif

						rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth);         // Keep best part data to current temporary data.
						xCopyYuv2Tmp(pcSubBestPartCU->getTotalNumPart() * uiPartUnitIdx, uhNextDepth);
					}
					else
					{
						pcSubBestPartCU->copyToPic(uhNextDepth);
						rpcTempCU->copyPartFrom(pcSubBestPartCU, uiPartUnitIdx, uhNextDepth);
					}
				}

			m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uhNextDepth][CI_NEXT_BEST]);
			if (!bBoundary)
			{
				m_pcEntropyCoder->resetBits();
				m_pcEntropyCoder->encodeSplitFlag(rpcTempCU, 0, uiDepth, true);

				rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // split bits
				rpcTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
			}
			rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion());

			if (uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
			{
				Bool hasResidual = false;
				for (UInt uiBlkIdx = 0; uiBlkIdx < rpcTempCU->getTotalNumPart(); uiBlkIdx++)
				{
					if ((rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Y)
						|| (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cb) && (numberValidComponents > COMPONENT_Cb))
						|| (rpcTempCU->getCbf(uiBlkIdx, COMPONENT_Cr) && (numberValidComponents > COMPONENT_Cr))))
					{
						hasResidual = true;
						break;
					}
				}

				if (hasResidual)
				{
					m_pcEntropyCoder->resetBits();
					m_pcEntropyCoder->encodeQP(rpcTempCU, 0, false);
					rpcTempCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
					rpcTempCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
					rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion());

					Bool foundNonZeroCbf = false;
					rpcTempCU->setQPSubCUs(rpcTempCU->getRefQP(0), 0, uiDepth, foundNonZeroCbf);
					assert(foundNonZeroCbf);
				}
				else
				{
					rpcTempCU->setQPSubParts(rpcTempCU->getRefQP(0), 0, uiDepth); // set QP to default QP
				}
			}

			m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

			// If the configuration being tested exceeds the maximum number of bytes for a slice / slice-segment, then
			// a proper RD evaluation cannot be performed. Therefore, termination of the
			// slice/slice-segment must be made prior to this CTU.
			// This can be achieved by forcing the decision to be that of the rpcTempCU.
			// The exception is each slice / slice-segment must have at least one CTU.
			if (rpcBestCU->getTotalCost() != MAX_DOUBLE)
			{
				const Bool isEndOfSlice = pcSlice->getSliceMode() == FIXED_NUMBER_OF_BYTES
					&& ((pcSlice->getSliceBits() + rpcBestCU->getTotalBits()) > pcSlice->getSliceArgument() << 3)
					&& rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceCurStartCtuTsAddr())
					&& rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
				const Bool isEndOfSliceSegment = pcSlice->getSliceSegmentMode() == FIXED_NUMBER_OF_BYTES
					&& ((pcSlice->getSliceSegmentBits() + rpcBestCU->getTotalBits()) > pcSlice->getSliceSegmentArgument() << 3)
					&& rpcBestCU->getCtuRsAddr() != pcPic->getPicSym()->getCtuTsToRsAddrMap(pcSlice->getSliceSegmentCurStartCtuTsAddr());
				// Do not need to check slice condition for slice-segment since a slice-segment is a subset of a slice.
				if (isEndOfSlice || isEndOfSliceSegment)
				{
					rpcBestCU->getTotalCost() = MAX_DOUBLE;
				}
			}

			xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTempDebug) DEBUG_STRING_PASS_INTO(false)); // RD compare current larger prediction
			// with sub partitioned prediction.
		}
	}

	DEBUG_STRING_APPEND(sDebug_, sDebug);

	rpcBestCU->copyToPic(uiDepth);                                                     // Copy Best data to Picture for next partition prediction.

	xCopyYuv2Pic(rpcBestCU->getPic(), rpcBestCU->getCtuRsAddr(), rpcBestCU->getZorderIdxInCtu(), uiDepth, uiDepth);   // Copy Yuv data to picture Yuv
	if (bBoundary)
	{
		return;
	}

	// Assert if Best prediction mode is NONE
	// Selected mode's RD-cost must be not MAX_DOUBLE.
	assert(rpcBestCU->getPartitionSize(0) != NUMBER_OF_PART_SIZES);
	assert(rpcBestCU->getPredictionMode(0) != NUMBER_OF_PREDICTION_MODES);
	assert(rpcBestCU->getTotalCost() != MAX_DOUBLE);
}

/** finish encoding a cu and handle end-of-slice conditions
* \param pcCU
* \param uiAbsPartIdx
* \param uiDepth
* \returns Void
*/
Void TEncCu::finishCU(TComDataCU* pcCU, UInt uiAbsPartIdx)
{
	TComPic* pcPic = pcCU->getPic();
	TComSlice* pcSlice = pcCU->getPic()->getSlice(pcCU->getPic()->getCurrSliceIdx());

	//Calculate end address
	const Int  currentCTUTsAddr = pcPic->getPicSym()->getCtuRsToTsAddrMap(pcCU->getCtuRsAddr());
	const Bool isLastSubCUOfCtu = pcCU->isLastSubCUOfCtu(uiAbsPartIdx);
	if (isLastSubCUOfCtu)
	{
		// The 1-terminating bit is added to all streams, so don't add it here when it's 1.
		// i.e. when the slice segment CurEnd CTU address is the current CTU address+1.
		if (pcSlice->getSliceSegmentCurEndCtuTsAddr() != currentCTUTsAddr + 1)
		{
			m_pcEntropyCoder->encodeTerminatingBit(0);
		}
	}
}

/** Compute QP for each CU
* \param pcCU Target CU
* \param uiDepth CU depth
* \returns quantization parameter
*/
Int TEncCu::xComputeQP(TComDataCU* pcCU, UInt uiDepth)
{
	Int iBaseQp = pcCU->getSlice()->getSliceQp();
	Int iQpOffset = 0;
	if (m_pcEncCfg->getUseAdaptiveQP())
	{
		TEncPic* pcEPic = dynamic_cast<TEncPic*>(pcCU->getPic());
		UInt uiAQDepth = min(uiDepth, pcEPic->getMaxAQDepth() - 1);
		TEncPicQPAdaptationLayer* pcAQLayer = pcEPic->getAQLayer(uiAQDepth);
		UInt uiAQUPosX = pcCU->getCUPelX() / pcAQLayer->getAQPartWidth();
		UInt uiAQUPosY = pcCU->getCUPelY() / pcAQLayer->getAQPartHeight();
		UInt uiAQUStride = pcAQLayer->getAQPartStride();
		TEncQPAdaptationUnit* acAQU = pcAQLayer->getQPAdaptationUnit();

		Double dMaxQScale = pow(2.0, m_pcEncCfg->getQPAdaptationRange() / 6.0);
		Double dAvgAct = pcAQLayer->getAvgActivity();
		Double dCUAct = acAQU[uiAQUPosY * uiAQUStride + uiAQUPosX].getActivity();
		Double dNormAct = (dMaxQScale * dCUAct + dAvgAct) / (dCUAct + dMaxQScale * dAvgAct);
		Double dQpOffset = log(dNormAct) / log(2.0) * 6.0;
		iQpOffset = Int(floor(dQpOffset + 0.49999));
	}
	return Clip3(-pcCU->getSlice()->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, iBaseQp + iQpOffset);
}

/** encode a CU block recursively
* \param pcCU
* \param uiAbsPartIdx
* \param uiDepth
* \returns Void
*/
Void TEncCu::xEncodeCU(TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth)
{
	TComPic* const pcPic = pcCU->getPic();
	TComSlice* const pcSlice = pcCU->getSlice();
	const TComSPS& sps = *(pcSlice->getSPS());
	const TComPPS& pps = *(pcSlice->getPPS());

	const UInt maxCUWidth = sps.getMaxCUWidth();
	const UInt maxCUHeight = sps.getMaxCUHeight();

	Bool bBoundary = false;
	UInt uiLPelX = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIdx]];
	const UInt uiRPelX = uiLPelX + (maxCUWidth >> uiDepth) - 1;
	UInt uiTPelY = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIdx]];
	const UInt uiBPelY = uiTPelY + (maxCUHeight >> uiDepth) - 1;

#if HIGHER_LAYER_IRAP_SKIP_FLAG
	if (m_pcEncCfg->getSkipPictureAtArcSwitch() && m_pcEncCfg->getAdaptiveResolutionChange() > 0 && pcSlice->getLayerId() == 1 && pcSlice->getPOC() == m_pcEncCfg->getAdaptiveResolutionChange())
	{
		pcCU->setSkipFlagSubParts(true, uiAbsPartIdx, uiDepth);
	}
#endif

	if ((uiRPelX < sps.getPicWidthInLumaSamples()) && (uiBPelY < sps.getPicHeightInLumaSamples()))
	{
		m_pcEntropyCoder->encodeSplitFlag(pcCU, uiAbsPartIdx, uiDepth);
	}
	else
	{
		bBoundary = true;
	}

	if (((uiDepth < pcCU->getDepth(uiAbsPartIdx)) && (uiDepth < sps.getLog2DiffMaxMinCodingBlockSize())) || bBoundary)
	{
		UInt uiQNumParts = (pcPic->getNumPartitionsInCtu() >> (uiDepth << 1)) >> 2;
		if (uiDepth == pps.getMaxCuDQPDepth() && pps.getUseDQP())
		{
			setdQPFlag(true);
		}

		if (uiDepth == pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
		{
			setCodeChromaQpAdjFlag(true);
		}

		for (UInt uiPartUnitIdx = 0; uiPartUnitIdx < 4; uiPartUnitIdx++, uiAbsPartIdx += uiQNumParts)
		{
			uiLPelX = pcCU->getCUPelX() + g_auiRasterToPelX[g_auiZscanToRaster[uiAbsPartIdx]];
			uiTPelY = pcCU->getCUPelY() + g_auiRasterToPelY[g_auiZscanToRaster[uiAbsPartIdx]];

			if ((uiLPelX < sps.getPicWidthInLumaSamples()) && (uiTPelY < sps.getPicHeightInLumaSamples()))
			{
				xEncodeCU(pcCU, uiAbsPartIdx, uiDepth + 1);
			}
		}
		return;
	}

	if (uiDepth <= pps.getMaxCuDQPDepth() && pps.getUseDQP())
	{
		setdQPFlag(true);
	}

	if (uiDepth <= pps.getPpsRangeExtension().getDiffCuChromaQpOffsetDepth() && pcSlice->getUseChromaQpAdj())
	{
		setCodeChromaQpAdjFlag(true);
	}

	if (pps.getTransquantBypassEnableFlag())
	{
		m_pcEntropyCoder->encodeCUTransquantBypassFlag(pcCU, uiAbsPartIdx);
	}

	if (!pcSlice->isIntra())
	{
		m_pcEntropyCoder->encodeSkipFlag(pcCU, uiAbsPartIdx);
	}

	if (pcCU->isSkipped(uiAbsPartIdx))
	{
		m_pcEntropyCoder->encodeMergeIndex(pcCU, uiAbsPartIdx);
		finishCU(pcCU, uiAbsPartIdx);
		return;
	}

	m_pcEntropyCoder->encodePredMode(pcCU, uiAbsPartIdx);
	m_pcEntropyCoder->encodePartSize(pcCU, uiAbsPartIdx, uiDepth);

	if (pcCU->isIntra(uiAbsPartIdx) && pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N)
	{
		m_pcEntropyCoder->encodeIPCMInfo(pcCU, uiAbsPartIdx);

		if (pcCU->getIPCMFlag(uiAbsPartIdx))
		{
			// Encode slice finish
			finishCU(pcCU, uiAbsPartIdx);
			return;
		}
	}

	// prediction Info ( Intra : direction mode, Inter : Mv, reference idx )
	m_pcEntropyCoder->encodePredInfo(pcCU, uiAbsPartIdx);

	// Encode Coefficients
	Bool bCodeDQP = getdQPFlag();
	Bool codeChromaQpAdj = getCodeChromaQpAdjFlag();
	m_pcEntropyCoder->encodeCoeff(pcCU, uiAbsPartIdx, uiDepth, bCodeDQP, codeChromaQpAdj);
	setCodeChromaQpAdjFlag(codeChromaQpAdj);
	setdQPFlag(bCodeDQP);

	// --- write terminating bit ---
	finishCU(pcCU, uiAbsPartIdx);
}

Int xCalcHADs8x8_ISlice(Pel* piOrg, Int iStrideOrg)
{
	Int k, i, j, jj;
	Int diff[64], m1[8][8], m2[8][8], m3[8][8], iSumHad = 0;

	for (k = 0; k < 64; k += 8)
	{
		diff[k + 0] = piOrg[0];
		diff[k + 1] = piOrg[1];
		diff[k + 2] = piOrg[2];
		diff[k + 3] = piOrg[3];
		diff[k + 4] = piOrg[4];
		diff[k + 5] = piOrg[5];
		diff[k + 6] = piOrg[6];
		diff[k + 7] = piOrg[7];

		piOrg += iStrideOrg;
	}

	//horizontal
	for (j = 0; j < 8; j++)
	{
		jj = j << 3;
		m2[j][0] = diff[jj] + diff[jj + 4];
		m2[j][1] = diff[jj + 1] + diff[jj + 5];
		m2[j][2] = diff[jj + 2] + diff[jj + 6];
		m2[j][3] = diff[jj + 3] + diff[jj + 7];
		m2[j][4] = diff[jj] - diff[jj + 4];
		m2[j][5] = diff[jj + 1] - diff[jj + 5];
		m2[j][6] = diff[jj + 2] - diff[jj + 6];
		m2[j][7] = diff[jj + 3] - diff[jj + 7];

		m1[j][0] = m2[j][0] + m2[j][2];
		m1[j][1] = m2[j][1] + m2[j][3];
		m1[j][2] = m2[j][0] - m2[j][2];
		m1[j][3] = m2[j][1] - m2[j][3];
		m1[j][4] = m2[j][4] + m2[j][6];
		m1[j][5] = m2[j][5] + m2[j][7];
		m1[j][6] = m2[j][4] - m2[j][6];
		m1[j][7] = m2[j][5] - m2[j][7];

		m2[j][0] = m1[j][0] + m1[j][1];
		m2[j][1] = m1[j][0] - m1[j][1];
		m2[j][2] = m1[j][2] + m1[j][3];
		m2[j][3] = m1[j][2] - m1[j][3];
		m2[j][4] = m1[j][4] + m1[j][5];
		m2[j][5] = m1[j][4] - m1[j][5];
		m2[j][6] = m1[j][6] + m1[j][7];
		m2[j][7] = m1[j][6] - m1[j][7];
	}

	//vertical
	for (i = 0; i < 8; i++)
	{
		m3[0][i] = m2[0][i] + m2[4][i];
		m3[1][i] = m2[1][i] + m2[5][i];
		m3[2][i] = m2[2][i] + m2[6][i];
		m3[3][i] = m2[3][i] + m2[7][i];
		m3[4][i] = m2[0][i] - m2[4][i];
		m3[5][i] = m2[1][i] - m2[5][i];
		m3[6][i] = m2[2][i] - m2[6][i];
		m3[7][i] = m2[3][i] - m2[7][i];

		m1[0][i] = m3[0][i] + m3[2][i];
		m1[1][i] = m3[1][i] + m3[3][i];
		m1[2][i] = m3[0][i] - m3[2][i];
		m1[3][i] = m3[1][i] - m3[3][i];
		m1[4][i] = m3[4][i] + m3[6][i];
		m1[5][i] = m3[5][i] + m3[7][i];
		m1[6][i] = m3[4][i] - m3[6][i];
		m1[7][i] = m3[5][i] - m3[7][i];

		m2[0][i] = m1[0][i] + m1[1][i];
		m2[1][i] = m1[0][i] - m1[1][i];
		m2[2][i] = m1[2][i] + m1[3][i];
		m2[3][i] = m1[2][i] - m1[3][i];
		m2[4][i] = m1[4][i] + m1[5][i];
		m2[5][i] = m1[4][i] - m1[5][i];
		m2[6][i] = m1[6][i] + m1[7][i];
		m2[7][i] = m1[6][i] - m1[7][i];
	}

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			iSumHad += abs(m2[i][j]);
		}
	}
	iSumHad -= abs(m2[0][0]);
	iSumHad = (iSumHad + 2) >> 2;
	return(iSumHad);
}

Int  TEncCu::updateCtuDataISlice(TComDataCU* pCtu, Int width, Int height)
{
	Int  xBl, yBl;
	const Int iBlkSize = 8;

	Pel* pOrgInit = pCtu->getPic()->getPicYuvOrg()->getAddr(COMPONENT_Y, pCtu->getCtuRsAddr(), 0);
	Int  iStrideOrig = pCtu->getPic()->getPicYuvOrg()->getStride(COMPONENT_Y);
	Pel* pOrg;

	Int iSumHad = 0;
	for (yBl = 0; (yBl + iBlkSize) <= height; yBl += iBlkSize)
	{
		for (xBl = 0; (xBl + iBlkSize) <= width; xBl += iBlkSize)
		{
			pOrg = pOrgInit + iStrideOrig * yBl + xBl;
			iSumHad += xCalcHADs8x8_ISlice(pOrg, iStrideOrig);
		}
	}
	return(iSumHad);
}

/** check RD costs for a CU block encoded with merge
* \param rpcBestCU
* \param rpcTempCU
* \param earlyDetectionSkipMode
*/
#if HIGHER_LAYER_IRAP_SKIP_FLAG
Void TEncCu::xCheckRDCostMerge2Nx2N(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool* earlyDetectionSkipMode, Bool bUseSkip)
#else
Void TEncCu::xCheckRDCostMerge2Nx2N(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU DEBUG_STRING_FN_DECLARE(sDebug), Bool* earlyDetectionSkipMode)
#endif
{
	assert(rpcTempCU->getSlice()->getSliceType() != I_SLICE);
	if (getFastDeltaQp())
	{
		return;   // never check merge in fast deltaqp mode
	}
	TComMvField  cMvFieldNeighbours[2 * MRG_MAX_NUM_CANDS]; // double length for mv of both lists
	UChar uhInterDirNeighbours[MRG_MAX_NUM_CANDS];
	Int numValidMergeCand = 0;
	const Bool bTransquantBypassFlag = rpcTempCU->getCUTransquantBypass(0);

	for (UInt ui = 0; ui < rpcTempCU->getSlice()->getMaxNumMergeCand(); ++ui)
	{
		uhInterDirNeighbours[ui] = 0;
	}
	UChar uhDepth = rpcTempCU->getDepth(0);
	rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to CTU level
	rpcTempCU->getInterMergeCandidates(0, 0, cMvFieldNeighbours, uhInterDirNeighbours, numValidMergeCand);

	Int mergeCandBuffer[MRG_MAX_NUM_CANDS];
	for (UInt ui = 0; ui < numValidMergeCand; ++ui)
	{
		mergeCandBuffer[ui] = 0;
	}

	Bool bestIsSkip = false;

	UInt iteration;
	if (rpcTempCU->isLosslessCoded(0))
	{
		iteration = 1;
	}
	else
	{
		iteration = 2;
	}
	DEBUG_STRING_NEW(bestStr)

#if HIGHER_LAYER_IRAP_SKIP_FLAG
		for (UInt uiNoResidual = bUseSkip ? 1 : 0; uiNoResidual < iteration; ++uiNoResidual)
#else
		for (UInt uiNoResidual = 0; uiNoResidual < iteration; ++uiNoResidual)
#endif
		{
			for (UInt uiMergeCand = 0; uiMergeCand < numValidMergeCand; ++uiMergeCand)
			{
#if REF_IDX_ME_ZEROMV
				Bool bZeroMVILR = rpcTempCU->xCheckZeroMVILRMerge(uhInterDirNeighbours[uiMergeCand], cMvFieldNeighbours[0 + 2 * uiMergeCand], cMvFieldNeighbours[1 + 2 * uiMergeCand]);
				if (bZeroMVILR)
				{
#endif
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
					if (!(rpcTempCU->isInterLayerReference(uhInterDirNeighbours[uiMergeCand], cMvFieldNeighbours[0 + 2 * uiMergeCand], cMvFieldNeighbours[1 + 2 * uiMergeCand]) && m_disableILP))
					{
#endif
						if (!(uiNoResidual == 1 && mergeCandBuffer[uiMergeCand] == 1))
						{
							if (!(bestIsSkip && uiNoResidual == 0))
							{
								DEBUG_STRING_NEW(tmpStr)
									// set MC parameters
									rpcTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth); // interprets depth relative to CTU level
								rpcTempCU->setCUTransquantBypassSubParts(bTransquantBypassFlag, 0, uhDepth);
								rpcTempCU->setChromaQpAdjSubParts(bTransquantBypassFlag ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth);
								rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth); // interprets depth relative to CTU level
								rpcTempCU->setMergeFlagSubParts(true, 0, 0, uhDepth); // interprets depth relative to CTU level
								rpcTempCU->setMergeIndexSubParts(uiMergeCand, 0, 0, uhDepth); // interprets depth relative to CTU level
								rpcTempCU->setInterDirSubParts(uhInterDirNeighbours[uiMergeCand], 0, 0, uhDepth); // interprets depth relative to CTU level
								rpcTempCU->getCUMvField(REF_PIC_LIST_0)->setAllMvField(cMvFieldNeighbours[0 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level
								rpcTempCU->getCUMvField(REF_PIC_LIST_1)->setAllMvField(cMvFieldNeighbours[1 + 2 * uiMergeCand], SIZE_2Nx2N, 0, 0); // interprets depth relative to rpcTempCU level

								// do MC
								m_pcPredSearch->motionCompensation(rpcTempCU, m_ppcPredYuvTemp[uhDepth]);
								// estimate residual and encode everything
								m_pcPredSearch->encodeResAndCalcRdInterCU(rpcTempCU,
									m_ppcOrigYuv[uhDepth],
									m_ppcPredYuvTemp[uhDepth],
									m_ppcResiYuvTemp[uhDepth],
									m_ppcResiYuvBest[uhDepth],
									m_ppcRecoYuvTemp[uhDepth],
									(uiNoResidual != 0) DEBUG_STRING_PASS_INTO(tmpStr));

#if DEBUG_STRING
								DebugInterPredResiReco(tmpStr, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

								if ((uiNoResidual == 0) && (rpcTempCU->getQtRootCbf(0) == 0))
								{
									// If no residual when allowing for one, then set mark to not try case where residual is forced to 0
									mergeCandBuffer[uiMergeCand] = 1;
								}

								Int orgQP = rpcTempCU->getQP(0);
								xCheckDQP(rpcTempCU);
								xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(bestStr) DEBUG_STRING_PASS_INTO(tmpStr));

								rpcTempCU->initEstData(uhDepth, orgQP, bTransquantBypassFlag);

								if (m_pcEncCfg->getUseFastDecisionForMerge() && !bestIsSkip)
								{
									bestIsSkip = rpcBestCU->getQtRootCbf(0) == 0;
								}
							}
						}
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
					}
#endif
#if REF_IDX_ME_ZEROMV 
				}
#endif
			}

			if (uiNoResidual == 0 && m_pcEncCfg->getUseEarlySkipDetection())
			{
				if (rpcBestCU->getQtRootCbf(0) == 0)
				{
					if (rpcBestCU->getMergeFlag(0))
					{
						*earlyDetectionSkipMode = true;
					}
					else if (m_pcEncCfg->getMotionEstimationSearchMethod() != MESEARCH_SELECTIVE)
					{
						Int absoulte_MV = 0;
						for (UInt uiRefListIdx = 0; uiRefListIdx < 2; uiRefListIdx++)
						{
							if (rpcBestCU->getSlice()->getNumRefIdx(RefPicList(uiRefListIdx)) > 0)
							{
								TComCUMvField* pcCUMvField = rpcBestCU->getCUMvField(RefPicList(uiRefListIdx));
								Int iHor = pcCUMvField->getMvd(0).getAbsHor();
								Int iVer = pcCUMvField->getMvd(0).getAbsVer();
								absoulte_MV += iHor + iVer;
							}
						}

						if (absoulte_MV == 0)
						{
							*earlyDetectionSkipMode = true;
						}
					}
				}
			}
		}
	DEBUG_STRING_APPEND(sDebug, bestStr)
}


#if AMP_MRG
Void TEncCu::xCheckRDCostInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize DEBUG_STRING_FN_DECLARE(sDebug), Bool bUseMRG)
#else
Void TEncCu::xCheckRDCostInter(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, PartSize ePartSize)
#endif
{
	DEBUG_STRING_NEW(sTest)

		if (getFastDeltaQp())
		{
			const TComSPS& sps = *(rpcTempCU->getSlice()->getSPS());
			const UInt fastDeltaQPCuMaxSize = Clip3(sps.getMaxCUHeight() >> (sps.getLog2DiffMaxMinCodingBlockSize()), sps.getMaxCUHeight(), 32u);
			if (ePartSize != SIZE_2Nx2N || rpcTempCU->getWidth(0) > fastDeltaQPCuMaxSize)
			{
				return; // only check necessary 2Nx2N Inter in fast deltaqp mode
			}
		}

	// prior to this, rpcTempCU will have just been reset using rpcTempCU->initEstData( uiDepth, iQP, bIsLosslessMode );
	UChar uhDepth = rpcTempCU->getDepth(0);

	rpcTempCU->setPartSizeSubParts(ePartSize, 0, uhDepth);
	rpcTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth);
	rpcTempCU->setChromaQpAdjSubParts(rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uhDepth);

#if SVC_EXTENSION
#if AMP_MRG
	rpcTempCU->setMergeAMP(true);
	Bool ret = m_pcPredSearch->predInterSearch(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] DEBUG_STRING_PASS_INTO(sTest), false, bUseMRG);
#else  
	Bool ret = m_pcPredSearch->predInterSearch(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth]);
#endif

	if (!ret)
	{
		return;
	}
#else
#if AMP_MRG
	rpcTempCU->setMergeAMP(true);
	m_pcPredSearch->predInterSearch(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth] DEBUG_STRING_PASS_INTO(sTest), false, bUseMRG);
#else
	m_pcPredSearch->predInterSearch(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth]);
#endif
#endif

#if AMP_MRG
	if (!rpcTempCU->getMergeAMP())
	{
		return;
	}
#endif

	m_pcPredSearch->encodeResAndCalcRdInterCU(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false DEBUG_STRING_PASS_INTO(sTest));
	rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion());

#if DEBUG_STRING
	DebugInterPredResiReco(sTest, *(m_ppcPredYuvTemp[uhDepth]), *(m_ppcResiYuvBest[uhDepth]), *(m_ppcRecoYuvTemp[uhDepth]), DebugStringGetPredModeMask(rpcTempCU->getPredictionMode(0)));
#endif

	xCheckDQP(rpcTempCU);
	xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
}

Void TEncCu::xCheckRDCostIntra(TComDataCU*& rpcBestCU,
	TComDataCU*& rpcTempCU,
	Double& cost,
	PartSize     eSize
	DEBUG_STRING_FN_DECLARE(sDebug))
{
	DEBUG_STRING_NEW(sTest)

		if (getFastDeltaQp())
		{
			const TComSPS& sps = *(rpcTempCU->getSlice()->getSPS());
			const UInt fastDeltaQPCuMaxSize = Clip3(sps.getMaxCUHeight() >> (sps.getLog2DiffMaxMinCodingBlockSize()), sps.getMaxCUHeight(), 32u);
			if (rpcTempCU->getWidth(0) > fastDeltaQPCuMaxSize)
			{
				return; // only check necessary 2Nx2N Intra in fast deltaqp mode
			}
		}

	UInt uiDepth = rpcTempCU->getDepth(0);

	rpcTempCU->setSkipFlagSubParts(false, 0, uiDepth);

	rpcTempCU->setPartSizeSubParts(eSize, 0, uiDepth);
	rpcTempCU->setPredModeSubParts(MODE_INTRA, 0, uiDepth);
	rpcTempCU->setChromaQpAdjSubParts(rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth);

	Pel resiLuma[NUMBER_OF_STORED_RESIDUAL_TYPES][MAX_CU_SIZE * MAX_CU_SIZE];

	m_pcPredSearch->estIntraPredLumaQT(rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest));

	m_ppcRecoYuvTemp[uiDepth]->copyToPicComponent(COMPONENT_Y, rpcTempCU->getPic()->getPicYuvRec(), rpcTempCU->getCtuRsAddr(), rpcTempCU->getZorderIdxInCtu());

	if (rpcBestCU->getPic()->getChromaFormat() != CHROMA_400)
	{
		m_pcPredSearch->estIntraPredChromaQT(rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth], resiLuma DEBUG_STRING_PASS_INTO(sTest));
	}

	m_pcEntropyCoder->resetBits();

	if (rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
	{
		m_pcEntropyCoder->encodeCUTransquantBypassFlag(rpcTempCU, 0, true);
	}

	m_pcEntropyCoder->encodeSkipFlag(rpcTempCU, 0, true);
	m_pcEntropyCoder->encodePredMode(rpcTempCU, 0, true);
	m_pcEntropyCoder->encodePartSize(rpcTempCU, 0, uiDepth, true);
	m_pcEntropyCoder->encodePredInfo(rpcTempCU, 0);
	m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true);

	// Encode Coefficients
	Bool bCodeDQP = getdQPFlag();
	Bool codeChromaQpAdjFlag = getCodeChromaQpAdjFlag();
	m_pcEntropyCoder->encodeCoeff(rpcTempCU, 0, uiDepth, bCodeDQP, codeChromaQpAdjFlag);
	setCodeChromaQpAdjFlag(codeChromaQpAdjFlag);
	setdQPFlag(bCodeDQP);

	m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

	rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
	rpcTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
	rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion());

	xCheckDQP(rpcTempCU);

	cost = rpcTempCU->getTotalCost();

	xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(sDebug) DEBUG_STRING_PASS_INTO(sTest));
}


/** Check R-D costs for a CU with PCM mode.
* \param rpcBestCU pointer to best mode CU data structure
* \param rpcTempCU pointer to testing mode CU data structure
* \returns Void
*
* \note Current PCM implementation encodes sample values in a lossless way. The distortion of PCM mode CUs are zero. PCM mode is selected if the best mode yields bits greater than that of PCM mode.
*/
Void TEncCu::xCheckIntraPCM(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU)
{
	if (getFastDeltaQp())
	{
		const TComSPS& sps = *(rpcTempCU->getSlice()->getSPS());
		const UInt fastDeltaQPCuMaxPCMSize = Clip3((UInt)1 << sps.getPCMLog2MinSize(), (UInt)1 << sps.getPCMLog2MaxSize(), 32u);
		if (rpcTempCU->getWidth(0) > fastDeltaQPCuMaxPCMSize)
		{
			return;   // only check necessary PCM in fast deltaqp mode
		}
	}

	UInt uiDepth = rpcTempCU->getDepth(0);

	rpcTempCU->setSkipFlagSubParts(false, 0, uiDepth);

	rpcTempCU->setIPCMFlag(0, true);
	rpcTempCU->setIPCMFlagSubParts(true, 0, rpcTempCU->getDepth(0));
	rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uiDepth);
	rpcTempCU->setPredModeSubParts(MODE_INTRA, 0, uiDepth);
	rpcTempCU->setTrIdxSubParts(0, 0, uiDepth);
	rpcTempCU->setChromaQpAdjSubParts(rpcTempCU->getCUTransquantBypass(0) ? 0 : m_cuChromaQpOffsetIdxPlus1, 0, uiDepth);

	m_pcPredSearch->IPCMSearch(rpcTempCU, m_ppcOrigYuv[uiDepth], m_ppcPredYuvTemp[uiDepth], m_ppcResiYuvTemp[uiDepth], m_ppcRecoYuvTemp[uiDepth]);

	m_pcRDGoOnSbacCoder->load(m_pppcRDSbacCoder[uiDepth][CI_CURR_BEST]);

	m_pcEntropyCoder->resetBits();

	if (rpcTempCU->getSlice()->getPPS()->getTransquantBypassEnableFlag())
	{
		m_pcEntropyCoder->encodeCUTransquantBypassFlag(rpcTempCU, 0, true);
	}

	m_pcEntropyCoder->encodeSkipFlag(rpcTempCU, 0, true);
	m_pcEntropyCoder->encodePredMode(rpcTempCU, 0, true);
	m_pcEntropyCoder->encodePartSize(rpcTempCU, 0, uiDepth, true);
	m_pcEntropyCoder->encodeIPCMInfo(rpcTempCU, 0, true);

	m_pcRDGoOnSbacCoder->store(m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]);

	rpcTempCU->getTotalBits() = m_pcEntropyCoder->getNumberOfWrittenBits();
	rpcTempCU->getTotalBins() = ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
	rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion());

	xCheckDQP(rpcTempCU);
	DEBUG_STRING_NEW(a)
		DEBUG_STRING_NEW(b)
		xCheckBestMode(rpcBestCU, rpcTempCU, uiDepth DEBUG_STRING_PASS_INTO(a) DEBUG_STRING_PASS_INTO(b));
}

/** check whether current try is the best with identifying the depth of current try
* \param rpcBestCU
* \param rpcTempCU
* \param uiDepth
*/
Void TEncCu::xCheckBestMode(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth DEBUG_STRING_FN_DECLARE(sParent) DEBUG_STRING_FN_DECLARE(sTest) DEBUG_STRING_PASS_INTO(Bool bAddSizeInfo))
{
	if (rpcTempCU->getTotalCost() < rpcBestCU->getTotalCost())
	{
		TComYuv* pcYuv;
		// Change Information data
		TComDataCU* pcCU = rpcBestCU;
		rpcBestCU = rpcTempCU;
		rpcTempCU = pcCU;

		// Change Prediction data
		pcYuv = m_ppcPredYuvBest[uiDepth];
		m_ppcPredYuvBest[uiDepth] = m_ppcPredYuvTemp[uiDepth];
		m_ppcPredYuvTemp[uiDepth] = pcYuv;

		// Change Reconstruction data
		pcYuv = m_ppcRecoYuvBest[uiDepth];
		m_ppcRecoYuvBest[uiDepth] = m_ppcRecoYuvTemp[uiDepth];
		m_ppcRecoYuvTemp[uiDepth] = pcYuv;

		pcYuv = NULL;
		pcCU = NULL;

		// store temp best CI for next CU coding
		m_pppcRDSbacCoder[uiDepth][CI_TEMP_BEST]->store(m_pppcRDSbacCoder[uiDepth][CI_NEXT_BEST]);


#if DEBUG_STRING
		DEBUG_STRING_SWAP(sParent, sTest)
			const PredMode predMode = rpcBestCU->getPredictionMode(0);
		if ((DebugOptionList::DebugString_Structure.getInt() & DebugStringGetPredModeMask(predMode)) && bAddSizeInfo)
		{
			std::stringstream ss(stringstream::out);
			ss << "###: " << (predMode == MODE_INTRA ? "Intra   " : "Inter   ") << partSizeToString[rpcBestCU->getPartitionSize(0)] << " CU at " << rpcBestCU->getCUPelX() << ", " << rpcBestCU->getCUPelY() << " width=" << UInt(rpcBestCU->getWidth(0)) << std::endl;
			sParent += ss.str();
		}
#endif
	}
}

Void TEncCu::xCheckDQP(TComDataCU* pcCU)
{
	UInt uiDepth = pcCU->getDepth(0);

	const TComPPS& pps = *(pcCU->getSlice()->getPPS());
	if (pps.getUseDQP() && uiDepth <= pps.getMaxCuDQPDepth())
	{
		if (pcCU->getQtRootCbf(0))
		{
			m_pcEntropyCoder->resetBits();
			m_pcEntropyCoder->encodeQP(pcCU, 0, false);
			pcCU->getTotalBits() += m_pcEntropyCoder->getNumberOfWrittenBits(); // dQP bits
			pcCU->getTotalBins() += ((TEncBinCABAC*)((TEncSbac*)m_pcEntropyCoder->m_pcEntropyCoderIf)->getEncBinIf())->getBinsCoded();
			pcCU->getTotalCost() = m_pcRdCost->calcRdCost(pcCU->getTotalBits(), pcCU->getTotalDistortion());
		}
		else
		{
			pcCU->setQPSubParts(pcCU->getRefQP(0), 0, uiDepth); // set QP to default QP
		}
	}
}

Void TEncCu::xCopyAMVPInfo(AMVPInfo* pSrc, AMVPInfo* pDst)
{
	pDst->iN = pSrc->iN;
	for (Int i = 0; i < pSrc->iN; i++)
	{
		pDst->m_acMvCand[i] = pSrc->m_acMvCand[i];
	}
}
Void TEncCu::xCopyYuv2Pic(TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth)
{
	UInt uiAbsPartIdxInRaster = g_auiZscanToRaster[uiAbsPartIdx];
	UInt uiSrcBlkWidth = rpcPic->getNumPartInCtuWidth() >> (uiSrcDepth);
	UInt uiBlkWidth = rpcPic->getNumPartInCtuWidth() >> (uiDepth);
	UInt uiPartIdxX = ((uiAbsPartIdxInRaster % rpcPic->getNumPartInCtuWidth()) % uiSrcBlkWidth) / uiBlkWidth;
	UInt uiPartIdxY = ((uiAbsPartIdxInRaster / rpcPic->getNumPartInCtuWidth()) % uiSrcBlkWidth) / uiBlkWidth;
	UInt uiPartIdx = uiPartIdxY * (uiSrcBlkWidth / uiBlkWidth) + uiPartIdxX;
	m_ppcRecoYuvBest[uiSrcDepth]->copyToPicYuv(rpcPic->getPicYuvRec(), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);

	m_ppcPredYuvBest[uiSrcDepth]->copyToPicYuv(rpcPic->getPicYuvPred(), uiCUAddr, uiAbsPartIdx, uiDepth - uiSrcDepth, uiPartIdx);
}

Void TEncCu::xCopyYuv2Tmp(UInt uiPartUnitIdx, UInt uiNextDepth)
{
	UInt uiCurrDepth = uiNextDepth - 1;
	m_ppcRecoYuvBest[uiNextDepth]->copyToPartYuv(m_ppcRecoYuvTemp[uiCurrDepth], uiPartUnitIdx);
	m_ppcPredYuvBest[uiNextDepth]->copyToPartYuv(m_ppcPredYuvBest[uiCurrDepth], uiPartUnitIdx);
}

/** Function for filling the PCM buffer of a CU using its original sample array
* \param pCU pointer to current CU
* \param pOrgYuv pointer to original sample array
*/
Void TEncCu::xFillPCMBuffer(TComDataCU* pCU, TComYuv* pOrgYuv)
{
	const ChromaFormat format = pCU->getPic()->getChromaFormat();
	const UInt numberValidComponents = getNumberValidComponents(format);
	for (UInt componentIndex = 0; componentIndex < numberValidComponents; componentIndex++)
	{
		const ComponentID component = ComponentID(componentIndex);

		const UInt width = pCU->getWidth(0) >> getComponentScaleX(component, format);
		const UInt height = pCU->getHeight(0) >> getComponentScaleY(component, format);

		Pel* source = pOrgYuv->getAddr(component, 0, width);
		Pel* destination = pCU->getPCMSample(component);

		const UInt sourceStride = pOrgYuv->getStride(component);

		for (Int line = 0; line < height; line++)
		{
			for (Int column = 0; column < width; column++)
			{
				destination[column] = source[column];
			}

			source += sourceStride;
			destination += width;
		}
	}
}

#if ADAPTIVE_QP_SELECTION
/** Collect ARL statistics from one block
*/
Int TEncCu::xTuCollectARLStats(TCoeff* rpcCoeff, TCoeff* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples)
{
	for (Int n = 0; n < NumCoeffInCU; n++)
	{
		TCoeff u = abs(rpcCoeff[n]);
		TCoeff absc = rpcArlCoeff[n];

		if (u != 0)
		{
			if (u < LEVEL_RANGE)
			{
				cSum[u] += (Double)absc;
				numSamples[u]++;
			}
			else
			{
				cSum[LEVEL_RANGE] += (Double)absc - (Double)(u << ARL_C_PRECISION);
				numSamples[LEVEL_RANGE]++;
			}
		}
	}

	return 0;
}

//! Collect ARL statistics from one CTU
Void TEncCu::xCtuCollectARLStats(TComDataCU* pCtu)
{
	Double cSum[LEVEL_RANGE + 1];     //: the sum of DCT coefficients corresponding to data type and quantization output
	UInt numSamples[LEVEL_RANGE + 1]; //: the number of coefficients corresponding to data type and quantization output

	TCoeff* pCoeffY = pCtu->getCoeff(COMPONENT_Y);
	TCoeff* pArlCoeffY = pCtu->getArlCoeff(COMPONENT_Y);
	const TComSPS& sps = *(pCtu->getSlice()->getSPS());

	const UInt uiMinCUWidth = sps.getMaxCUWidth() >> sps.getMaxTotalCUDepth(); // NOTE: ed - this is not the minimum CU width. It is the square-root of the number of coefficients per part.
	const UInt uiMinNumCoeffInCU = 1 << uiMinCUWidth;                          // NOTE: ed - what is this?

	memset(cSum, 0, sizeof(Double) * (LEVEL_RANGE + 1));
	memset(numSamples, 0, sizeof(UInt) * (LEVEL_RANGE + 1));

	// Collect stats to cSum[][] and numSamples[][]
	for (Int i = 0; i < pCtu->getTotalNumPart(); i++)
	{
		UInt uiTrIdx = pCtu->getTransformIdx(i);

		if (pCtu->isInter(i) && pCtu->getCbf(i, COMPONENT_Y, uiTrIdx))
		{
			xTuCollectARLStats(pCoeffY, pArlCoeffY, uiMinNumCoeffInCU, cSum, numSamples);
		}//Note that only InterY is processed. QP rounding is based on InterY data only.

		pCoeffY += uiMinNumCoeffInCU;
		pArlCoeffY += uiMinNumCoeffInCU;
	}

	for (Int u = 1; u < LEVEL_RANGE; u++)
	{
		m_pcTrQuant->getSliceSumC()[u] += cSum[u];
		m_pcTrQuant->getSliceNSamples()[u] += numSamples[u];
	}
	m_pcTrQuant->getSliceSumC()[LEVEL_RANGE] += cSum[LEVEL_RANGE];
	m_pcTrQuant->getSliceNSamples()[LEVEL_RANGE] += numSamples[LEVEL_RANGE];
}
#endif

#if SVC_EXTENSION
#if N0383_IL_CONSTRAINED_TILE_SETS_SEI
Bool TEncCu::xCheckTileSetConstraint(TComDataCU*& rpcCU)
{
	Bool disableILP = false;

	if (rpcCU->getPic()->getLayerId() == (m_pcEncCfg->getNumLayer() - 1) && m_pcEncCfg->getInterLayerConstrainedTileSetsSEIEnabled() && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(rpcCU->getCtuRsAddr()) >= 0)
	{
		if (rpcCU->getPic()->getPicSym()->getTileSetType(rpcCU->getCtuRsAddr()) == 2)
		{
			disableILP = true;
		}
		if (rpcCU->getPic()->getPicSym()->getTileSetType(rpcCU->getCtuRsAddr()) == 1)
		{
			Int currCUaddr = rpcCU->getCtuRsAddr();
			Int frameWitdhInCU = rpcCU->getPic()->getPicSym()->getFrameWidthInCtus();
			Int frameHeightInCU = rpcCU->getPic()->getPicSym()->getFrameHeightInCtus();
			Bool leftCUExists = (currCUaddr % frameWitdhInCU) > 0;
			Bool aboveCUExists = (currCUaddr / frameWitdhInCU) > 0;
			Bool rightCUExists = (currCUaddr % frameWitdhInCU) < (frameWitdhInCU - 1);
			Bool belowCUExists = (currCUaddr / frameWitdhInCU) < (frameHeightInCU - 1);
			Int currTileSetIdx = rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr);
			// Check if CU is at tile set boundary
			if ((leftCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr - 1) != currTileSetIdx) ||
				(leftCUExists && aboveCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr - frameWitdhInCU - 1) != currTileSetIdx) ||
				(aboveCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr - frameWitdhInCU) != currTileSetIdx) ||
				(aboveCUExists && rightCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr - frameWitdhInCU + 1) != currTileSetIdx) ||
				(rightCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr + 1) != currTileSetIdx) ||
				(rightCUExists && belowCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr + frameWitdhInCU + 1) != currTileSetIdx) ||
				(belowCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr + frameWitdhInCU) != currTileSetIdx) ||
				(belowCUExists && leftCUExists && rpcCU->getPic()->getPicSym()->getTileSetIdxMap(currCUaddr + frameWitdhInCU - 1) != currTileSetIdx))
			{
				disableILP = true;  // Disable ILP in tile set boundary CU
			}
		}
	}

	return disableILP;
}

Void TEncCu::xVerifyTileSetConstraint(TComDataCU*& rpcCU)
{
	if (rpcCU->getPic()->getLayerId() == (m_pcEncCfg->getNumLayer() - 1) && m_pcEncCfg->getInterLayerConstrainedTileSetsSEIEnabled() &&
		rpcCU->getPic()->getPicSym()->getTileSetIdxMap(rpcCU->getCtuRsAddr()) >= 0 && m_disableILP)
	{
		UInt numPartitions = rpcCU->getPic()->getNumPartitionsInCtu();
		for (UInt i = 0; i < numPartitions; i++)
		{
			if (!rpcCU->isIntra(i))
			{
				for (UInt refList = 0; refList < 2; refList++)
				{
					if (rpcCU->getInterDir(i) & (1 << refList))
					{
						TComCUMvField* mvField = rpcCU->getCUMvField(RefPicList(refList));
						if (mvField->getRefIdx(i) >= 0)
						{
							assert(!(rpcCU->getSlice()->getRefPic(RefPicList(refList), mvField->getRefIdx(i))->isILR(rpcCU->getPic()->getLayerId())));
						}
					}
				}
			}
		}
	}
}
#endif

#if ENCODER_FAST_MODE
Void TEncCu::xCheckRDCostILRUni(TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt refLayerId)
{
	UChar uhDepth = rpcTempCU->getDepth(0);
	rpcTempCU->setDepthSubParts(uhDepth, 0);
#if SKIP_FLAG
	rpcTempCU->setSkipFlagSubParts(false, 0, uhDepth);
#endif
	rpcTempCU->setPartSizeSubParts(SIZE_2Nx2N, 0, uhDepth);  //2Nx2N
	rpcTempCU->setPredModeSubParts(MODE_INTER, 0, uhDepth);
	rpcTempCU->setCUTransquantBypassSubParts(m_pcEncCfg->getCUTransquantBypassFlagForceValue(), 0, uhDepth);
	Bool exitILR = m_pcPredSearch->predInterSearchILRUni(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcRecoYuvTemp[uhDepth], refLayerId);
	if (!exitILR)
	{
		return;
	}
	m_pcPredSearch->encodeResAndCalcRdInterCU(rpcTempCU, m_ppcOrigYuv[uhDepth], m_ppcPredYuvTemp[uhDepth], m_ppcResiYuvTemp[uhDepth], m_ppcResiYuvBest[uhDepth], m_ppcRecoYuvTemp[uhDepth], false);
	rpcTempCU->getTotalCost() = m_pcRdCost->calcRdCost(rpcTempCU->getTotalBits(), rpcTempCU->getTotalDistortion());
	xCheckDQP(rpcTempCU);
	xCheckBestMode(rpcBestCU, rpcTempCU, uhDepth);
	return;
}
#endif
#endif //SVC_EXTENSION
//! \}