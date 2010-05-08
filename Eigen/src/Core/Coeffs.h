// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2006-2010 Benoit Jacob <jacob.benoit.1@gmail.com>
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

#ifndef EIGEN_COEFFS_H
#define EIGEN_COEFFS_H

template<typename Derived, bool EnableDirectAccessAPI>
class DenseCoeffsBase : public EigenBase<Derived>
{
  public:
    typedef typename ei_traits<Derived>::Scalar Scalar;
    typedef typename ei_meta_if<ei_has_direct_access<Derived>::ret, const Scalar&, Scalar>::ret CoeffReturnType;

    typedef EigenBase<Derived> Base;
    using Base::rows;
    using Base::cols;
    using Base::size;
    using Base::derived;
    
    EIGEN_STRONG_INLINE int rowIndexByOuterInner(int outer, int inner) const
    {
      return int(Derived::RowsAtCompileTime) == 1 ? 0
          : int(Derived::ColsAtCompileTime) == 1 ? inner
          : int(Derived::Flags)&RowMajorBit ? outer
          : inner;
    }

    EIGEN_STRONG_INLINE int colIndexByOuterInner(int outer, int inner) const
    {
      return int(Derived::ColsAtCompileTime) == 1 ? 0
          : int(Derived::RowsAtCompileTime) == 1 ? inner
          : int(Derived::Flags)&RowMajorBit ? inner
          : outer;
    }

    /** Short version: don't use this function, use
      * \link operator()(int,int) const \endlink instead.
      *
      * Long version: this function is similar to
      * \link operator()(int,int) const \endlink, but without the assertion.
      * Use this for limiting the performance cost of debugging code when doing
      * repeated coefficient access. Only use this when it is guaranteed that the
      * parameters \a row and \a col are in range.
      *
      * If EIGEN_INTERNAL_DEBUGGING is defined, an assertion will be made, making this
      * function equivalent to \link operator()(int,int) const \endlink.
      *
      * \sa operator()(int,int) const, coeffRef(int,int), coeff(int) const
      */
    EIGEN_STRONG_INLINE const CoeffReturnType coeff(int row, int col) const
    {
      ei_internal_assert(row >= 0 && row < rows()
                        && col >= 0 && col < cols());
      return derived().coeff(row, col);
    }

    EIGEN_STRONG_INLINE const CoeffReturnType coeffByOuterInner(int outer, int inner) const
    {
      return coeff(rowIndexByOuterInner(outer, inner),
                   colIndexByOuterInner(outer, inner));
    }

    /** \returns the coefficient at given the given row and column.
      *
      * \sa operator()(int,int), operator[](int)
      */
    EIGEN_STRONG_INLINE const CoeffReturnType operator()(int row, int col) const
    {
      ei_assert(row >= 0 && row < rows()
          && col >= 0 && col < cols());
      return derived().coeff(row, col);
    }

    /** Short version: don't use this function, use
      * \link operator[](int) const \endlink instead.
      *
      * Long version: this function is similar to
      * \link operator[](int) const \endlink, but without the assertion.
      * Use this for limiting the performance cost of debugging code when doing
      * repeated coefficient access. Only use this when it is guaranteed that the
      * parameter \a index is in range.
      *
      * If EIGEN_INTERNAL_DEBUGGING is defined, an assertion will be made, making this
      * function equivalent to \link operator[](int) const \endlink.
      *
      * \sa operator[](int) const, coeffRef(int), coeff(int,int) const
      */

    EIGEN_STRONG_INLINE const CoeffReturnType
    coeff(int index) const
    {
      ei_internal_assert(index >= 0 && index < size());
      return derived().coeff(index);
    }


    /** \returns the coefficient at given index.
      *
      * This method is allowed only for vector expressions, and for matrix expressions having the LinearAccessBit.
      *
      * \sa operator[](int), operator()(int,int) const, x() const, y() const,
      * z() const, w() const
      */

    EIGEN_STRONG_INLINE const CoeffReturnType
    operator[](int index) const
    {
      EIGEN_STATIC_ASSERT(Derived::IsVectorAtCompileTime,
                          THE_BRACKET_OPERATOR_IS_ONLY_FOR_VECTORS__USE_THE_PARENTHESIS_OPERATOR_INSTEAD)
      ei_assert(index >= 0 && index < size());
      return derived().coeff(index);
    }

    /** \returns the coefficient at given index.
      *
      * This is synonymous to operator[](int) const.
      *
      * This method is allowed only for vector expressions, and for matrix expressions having the LinearAccessBit.
      *
      * \sa operator[](int), operator()(int,int) const, x() const, y() const,
      * z() const, w() const
      */

