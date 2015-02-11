/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "Filter.h"


/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvCreateParticleFilter
//    Purpose: Creating CvParticleFilter structure and allocating memory for it
//    Context:
//    Parameters:
//      DP         - dimension of the dynamical vector
//      MP         - dimension of the measurement vector
//      SamplesNum - number of samples in sample set used in algorithm 
//    Returns: pointer to CvParticleFilter structure
//    Notes:
//      
//F*/

CV_INLINE void icvScaleVector_32f( const float* src, float* dst,
								  int len, double scale )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = (float)(src[i]*scale);
	
    icvCheckVector_32f( dst, len );
}

CV_INLINE void icvAddVector_32f( const float* src1, const float* src2,
								float* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src1[i] + src2[i];
	
    icvCheckVector_32f( dst, len );
}

CV_INLINE void icvMulMatrix_32f( const float* src1, int w1, int h1,
								const float* src2, int w2, int h2,
								float* dst )
{
    int i, j, k;
	
    if( w1 != h2 )
    {
        assert(0);
        return;
    }
	
    for( i = 0; i < h1; i++, src1 += w1, dst += w2 )
        for( j = 0; j < w2; j++ )
        {
            double s = 0;
            for( k = 0; k < w1; k++ )
                s += src1[k]*src2[j + k*w2];
            dst[j] = (float)s;
        }
		
		icvCheckVector_32f( dst, h1*w2 );
}





