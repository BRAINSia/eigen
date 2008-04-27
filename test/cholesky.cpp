// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
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

#include <Eigen/Cholesky>

namespace Eigen {

template<typename MatrixType> void cholesky(const MatrixType& m)
{
  /* this test covers the following files:
     Cholesky.h CholeskyWithoutSquareRoot.h
  */
  int rows = m.rows();
  int cols = m.cols();

  typedef typename MatrixType::Scalar Scalar;
  typedef Matrix<Scalar, MatrixType::ColsAtCompileTime, MatrixType::ColsAtCompileTime> SquareMatrixType;
  typedef Matrix<Scalar, MatrixType::ColsAtCompileTime, 1> VectorType;

  MatrixType a = MatrixType::random(rows,cols).transpose();
  VectorType b = VectorType::random(cols);
  SquareMatrixType covMat =  a.transpose() * a;

  CholeskyWithoutSquareRoot<SquareMatrixType> cholnosqrt(covMat);
  VERIFY_IS_APPROX(covMat, cholnosqrt.matrixU().transpose() * cholnosqrt.vectorD().asDiagonal() * cholnosqrt.matrixU());
  VERIFY_IS_APPROX(covMat * cholnosqrt.solve(b), b);

  Cholesky<SquareMatrixType> chol(covMat);
  VERIFY_IS_APPROX(covMat, chol.matrixU().transpose() * chol.matrixU());
  VERIFY_IS_APPROX(covMat * chol.solve(b), b);
}

void EigenTest::testCholesky()
{
  for(int i = 0; i < 1; i++) {
    cholesky(Matrix3f());
    cholesky(Matrix4d());
    cholesky(MatrixXd(17,17));
  }
}

} // namespace Eigen
