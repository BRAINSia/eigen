// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2012 Desire Nuentsa <desire.nuentsa_wakam@inria.fr>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef EIGEN_SUITESPARSEQRSUPPORT_H
#define EIGEN_SUITESPARSEQRSUPPORT_H

namespace Eigen {
  
  template<typename MatrixType> class SPQR; 
  template<typename SPQRType> struct SPQRMatrixQReturnType; 
  template<typename SPQRType> struct SPQRMatrixQTransposeReturnType; 
  template <typename SPQRType, typename Derived> struct SPQR_QProduct;
  namespace internal {
    template <typename SPQRType> struct traits<SPQRMatrixQReturnType<SPQRType> >
    {
      typedef typename SPQRType::MatrixType ReturnType;
    };
    template <typename SPQRType> struct traits<SPQRMatrixQTransposeReturnType<SPQRType> >
    {
      typedef typename SPQRType::MatrixType ReturnType;
    };
    template <typename SPQRType, typename Derived> struct traits<SPQR_QProduct<SPQRType, Derived> >
    {
      typedef typename Derived::PlainObject ReturnType;
    };
  } // End namespace internal
  
/**
 * \ingroup SPQRSupport_Module
 * \class SPQR
 * \brief Sparse QR factorization based on SuiteSparseQR library
 * 
 * This class is used to perform a multithreaded and multifrontal rank-revealing QR decomposition 
 * of sparse matrices. The result is then used to solve linear leasts_square systems.
 * Clearly, a QR factorization is returned such that A*P = Q*R where :
 * 
 * P is the column permutation. Use colsPermutation() to get it.
 * 
 * Q is the orthogonal matrix represented as Householder reflectors. 
 * Use matrixQ() to get an expression and matrixQ().transpose() to get the transpose.
 * You can then apply it to a vector.
 * 
 * R is the sparse triangular factor. Use matrixQR() to get it as SparseMatrix.
 * NOTE : The Index type of R is always UF_long. You can get it with SPQR::Index
 * 
 * \tparam _MatrixType The type of the sparse matrix A, must be a SparseMatrix<>, either row-major or column-major. 
 * NOTE 
 * 
 */
template<typename _MatrixType>
class SPQR
{
  public:
    typedef typename _MatrixType::Scalar Scalar;
    typedef typename _MatrixType::RealScalar RealScalar;
    typedef UF_long Index ; 
    typedef SparseMatrix<Scalar, _MatrixType::Flags, Index> MatrixType;
    typedef PermutationMatrix<Dynamic, Dynamic, Index> PermutationType;
  public:
    SPQR() 
    : m_ordering(SPQR_ORDERING_DEFAULT),
      m_allow_tol(SPQR_DEFAULT_TOL),
      m_tolerance (NumTraits<Scalar>::epsilon())
    { 
      cholmod_l_start(&m_cc);
    }
    
    SPQR(const _MatrixType& matrix) : SPQR()
    {
      compute(matrix);
    }
    
    ~SPQR()
    {
      // Calls SuiteSparseQR_free()
      cholmod_free_sparse(&m_H, &m_cc); 
      cholmod_free_dense(&m_HTau, &m_cc);
      delete[] m_E;
      delete[] m_HPinv; 
    }
    void compute(const MatrixType& matrix)
    {
      MatrixType mat(matrix);
      cholmod_sparse A; 
      A = viewAsCholmod(mat);
      Index col = matrix.cols();
      m_rank = SuiteSparseQR<Scalar>(m_ordering, m_tolerance, col, &A, 
                             &m_cR, &m_E, &m_H, &m_HPinv, &m_HTau, &m_cc);

      if (!m_cR)
      {
        m_info = NumericalIssue; 
        m_isInitialized = false;
        return;
      }
      m_info = Success;
      m_isInitialized = true;
    }
    template<typename Rhs, typename Dest>
    void _solve(const MatrixBase<Rhs> &b, MatrixBase<Dest> &dest) const
    {
      eigen_assert(m_isInitialized && " The QR factorization should be computed first, call compute()");
      eigen_assert(b.cols()==1 && "This method is for vectors only");
      
      //Compute Q^T * b
      // NOTE : We may have called directly the corresponding routines in SPQR codes.
      // This version is used to test directly the corresponding part of the code
      dest = matrixQ().transpose() * b;
      
        // Solves with the triangular matrix R
      Dest y; 
      y = this->matrixQR().template triangularView<Upper>().solve(dest.derived());
      // Apply the column permutation 
      dest = colsPermutation() * y;
      
      m_info = Success;
    }
    /// Get the sparse triangular matrix R. It is a sparse matrix
    MatrixType matrixQR() const
    {
      MatrixType R; 
      R = viewAsEigen<Scalar, MatrixType::Flags, Index>(*m_cR);
      return R; 
    }
    /// Get an expression of the matrix Q
    SPQRMatrixQReturnType<SPQR> matrixQ() const
    {
      return SPQRMatrixQReturnType<SPQR>(*this);
    }
    /// Get the permutation that was applied to columns of A
    PermutationType colsPermutation() const
    { 
      eigen_assert(m_isInitialized && "Decomposition is not initialized.");
      Index n = m_cR->ncol;
      PermutationType colsPerm(n);
      for(Index j = 0; j <n; j++) colsPerm.indices()(j) = m_E[j];
      return colsPerm; 
      
    }
    /**
     * Gets the rank of the matrix. 
     * It should be equal to matrixQR().cols if the matrix is full-rank
     */
    Index rank() const
    {
      eigen_assert(m_isInitialized && "Decomposition is not initialized.");
      return m_cc.SPQR_istat[4];
    }
    /// Set the fill-reducing ordering method to be used
    void setOrdering(int ord) { m_ordering = ord;}
    /// Set the tolerance tol to treat columns with 2-norm < =tol as zero
    void setThreshold(RealScalar tol) { m_tolerance = tol; }
    
