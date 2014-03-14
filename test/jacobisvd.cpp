// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008 Gael Guennebaud <gael.guennebaud@inria.fr>
// Copyright (C) 2009 Benoit Jacob <jacob.benoit.1@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

// discard stack allocation as that too bypasses malloc
#define EIGEN_STACK_ALLOCATION_LIMIT 0
#define EIGEN_RUNTIME_NO_MALLOC
#include "main.h"
#include <Eigen/SVD>

template<typename MatrixType, int QRPreconditioner>
void jacobisvd_check_full(const MatrixType& m, const JacobiSVD<MatrixType, QRPreconditioner>& svd)
{
  typedef typename MatrixType::Index Index;
  Index rows = m.rows();
  Index cols = m.cols();

  enum {
    RowsAtCompileTime = MatrixType::RowsAtCompileTime,
    ColsAtCompileTime = MatrixType::ColsAtCompileTime
  };

  typedef typename MatrixType::Scalar Scalar;
  typedef Matrix<Scalar, RowsAtCompileTime, RowsAtCompileTime> MatrixUType;
  typedef Matrix<Scalar, ColsAtCompileTime, ColsAtCompileTime> MatrixVType;

  MatrixType sigma = MatrixType::Zero(rows,cols);
  sigma.diagonal() = svd.singularValues().template cast<Scalar>();
  MatrixUType u = svd.matrixU();
  MatrixVType v = svd.matrixV();

  VERIFY_IS_APPROX(m, u * sigma * v.adjoint());
  VERIFY_IS_UNITARY(u);
  VERIFY_IS_UNITARY(v);
}

template<typename MatrixType, int QRPreconditioner>
void jacobisvd_compare_to_full(const MatrixType& m,
                               unsigned int computationOptions,
                               const JacobiSVD<MatrixType, QRPreconditioner>& referenceSvd)
{
  typedef typename MatrixType::Index Index;
  Index rows = m.rows();
  Index cols = m.cols();
  Index diagSize = (std::min)(rows, cols);

  JacobiSVD<MatrixType, QRPreconditioner> svd(m, computationOptions);

  VERIFY_IS_APPROX(svd.singularValues(), referenceSvd.singularValues());
  if(computationOptions & ComputeFullU)
    VERIFY_IS_APPROX(svd.matrixU(), referenceSvd.matrixU());
  if(computationOptions & ComputeThinU)
    VERIFY_IS_APPROX(svd.matrixU(), referenceSvd.matrixU().leftCols(diagSize));
  if(computationOptions & ComputeFullV)
    VERIFY_IS_APPROX(svd.matrixV(), referenceSvd.matrixV());
  if(computationOptions & ComputeThinV)
    VERIFY_IS_APPROX(svd.matrixV(), referenceSvd.matrixV().leftCols(diagSize));
}

