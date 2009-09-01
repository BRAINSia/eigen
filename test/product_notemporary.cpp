// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2006-2008 Benoit Jacob <jacob.benoit.1@gmail.com>
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

static int nb_temporaries;

#define EIGEN_DEBUG_MATRIX_CTOR(MTYPE) { \
    if(MTYPE::SizeAtCompileTime==Dynamic) \
      nb_temporaries++; \
 }

#include "main.h"
#include <Eigen/Array>

#define VERIFY_EVALUATION_COUNT(XPR,N) {\
    nb_temporaries = 0; \
    XPR; \
    if(nb_temporaries!=N) std::cerr << "nb_temporaries == " << nb_temporaries << "\n"; \
    VERIFY( (#XPR) && nb_temporaries==N ); \
  }

template<typename MatrixType> void product_notemporary(const MatrixType& m)
{
  /* This test checks the number of tempories created
   * during the evaluation of a complex expression */

  typedef typename MatrixType::Scalar Scalar;
  typedef Matrix<Scalar, 1, Dynamic> RowVectorType;
  typedef Matrix<Scalar, Dynamic, 1> ColVectorType;
  typedef Matrix<Scalar, Dynamic, Dynamic, RowMajor> RowMajorMatrixType;

  int rows = m.rows();
  int cols = m.cols();

  MatrixType m1 = MatrixType::Random(rows, cols),
             m2 = MatrixType::Random(rows, cols),
             m3(rows, cols);
  RowVectorType rv1 = RowVectorType::Random(rows), rvres(rows);
  ColVectorType vc2 = ColVectorType::Random(cols), cvres(cols);
  RowMajorMatrixType rm3(rows, cols);

  Scalar s1 = ei_random<Scalar>(),
         s2 = ei_random<Scalar>(),
         s3 = ei_random<Scalar>();

  int c0 = ei_random<int>(4,cols-8),
      c1 = ei_random<int>(8,cols-c0),
      r0 = ei_random<int>(4,cols-8),
      r1 = ei_random<int>(8,rows-r0);

  VERIFY_EVALUATION_COUNT( m3 = (m1 * m2.adjoint()), 1);
  VERIFY_EVALUATION_COUNT( m3 = (m1 * m2.adjoint()).lazy(), 0);
  VERIFY_EVALUATION_COUNT( m3.noalias() = m1 * m2.adjoint(), 0);

  // NOTE in this case the slow product is used:
  // FIXME:
  VERIFY_EVALUATION_COUNT( m3.noalias() = s1 * (m1 * m2.transpose()), 0);

  VERIFY_EVALUATION_COUNT( m3.noalias() = s1 * m1 * s2 * m2.adjoint(), 0);
  VERIFY_EVALUATION_COUNT( m3.noalias() = s1 * m1 * s2 * (m1*s3+m2*s2).adjoint(), 1);
  VERIFY_EVALUATION_COUNT( m3.noalias() = (s1 * m1).adjoint() * s2 * m2, 0);
  VERIFY_EVALUATION_COUNT( m3 += (s1 * (-m1*s3).adjoint() * (s2 * m2 * s3)).lazy(), 0);
  VERIFY_EVALUATION_COUNT( m3.noalias() += s1 * (-m1*s3).adjoint() * (s2 * m2 * s3), 0);
  VERIFY_EVALUATION_COUNT( m3 -= (s1 * (m1.transpose() * m2)).lazy(), 0);
  VERIFY_EVALUATION_COUNT( m3.noalias() -= s1 * (m1.transpose() * m2), 0);

  VERIFY_EVALUATION_COUNT(( m3.block(r0,r0,r1,r1).noalias() += -m1.block(r0,c0,r1,c1) * (s2*m2.block(r0,c0,r1,c1)).adjoint() ), 0);
  VERIFY_EVALUATION_COUNT(( m3.block(r0,r0,r1,r1).noalias() -= s1 * m1.block(r0,c0,r1,c1) * m2.block(c0,r0,c1,r1) ), 0);

  // NOTE this is because the Block expression is not handled yet by our expression analyser
  VERIFY_EVALUATION_COUNT(( m3.block(r0,r0,r1,r1) = (s1 * m1.block(r0,c0,r1,c1) * (s1*m2).block(c0,r0,c1,r1)).lazy() ), 1);

  VERIFY_EVALUATION_COUNT( m3.noalias() -= (s1 * m1).template triangularView<LowerTriangular>() * m2, 0);
  VERIFY_EVALUATION_COUNT( rm3.noalias() = (s1 * m1.adjoint()).template triangularView<UpperTriangular>() * (m2+m2), 1);
  VERIFY_EVALUATION_COUNT( rm3.noalias() = (s1 * m1.adjoint()).template triangularView<UnitUpperTriangular>() * m2.adjoint(), 0);

  VERIFY_EVALUATION_COUNT( rm3.col(c0).noalias() = (s1 * m1.adjoint()).template triangularView<UnitUpperTriangular>() * (s2*m2.row(c0)).adjoint(), 0);

  VERIFY_EVALUATION_COUNT( m1.template triangularView<LowerTriangular>().solveInPlace(m3), 0);
  VERIFY_EVALUATION_COUNT( m1.adjoint().template triangularView<LowerTriangular>().solveInPlace(m3.transpose()), 0);

  VERIFY_EVALUATION_COUNT( m3.noalias() -= (s1 * m1).adjoint().template selfadjointView<LowerTriangular>() * (-m2*s3).adjoint(), 0);
  VERIFY_EVALUATION_COUNT( m3.noalias() = s2 * m2.adjoint() * (s1 * m1.adjoint()).template selfadjointView<UpperTriangular>(), 0);
  VERIFY_EVALUATION_COUNT( rm3.noalias() = (s1 * m1.adjoint()).template selfadjointView<LowerTriangular>() * m2.adjoint(), 0);

  VERIFY_EVALUATION_COUNT( m3.col(c0).noalias() = (s1 * m1).adjoint().template selfadjointView<LowerTriangular>() * (-m2.row(c0)*s3).adjoint(), 0);
  VERIFY_EVALUATION_COUNT( m3.col(c0).noalias() -= (s1 * m1).adjoint().template selfadjointView<UpperTriangular>() * (-m2.row(c0)*s3).adjoint(), 0);

  VERIFY_EVALUATION_COUNT( m3.block(r0,c0,r1,c1).noalias() += m1.block(r0,r0,r1,r1).template selfadjointView<UpperTriangular>() * (s1*m2.block(r0,c0,r1,c1)), 0);
  VERIFY_EVALUATION_COUNT( m3.block(r0,c0,r1,c1).noalias() = m1.block(r0,r0,r1,r1).template selfadjointView<UpperTriangular>() * m2.block(r0,c0,r1,c1), 0);

  VERIFY_EVALUATION_COUNT( m3.template selfadjointView<LowerTriangular>().rankUpdate(m2.adjoint()), 0);

  m3.resize(1,1);
  VERIFY_EVALUATION_COUNT( m3.noalias() = m1.block(r0,r0,r1,r1).template selfadjointView<LowerTriangular>() * m2.block(r0,c0,r1,c1), 0);
  m3.resize(1,1);
  VERIFY_EVALUATION_COUNT( m3.noalias() = m1.block(r0,r0,r1,r1).template triangularView<UnitUpperTriangular>()  * m2.block(r0,c0,r1,c1), 0);
}

void test_product_notemporary()
{
  int s;
  for(int i = 0; i < g_repeat; i++) {
    s = ei_random<int>(16,320);
    CALL_SUBTEST( product_notemporary(MatrixXf(s, s)) );
    s = ei_random<int>(16,120);
    CALL_SUBTEST( product_notemporary(MatrixXcd(s,s)) );
  }
}