extern "C" CvParticleFilter* cvCreateParticleFilter( int DP, int MP, int SamplesNum )
{
    int i;
    CvParticleFilter *PF = 0;

    CV_FUNCNAME( "cvCreateParticleFilter" );
    __BEGIN__;
    
    if( DP < 0 || MP < 0 || SamplesNum < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );
    
    /* allocating memory for the structure */
    CV_CALL( PF = (CvParticleFilter *) cvAlloc( sizeof( CvParticleFilter )));
    /* setting structure params */
    PF->SamplesNum = SamplesNum;
    PF->DP = DP;
    PF->MP = MP;
    /* allocating memory for structure fields */
    CV_CALL( PF->flSamples = (float **) cvAlloc( sizeof( float * ) * SamplesNum ));
    CV_CALL( PF->flNewSamples = (float **) cvAlloc( sizeof( float * ) * SamplesNum ));
    CV_CALL( PF->flSamples[0] = (float *) cvAlloc( sizeof( float ) * SamplesNum * DP ));//�߱�dpά
    CV_CALL( PF->flNewSamples[0] = (float *) cvAlloc( sizeof( float ) * SamplesNum * DP ));

    /* setting pointers in pointer's arrays */
    for( i = 1; i < SamplesNum; i++ )
    {
        PF->flSamples[i] = PF->flSamples[i - 1] + DP;
        PF->flNewSamples[i] = PF->flNewSamples[i - 1] + DP;//ָ��DPά
    }

    CV_CALL( PF->State = (float *) cvAlloc( sizeof( float ) * DP ));
    CV_CALL( PF->DynamMatr = (float *) cvAlloc( sizeof( float ) * DP * DP ));
    CV_CALL( PF->flConfidence = (float *) cvAlloc( sizeof( float ) * SamplesNum ));
    CV_CALL( PF->flCumulative = (float *) cvAlloc( sizeof( float ) * SamplesNum ));
	
    CV_CALL( PF->RandS = (CvRandState *) cvAlloc( sizeof( CvRandState ) * DP ));//CvRandState�ռ�
    CV_CALL( PF->Temp = (float *) cvAlloc( sizeof( float ) * DP ));
    CV_CALL( PF->RandomSample = (float *) cvAlloc( sizeof( float ) * DP ));

    /* Returning created structure */
    __END__;

    return PF;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvReleaseParticleFilter
//    Purpose: Releases CvParticleFilter structure and frees memory allocated for it
//    Context:
//    Parameters:
//    ParticleFilter     - double pointer to CvParticleFilter structure
//    Returns:
//    Notes:
//      
//F*/

extern "C" void 
cvReleaseParticleFilter( CvParticleFilter ** ParticleFilter )
{
    CV_FUNCNAME( "cvReleaseParticleFilter" );
    __BEGIN__;
    
    CvParticleFilter *PF = *ParticleFilter;
    
    if( !ParticleFilter )
        CV_ERROR( CV_StsNullPtr, "" );

    /* freeing the memory */
	cvFree( (void**)&PF->State );
    cvFree( (void**)&PF->DynamMatr);
    cvFree( (void**)&PF->flConfidence );
    cvFree( (void**)&PF->flCumulative );
    cvFree( (void**)&PF->flSamples[0] );
    cvFree( (void**)&PF->flNewSamples[0] );
    cvFree( (void**)&PF->flSamples );
    cvFree( (void**)&PF->flNewSamples );
    cvFree( (void**)&PF->Temp );
    cvFree( (void**)&PF->RandS );
    cvFree( (void**)&PF->RandomSample );
    /* release structure */
    cvFree( (void**)ParticleFilter );
    
    __END__;

}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvParticleFilterPredict
//    Purpose: Performing Time Update routine for Particle Filter algorithm
//    Context:
//    Parameters:
//      ParticleFilter     - pointer to CvParticleFilter structure
//    Returns:
//    Notes:
//      
//F*/

extern "C" void
cvParticleFilterPredict( CvParticleFilter * ParticleFilter )
{
    int i, j;
    float Sum = 0;

    CV_FUNCNAME( "cvParticleFilterPredict" );
    __BEGIN__;

    CvParticleFilter *PF = ParticleFilter;

    if( !ParticleFilter )
        CV_ERROR( CV_StsNullPtr, "" );

    /* Sets Temp to Zero */
     icvSetZero_32f( PF->Temp, PF->DP, 1 );//memset 

    /* Calculating the Mean */
     for( i = 0; i < PF->SamplesNum; i++ )
    {
        icvScaleVector_32f( PF->flSamples[i], PF->State, PF->DP, PF->flConfidence[i] );
        icvAddVector_32f( PF->Temp, PF->State, PF->Temp, PF->DP );
        Sum += PF->flConfidence[i];
        PF->flCumulative[i] = Sum;
    }

    /* Taking the new vector from transformation of mean by dynamics matrix ����F*/

   icvScaleVector_32f( PF->Temp, PF->Temp, PF->DP, 1.f / Sum );
    icvTransformVector_32f( PF->DynamMatr, PF->Temp, PF->State, PF->DP, PF->DP );
    Sum = Sum / PF->SamplesNum;   //���þ���F����PF->State

    /* Updating the set of random samples */
      for( i = 0; i < PF->SamplesNum; i++ )
    {
        j = 0;
        while( (PF->flCumulative[j] <= (float) i * Sum)&&(j<PF->SamplesNum-1))
        {
            j++;
        }
        icvCopyVector_32f( PF->flSamples[j], PF->DP, PF->flNewSamples[i] );
    }//�ҳ���Ȩֵ���Ӳ����Ƶõ�PF->flNewSamples[i]
 
    /* Adding the random-generated vector to every vector in sample set */
    for( i = 0; i < PF->SamplesNum; i++ )
    {
        for( j = 0; j < PF->DP; j++ )
        {
            cvbRand( PF->RandS + j, PF->RandomSample + j, 1 );
        }
                  //���þ������PF->flNewSamples[i]��PF->flSamples[i]
        icvTransformVector_32f( PF->DynamMatr, PF->flNewSamples[i], PF->flSamples[i], PF->DP, PF->DP );
        icvAddVector_32f( PF->flSamples[i], PF->RandomSample, PF->flSamples[i], PF->DP );
    }

    __END__;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvParticleFilterInitSampleSet
//    Purpose: Initializing sample set for Particle Filter algorithm
//    Context:
//    Parameters:
//    ParticleFilter    - pointer to CvParticleFilter structure
//    lowerBound        - vector of lower bounds used to random update of sample set
//    upperBound        - vector of upper bounds used to random update of sample set
//    Returns:
//    Notes:
//      
//F*/

extern "C" void
cvParticleFilterInitSampleSet( CvParticleFilter * ParticleFilter, CvMat * lowerBound, CvMat * upperBound )
{
    int i, j;
    float *LBound;//��1�� ����ΪDP ��4
    float *UBound;
    float Prob = 1.f / ParticleFilter->SamplesNum;//Ȩ��

    CV_FUNCNAME( "cvParticleFilterInitSampleSet" );
    __BEGIN__;

    CvParticleFilter *PF = ParticleFilter;

    if( !ParticleFilter || !lowerBound || !upperBound )
        CV_ERROR( CV_StsNullPtr, "" );

    if( CV_MAT_TYPE(lowerBound->type) != CV_32FC1 ||
        !CV_ARE_TYPES_EQ(lowerBound,upperBound) )
        CV_ERROR( CV_StsBadArg, "source  has not appropriate format" );

    if( (lowerBound->cols != 1) || (upperBound->cols != 1) )
        CV_ERROR( CV_StsBadArg, "source  has not appropriate size" );

    if( (lowerBound->rows != ParticleFilter->DP) || (upperBound->rows != ParticleFilter->DP) )
        CV_ERROR( CV_StsBadArg, "source  has not appropriate size" );

    LBound = lowerBound->data.fl;
    UBound = upperBound->data.fl;

    /* Initializing the structures to create initial Sample set ��ʼ���ṹPF->RandS*/
    for( i = 0; i < PF->DP/2; i++ )//�����������ǰ��ά 
    {
        cvRandInit( &(PF->RandS[i]),//������½� ���� ��ʼ������������� ���ṹRandS[i]�Ĳ�����ֵ
                    LBound[i],
                    UBound[i],
                    i );//Ĭ��ΪCV_RAND_UNI 
    }
    /* Generating the samples */
    for( j = 0; j < PF->SamplesNum; j++ )
    {
        for( i = 0; i < PF->DP/2; i++ )//PF->flSamples[j]ǰ��ά
        {
            cvbRand( PF->RandS + i, PF->flSamples[j] + i, 1 );//������̬�ֲ�������� flSamples����Ϊ1
        }
        PF->flConfidence[j] = Prob;//����Ȩֵ
    }

	for( j = 0; j < PF->SamplesNum; j++ )
    {
        for( i = 0; i < PF->DP/2; i++ )//PF->flSamples[j]����ά
        {
            *(PF->flSamples[j]+i+PF->DP/2) = *(PF->flSamples[j]+i);
        }
    }

    /* Reinitializes the structures to update samples randomly �ٴ������ʼ���ṹ*/
    for( i = 0; i < PF->DP/2; i++ )
    {
        cvRandInit( &(PF->RandS[i]),
                    0,
                    1,
                    i, CV_RAND_NORMAL);//��̬�ֲ������� ��ֵ 
    }

    __END__;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvParticleFilterPropagate
//    Purpose: Propagating Particle Filter sample set
//    Context:
//    Parameters:
//      ParticleFilter     - pointer to CvParticleFilter structure
//    Returns:
//    Notes:
//      
//F*/

extern "C" void
cvParticleFilterPropagate( CvParticleFilter* ParticleFilter)
{
    int i, j;

    CV_FUNCNAME( "cvParticleFilterPropagate" );
    __BEGIN__;

    CvParticleFilter *PF = ParticleFilter;

    if( !ParticleFilter )
        CV_ERROR( CV_StsNullPtr, "" );

    /* Adding the random-generated ������������� vector to every vector in sample set */
	for( j = 0; j < PF->DP; j++)
	{
		*(PF->RandomSample+j) = 0;
	}

    for( i = 0; i < PF->SamplesNum; i++ )
    {
        for( j = 0; j < PF->DP/2; j++ )
        {
            cvbRand( PF->RandS + j, PF->RandomSample + j, 1 );
        }
        //������� PF->flNewSamples[i] ��������ת�ƾ������˻�����������״̬����
        icvTransformVector_32f( PF->DynamMatr, PF->flSamples[i], PF->flNewSamples[i], PF->DP, PF->DP );
        icvAddVector_32f( PF->flNewSamples[i], PF->RandomSample, PF->flSamples[i], PF->DP );//������ӽ����flsamples
    }

    __END__;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvParticleFilterResample
//    Purpose: Resampling Particle Filter sample set
//    Context:
//    Parameters:
//      ParticleFilter     - pointer to CvParticleFilter structure
//    Returns:
//    Notes:
//      
//F*/

extern "C" void
cvParticleFilterResample( CvParticleFilter* ParticleFilter)
{
    int i, j;
    float Sum = 0;

    CV_FUNCNAME( "cvParticleFilterResample" );
    __BEGIN__;

    CvParticleFilter *PF = ParticleFilter;

    if( !ParticleFilter )
        CV_ERROR( CV_StsNullPtr, "" );

    /* Calculating the Distribution Function */
    for( i = 0; i < PF->SamplesNum; i++ )
    {
        Sum += PF->flConfidence[i];
        PF->flCumulative[i] = Sum;
    }

    Sum = Sum / PF->SamplesNum;//sumȡ��ֵ

    /* Updating the set of random samples */ //�������Ӷ����� ����ƶ��
    for( i = 0; i < PF->SamplesNum; i++ )
    {
        j = 0;
        while( (PF->flCumulative[j] <= (float) i * Sum)&&(j<PF->SamplesNum-1))
        {
            j++;//�ҵ��ۻ�Ȩֵ�ʹ��ھ�ֵ�͵�Ȩֵ����
        }
        icvCopyVector_32f( PF->flSamples[j], PF->DP, PF->flNewSamples[i] );//����flsamples[j]��ָdp���ֽڵ�PF->flNewSamples[i] 
    }

	for( i = 0; i < PF->SamplesNum; i++ )
	{
		icvCopyVector_32f( PF->flNewSamples[i], PF->DP, PF->flSamples[i] );
		PF->flConfidence[i] = 1.f/PF->SamplesNum;//������ӵ��ز��� �����е�Ȩֵ����Ϊ��ͬ
	}

    __END__;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name:    cvParticleFilterExpectation
//    Purpose: Generating expectation of the state
//    Context:
//    Parameters:
//      ParticleFilter     - pointer to CvParticleFilter structure
//    Returns:
//    Notes:
//      
//F*/

extern "C" void
cvParticleFilterExpectation( CvParticleFilter* ParticleFilter)
{
    int i;

    CV_FUNCNAME( "cvParticleFilterExpectation" );
    __BEGIN__;

    CvParticleFilter *PF = ParticleFilter;

    if( !ParticleFilter )
        CV_ERROR( CV_StsNullPtr, "" );

    /* Sets Temp to Zero */
    icvSetZero_32f( PF->Temp, PF->DP, 1 );//DP��1������

    /* Calculating the Mean */
    for( i = 0; i < PF->SamplesNum; i++ )
    {
        icvScaleVector_32f( PF->flSamples[i], PF->State, PF->DP, PF->flConfidence[i] );//PF->flSamples[i]��Ȩֵ��state
        icvAddVector_32f( PF->Temp, PF->State, PF->Temp, PF->DP );//f[i j]��i�����ӵ�jά
    }//���Ӽ�Ȩ��tmp

	icvCopyVector_32f(PF->Temp, PF->DP, PF->State);//���״̬���� �õ�State

    __END__;	
}