template<typename MatrixType, int QRPreconditioner>
void jacobisvd_solve(const MatrixType& m, unsigned int computationOptions)
{
  typedef typename MatrixType::Scalar Scalar;
  typedef typename MatrixType::Index Index;
  Index rows = m.rows();
  Index cols = m.cols();

  enum {
    RowsAtCompileTime = MatrixType::RowsAtCompileTime,
    ColsAtCompileTime = MatrixType::ColsAtCompileTime
  };

  typedef Matrix<Scalar, RowsAtCompileTime, Dynamic> RhsType;
  typedef Matrix<Scalar, ColsAtCompileTime, Dynamic> SolutionType;

  RhsType rhs = RhsType::Random(rows, internal::random<Index>(1, cols));
  JacobiSVD<MatrixType, QRPreconditioner> svd(m, computationOptions);
  SolutionType x = svd.solve(rhs);
  // evaluate normal equation which works also for least-squares solutions
  VERIFY_IS_APPROX(m.adjoint()*m*x,m.adjoint()*rhs);
  
  // check minimal norm solutions
  {
    // generate a full-rank m x n problem with m<n
    enum {
      RankAtCompileTime2 = ColsAtCompileTime==Dynamic ? Dynamic : (ColsAtCompileTime)/2+1,
      RowsAtCompileTime3 = ColsAtCompileTime==Dynamic ? Dynamic : ColsAtCompileTime+1
    };
    typedef Matrix<Scalar, RankAtCompileTime2, ColsAtCompileTime> MatrixType2;
    typedef Matrix<Scalar, RankAtCompileTime2, 1> RhsType2;
    typedef Matrix<Scalar, ColsAtCompileTime, RankAtCompileTime2> MatrixType2T;
    Index rank = RankAtCompileTime2==Dynamic ? internal::random<Index>(1,cols) : Index(RankAtCompileTime2);
    MatrixType2 m2(rank,cols);
    int guard = 0;
    do {
      m2.setRandom();
    } while(m2.jacobiSvd().setThreshold(test_precision<Scalar>()).rank()!=rank && (++guard)<10);
    VERIFY(guard<10);
    RhsType2 rhs2 = RhsType2::Random(rank);
    // use QR to find a reference minimal norm solution
    HouseholderQR<MatrixType2T> qr(m2.adjoint());
    Matrix<Scalar,Dynamic,1> tmp = qr.matrixQR().topLeftCorner(rank,rank).template triangularView<Upper>().adjoint().solve(rhs2);
    tmp.conservativeResize(cols);
    tmp.tail(cols-rank).setZero();
    SolutionType x21 = qr.householderQ() * tmp;
    // now check with SVD
    JacobiSVD<MatrixType2, ColPivHouseholderQRPreconditioner> svd2(m2, computationOptions);
    SolutionType x22 = svd2.solve(rhs2);
    VERIFY_IS_APPROX(m2*x21, rhs2);
    VERIFY_IS_APPROX(m2*x22, rhs2);
    VERIFY_IS_APPROX(x21, x22);
    
    // Now check with a rank deficient matrix
    typedef Matrix<Scalar, RowsAtCompileTime3, ColsAtCompileTime> MatrixType3;
    typedef Matrix<Scalar, RowsAtCompileTime3, 1> RhsType3;
    Index rows3 = RowsAtCompileTime3==Dynamic ? internal::random<Index>(rank+1,2*cols) : Index(RowsAtCompileTime3);
    Matrix<Scalar,RowsAtCompileTime3,Dynamic> C = Matrix<Scalar,RowsAtCompileTime3,Dynamic>::Random(rows3,rank);
    MatrixType3 m3 = C * m2;
    RhsType3 rhs3 = C * rhs2;
    JacobiSVD<MatrixType3, ColPivHouseholderQRPreconditioner> svd3(m3, computationOptions);
    SolutionType x3 = svd3.solve(rhs3);
    VERIFY_IS_APPROX(m3*x3, rhs3);
    VERIFY_IS_APPROX(m3*x21, rhs3);
    VERIFY_IS_APPROX(m2*x3, rhs2);
    
    VERIFY_IS_APPROX(x21, x3);
  }
}

template<typename MatrixType, int QRPreconditioner>
void jacobisvd_test_all_computation_options(const MatrixType& m)
{
  if (QRPreconditioner == NoQRPreconditioner && m.rows() != m.cols())
    return;
  JacobiSVD<MatrixType, QRPreconditioner> fullSvd(m, ComputeFullU|ComputeFullV);

  jacobisvd_check_full(m, fullSvd);
  jacobisvd_solve<MatrixType, QRPreconditioner>(m, ComputeFullU | ComputeFullV);

  #if defined __INTEL_COMPILER
  // remark #111: statement is unreachable
  #pragma warning disable 111
  #endif
  if(QRPreconditioner == FullPivHouseholderQRPreconditioner)
    return;

  jacobisvd_compare_to_full(m, ComputeFullU, fullSvd);
  jacobisvd_compare_to_full(m, ComputeFullV, fullSvd);
  jacobisvd_compare_to_full(m, 0, fullSvd);

  if (MatrixType::ColsAtCompileTime == Dynamic) {
    // thin U/V are only available with dynamic number of columns
    jacobisvd_compare_to_full(m, ComputeFullU|ComputeThinV, fullSvd);
    jacobisvd_compare_to_full(m,              ComputeThinV, fullSvd);
    jacobisvd_compare_to_full(m, ComputeThinU|ComputeFullV, fullSvd);
    jacobisvd_compare_to_full(m, ComputeThinU             , fullSvd);
    jacobisvd_compare_to_full(m, ComputeThinU|ComputeThinV, fullSvd);
    jacobisvd_solve<MatrixType, QRPreconditioner>(m, ComputeFullU | ComputeThinV);
    jacobisvd_solve<MatrixType, QRPreconditioner>(m, ComputeThinU | ComputeFullV);
    jacobisvd_solve<MatrixType, QRPreconditioner>(m, ComputeThinU | ComputeThinV);

    // test reconstruction
    typedef typename MatrixType::Index Index;
    Index diagSize = (std::min)(m.rows(), m.cols());
    JacobiSVD<MatrixType, QRPreconditioner> svd(m, ComputeThinU | ComputeThinV);
    VERIFY_IS_APPROX(m, svd.matrixU().leftCols(diagSize) * svd.singularValues().asDiagonal() * svd.matrixV().leftCols(diagSize).adjoint());
  }
}

