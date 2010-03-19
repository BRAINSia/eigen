// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2007-2010 Benoit Jacob <jacob.benoit.1@gmail.com>
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

#ifndef EIGEN_MAP_H
#define EIGEN_MAP_H

/** \class Map
  *
  * \brief A matrix or vector expression mapping an existing array of data.
  *
  * \param MatrixType the equivalent matrix type of the mapped data
  * \param Options specifies whether the pointer is \c Aligned, or \c Unaligned.
  *                The default is \c Unaligned.
  * \param StrideType optionnally specifies strides. By default, Map assumes the memory layout
  *                   of an ordinary, contiguous array. This can be overridden by specifying strides.
  *                   The type passed here must be a specialization of the Stride template, see examples below.
  *
  * This class represents a matrix or vector expression mapping an existing array of data.
  * It can be used to let Eigen interface without any overhead with non-Eigen data structures,
  * such as plain C arrays or structures from other libraries. By default, it assumes that the
  * data is laid out contiguously in memory. You can however override this by explicitly specifying
  * inner and outer strides.
  *
  * Here's an example of simply mapping a contiguous array as a column-major matrix:
  * \include Map_simple.cpp
  * Output: \verbinclude Map_simple.out
  *
  * If you need to map non-contiguous arrays, you can do so by specifying strides:
  *
  * Here's an example of mapping an array as a vector, specifying an inner stride, that is, the pointer
  * increment between two consecutive coefficients. Here, we're specifying the inner stride as a compile-time
  * fixed value.
  * \include Map_inner_stride.cpp
  * Output: \verbinclude Map_inner_stride.out
  *
  * Here's an example of mapping an array while specifying an outer stride. Here, since we're mapping
  * as a column-major matrix, 'outer stride' means the pointer increment between two consecutive columns.
  * Here, we're specifying the outer stride as a runtime parameter.
  * \include Map_outer_stride.cpp
  * Output: \verbinclude Map_outer_stride.out
  *
  * For more details and for an example of specifying both an inner and an outer stride, see class Stride.
  *
  * \b Tip: to change the array of data mapped by a Map object, you can use the C++
  * placement new syntax:
  *
  * Example: \include Map_placement_new.cpp
  * Output: \verbinclude Map_placement_new.out
  *
  * This class is the return type of Matrix::Map() but can also be used directly.
  *
  * \sa Matrix::Map()
  */
template<typename MatrixType, int Options, typename StrideType>
struct ei_traits<Map<MatrixType, Options, StrideType> >
  : public ei_traits<MatrixType>
{
  typedef typename MatrixType::Scalar Scalar;
  enum {
    InnerStride = StrideType::InnerStrideAtCompileTime,
    OuterStride = StrideType::OuterStrideAtCompileTime,
    HasNoInnerStride = InnerStride <= 1,
    HasNoOuterStride = OuterStride == 0,
    HasNoStride = HasNoInnerStride && HasNoOuterStride,
    IsAligned = int(int(Options)&Aligned)==Aligned,
    IsDynamicSize = MatrixType::SizeAtCompileTime==Dynamic,
    KeepsPacketAccess = bool(HasNoInnerStride)
                        && ( bool(IsDynamicSize)
                           || HasNoOuterStride
                           || ( OuterStride!=Dynamic && ((int(OuterStride)*sizeof(Scalar))%16)==0 ) ),
    Flags0 = ei_traits<MatrixType>::Flags,
    Flags1 = IsAligned ? int(Flags0) |  AlignedBit : int(Flags0) & ~AlignedBit,
    Flags2 = HasNoStride ? int(Flags1) : int(Flags1 & ~LinearAccessBit),
    Flags = KeepsPacketAccess ? int(Flags2) : (int(Flags2) & ~PacketAccessBit)
  };
};

template<typename MatrixType, int Options, typename StrideType> class Map
  : public MapBase<Map<MatrixType, Options, StrideType>,
                   typename MatrixType::template MakeBase<
                     Map<MatrixType, Options, StrideType>
                   >::Type>
{
  public:

    typedef MapBase<Map,typename MatrixType::template MakeBase<Map>::Type> Base;

    EIGEN_DENSE_PUBLIC_INTERFACE(Map)

    inline int innerStride() const
    {
      return StrideType::InnerStrideAtCompileTime != 0 ? m_stride.inner() : 1;
    }

    inline int outerStride() const
    {
      return StrideType::OuterStrideAtCompileTime != 0 ? m_stride.outer()
           : IsVectorAtCompileTime ? this->size()
           : int(Flags)&RowMajorBit ? this->cols()
           : this->rows();
    }

    /** Constructor in the fixed-size case.
      *
      * \param data pointer to the array to map
      * \param stride optional Stride object, passing the strides.
      */
    inline Map(const Scalar* data, const StrideType& stride = StrideType())
      : Base(data), m_stride(stride)
    {
      MatrixType::Base::_check_template_params();
    }

    /** Constructor in the dynamic-size vector case.
      *
      * \param data pointer to the array to map
      * \param size the size of the vector expression
      * \param stride optional Stride object, passing the strides.
      */
    inline Map(const Scalar* data, int size, const StrideType& stride = StrideType())
      : Base(data, size), m_stride(stride)
    {
      MatrixType::Base::_check_template_params();
    }

    /** Constructor in the dynamic-size matrix case.
      *
      * \param data pointer to the array to map
      * \param rows the number of rows of the matrix expression
      * \param cols the number of columns of the matrix expression
      * \param stride optional Stride object, passing the strides.
      */
    inline Map(const Scalar* data, int rows, int cols, const StrideType& stride = StrideType())
      : Base(data, rows, cols), m_stride(stride)
    {
      MatrixType::Base::_check_template_params();
    }


    EIGEN_INHERIT_ASSIGNMENT_OPERATORS(Map)

  protected:
    StrideType m_stride;
};

template<typename _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
inline Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>
  ::Matrix(const Scalar *data)
{
  _set_noalias(Eigen::Map<Matrix>(data));
}

#endif // EIGEN_MAP_H