    EIGEN_STRONG_INLINE const CoeffReturnType
    operator()(int index) const
    {
      ei_assert(index >= 0 && index < size());
      return derived().coeff(index);
    }

    /** equivalent to operator[](0).  */

    EIGEN_STRONG_INLINE const CoeffReturnType
    x() const { return (*this)[0]; }

    /** equivalent to operator[](1).  */

    EIGEN_STRONG_INLINE const CoeffReturnType
    y() const { return (*this)[1]; }

    /** equivalent to operator[](2).  */

    EIGEN_STRONG_INLINE const CoeffReturnType
    z() const { return (*this)[2]; }

    /** equivalent to operator[](3).  */

    EIGEN_STRONG_INLINE const CoeffReturnType
    w() const { return (*this)[3]; }

    /** \returns the packet of coefficients starting at the given row and column. It is your responsibility
      * to ensure that a packet really starts there. This method is only available on expressions having the
      * PacketAccessBit.
      *
      * The \a LoadMode parameter may have the value \a Aligned or \a Unaligned. Its effect is to select
      * the appropriate vectorization instruction. Aligned access is faster, but is only possible for packets
      * starting at an address which is a multiple of the packet size.
      */

    template<int LoadMode>
    EIGEN_STRONG_INLINE typename ei_packet_traits<Scalar>::type
    packet(int row, int col) const
    {
      ei_internal_assert(row >= 0 && row < rows()
                        && col >= 0 && col < cols());
      return derived().template packet<LoadMode>(row,col);
    }


    template<int LoadMode>
    EIGEN_STRONG_INLINE typename ei_packet_traits<Scalar>::type
    packetByOuterInner(int outer, int inner) const
    {
      return packet<LoadMode>(rowIndexByOuterInner(outer, inner),
                              colIndexByOuterInner(outer, inner));
    }

    /** \returns the packet of coefficients starting at the given index. It is your responsibility
      * to ensure that a packet really starts there. This method is only available on expressions having the
      * PacketAccessBit and the LinearAccessBit.
      *
      * The \a LoadMode parameter may have the value \a Aligned or \a Unaligned. Its effect is to select
      * the appropriate vectorization instruction. Aligned access is faster, but is only possible for packets
      * starting at an address which is a multiple of the packet size.
      */

    template<int LoadMode>
    EIGEN_STRONG_INLINE typename ei_packet_traits<Scalar>::type
    packet(int index) const
    {
      ei_internal_assert(index >= 0 && index < size());
      return derived().template packet<LoadMode>(index);
    }

    void coeffRef();
    void coeffRefByOuterInner();
    void writePacket();
    void writePacketByOuterInner();
    void copyCoeff();
    void copyCoeffByOuterInner();
    void copyPacket();
    void copyPacketByOuterInner();
};

template<typename Derived>
class DenseCoeffsBase<Derived, true> : public DenseCoeffsBase<Derived, false>
{
  public:

    typedef DenseCoeffsBase<Derived, false> Base;
    typedef typename ei_traits<Derived>::Scalar Scalar;
    using Base::CoeffReturnType;
    using Base::coeff;
    using Base::rows;
    using Base::cols;
    using Base::size;
    using Base::derived;
    using Base::rowIndexByOuterInner;
    using Base::colIndexByOuterInner;

    /** Short version: don't use this function, use
      * \link operator()(int,int) \endlink instead.
      *
      * Long version: this function is similar to
      * \link operator()(int,int) \endlink, but without the assertion.
      * Use this for limiting the performance cost of debugging code when doing
      * repeated coefficient access. Only use this when it is guaranteed that the
      * parameters \a row and \a col are in range.
      *
      * If EIGEN_INTERNAL_DEBUGGING is defined, an assertion will be made, making this
      * function equivalent to \link operator()(int,int) \endlink.
      *
      * \sa operator()(int,int), coeff(int, int) const, coeffRef(int)
      */
    EIGEN_STRONG_INLINE Scalar& coeffRef(int row, int col)
    {
      ei_internal_assert(row >= 0 && row < rows()
                        && col >= 0 && col < cols());
      return derived().coeffRef(row, col);
    }

    EIGEN_STRONG_INLINE Scalar&
    coeffRefByOuterInner(int outer, int inner)
    {
      return coeffRef(rowIndexByOuterInner(outer, inner),
                      colIndexByOuterInner(outer, inner));
    }

