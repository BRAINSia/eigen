// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2006-2007 Benoit Jacob <jacob@math.jussieu.fr>
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

#ifndef EIGEN_SUM_H
#define EIGEN_SUM_H

template<typename Lhs, typename Rhs> class Sum : NoOperatorEquals,
    public MatrixBase<typename Lhs::Scalar, Sum<Lhs, Rhs> >
{
  public:
    typedef typename Lhs::Scalar Scalar;
    typedef typename Lhs::Ref LhsRef;
    typedef typename Rhs::Ref RhsRef;
    friend class MatrixBase<Scalar, Sum>;
    
    Sum(const LhsRef& lhs, const RhsRef& rhs)
      : m_lhs(lhs), m_rhs(rhs)
    {
      assert(lhs.rows() == rhs.rows() && lhs.cols() == rhs.cols());
    }

    Sum(const Sum& other) : m_lhs(other.m_lhs), m_rhs(other.m_rhs) {}

  private:
    static const int _RowsAtCompileTime = Lhs::RowsAtCompileTime,
                     _ColsAtCompileTime = Rhs::ColsAtCompileTime;

    const Sum& _ref() const { return *this; }
    int _rows() const { return m_lhs.rows(); }
    int _cols() const { return m_lhs.cols(); }

    Scalar _coeff(int row, int col) const
    {
      return m_lhs.coeff(row, col) + m_rhs.coeff(row, col);
    }
    
  protected:
    const LhsRef m_lhs;
    const RhsRef m_rhs;
};

template<typename Scalar, typename Derived1, typename Derived2>
const Sum<Derived1, Derived2>
operator+(const MatrixBase<Scalar, Derived1> &mat1, const MatrixBase<Scalar, Derived2> &mat2)
{
  return Sum<Derived1, Derived2>(mat1.ref(), mat2.ref());
}

template<typename Scalar, typename Derived>
template<typename OtherDerived>
Derived &
MatrixBase<Scalar, Derived>::operator+=(const MatrixBase<Scalar, OtherDerived>& other)
{
  return *this = *this + other;
}

#endif // EIGEN_SUM_H