template<typename MatrixType>
void jacobisvd(const MatrixType& a = MatrixType(), bool pickrandom = true)
{
  MatrixType m = pickrandom ? MatrixType::Random(a.rows(), a.cols()) : a;

  jacobisvd_test_all_computation_options<MatrixType, FullPivHouseholderQRPreconditioner>(m);
  jacobisvd_test_all_computation_options<MatrixType, ColPivHouseholderQRPreconditioner>(m);
  jacobisvd_test_all_computation_options<MatrixType, HouseholderQRPreconditioner>(m);
  jacobisvd_test_all_computation_options<MatrixType, NoQRPreconditioner>(m);
}

template<typename MatrixType> void jacobisvd_verify_assert(const MatrixType& m)
{
  typedef typename MatrixType::Scalar Scalar;
  typedef typename MatrixType::Index Index;
  Index rows = m.rows();
  Index cols = m.cols();

  enum {
    RowsAtCompileTime = MatrixType::RowsAtCompileTime,
    ColsAtCompileTime = MatrixType::ColsAtCompileTime
  };

  typedef Matrix<Scalar, RowsAtCompileTime, 1> RhsType;

  RhsType rhs(rows);

  JacobiSVD<MatrixType> svd;
  VERIFY_RAISES_ASSERT(svd.matrixU())
  VERIFY_RAISES_ASSERT(svd.singularValues())
  VERIFY_RAISES_ASSERT(svd.matrixV())
  VERIFY_RAISES_ASSERT(svd.solve(rhs))

  MatrixType a = MatrixType::Zero(rows, cols);
  a.setZero();
  svd.compute(a, 0);
  VERIFY_RAISES_ASSERT(svd.matrixU())
  VERIFY_RAISES_ASSERT(svd.matrixV())
  svd.singularValues();
  VERIFY_RAISES_ASSERT(svd.solve(rhs))

  if (ColsAtCompileTime == Dynamic)
  {
    svd.compute(a, ComputeThinU);
    svd.matrixU();
    VERIFY_RAISES_ASSERT(svd.matrixV())
    VERIFY_RAISES_ASSERT(svd.solve(rhs))

    svd.compute(a, ComputeThinV);
    svd.matrixV();
    VERIFY_RAISES_ASSERT(svd.matrixU())
    VERIFY_RAISES_ASSERT(svd.solve(rhs))

    JacobiSVD<MatrixType, FullPivHouseholderQRPreconditioner> svd_fullqr;
    VERIFY_RAISES_ASSERT(svd_fullqr.compute(a, ComputeFullU|ComputeThinV))
    VERIFY_RAISES_ASSERT(svd_fullqr.compute(a, ComputeThinU|ComputeThinV))
    VERIFY_RAISES_ASSERT(svd_fullqr.compute(a, ComputeThinU|ComputeFullV))
  }
  else
  {
    VERIFY_RAISES_ASSERT(svd.compute(a, ComputeThinU))
    VERIFY_RAISES_ASSERT(svd.compute(a, ComputeThinV))
  }
}

