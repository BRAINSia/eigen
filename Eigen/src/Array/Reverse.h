// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2006-2008 Benoit Jacob <jacob.benoit.1@gmail.com>
// Copyright (C) 2009 Ricard Marxer <email@ricardmarxer.com>
// Copyright (C) 2009 Gael Guennebaud <g.gael@free.fr>
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

#ifndef EIGEN_REVERSE_H
#define EIGEN_REVERSE_H

/** \array_module \ingroup Array_Module
  *
  * \class Reverse
  *
  * \brief Expression of the reverse of a vector or matrix
  *
  * \param MatrixType the type of the object of which we are taking the reverse
  *
  * This class represents an expression of the reverse of a vector.
  * It is the return type of MatrixBase::reverse() and VectorwiseOp::reverse()
  * and most of the time this is the only way it is used.
  *
  * \sa MatrixBase::reverse(), VectorwiseOp::reverse()
  */
template<typename MatrixType, int Direction>
struct ei_traits<Reverse<MatrixType, Direction> >
 : ei_traits<MatrixType>
{
  typedef typename MatrixType::Scalar Scalar;
  typedef typename ei_traits<MatrixType>::StorageType StorageType;
  typedef typename ei_nested<MatrixType>::type MatrixTypeNested;
  typedef typename ei_unref<MatrixTypeNested>::type _MatrixTypeNested;
  enum {
    RowsAtCompileTime = MatrixType::RowsAtCompileTime,
    ColsAtCompileTime = MatrixType::ColsAtCompileTime,
    MaxRowsAtCompileTime = MatrixType::MaxRowsAtCompileTime,
    MaxColsAtCompileTime = MatrixType::MaxColsAtCompileTime,

    // let's enable LinearAccess only with vectorization because of the product overhead
    LinearAccess = ( (Direction==BothDirections) && (int(_MatrixTypeNested::Flags)&PacketAccessBit) )
                 ? LinearAccessBit : 0,

    Flags = (int(_MatrixTypeNested::Flags) & (HereditaryBits | PacketAccessBit | LinearAccess))
          | (int(_MatrixTypeNested::Flags)&UpperTriangularBit ? LowerTriangularBit : 0)
          | (int(_MatrixTypeNested::Flags)&LowerTriangularBit ? UpperTriangularBit : 0),

    CoeffReadCost = _MatrixTypeNested::CoeffReadCost
  };
};

template<typename PacketScalar, bool ReversePacket> struct ei_reverse_packet_cond
{
  static inline PacketScalar run(const PacketScalar& x) { return ei_preverse(x); }
};
template<typename PacketScalar> struct ei_reverse_packet_cond<PacketScalar,false>
{
  static inline PacketScalar run(const PacketScalar& x) { return x; }
};

template<typename MatrixType, int Direction> class Reverse
  : public MatrixType::template MakeBase< Reverse<MatrixType, Direction> >::Type
{
  public:

    typedef typename MatrixType::template MakeBase< Reverse<MatrixType, Direction> >::Type Base;
    _EIGEN_GENERIC_PUBLIC_INTERFACE(Reverse)

  protected:
    enum {
      PacketSize = ei_packet_traits<Scalar>::size,
      IsRowMajor = Flags & RowMajorBit,
      IsColMajor = !IsRowMajor,
      ReverseRow = (Direction == Vertical)   || (Direction == BothDirections),
      ReverseCol = (Direction == Horizontal) || (Direction == BothDirections),
      OffsetRow  = ReverseRow && IsColMajor ? PacketSize : 1,
      OffsetCol  = ReverseCol && IsRowMajor ? PacketSize : 1,
      ReversePacket = (Direction == BothDirections)
                    || ((Direction == Vertical)   && IsColMajor)
                    || ((Direction == Horizontal) && IsRowMajor)
    };
    typedef ei_reverse_packet_cond<PacketScalar,ReversePacket> reverse_packet;
  public:

    inline Reverse(const MatrixType& matrix) : m_matrix(matrix) { }

    EIGEN_INHERIT_ASSIGNMENT_OPERATORS(Reverse)

    inline int rows() const { return m_matrix.rows(); }
    inline int cols() const { return m_matrix.cols(); }

    inline Scalar& coeffRef(int row, int col)
    {
      return m_matrix.const_cast_derived().coeffRef(ReverseRow ? m_matrix.rows() - row - 1 : row,
                                                    ReverseCol ? m_matrix.cols() - col - 1 : col);
    }

    inline const Scalar coeff(int row, int col) const
    {
      return m_matrix.coeff(ReverseRow ? m_matrix.rows() - row - 1 : row,
                            ReverseCol ? m_matrix.cols() - col - 1 : col);
    }

    inline const Scalar coeff(int index) const
    {
      return m_matrix.coeff(m_matrix.size() - index - 1);
    }

    inline Scalar& coeffRef(int index)
    {
      return m_matrix.const_cast_derived().coeffRef(m_matrix.size() - index - 1);
    }

    template<int LoadMode>
    inline const PacketScalar packet(int row, int col) const
    {
      return reverse_packet::run(m_matrix.template packet<LoadMode>(
                                    ReverseRow ? m_matrix.rows() - row - OffsetRow : row,
                                    ReverseCol ? m_matrix.cols() - col - OffsetCol : col));
    }

    template<int LoadMode>
    inline void writePacket(int row, int col, const PacketScalar& x)
    {
      m_matrix.const_cast_derived().template writePacket<LoadMode>(
                                      ReverseRow ? m_matrix.rows() - row - OffsetRow : row,
                                      ReverseCol ? m_matrix.cols() - col - OffsetCol : col,
                                      reverse_packet::run(x));
    }

    template<int LoadMode>
    inline const PacketScalar packet(int index) const
    {
      return ei_preverse(m_matrix.template packet<LoadMode>( m_matrix.size() - index - PacketSize ));
    }

    template<int LoadMode>
    inline void writePacket(int index, const PacketScalar& x)
    {
      m_matrix.const_cast_derived().template writePacket<LoadMode>(m_matrix.size() - index - PacketSize, ei_preverse(x));
    }

  protected:
    const typename MatrixType::Nested m_matrix;
};

/** \returns an expression of the reverse of *this.
  *
  * Example: \include MatrixBase_reverse.cpp
  * Output: \verbinclude MatrixBase_reverse.out
  *
  */
template<typename Derived>
inline Reverse<Derived, BothDirections>
DenseBase<Derived>::reverse()
{
  return derived();
}

/** This is the const version of reverse(). */
template<typename Derived>
inline const Reverse<Derived, BothDirections>
DenseBase<Derived>::reverse() const
{
  return derived();
}

/** This is the "in place" version of reverse: it reverses \c *this.
  *
  * In most cases it is probably better to simply use the reversed expression
  * of a matrix. However, when reversing the matrix data itself is really needed,
  * then this "in-place" version is probably the right choice because it provides
  * the following additional features:
  *  - less error prone: doing the same operation with .reverse() requires special care:
  *    \code m = m.reverse().eval(); \endcode
  *  - no temporary object is created (currently there is one created but could be avoided using swap)
  *  - it allows future optimizations (cache friendliness, etc.)
  *
  * \sa reverse() */
template<typename Derived>
inline void DenseBase<Derived>::reverseInPlace()
{
  derived() = derived().reverse().eval();
}


#endif // EIGEN_REVERSE_H