    /** \returns a reference to the coefficient at given the given row and column.
      *
      * \sa operator[](int)
      */

    EIGEN_STRONG_INLINE Scalar&
    operator()(int row, int col)
    {
      ei_assert(row >= 0 && row < rows()
          && col >= 0 && col < cols());
      return derived().coeffRef(row, col);
    }


    /** Short version: don't use this function, use
      * \link operator[](int) \endlink instead.
      *
      * Long version: this function is similar to
      * \link operator[](int) \endlink, but without the assertion.
      * Use this for limiting the performance cost of debugging code when doing
      * repeated coefficient access. Only use this when it is guaranteed that the
      * parameters \a row and \a col are in range.
      *
      * If EIGEN_INTERNAL_DEBUGGING is defined, an assertion will be made, making this
      * function equivalent to \link operator[](int) \endlink.
      *
      * \sa operator[](int), coeff(int) const, coeffRef(int,int)
      */

    EIGEN_STRONG_INLINE Scalar&
    coeffRef(int index)
    {
      ei_internal_assert(index >= 0 && index < size());
      return derived().coeffRef(index);
    }

    /** \returns a reference to the coefficient at given index.
      *
      * This method is allowed only for vector expressions, and for matrix expressions having the LinearAccessBit.
      *
      * \sa operator[](int) const, operator()(int,int), x(), y(), z(), w()
      */

    EIGEN_STRONG_INLINE Scalar&
    operator[](int index)
    {
      EIGEN_STATIC_ASSERT(Derived::IsVectorAtCompileTime,
                          THE_BRACKET_OPERATOR_IS_ONLY_FOR_VECTORS__USE_THE_PARENTHESIS_OPERATOR_INSTEAD)
      ei_assert(index >= 0 && index < size());
      return derived().coeffRef(index);
    }

    /** \returns a reference to the coefficient at given index.
      *
      * This is synonymous to operator[](int).
      *
      * This method is allowed only for vector expressions, and for matrix expressions having the LinearAccessBit.
      *
      * \sa operator[](int) const, operator()(int,int), x(), y(), z(), w()
      */

    EIGEN_STRONG_INLINE Scalar&
    operator()(int index)
    {
      ei_assert(index >= 0 && index < size());
      return derived().coeffRef(index);
    }

    /** equivalent to operator[](0).  */

    EIGEN_STRONG_INLINE Scalar&
    x() { return (*this)[0]; }

    /** equivalent to operator[](1).  */

    EIGEN_STRONG_INLINE Scalar&
    y() { return (*this)[1]; }

    /** equivalent to operator[](2).  */

    EIGEN_STRONG_INLINE Scalar&
    z() { return (*this)[2]; }

    /** equivalent to operator[](3).  */

    EIGEN_STRONG_INLINE Scalar&
    w() { return (*this)[3]; }

    /** Stores the given packet of coefficients, at the given row and column of this expression. It is your responsibility
      * to ensure that a packet really starts there. This method is only available on expressions having the
      * PacketAccessBit.
      *
      * The \a LoadMode parameter may have the value \a Aligned or \a Unaligned. Its effect is to select
      * the appropriate vectorization instruction. Aligned access is faster, but is only possible for packets
      * starting at an address which is a multiple of the packet size.
      */

    template<int StoreMode>
    EIGEN_STRONG_INLINE void writePacket
    (int row, int col, const typename ei_packet_traits<Scalar>::type& x)
    {
      ei_internal_assert(row >= 0 && row < rows()
                        && col >= 0 && col < cols());
      derived().template writePacket<StoreMode>(row,col,x);
    }


    template<int StoreMode>
    EIGEN_STRONG_INLINE void writePacketByOuterInner
    (int outer, int inner, const typename ei_packet_traits<Scalar>::type& x)
    {
      writePacket<StoreMode>(rowIndexByOuterInner(outer, inner),
                            colIndexByOuterInner(outer, inner),
                            x);
    }

    /** Stores the given packet of coefficients, at the given index in this expression. It is your responsibility
      * to ensure that a packet really starts there. This method is only available on expressions having the
      * PacketAccessBit and the LinearAccessBit.
      *
      * The \a LoadMode parameter may have the value \a Aligned or \a Unaligned. Its effect is to select
      * the appropriate vectorization instruction. Aligned access is faster, but is only possible for packets
      * starting at an address which is a multiple of the packet size.
      */

    template<int StoreMode>
    EIGEN_STRONG_INLINE void writePacket
    (int index, const typename ei_packet_traits<Scalar>::type& x)
    {
      ei_internal_assert(index >= 0 && index < size());
      derived().template writePacket<StoreMode>(index,x);
    }

#ifndef EIGEN_PARSED_BY_DOXYGEN

