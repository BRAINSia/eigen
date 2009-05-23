// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2009 Mark Borgerding mark a borgerding net
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#include "main.h"
#include <unsupported/Eigen/FFT.h>


using namespace std;

template < typename T>
complex<long double>  promote(complex<T> x) { return complex<long double>(x.real(),x.imag()); }

complex<long double>  promote(float x) { return complex<long double>( x); }
complex<long double>  promote(double x) { return complex<long double>( x); }
complex<long double>  promote(long double x) { return complex<long double>( x); }
    

    template <typename T1,typename T2>
    long double fft_rmse( const vector<T1> & fftbuf,const vector<T2> & timebuf)
    {
        long double totalpower=0;
        long double difpower=0;
        for (size_t k0=0;k0<fftbuf.size();++k0) {
            complex<long double> acc = 0;
            long double phinc = -2.*k0* M_PIl / timebuf.size();
            for (size_t k1=0;k1<timebuf.size();++k1) {
                acc +=  promote( timebuf[k1] ) * exp( complex<long double>(0,k1*phinc) );
            }
            totalpower += norm(acc);
            complex<long double> x = promote(fftbuf[k0]); 
            complex<long double> dif = acc - x;
            difpower += norm(dif);
            cerr << k0 << ":" << acc << " " <<  x << endl;
        }
        cerr << "rmse:" << sqrt(difpower/totalpower) << endl;
        return sqrt(difpower/totalpower);
    }

    template <typename T1,typename T2>
    long double dif_rmse( const vector<T1> buf1,const vector<T2> buf2)
    {
        long double totalpower=0;
        long double difpower=0;
        size_t n = min( buf1.size(),buf2.size() );
        for (size_t k=0;k<n;++k) {
            totalpower += (norm( buf1[k] ) + norm(buf2[k]) )/2.;
            difpower += norm(buf1[k] - buf2[k]);
        }
        return sqrt(difpower/totalpower);
    }

template <class T>
void test_scalar(int nfft)
{
    typedef typename Eigen::FFT<T>::Complex Complex;
    typedef typename Eigen::FFT<T>::Scalar Scalar;

    FFT<T> fft;
    vector<Scalar> inbuf(nfft);
    vector<Complex> outbuf;
    for (int k=0;k<nfft;++k)
        inbuf[k]= (T)(rand()/(double)RAND_MAX - .5);
    fft.fwd( outbuf,inbuf);
    VERIFY( fft_rmse(outbuf,inbuf) < 1e-5 );// gross check
}

template <class T>
void test_complex(int nfft)
{
    typedef typename Eigen::FFT<T>::Complex Complex;

    FFT<T> fft;

    vector<Complex> inbuf(nfft);
    vector<Complex> outbuf;
    vector<Complex> buf3;
    for (int k=0;k<nfft;++k)
        inbuf[k]= Complex( (T)(rand()/(double)RAND_MAX - .5), (T)(rand()/(double)RAND_MAX - .5) );
    fft.fwd( outbuf , inbuf);

    VERIFY( fft_rmse(outbuf,inbuf) < 1e-5 );// gross check

    fft.inv( buf3 , outbuf);

    VERIFY( dif_rmse(inbuf,buf3) < 1e-5 );// gross check
}

void test_FFT()
{
  CALL_SUBTEST( test_complex<float>(32) ); CALL_SUBTEST( test_complex<double>(32) ); CALL_SUBTEST( test_complex<long double>(32) );
  CALL_SUBTEST( test_complex<float>(1024) ); CALL_SUBTEST( test_complex<double>(1024) ); CALL_SUBTEST( test_complex<long double>(1024) );
  CALL_SUBTEST( test_complex<float>(3*8) ); CALL_SUBTEST( test_complex<double>(3*8) ); CALL_SUBTEST( test_complex<long double>(3*8) );
  CALL_SUBTEST( test_complex<float>(5*32) ); CALL_SUBTEST( test_complex<double>(5*32) ); CALL_SUBTEST( test_complex<long double>(5*32) );
  CALL_SUBTEST( test_complex<float>(2*3*4) ); CALL_SUBTEST( test_complex<double>(2*3*4) ); CALL_SUBTEST( test_complex<long double>(2*3*4) );
  CALL_SUBTEST( test_complex<float>(2*3*4*5) ); CALL_SUBTEST( test_complex<double>(2*3*4*5) ); CALL_SUBTEST( test_complex<long double>(2*3*4*5) );
  CALL_SUBTEST( test_complex<float>(2*3*4*5*7) ); CALL_SUBTEST( test_complex<double>(2*3*4*5*7) ); CALL_SUBTEST( test_complex<long double>(2*3*4*5*7) );
/*
  CALL_SUBTEST( test_scalar<float>(32) ); CALL_SUBTEST( test_scalar<double>(32) ); CALL_SUBTEST( test_scalar<long double>(32) );
  CALL_SUBTEST( test_scalar<float>(1024) ); CALL_SUBTEST( test_scalar<double>(1024) ); CALL_SUBTEST( test_scalar<long double>(1024) );
  CALL_SUBTEST( test_scalar<float>(2*3*4*5*7) ); CALL_SUBTEST( test_scalar<double>(2*3*4*5*7) ); CALL_SUBTEST( test_scalar<long double>(2*3*4*5*7) );
  */
}