    /// Return a pointer to SPQR workspace 
    cholmod_common *cc() const { return &m_cc; }
    cholmod_sparse * H() const { return m_H; }
    Index  *HPinv() const { return m_HPinv; }
    cholmod_dense* HTau() const { return m_HTau; }
    
    
    /** \brief Reports whether previous computation was successful.
      *
      * \returns \c Success if computation was succesful,
      *          \c NumericalIssue if the sparse QR can not be computed
      */
    ComputationInfo info() const
    {
      eigen_assert(m_isInitialized && "Decomposition is not initialized.");
      return m_info;
    }
  protected:
    bool m_isInitialized;
    bool m_analysisIsOk;
    bool m_factorizationIsOk;
    mutable ComputationInfo m_info;
    int m_ordering; // Ordering method to use, see SPQR's manual
    int m_allow_tol; // Allow to use some tolerance during numerical factorization.
    RealScalar m_tolerance; // treat columns with 2-norm below this tolerance as zero
    mutable cholmod_sparse *m_cR; // The sparse R factor in cholmod format
    mutable Index *m_E; // The permutation applied to columns
    mutable cholmod_sparse *m_H;  //The householder vectors
    mutable Index *m_HPinv; // The row permutation of H
    mutable cholmod_dense *m_HTau; // The Householder coefficients
    mutable Index m_rank; // The rank of the matrix
    mutable cholmod_common m_cc; // Workspace and parameters
};

template <typename SPQRType, typename Derived>
struct SPQR_QProduct : ReturnByValue<SPQR_QProduct<SPQRType,Derived> >
{
  typedef typename SPQRType::Scalar Scalar;
  //Define the constructor to get reference to argument types
  SPQR_QProduct(const SPQRType& spqr, const Derived& other, bool transpose) : m_spqr(spqr),m_other(other),m_transpose(transpose) {}
  
  // Assign to a vector
  template<typename ResType>
  void evalTo(ResType& res) const
  {
    cholmod_dense y_cd;
    cholmod_dense *x_cd; 
    int method = m_transpose ? SPQR_QTX : SPQR_QX; 
    cholmod_common *cc = m_spqr.cc();
    y_cd = viewAsCholmod(m_other.const_cast_derived());
    x_cd = SuiteSparseQR_qmult<Scalar>(method, m_spqr.H(), m_spqr.HTau(), m_spqr.HPinv(), &y_cd, cc);
    res = Matrix<Scalar,ResType::RowsAtCompileTime,ResType::ColsAtCompileTime>::Map(reinterpret_cast<Scalar*>(x_cd->x), x_cd->nrow, x_cd->ncol);
    cholmod_free_dense(&x_cd, cc); 
  }
  const SPQRType& m_spqr; 
  const Derived& m_other; 
  bool m_transpose; 
  
};
template<typename SPQRType>
struct SPQRMatrixQReturnType{
  
  SPQRMatrixQReturnType(const SPQRType& spqr) : m_spqr(spqr) {}
  template<typename Derived>
  SPQR_QProduct<SPQRType, Derived> operator*(const MatrixBase<Derived>& other)
  {
    return SPQR_QProduct<SPQRType,Derived>(m_spqr,other.derived(),false);
  }
  // To use for operations with the transpose of Q
  SPQRMatrixQTransposeReturnType<SPQRType> transpose() const
  {
    return SPQRMatrixQTransposeReturnType<SPQRType>(m_spqr);
  }
  const SPQRType& m_spqr;
};

template<typename SPQRType>
struct SPQRMatrixQTransposeReturnType{
  SPQRMatrixQTransposeReturnType(const SPQRType& spqr) : m_spqr(spqr) {}
  template<typename Derived>
  SPQR_QProduct<SPQRType,Derived> operator*(const MatrixBase<Derived>& other)
  {
    return SPQR_QProduct<SPQRType,Derived>(m_spqr,other.derived(), true);
  }
  const SPQRType& m_spqr;
};
}// End namespace Eigen
#endif