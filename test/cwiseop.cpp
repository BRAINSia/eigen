// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
// Copyright (C) 2006-2008 Benoit Jacob <jacob@math.jussieu.fr>
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
#include <functional>
#include <Eigen/Array>

using namespace std;

template<typename Scalar> struct AddIfNull {
    const Scalar operator() (const Scalar a, const Scalar b) const {return a<=1e-3 ? b : a;}
    enum { Cost = NumTraits<Scalar>::AddCost };
};

template<typename MatrixType> void cwiseops(const MatrixType& m)
{
  typedef typename MatrixType::Scalar Scalar;
  typedef typename NumTraits<Scalar>::Real RealScalar;
  typedef Matrix<Scalar, MatrixType::RowsAtCompileTime, 1> VectorType;

  int rows = m.rows();
  int cols = m.cols();

  MatrixType m1 = test_random_matrix<MatrixType>(rows, cols),
             m2 = test_random_matrix<MatrixType>(rows, cols),
             m3(rows, cols),
             mzero = MatrixType::Zero(rows, cols),
             mones = MatrixType::Ones(rows, cols),
             identity = Matrix<Scalar, MatrixType::RowsAtCompileTime, MatrixType::RowsAtCompileTime>
                              ::Identity(rows, rows),
             square = test_random_matrix<Matrix<Scalar, MatrixType::RowsAtCompileTime, MatrixType::RowsAtCompileTime> >(rows, rows);
  VectorType v1 = test_random_matrix<VectorType>(rows),
             v2 = test_random_matrix<VectorType>(rows),
             vzero = VectorType::Zero(rows);

  int r = ei_random<int>(0, rows-1),
      c = ei_random<int>(0, cols-1);

  m2 = m2.template binaryExpr<AddIfNull<Scalar> >(mones);

  VERIFY_IS_APPROX(m1.cwise().pow(2), m1.cwise().abs2());
  VERIFY_IS_APPROX(m1.cwise().pow(2), m1.cwise().square());
  VERIFY_IS_APPROX(m1.cwise().pow(3), m1.cwise().cube());

  VERIFY_IS_APPROX(m1 + mones, m1.cwise()+Scalar(1));
  VERIFY_IS_APPROX(m1 - mones, m1.cwise()-Scalar(1));
  m3 = m1; m3.cwise() += 1;
  VERIFY_IS_APPROX(m1 + mones, m3);
  m3 = m1; m3.cwise() -= 1;
  VERIFY_IS_APPROX(m1 - mones, m3);

  VERIFY_IS_APPROX(m2, m2.cwise() * mones);
  VERIFY_IS_APPROX(m1.cwise() * m2,  m2.cwise() * m1);
  
  VERIFY_IS_APPROX(mones,    m2.cwise()/m2);
  if(NumTraits<Scalar>::HasFloatingPoint)
  {
    VERIFY_IS_APPROX(m1.cwise() / m2,    m1.cwise() * (m2.cwise().inverse()));
    m3 = m1.cwise().abs().cwise().sqrt();
    VERIFY_IS_APPROX(m3.cwise().square(), m1.cwise().abs());
    VERIFY_IS_APPROX(m1.cwise().square().cwise().sqrt(), m1.cwise().abs());
    VERIFY_IS_APPROX(m1.cwise().abs().cwise().log().cwise().exp() , m1.cwise().abs());
    
//     VERIFY_IS_APPROX(m1.cwise().pow(-1), m1.cwise().inverse());
//     VERIFY_IS_APPROX(m1.cwise().pow(0.5), m1.cwise().sqrt());
//     VERIFY_IS_APPROX(m1.cwise().tan(), m1.cwise().sin().cwise() / m1.cwise().cos());
    VERIFY_IS_APPROX(mones, m1.cwise().sin().cwise().square() + m1.cwise().cos().cwise().square());
  }

  // check min
  VERIFY_IS_APPROX( m1.cwise().min(m2), m2.cwise().min(m1) );
  VERIFY_IS_APPROX( m1.cwise().min(m1+mones), m1 );
  VERIFY_IS_APPROX( m1.cwise().min(m1-mones), m1-mones );

  // check max
  VERIFY_IS_APPROX( m1.cwise().max(m2), m2.cwise().max(m1) );
  VERIFY_IS_APPROX( m1.cwise().max(m1-mones), m1 );
  VERIFY_IS_APPROX( m1.cwise().max(m1+mones), m1+mones );
  
  VERIFY( (m1.cwise() == m1).all() );
  VERIFY( (m1.cwise() != m2).any() );
  VERIFY(!(m1.cwise() == (m1+mones)).any() );
  if (rows*cols>1)
  {
    m3 = m1;
    m3(r,c) += 1;
    VERIFY( (m1.cwise() == m3).any() );
    VERIFY( !(m1.cwise() == m3).all() );
  }
  VERIFY( (m1.cwise().min(m2).cwise() <= m2).all() );
  VERIFY( (m1.cwise().max(m2).cwise() >= m2).all() );
  VERIFY( (m1.cwise().min(m2).cwise() < (m1+mones)).all() );
  VERIFY( (m1.cwise().max(m2).cwise() > (m1-mones)).all() );

  VERIFY( (m1.cwise()<m1.unaryExpr(bind2nd(plus<Scalar>(), Scalar(1)))).all() );
  VERIFY( !(m1.cwise()<m1.unaryExpr(bind2nd(minus<Scalar>(), Scalar(1)))).all() );
  VERIFY( !(m1.cwise()>m1.unaryExpr(bind2nd(plus<Scalar>(), Scalar(1)))).any() );
}

void test_cwiseop()
{
  for(int i = 0; i < g_repeat ; i++) {
    CALL_SUBTEST( cwiseops(Matrix<float, 1, 1>()) );
    CALL_SUBTEST( cwiseops(Matrix4d()) );
    CALL_SUBTEST( cwiseops(MatrixXf(3, 3)) );
    CALL_SUBTEST( cwiseops(MatrixXf(22, 22)) );
    CALL_SUBTEST( cwiseops(MatrixXi(8, 12)) );
    CALL_SUBTEST( cwiseops(MatrixXd(20, 20)) );
  }
}
