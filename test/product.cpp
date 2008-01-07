// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2006-2008 Benoit Jacob <jacob@math.jussieu.fr>
//
// Eigen is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along
// with Eigen; if not, write to the Free Software Foundation, Inc., 51
// Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. This exception does not invalidate any other reasons why a work
// based on this file might be covered by the GNU General Public License.

#include "main.h"

namespace Eigen {

template<typename MatrixType> void product(const MatrixType& m)
{
  /* this test covers the following files:
     Identity.h Product.h
  */

  typedef typename MatrixType::Scalar Scalar;
  typedef Matrix<Scalar, MatrixType::Traits::RowsAtCompileTime, 1> VectorType;
  
  int rows = m.rows();
  int cols = m.cols();
  
  // this test relies a lot on Random.h, and there's not much more that we can do
  // to test it, hence I consider that we will have tested Random.h
  MatrixType m1 = MatrixType::random(rows, cols),
             m2 = MatrixType::random(rows, cols),
             m3(rows, cols),
             mzero = MatrixType::zero(rows, cols),
             identity = Matrix<Scalar, MatrixType::Traits::RowsAtCompileTime, MatrixType::Traits::RowsAtCompileTime>
                              ::identity(rows),
             square = Matrix<Scalar, MatrixType::Traits::RowsAtCompileTime, MatrixType::Traits::RowsAtCompileTime>
                              ::random(rows, rows);
  VectorType v1 = VectorType::random(rows),
             v2 = VectorType::random(rows),
             vzero = VectorType::zero(rows);

  Scalar s1 = random<Scalar>();
  
  int r = random<int>(0, rows-1),
      c = random<int>(0, cols-1);
  
  // begin testing Product.h: only associativity for now
  // (we use Transpose.h but this doesn't count as a test for it)
  VERIFY_IS_APPROX((m1*m1.transpose())*m2,  m1*(m1.transpose()*m2));
  m3 = m1;
  m3 *= (m1.transpose() * m2);
  VERIFY_IS_APPROX(m3,                      m1*(m1.transpose()*m2));
  VERIFY_IS_APPROX(m3,                      m1.lazyProduct(m1.transpose()*m2));
  
  // continue testing Product.h: distributivity
  VERIFY_IS_APPROX(square*(m1 + m2),        square*m1+square*m2);
  VERIFY_IS_APPROX(square*(m1 - m2),        square*m1-square*m2);
  
  // continue testing Product.h: compatibility with ScalarMultiple.h
  VERIFY_IS_APPROX(s1*(square*m1),          (s1*square)*m1);
  VERIFY_IS_APPROX(s1*(square*m1),          square*(m1*s1));
  
  // continue testing Product.h: lazyProduct
  VERIFY_IS_APPROX(square.lazyProduct(m1),  square*m1);
  // again, test operator() to check const-qualification
  s1 += square.lazyProduct(m1)(r,c);
  
  // test Product.h together with Identity.h
  VERIFY_IS_APPROX(m1,                      identity*m1);
  VERIFY_IS_APPROX(v1,                      identity*v1);
  // again, test operator() to check const-qualification
  VERIFY_IS_APPROX(MatrixType::identity(std::max(rows,cols))(r,c), static_cast<Scalar>(r==c));
}

void EigenTest::testProduct()
{
  for(int i = 0; i < m_repeat; i++) {
    product(Matrix<float, 1, 1>());
    product(Matrix4d());
    product(MatrixXcf(3, 3));
    product(MatrixXi(8, 12));
    product(MatrixXcd(20, 20));
  }
}

} // namespace Eigen
