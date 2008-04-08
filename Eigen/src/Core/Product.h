// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2006-2008 Benoit Jacob <jacob@math.jussieu.fr>
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

#ifndef EIGEN_PRODUCT_H
#define EIGEN_PRODUCT_H

template<int Index, int Size, typename Lhs, typename Rhs>
struct ei_product_unroller
{
  static void run(int row, int col, const Lhs& lhs, const Rhs& rhs,
                  typename Lhs::Scalar &res)
  {
    ei_product_unroller<Index-1, Size, Lhs, Rhs>::run(row, col, lhs, rhs, res);
    res += lhs.coeff(row, Index) * rhs.coeff(Index, col);
  }
};

template<int Size, typename Lhs, typename Rhs>
struct ei_product_unroller<0, Size, Lhs, Rhs>
{
  static void run(int row, int col, const Lhs& lhs, const Rhs& rhs,
                  typename Lhs::Scalar &res)
  {
    res = lhs.coeff(row, 0) * rhs.coeff(0, col);
  }
};

template<int Index, typename Lhs, typename Rhs>
struct ei_product_unroller<Index, Dynamic, Lhs, Rhs>
{
  static void run(int, int, const Lhs&, const Rhs&, typename Lhs::Scalar&) {}
};

// prevent buggy user code from causing an infinite recursion
template<int Index, typename Lhs, typename Rhs>
struct ei_product_unroller<Index, 0, Lhs, Rhs>
{
  static void run(int, int, const Lhs&, const Rhs&, typename Lhs::Scalar&) {}
};

/** \class Product
  *
  * \brief Expression of the product of two matrices
  *
  * \param Lhs the type of the left-hand side
  * \param Rhs the type of the right-hand side
  * \param EvalMode internal use only
  *
  * This class represents an expression of the product of two matrices.
  * It is the return type of the operator* between matrices, and most of the time
  * this is the only way it is used.
  *
  * \sa class Sum, class Difference
  */
template<typename Lhs, typename Rhs> struct ei_product_eval_mode
{
  enum{ value = Lhs::MaxRowsAtCompileTime >= 8 && Rhs::MaxColsAtCompileTime >= 8
              ? CacheOptimal : UnrolledDotProduct };
};

template<typename Lhs, typename Rhs, int EvalMode>
struct ei_traits<Product<Lhs, Rhs, EvalMode> >
{
  typedef typename Lhs::Scalar Scalar;
  typedef typename ei_xpr_copy<Lhs,Rhs::ColsAtCompileTime>::type LhsXprCopy;
  typedef typename ei_xpr_copy<Rhs,Lhs::RowsAtCompileTime>::type RhsXprCopy;
  typedef typename ei_unref<LhsXprCopy>::type _LhsXprCopy;
  typedef typename ei_unref<RhsXprCopy>::type _RhsXprCopy;
  enum {
    LhsCoeffReadCost = _LhsXprCopy::CoeffReadCost,
    RhsCoeffReadCost = _RhsXprCopy::CoeffReadCost,
    LhsFlags = _LhsXprCopy::Flags,
    RhsFlags = _RhsXprCopy::Flags,
    RowsAtCompileTime = Lhs::RowsAtCompileTime,
    ColsAtCompileTime = Rhs::ColsAtCompileTime,
    MaxRowsAtCompileTime = Lhs::MaxRowsAtCompileTime,
    MaxColsAtCompileTime = Rhs::MaxColsAtCompileTime,
    Flags = ( (RowsAtCompileTime == Dynamic || ColsAtCompileTime == Dynamic)
              ? (unsigned int)(LhsFlags | RhsFlags)
              : (unsigned int)(LhsFlags | RhsFlags) & ~LargeBit )
          | EvalBeforeAssigningBit
          | (ei_product_eval_mode<Lhs, Rhs>::value == (int)CacheOptimal ? EvalBeforeNestingBit : 0),
    CoeffReadCost
      = Lhs::ColsAtCompileTime == Dynamic
      ? Dynamic
      : Lhs::ColsAtCompileTime
        * (NumTraits<Scalar>::MulCost + LhsCoeffReadCost + RhsCoeffReadCost)
        + (Lhs::ColsAtCompileTime - 1) * NumTraits<Scalar>::AddCost
  };
};