    /** \internal Copies the coefficient at position (row,col) of other into *this.
      *
      * This method is overridden in SwapWrapper, allowing swap() assignments to share 99% of their code
      * with usual assignments.
      *
      * Outside of this internal usage, this method has probably no usefulness. It is hidden in the public API dox.
      */

    template<typename OtherDerived>
    EIGEN_STRONG_INLINE void copyCoeff(int row, int col, const DenseBase<OtherDerived>& other)
    {
      ei_internal_assert(row >= 0 && row < rows()
                        && col >= 0 && col < cols());
      derived().coeffRef(row, col) = other.derived().coeff(row, col);
    }

    /** \internal Copies the coefficient at the given index of other into *this.
      *
      * This method is overridden in SwapWrapper, allowing swap() assignments to share 99% of their code
      * with usual assignments.
      *
      * Outside of this internal usage, this method has probably no usefulness. It is hidden in the public API dox.
      */

    template<typename OtherDerived>
    EIGEN_STRONG_INLINE void copyCoeff(int index, const DenseBase<OtherDerived>& other)
    {
      ei_internal_assert(index >= 0 && index < size());
      derived().coeffRef(index) = other.derived().coeff(index);
    }


    template<typename OtherDerived>
    EIGEN_STRONG_INLINE void copyCoeffByOuterInner(int outer, int inner, const DenseBase<OtherDerived>& other)
    {
      const int row = rowIndexByOuterInner(outer,inner);
      const int col = colIndexByOuterInner(outer,inner);
      // derived() is important here: copyCoeff() may be reimplemented in Derived!
      derived().copyCoeff(row, col, other);
    }

    /** \internal Copies the packet at position (row,col) of other into *this.
      *
      * This method is overridden in SwapWrapper, allowing swap() assignments to share 99% of their code
      * with usual assignments.
      *
      * Outside of this internal usage, this method has probably no usefulness. It is hidden in the public API dox.
      */

    template<typename OtherDerived, int StoreMode, int LoadMode>
    EIGEN_STRONG_INLINE void copyPacket(int row, int col, const DenseBase<OtherDerived>& other)
    {
      ei_internal_assert(row >= 0 && row < rows()
                        && col >= 0 && col < cols());
      derived().template writePacket<StoreMode>(row, col,
        other.derived().template packet<LoadMode>(row, col));
    }

    /** \internal Copies the packet at the given index of other into *this.
      *
      * This method is overridden in SwapWrapper, allowing swap() assignments to share 99% of their code
      * with usual assignments.
      *
      * Outside of this internal usage, this method has probably no usefulness. It is hidden in the public API dox.
      */

    template<typename OtherDerived, int StoreMode, int LoadMode>
    EIGEN_STRONG_INLINE void copyPacket(int index, const DenseBase<OtherDerived>& other)
    {
      ei_internal_assert(index >= 0 && index < size());
      derived().template writePacket<StoreMode>(index,
        other.derived().template packet<LoadMode>(index));
    }

    template<typename OtherDerived, int StoreMode, int LoadMode>
    EIGEN_STRONG_INLINE void copyPacketByOuterInner(int outer, int inner, const DenseBase<OtherDerived>& other)
    {
      const int row = rowIndexByOuterInner(outer,inner);
      const int col = colIndexByOuterInner(outer,inner);
      // derived() is important here: copyCoeff() may be reimplemented in Derived!
      derived().copyPacket<OtherDerived, StoreMode, LoadMode>(row, col, other);
    }
#endif
};

template<typename Derived, bool JustReturnZero>
struct ei_first_aligned_impl
{
  inline static int run(const Derived&)
  { return 0; }
};

template<typename Derived>
struct ei_first_aligned_impl<Derived, false>
{
  inline static int run(const Derived& m)
  {
    return ei_first_aligned(&m.const_cast_derived().coeffRef(0,0), m.size());
  }
};

/** \internal \returns the index of the first element of the array that is well aligned for vectorization.
  *
  * There is also the variant ei_first_aligned(const Scalar*, Integer) defined in Memory.h. See it for more
  * documentation.
  */
template<typename Derived>
inline static int ei_first_aligned(const Derived& m)
{
  return ei_first_aligned_impl
          <Derived, (Derived::Flags & AlignedBit) || !(Derived::Flags & DirectAccessBit)>
          ::run(m);
}

#endif // EIGEN_COEFFS_H
