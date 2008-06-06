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

#ifndef EIGEN_SELFADJOINTEIGENSOLVER_H
#define EIGEN_SELFADJOINTEIGENSOLVER_H

/** \qr_module
  *
  * \class SelfAdjointEigenSolver
  *
  * \brief Eigen values/vectors solver for selfadjoint matrix
  *
  * \param MatrixType the type of the matrix of which we are computing the eigen decomposition
  *
  * \note MatrixType must be an actual Matrix type, it can't be an expression type.
  *
  * \sa MatrixBase::eigenvalues(), class EigenSolver
  */
template<typename _MatrixType> class SelfAdjointEigenSolver
{
  public:

    typedef _MatrixType MatrixType;
    typedef typename MatrixType::Scalar Scalar;
    typedef typename NumTraits<Scalar>::Real RealScalar;
    typedef std::complex<RealScalar> Complex;
    typedef Matrix<RealScalar, MatrixType::ColsAtCompileTime, 1> RealVectorType;
    typedef Matrix<RealScalar, Dynamic, 1> RealVectorTypeX;
    typedef Tridiagonalization<MatrixType> TridiagonalizationType;

    SelfAdjointEigenSolver(const MatrixType& matrix, bool computeEigenvectors = true)
      : m_eivec(matrix.rows(), matrix.cols()),
        m_eivalues(matrix.cols())
    {
      compute(matrix, computeEigenvectors);
    }

    void compute(const MatrixType& matrix, bool computeEigenvectors = true);

    MatrixType eigenvectors(void) const { ei_assert(m_eigenvectorsOk); return m_eivec; }

    RealVectorType eigenvalues(void) const { return m_eivalues; }

  protected:
    MatrixType m_eivec;
    RealVectorType m_eivalues;
    #ifndef NDEBUG
    bool m_eigenvectorsOk;
    #endif
};

// from Golub's "Matrix Computations", algorithm 5.1.3
template<typename Scalar>
static void ei_givens_rotation(Scalar a, Scalar b, Scalar& c, Scalar& s)
{
  if (b==0)
  {
    c = 1; s = 0;
  }
  else if (ei_abs(b)>ei_abs(a))
  {
    Scalar t = -a/b;
    s = Scalar(1)/ei_sqrt(1+t*t);
    c = s * t;
  }
  else
  {
    Scalar t = -b/a;
    c = Scalar(1)/ei_sqrt(1+t*t);
    s = c * t;
  }
}

/** \internal
  *
  * \qr_module
  *
  * Performs a QR step on a tridiagonal symmetric matrix represented as a
  * pair of two vectors \a diag and \a subdiag.
  *
  * \param matA the input selfadjoint matrix
  * \param hCoeffs returned Householder coefficients
  *
  * For compilation efficiency reasons, this procedure does not use eigen expression
  * for its arguments.
  *
  * Implemented from Golub's "Matrix Computations", algorithm 8.3.2:
  * "implicit symmetric QR step with Wilkinson shift"
  */
template<typename RealScalar, typename Scalar>
static void ei_tridiagonal_qr_step(RealScalar* diag, RealScalar* subdiag, int start, int end, Scalar* matrixQ, int n);

template<typename MatrixType>
void SelfAdjointEigenSolver<MatrixType>::compute(const MatrixType& matrix, bool computeEigenvectors)
{
  m_eigenvectorsOk = computeEigenvectors;
  assert(matrix.cols() == matrix.rows());
  int n = matrix.cols();
  m_eivalues.resize(n,1);
  m_eivec = matrix;

  // FIXME, should tridiag be a local variable of this function or an attribute of SelfAdjointEigenSolver ?
  // the latter avoids multiple memory allocation when the same SelfAdjointEigenSolver is used multiple times...
  // (same for diag and subdiag)
  RealVectorType& diag = m_eivalues;
  typename TridiagonalizationType::SubDiagonalType subdiag(n-1);
  TridiagonalizationType::decomposeInPlace(m_eivec, diag, subdiag, computeEigenvectors);

  int end = n-1;
  int start = 0;
  while (end>0)
  {
    for (int i = start; i<end; ++i)
      if (ei_isMuchSmallerThan(ei_abs(subdiag[i]),(ei_abs(diag[i])+ei_abs(diag[i+1]))))
        subdiag[i] = 0;

    // find the largest unreduced block
    while (end>0 && subdiag[end-1]==0)
      end--;
    if (end<=0)
      break;
    start = end - 1;
    while (start>0 && subdiag[start-1]!=0)
      start--;

    ei_tridiagonal_qr_step(diag.data(), subdiag.data(), start, end, computeEigenvectors ? m_eivec.data() : (Scalar*)0, n);
  }

  // Sort eigenvalues and corresponding vectors.
  // TODO make the sort optional ?
  // TODO use a better sort algorithm !!
  for (int i = 0; i < n-1; i++)
  {
    int k;
    m_eivalues.block(i,n-i).minCoeff(&k);
    if (k > 0)
    {
      std::swap(m_eivalues[i], m_eivalues[k+i]);
      m_eivec.col(i).swap(m_eivec.col(k+i));
    }
  }
}