template<typename MatrixType>
void jacobisvd_method()
{
  enum { Size = MatrixType::RowsAtCompileTime };
  typedef typename MatrixType::RealScalar RealScalar;
  typedef Matrix<RealScalar, Size, 1> RealVecType;
  MatrixType m = MatrixType::Identity();
  VERIFY_IS_APPROX(m.jacobiSvd().singularValues(), RealVecType::Ones());
  VERIFY_RAISES_ASSERT(m.jacobiSvd().matrixU());
  VERIFY_RAISES_ASSERT(m.jacobiSvd().matrixV());
  VERIFY_IS_APPROX(m.jacobiSvd(ComputeFullU|ComputeFullV).solve(m), m);
}

// work around stupid msvc error when constructing at compile time an expression that involves
// a division by zero, even if the numeric type has floating point
template<typename Scalar>
EIGEN_DONT_INLINE Scalar zero() { return Scalar(0); }

// workaround aggressive optimization in ICC
template<typename T> EIGEN_DONT_INLINE  T sub(T a, T b) { return a - b; }

template<typename MatrixType>
void jacobisvd_inf_nan()
{
  // all this function does is verify we don't iterate infinitely on nan/inf values

  JacobiSVD<MatrixType> svd;
  typedef typename MatrixType::Scalar Scalar;
  Scalar some_inf = Scalar(1) / zero<Scalar>();
  VERIFY(sub(some_inf, some_inf) != sub(some_inf, some_inf));
  svd.compute(MatrixType::Constant(10,10,some_inf), ComputeFullU | ComputeFullV);

  Scalar some_nan = zero<Scalar>() / zero<Scalar>();
  VERIFY(some_nan != some_nan);
  svd.compute(MatrixType::Constant(10,10,some_nan), ComputeFullU | ComputeFullV);

  MatrixType m = MatrixType::Zero(10,10);
  m(internal::random<int>(0,9), internal::random<int>(0,9)) = some_inf;
  svd.compute(m, ComputeFullU | ComputeFullV);

  m = MatrixType::Zero(10,10);
  m(internal::random<int>(0,9), internal::random<int>(0,9)) = some_nan;
  svd.compute(m, ComputeFullU | ComputeFullV);
}

// Regression test for bug 286: JacobiSVD loops indefinitely with some
// matrices containing denormal numbers.
void jacobisvd_underoverflow()
{
#if defined __INTEL_COMPILER
// shut up warning #239: floating point underflow
#pragma warning push
#pragma warning disable 239
#endif
  Matrix2d M;
  M << -7.90884e-313, -4.94e-324,
                 0, 5.60844e-313;
#if defined __INTEL_COMPILER
#pragma warning pop
#endif
  JacobiSVD<Matrix2d> svd;
  svd.compute(M); // just check we don't loop indefinitely
  
  // Check for overflow:
  Matrix3d M3;
  M3 << 4.4331978442502944e+307, -5.8585363752028680e+307,  6.4527017443412964e+307,
        3.7841695601406358e+307,  2.4331702789740617e+306, -3.5235707140272905e+307,
       -8.7190887618028355e+307, -7.3453213709232193e+307, -2.4367363684472105e+307;

  JacobiSVD<Matrix3d> svd3;
  svd3.compute(M3); // just check we don't loop indefinitely
}

void jacobisvd_preallocate()
{
  Vector3f v(3.f, 2.f, 1.f);
  MatrixXf m = v.asDiagonal();

  internal::set_is_malloc_allowed(false);
  VERIFY_RAISES_ASSERT(VectorXf tmp(10);)
  JacobiSVD<MatrixXf> svd;
  internal::set_is_malloc_allowed(true);
  svd.compute(m);
  VERIFY_IS_APPROX(svd.singularValues(), v);

  JacobiSVD<MatrixXf> svd2(3,3);
  internal::set_is_malloc_allowed(false);
  svd2.compute(m);
  internal::set_is_malloc_allowed(true);
  VERIFY_IS_APPROX(svd2.singularValues(), v);
  VERIFY_RAISES_ASSERT(svd2.matrixU());
  VERIFY_RAISES_ASSERT(svd2.matrixV());
  svd2.compute(m, ComputeFullU | ComputeFullV);
  VERIFY_IS_APPROX(svd2.matrixU(), Matrix3f::Identity());
  VERIFY_IS_APPROX(svd2.matrixV(), Matrix3f::Identity());
  internal::set_is_malloc_allowed(false);
  svd2.compute(m);
  internal::set_is_malloc_allowed(true);

  JacobiSVD<MatrixXf> svd3(3,3,ComputeFullU|ComputeFullV);
  internal::set_is_malloc_allowed(false);
  svd2.compute(m);
  internal::set_is_malloc_allowed(true);
  VERIFY_IS_APPROX(svd2.singularValues(), v);
  VERIFY_IS_APPROX(svd2.matrixU(), Matrix3f::Identity());
  VERIFY_IS_APPROX(svd2.matrixV(), Matrix3f::Identity());
  internal::set_is_malloc_allowed(false);
  svd2.compute(m, ComputeFullU|ComputeFullV);
  internal::set_is_malloc_allowed(true);
}

