// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
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

#ifndef EIGEN_TRIDIAGONALIZATION_H
#define EIGEN_TRIDIAGONALIZATION_H

/** \ingroup QR_Module
  * \nonstableyet
  *
  * \class Tridiagonalization
  *
  * \brief Trigiagonal decomposition of a selfadjoint matrix
  *
  * \param MatrixType the type of the matrix of which we are performing the tridiagonalization
  *
  * This class performs a tridiagonal decomposition of a selfadjoint matrix \f$ A \f$ such that:
  * \f$ A = Q T Q^* \f$ where \f$ Q \f$ is unitary and \f$ T \f$ a real symmetric tridiagonal matrix.
  *
  * \sa MatrixBase::tridiagonalize()
  */
template<typename _MatrixType> class Tridiagonalization
{
  public:

    typedef _MatrixType MatrixType;
    typedef typename MatrixType::Scalar Scalar;
    typedef typename NumTraits<Scalar>::Real RealScalar;
    typedef typename ei_packet_traits<Scalar>::type Packet;

    enum {
      Size = MatrixType::RowsAtCompileTime,
      SizeMinusOne = MatrixType::RowsAtCompileTime==Dynamic
                   ? Dynamic
                   : MatrixType::RowsAtCompileTime-1,
      PacketSize = ei_packet_traits<Scalar>::size
    };

    typedef Matrix<Scalar, SizeMinusOne, 1> CoeffVectorType;
    typedef Matrix<RealScalar, Size, 1> DiagonalType;
    typedef Matrix<RealScalar, SizeMinusOne, 1> SubDiagonalType;

    typedef typename ei_meta_if<NumTraits<Scalar>::IsComplex,
              typename NestByValue<Diagonal<MatrixType,0> >::RealReturnType,
              Diagonal<MatrixType,0>
            >::ret DiagonalReturnType;

    typedef typename ei_meta_if<NumTraits<Scalar>::IsComplex,
              typename NestByValue<Diagonal<
                NestByValue<Block<MatrixType,SizeMinusOne,SizeMinusOne> >,0 > >::RealReturnType,
              Diagonal<
                NestByValue<Block<MatrixType,SizeMinusOne,SizeMinusOne> >,0 >
            >::ret SubDiagonalReturnType;

    /** This constructor initializes a Tridiagonalization object for
      * further use with Tridiagonalization::compute()
      */
    Tridiagonalization(int size = Size==Dynamic ? 2 : Size)
      : m_matrix(size,size), m_hCoeffs(size-1)
    {}

    Tridiagonalization(const MatrixType& matrix)
      : m_matrix(matrix), m_hCoeffs(matrix.cols()-1)
    {
      _compute(m_matrix, m_hCoeffs);
    }

    /** Computes or re-compute the tridiagonalization for the matrix \a matrix.
      *
      * This method allows to re-use the allocated data.
      */
    void compute(const MatrixType& matrix)
    {
      m_matrix = matrix;
      m_hCoeffs.resize(matrix.rows()-1, 1);
      _compute(m_matrix, m_hCoeffs);
    }

    /** \returns the householder coefficients allowing to
      * reconstruct the matrix Q from the packed data.
      *
      * \sa packedMatrix()
      */
    inline CoeffVectorType householderCoefficients(void) const { return m_hCoeffs; }

    /** \returns the internal result of the decomposition.
      *
      * The returned matrix contains the following information:
      *  - the strict upper part is equal to the input matrix A
      *  - the diagonal and lower sub-diagonal represent the tridiagonal symmetric matrix (real).
      *  - the rest of the lower part contains the Householder vectors that, combined with
      *    Householder coefficients returned by householderCoefficients(),
      *    allows to reconstruct the matrix Q as follow:
      *       Q = H_{N-1} ... H_1 H_0
      *    where the matrices H are the Householder transformations:
      *       H_i = (I - h_i * v_i * v_i')
      *    where h_i == householderCoefficients()[i] and v_i is a Householder vector:
      *       v_i = [ 0, ..., 0, 1, M(i+2,i), ..., M(N-1,i) ]
      *
      * See LAPACK for further details on this packed storage.
      */
    inline const MatrixType& packedMatrix(void) const { return m_matrix; }

    MatrixType matrixQ() const;
    template<typename QDerived> void matrixQInPlace(MatrixBase<QDerived>* q) const;
    MatrixType matrixT() const;
    const DiagonalReturnType diagonal(void) const;
    const SubDiagonalReturnType subDiagonal(void) const;

    static void decomposeInPlace(MatrixType& mat, DiagonalType& diag, SubDiagonalType& subdiag, bool extractQ = true);

    static void _compute(MatrixType& matA, CoeffVectorType& hCoeffs);

  protected:

    static void _decomposeInPlace3x3(MatrixType& mat, DiagonalType& diag, SubDiagonalType& subdiag, bool extractQ = true);

    MatrixType m_matrix;
    CoeffVectorType m_hCoeffs;
};