/** \qr_module
  *
  * \returns a vector listing the eigenvalues of this matrix.
  */
template<typename Derived>
inline Matrix<typename NumTraits<typename ei_traits<Derived>::Scalar>::Real, ei_traits<Derived>::ColsAtCompileTime, 1>
MatrixBase<Derived>::eigenvalues() const
{
  ei_assert(Flags&SelfAdjointBit);
  return SelfAdjointEigenSolver<typename Derived::Eval>(eval(),false).eigenvalues();
}

template<typename Derived, bool IsSelfAdjoint>
struct ei_matrixNorm_selector
{
  static inline typename NumTraits<typename ei_traits<Derived>::Scalar>::Real
  matrixNorm(const MatrixBase<Derived>& m)
  {
    // FIXME if it is really guaranteed that the eigenvalues are already sorted,
    // then we don't need to compute a maxCoeff() here, comparing the 1st and last ones is enough.
    return m.eigenvalues().cwiseAbs().maxCoeff();
  }
};

template<typename Derived> struct ei_matrixNorm_selector<Derived, false>
{
  static inline typename NumTraits<typename ei_traits<Derived>::Scalar>::Real
  matrixNorm(const MatrixBase<Derived>& m)
  {
    typename Derived::Eval m_eval(m);
    // FIXME if it is really guaranteed that the eigenvalues are already sorted,
    // then we don't need to compute a maxCoeff() here, comparing the 1st and last ones is enough.
    return ei_sqrt(
             (m_eval*m_eval.adjoint())
             .template marked<SelfAdjoint>()
             .eigenvalues()
             .maxCoeff()
           );
  }
};

/** \qr_module
  *
  * \returns the matrix norm of this matrix.
  */
template<typename Derived>
inline typename NumTraits<typename ei_traits<Derived>::Scalar>::Real
MatrixBase<Derived>::matrixNorm() const
{
  return ei_matrixNorm_selector<Derived, Flags&SelfAdjointBit>
       ::matrixNorm(derived());
}

#ifndef EIGEN_EXTERN_INSTANCIATIONS
template<typename RealScalar, typename Scalar>
static void ei_tridiagonal_qr_step(RealScalar* diag, RealScalar* subdiag, int start, int end, Scalar* matrixQ, int n)
{
  RealScalar td = (diag[end-1] - diag[end])*0.5;
  RealScalar e2 = ei_abs2(subdiag[end-1]);
  RealScalar mu = diag[end] - e2 / (td + (td>0 ? 1 : -1) * ei_sqrt(td*td + e2));
  RealScalar x = diag[start] - mu;
  RealScalar z = subdiag[start];

  for (int k = start; k < end; ++k)
  {
    RealScalar c, s;
    ei_givens_rotation(x, z, c, s);

    // do T = G' T G
    RealScalar sdk = s * diag[k] + c * subdiag[k];
    RealScalar dkp1 = s * subdiag[k] + c * diag[k+1];

    diag[k] = c * (c * diag[k] - s * subdiag[k]) - s * (c * subdiag[k] - s * diag[k+1]);
    diag[k+1] = s * sdk + c * dkp1;
    subdiag[k] = c * sdk - s * dkp1;

    if (k > start)
      subdiag[k - 1] = c * subdiag[k-1] - s * z;

    x = subdiag[k];
    z = -s * subdiag[k+1];

    if (k < end - 1)
      subdiag[k + 1] = c * subdiag[k+1];

    // apply the givens rotation to the unit matrix Q = Q * G
    // G only modifies the two columns k and k+1
    if (matrixQ)
    {
      #ifdef EIGEN_DEFAULT_TO_ROW_MAJOR
      #else
      int kn = k*n;
      int kn1 = (k+1)*n;
      #endif
      // let's do the product manually to avoid the need of temporaries...
      for (int i=0; i<n; ++i)
      {
        #ifdef EIGEN_DEFAULT_TO_ROW_MAJOR
        Scalar matrixQ_i_k = matrixQ[i*n+k];
        matrixQ[i*n+k]   = c * matrixQ_i_k - s * matrixQ[i*n+k+1];
        matrixQ[i*n+k+1] = s * matrixQ_i_k + c * matrixQ[i*n+k+1];
        #else
        Scalar matrixQ_i_k = matrixQ[i+kn];
        matrixQ[i+kn]  = c * matrixQ_i_k - s * matrixQ[i+kn1];
        matrixQ[i+kn1] = s * matrixQ_i_k + c * matrixQ[i+kn1];
        #endif
      }
    }
  }
}
#endif

#endif // EIGEN_SELFADJOINTEIGENSOLVER_H