void test_jacobisvd()
{
  CALL_SUBTEST_3(( jacobisvd_verify_assert(Matrix3f()) ));
  CALL_SUBTEST_4(( jacobisvd_verify_assert(Matrix4d()) ));
  CALL_SUBTEST_7(( jacobisvd_verify_assert(MatrixXf(10,12)) ));
  CALL_SUBTEST_8(( jacobisvd_verify_assert(MatrixXcd(7,5)) ));

  for(int i = 0; i < g_repeat; i++) {
    Matrix2cd m;
    m << 0, 1,
         0, 1;
    CALL_SUBTEST_1(( jacobisvd(m, false) ));
    m << 1, 0,
         1, 0;
    CALL_SUBTEST_1(( jacobisvd(m, false) ));

    Matrix2d n;
    n << 0, 0,
         0, 0;
    CALL_SUBTEST_2(( jacobisvd(n, false) ));
    n << 0, 0,
         0, 1;
    CALL_SUBTEST_2(( jacobisvd(n, false) ));
    
    CALL_SUBTEST_3(( jacobisvd<Matrix3f>() ));
    CALL_SUBTEST_4(( jacobisvd<Matrix4d>() ));
    CALL_SUBTEST_5(( jacobisvd<Matrix<float,3,5> >() ));
    CALL_SUBTEST_6(( jacobisvd<Matrix<double,Dynamic,2> >(Matrix<double,Dynamic,2>(10,2)) ));

    int r = internal::random<int>(1, 30),
        c = internal::random<int>(1, 30);
    
    TEST_SET_BUT_UNUSED_VARIABLE(r)
    TEST_SET_BUT_UNUSED_VARIABLE(c)
    
    CALL_SUBTEST_7(( jacobisvd<MatrixXf>(MatrixXf(r,c)) ));
    CALL_SUBTEST_8(( jacobisvd<MatrixXcd>(MatrixXcd(r,c)) ));
    (void) r;
    (void) c;

    // Test on inf/nan matrix
    CALL_SUBTEST_7( jacobisvd_inf_nan<MatrixXf>() );
  }

  CALL_SUBTEST_7(( jacobisvd<MatrixXf>(MatrixXf(internal::random<int>(EIGEN_TEST_MAX_SIZE/4, EIGEN_TEST_MAX_SIZE/2), internal::random<int>(EIGEN_TEST_MAX_SIZE/4, EIGEN_TEST_MAX_SIZE/2))) ));
  CALL_SUBTEST_8(( jacobisvd<MatrixXcd>(MatrixXcd(internal::random<int>(EIGEN_TEST_MAX_SIZE/4, EIGEN_TEST_MAX_SIZE/3), internal::random<int>(EIGEN_TEST_MAX_SIZE/4, EIGEN_TEST_MAX_SIZE/3))) ));

  // test matrixbase method
  CALL_SUBTEST_1(( jacobisvd_method<Matrix2cd>() ));
  CALL_SUBTEST_3(( jacobisvd_method<Matrix3f>() ));

  // Test problem size constructors
  CALL_SUBTEST_7( JacobiSVD<MatrixXf>(10,10) );

  // Check that preallocation avoids subsequent mallocs
  CALL_SUBTEST_9( jacobisvd_preallocate() );

  // Regression check for bug 286
  CALL_SUBTEST_2( jacobisvd_underoverflow() );
}