/** \returns an expression of the diagonal vector */
template<typename MatrixType>
const typename Tridiagonalization<MatrixType>::DiagonalReturnType
Tridiagonalization<MatrixType>::diagonal(void) const
{
  return m_matrix.diagonal().nestByValue();
}

/** \returns an expression of the sub-diagonal vector */
template<typename MatrixType>
const typename Tridiagonalization<MatrixType>::SubDiagonalReturnType
Tridiagonalization<MatrixType>::subDiagonal(void) const
{
  int n = m_matrix.rows();
  return Block<MatrixType,SizeMinusOne,SizeMinusOne>(m_matrix, 1, 0, n-1,n-1)
    .nestByValue().diagonal().nestByValue();
}

/** constructs and returns the tridiagonal matrix T.
  * Note that the matrix T is equivalent to the diagonal and sub-diagonal of the packed matrix.
  * Therefore, it might be often sufficient to directly use the packed matrix, or the vector
  * expressions returned by diagonal() and subDiagonal() instead of creating a new matrix.
  */
template<typename MatrixType>
typename Tridiagonalization<MatrixType>::MatrixType
Tridiagonalization<MatrixType>::matrixT(void) const
{
  // FIXME should this function (and other similar ones) rather take a matrix as argument
  // and fill it ? (to avoid temporaries)
  int n = m_matrix.rows();
  MatrixType matT = m_matrix;
  matT.corner(TopRight,n-1, n-1).diagonal() = subDiagonal().template cast<Scalar>().conjugate();
  if (n>2)
  {
    matT.corner(TopRight,n-2, n-2).template part<UpperTriangular>().setZero();
    matT.corner(BottomLeft,n-2, n-2).template part<LowerTriangular>().setZero();
  }
  return matT;
}

#ifndef EIGEN_HIDE_HEAVY_CODE

/** \internal
  * Performs a tridiagonal decomposition of \a matA in place.
  *
  * \param matA the input selfadjoint matrix
  * \param hCoeffs returned Householder coefficients
  *
  * The result is written in the lower triangular part of \a matA.
  *
  * Implemented from Golub's "Matrix Computations", algorithm 8.3.1.
  *
  * \sa packedMatrix()
  */
template<typename MatrixType>
void Tridiagonalization<MatrixType>::_compute(MatrixType& matA, CoeffVectorType& hCoeffs)
{
  assert(matA.rows()==matA.cols());
  int n = matA.rows();
  for (int i = 0; i<n-2; ++i)
  {
    // let's consider the vector v = i-th column starting at position i+1

    // start of the householder transformation
    // squared norm of the vector v skipping the first element
    RealScalar v1norm2 = matA.col(i).end(n-(i+2)).squaredNorm();

    // FIXME comparing against 1
    if (ei_isMuchSmallerThan(v1norm2,static_cast<Scalar>(1)))
    {
      hCoeffs.coeffRef(i) = 0.;
    }
    else
    {
      Scalar v0 = matA.col(i).coeff(i+1);
      RealScalar beta = ei_sqrt(ei_abs2(v0)+v1norm2);
      if (ei_real(v0)>=0.)
        beta = -beta;
      matA.col(i).end(n-(i+2)) *= (Scalar(1)/(v0-beta));
      matA.col(i).coeffRef(i+1) = beta;
      Scalar h = (beta - v0) / beta;
      // end of the householder transformation

      // Apply similarity transformation to remaining columns,
      // i.e., A = H' A H where H = I - h v v' and v = matA.col(i).end(n-i-1)
      matA.col(i).coeffRef(i+1) = 1;

//       hCoeffs.end(n-i-1) = (matA.corner(BottomRight,n-i-1,n-i-1).template selfadjointView<LowerTriangular>()
//                          * (h * matA.col(i).end(n-i-1)));

      hCoeffs.end(n-i-1).setZero();
      ei_product_selfadjoint_vector<Scalar,MatrixType::Flags&RowMajorBit,LowerTriangular,false,false>
        (n-i-1,matA.corner(BottomRight,n-i-1,n-i-1).data(), matA.stride(), matA.col(i).end(n-i-1).data(), 1, const_cast<Scalar*>(hCoeffs.end(n-i-1).data()), h);

      hCoeffs.end(n-i-1) += (h*Scalar(-0.5)*(matA.col(i).end(n-i-1).dot(hCoeffs.end(n-i-1)))) * matA.col(i).end(n-i-1);

      matA.corner(BottomRight, n-i-1, n-i-1).template selfadjointView<LowerTriangular>()
        .rankUpdate(matA.col(i).end(n-i-1), hCoeffs.end(n-i-1), -1);

      // note: at that point matA(i+1,i+1) is the (i+1)-th element of the final diagonal
      // note: the sequence of the beta values leads to the subdiagonal entries
      matA.col(i).coeffRef(i+1) = beta;

      hCoeffs.coeffRef(i) = h;
    }
  }
  if (NumTraits<Scalar>::IsComplex)
  {
    // Householder transformation on the remaining single scalar
    int i = n-2;
    Scalar v0 = matA.col(i).coeff(i+1);
    RealScalar beta = ei_abs(v0);
    if (ei_real(v0)>=RealScalar(0))
      beta = -beta;
    matA.col(i).coeffRef(i+1) = beta;
    // FIXME comparing against 1
    if(ei_isMuchSmallerThan(beta, Scalar(1))) hCoeffs.coeffRef(i) = Scalar(0);
    else hCoeffs.coeffRef(i) = (beta - v0) / beta;
  }
  else
  {
    hCoeffs.coeffRef(n-2) = 0;
  }
}