template<typename Lhs, typename Rhs, int EvalMode> class Product : ei_no_assignment_operator,
  public MatrixBase<Product<Lhs, Rhs, EvalMode> >
{
  public:

    EIGEN_GENERIC_PUBLIC_INTERFACE(Product)
    typedef typename ei_traits<Product>::LhsXprCopy LhsXprCopy;
    typedef typename ei_traits<Product>::RhsXprCopy RhsXprCopy;
    typedef typename ei_traits<Product>::_LhsXprCopy _LhsXprCopy;
    typedef typename ei_traits<Product>::_RhsXprCopy _RhsXprCopy;

    Product(const Lhs& lhs, const Rhs& rhs)
      : m_lhs(lhs), m_rhs(rhs)
    {
      ei_assert(lhs.cols() == rhs.rows());
    }

    /** \internal */
    template<typename DestDerived>
    void _cacheOptimalEval(DestDerived& res) const;

  private:

    int _rows() const { return m_lhs.rows(); }
    int _cols() const { return m_rhs.cols(); }

    const Scalar _coeff(int row, int col) const
    {
      Scalar res;
      const bool unroll = CoeffReadCost <= EIGEN_UNROLLING_LIMIT;
      if(unroll)
      {
        ei_product_unroller<Lhs::ColsAtCompileTime-1,
                            unroll ? Lhs::ColsAtCompileTime : Dynamic,
                            _LhsXprCopy, _RhsXprCopy>
          ::run(row, col, m_lhs, m_rhs, res);
      }
      else
      {
        res = m_lhs.coeff(row, 0) * m_rhs.coeff(0, col);
        for(int i = 1; i < m_lhs.cols(); i++)
          res += m_lhs.coeff(row, i) * m_rhs.coeff(i, col);
      }
      return res;
    }

  protected:
    const LhsXprCopy m_lhs;
    const RhsXprCopy m_rhs;
};

/** \returns the matrix product of \c *this and \a other.
  *
  * \note This function causes an immediate evaluation. If you want to perform a matrix product
  * without immediate evaluation, call .lazy() on one of the matrices before taking the product.
  *
  * \sa lazy(), operator*=(const MatrixBase&)
  */
template<typename Derived>
template<typename OtherDerived>
const Product<Derived,OtherDerived>
MatrixBase<Derived>::operator*(const MatrixBase<OtherDerived> &other) const
{
  return Product<Derived,OtherDerived>(derived(), other.derived());
}

/** replaces \c *this by \c *this * \a other.
  *
  * \returns a reference to \c *this
  */
template<typename Derived>
template<typename OtherDerived>
Derived &
MatrixBase<Derived>::operator*=(const MatrixBase<OtherDerived> &other)
{
  return *this = *this * other;
}

template<typename Derived>
template<typename Derived1, typename Derived2>
Derived& MatrixBase<Derived>::lazyAssign(const Product<Derived1,Derived2,CacheOptimal>& product)
{
  product._cacheOptimalEval(*this);
  return derived();
}

template<typename Lhs, typename Rhs, int EvalMode>
template<typename DestDerived>
void Product<Lhs,Rhs,EvalMode>::_cacheOptimalEval(DestDerived& res) const
{
  res.setZero();
  const int cols4 = m_lhs.cols()&0xfffffffC;
  for (int k=0; k<m_rhs.cols(); ++k)
  {
    int j=0;
    for (; j<cols4; j+=4)
    {
      const Scalar tmp0 = m_rhs.coeff(j  ,k);
      const Scalar tmp1 = m_rhs.coeff(j+1,k);
      const Scalar tmp2 = m_rhs.coeff(j+2,k);
      const Scalar tmp3 = m_rhs.coeff(j+3,k);
      for (int i=0; i<m_lhs.rows(); ++i)
        res.coeffRef(i,k) += tmp0 * m_lhs.coeff(i,j) + tmp1 * m_lhs.coeff(i,j+1)
                           + tmp2 * m_lhs.coeff(i,j+2) + tmp3 * m_lhs.coeff(i,j+3);
    }
    for (; j<m_lhs.cols(); ++j)
    {
      const Scalar tmp = m_rhs.coeff(j,k);
      for (int i=0; i<m_lhs.rows(); ++i)
        res.coeffRef(i,k) += tmp * m_lhs.coeff(i,j);
    }
  }
}

#endif // EIGEN_PRODUCT_H