/** reconstructs and returns the matrix Q */
template<typename MatrixType>
typename Tridiagonalization<MatrixType>::MatrixType
Tridiagonalization<MatrixType>::matrixQ(void) const
{
  MatrixType matQ;
  matrixQInPlace(&matQ);
  return matQ;
}

template<typename MatrixType>
template<typename QDerived>
void Tridiagonalization<MatrixType>::matrixQInPlace(MatrixBase<QDerived>* q) const
{
  QDerived& matQ = q->derived();
  int n = m_matrix.rows();
  matQ = MatrixType::Identity(n,n);
  Matrix<Scalar,1,Dynamic> aux(n);
  for (int i = n-2; i>=0; i--)
  {
    Scalar tmp = m_matrix.coeff(i+1,i);
    m_matrix.const_cast_derived().coeffRef(i+1,i) = 1;

    aux.end(n-i-1) = (m_hCoeffs.coeff(i) * m_matrix.col(i).end(n-i-1).adjoint() * matQ.corner(BottomRight,n-i-1,n-i-1)).lazy();
    // rank one update, TODO ! make it works efficiently as expected
    for (int j=i+1;j<n;++j)
      matQ.col(j).end(n-i-1) -= ( aux.coeff(j)) * m_matrix.col(i).end(n-i-1);
//     matQ.corner(BottomRight,n-i-1,n-i-1) -= (m_matrix.col(i).end(n-i-1) * aux.end(n-i-1)).lazy();

    m_matrix.const_cast_derived().coeffRef(i+1,i) = tmp;
  }
}

/** Performs a full decomposition in place */
template<typename MatrixType>
void Tridiagonalization<MatrixType>::decomposeInPlace(MatrixType& mat, DiagonalType& diag, SubDiagonalType& subdiag, bool extractQ)
{
  int n = mat.rows();
  ei_assert(mat.cols()==n && diag.size()==n && subdiag.size()==n-1);
  if (n==3 && (!NumTraits<Scalar>::IsComplex) )
  {
    _decomposeInPlace3x3(mat, diag, subdiag, extractQ);
  }
  else
  {
    Tridiagonalization tridiag(mat);
    diag = tridiag.diagonal();
    subdiag = tridiag.subDiagonal();
    if (extractQ)
      tridiag.matrixQInPlace(&mat);
  }
}

/** \internal
  * Optimized path for 3x3 matrices.
  * Especially useful for plane fitting.
  */
template<typename MatrixType>
void Tridiagonalization<MatrixType>::_decomposeInPlace3x3(MatrixType& mat, DiagonalType& diag, SubDiagonalType& subdiag, bool extractQ)
{
  diag[0] = ei_real(mat(0,0));
  RealScalar v1norm2 = ei_abs2(mat(0,2));
  if (ei_isMuchSmallerThan(v1norm2, RealScalar(1)))
  {
    diag[1] = ei_real(mat(1,1));
    diag[2] = ei_real(mat(2,2));
    subdiag[0] = ei_real(mat(0,1));
    subdiag[1] = ei_real(mat(1,2));
    if (extractQ)
      mat.setIdentity();
  }
  else
  {
    RealScalar beta = ei_sqrt(ei_abs2(mat(0,1))+v1norm2);
    RealScalar invBeta = RealScalar(1)/beta;
    Scalar m01 = mat(0,1) * invBeta;
    Scalar m02 = mat(0,2) * invBeta;
    Scalar q = RealScalar(2)*m01*mat(1,2) + m02*(mat(2,2) - mat(1,1));
    diag[1] = ei_real(mat(1,1) + m02*q);
    diag[2] = ei_real(mat(2,2) - m02*q);
    subdiag[0] = beta;
    subdiag[1] = ei_real(mat(1,2) - m01 * q);
    if (extractQ)
    {
      mat(0,0) = 1;
      mat(0,1) = 0;
      mat(0,2) = 0;
      mat(1,0) = 0;
      mat(1,1) = m01;
      mat(1,2) = m02;
      mat(2,0) = 0;
      mat(2,1) = m02;
      mat(2,2) = -m01;
    }
  }
}

#endif // EIGEN_HIDE_HEAVY_CODE

#endif // EIGEN_TRIDIAGONALIZATION_H
