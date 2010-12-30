// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2009 Gael Guennebaud <gael.guennebaud@inria.fr>
// Copyright (C) 2010 Benoit Jacob <jacob.benoit.1@gmail.com>
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

#ifndef EIGEN_HOUSEHOLDER_SEQUENCE_H
#define EIGEN_HOUSEHOLDER_SEQUENCE_H

/** \ingroup Householder_Module
  * \householder_module
  * \class HouseholderSequence
  * \brief Represents a sequence of householder reflections with decreasing size
  *
  * This class represents a product sequence of householder reflections \f$ H = \Pi_0^{n-1} H_i \f$
  * where \f$ H_i \f$ is the i-th householder transformation \f$ I - h_i v_i v_i^* \f$,
  * \f$ v_i \f$ is the i-th householder vector \f$ [ 1, m_vectors(i+1,i), m_vectors(i+2,i), ...] \f$
  * and \f$ h_i \f$ is the i-th householder coefficient \c m_coeffs[i].
  *
  * Typical usages are listed below, where H is a HouseholderSequence:
  * \code
  * A.applyOnTheRight(H);             // A = A * H
  * A.applyOnTheLeft(H);              // A = H * A
  * A.applyOnTheRight(H.adjoint());   // A = A * H^*
  * A.applyOnTheLeft(H.adjoint());    // A = H^* * A
  * MatrixXd Q = H;                   // conversion to a dense matrix
  * \endcode
  * In addition to the adjoint, you can also apply the inverse (=adjoint), the transpose, and the conjugate operators.
  *
  * \sa MatrixBase::applyOnTheLeft(), MatrixBase::applyOnTheRight()
  */

namespace internal {

template<typename VectorsType, typename CoeffsType, int Side>
struct traits<HouseholderSequence<VectorsType,CoeffsType,Side> >
{
  typedef typename VectorsType::Scalar Scalar;
  typedef typename VectorsType::Index Index;
  typedef typename VectorsType::StorageKind StorageKind;
  enum {
    RowsAtCompileTime = Side==OnTheLeft ? traits<VectorsType>::RowsAtCompileTime
                                        : traits<VectorsType>::ColsAtCompileTime,
    ColsAtCompileTime = RowsAtCompileTime,
    MaxRowsAtCompileTime = Side==OnTheLeft ? traits<VectorsType>::MaxRowsAtCompileTime
                                           : traits<VectorsType>::MaxColsAtCompileTime,
    MaxColsAtCompileTime = MaxRowsAtCompileTime,
    Flags = 0
  };
};

template<typename VectorsType, typename CoeffsType, int Side>
struct hseq_side_dependent_impl
{
  typedef Block<const VectorsType, Dynamic, 1> EssentialVectorType;
  typedef HouseholderSequence<VectorsType, CoeffsType, OnTheLeft> HouseholderSequenceType;
  typedef typename VectorsType::Index Index;
  static inline const EssentialVectorType essentialVector(const HouseholderSequenceType& h, Index k)
  {
    Index start = k+1+h.m_shift;
    return Block<const VectorsType,Dynamic,1>(h.m_vectors, start, k, h.rows()-start, 1);
  }
};

template<typename VectorsType, typename CoeffsType>
struct hseq_side_dependent_impl<VectorsType, CoeffsType, OnTheRight>
{
  typedef Transpose<Block<const VectorsType, 1, Dynamic> > EssentialVectorType;
  typedef HouseholderSequence<VectorsType, CoeffsType, OnTheRight> HouseholderSequenceType;
  typedef typename VectorsType::Index Index;
  static inline const EssentialVectorType essentialVector(const HouseholderSequenceType& h, Index k)
  {
    Index start = k+1+h.m_shift;
    return Block<const VectorsType,1,Dynamic>(h.m_vectors, k, start, 1, h.rows()-start).transpose();
  }
};

template<typename OtherScalarType, typename MatrixType> struct matrix_type_times_scalar_type
{
  typedef typename scalar_product_traits<OtherScalarType, typename MatrixType::Scalar>::ReturnType
    ResultScalar;
  typedef Matrix<ResultScalar, MatrixType::RowsAtCompileTime, MatrixType::ColsAtCompileTime,
                 0, MatrixType::MaxRowsAtCompileTime, MatrixType::MaxColsAtCompileTime> Type;
};

} // end namespace internal

template<typename VectorsType, typename CoeffsType, int Side> class HouseholderSequence
  : public EigenBase<HouseholderSequence<VectorsType,CoeffsType,Side> >
{
    enum {
      RowsAtCompileTime = internal::traits<HouseholderSequence>::RowsAtCompileTime,
      ColsAtCompileTime = internal::traits<HouseholderSequence>::ColsAtCompileTime,
      MaxRowsAtCompileTime = internal::traits<HouseholderSequence>::MaxRowsAtCompileTime,
      MaxColsAtCompileTime = internal::traits<HouseholderSequence>::MaxColsAtCompileTime
    };
    typedef typename internal::traits<HouseholderSequence>::Scalar Scalar;
    typedef typename VectorsType::Index Index;

    typedef typename internal::hseq_side_dependent_impl<VectorsType,CoeffsType,Side>::EssentialVectorType
            EssentialVectorType;

  public:

    typedef HouseholderSequence<
      VectorsType,
      typename internal::conditional<NumTraits<Scalar>::IsComplex,
        typename internal::remove_all<typename CoeffsType::ConjugateReturnType>::type,
        CoeffsType>::type,
      Side
    > ConjugateReturnType;

    HouseholderSequence(const VectorsType& v, const CoeffsType& h)
      : m_vectors(v), m_coeffs(h), m_trans(false), m_length(v.diagonalSize()),
        m_shift(0)
    {
    }

    HouseholderSequence(const HouseholderSequence& other)
      : m_vectors(other.m_vectors),
        m_coeffs(other.m_coeffs),
        m_trans(other.m_trans),
        m_length(other.m_length),
        m_shift(other.m_shift)
    {
    }

    Index rows() const { return Side==OnTheLeft ? m_vectors.rows() : m_vectors.cols(); }
    Index cols() const { return rows(); }

    const EssentialVectorType essentialVector(Index k) const
    {
      eigen_assert(k >= 0 && k < m_length);
      return internal::hseq_side_dependent_impl<VectorsType,CoeffsType,Side>::essentialVector(*this, k);
    }

    HouseholderSequence transpose() const
    {
      return HouseholderSequence(*this).setTrans(!m_trans);
    }

    ConjugateReturnType conjugate() const
    {
      return ConjugateReturnType(m_vectors, m_coeffs.conjugate())
             .setTrans(m_trans)
             .setLength(m_length)
             .setShift(m_shift);
    }

    ConjugateReturnType adjoint() const
    {
      return conjugate().setTrans(!m_trans);
    }

    ConjugateReturnType inverse() const { return adjoint(); }

    /** \internal */
    template<typename DestType> void evalTo(DestType& dst) const
    {
      Index vecs = m_length;
      // FIXME find a way to pass this temporary if the user wants to
      Matrix<Scalar, DestType::RowsAtCompileTime, 1,
             AutoAlign|ColMajor, DestType::MaxRowsAtCompileTime, 1> temp(rows());
      if(    internal::is_same<typename internal::remove_all<VectorsType>::type,DestType>::value
          && internal::extract_data(dst) == internal::extract_data(m_vectors))
      {
        // in-place
        dst.diagonal().setOnes();
        dst.template triangularView<StrictlyUpper>().setZero();
        for(Index k = vecs-1; k >= 0; --k)
        {
          Index cornerSize = rows() - k - m_shift;
          if(m_trans)
            dst.bottomRightCorner(cornerSize, cornerSize)
            .applyHouseholderOnTheRight(essentialVector(k), m_coeffs.coeff(k), &temp.coeffRef(0));
          else
            dst.bottomRightCorner(cornerSize, cornerSize)
              .applyHouseholderOnTheLeft(essentialVector(k), m_coeffs.coeff(k), &temp.coeffRef(0));

          // clear the off diagonal vector
          dst.col(k).tail(rows()-k-1).setZero();
        }
        // clear the remaining columns if needed
        for(Index k = 0; k<cols()-vecs ; ++k)
          dst.col(k).tail(rows()-k-1).setZero();
      }
      else
      {
        dst.setIdentity(rows(), rows());
        for(Index k = vecs-1; k >= 0; --k)
        {
          Index cornerSize = rows() - k - m_shift;
          if(m_trans)
            dst.bottomRightCorner(cornerSize, cornerSize)
            .applyHouseholderOnTheRight(essentialVector(k), m_coeffs.coeff(k), &temp.coeffRef(0));
          else
            dst.bottomRightCorner(cornerSize, cornerSize)
              .applyHouseholderOnTheLeft(essentialVector(k), m_coeffs.coeff(k), &temp.coeffRef(0));
        }
      }
    }

    /** \internal */
    template<typename Dest> inline void applyThisOnTheRight(Dest& dst) const
    {
      Matrix<Scalar,1,Dest::RowsAtCompileTime> temp(dst.rows());
      for(Index k = 0; k < m_length; ++k)
      {
        Index actual_k = m_trans ? m_length-k-1 : k;
        dst.rightCols(rows()-m_shift-actual_k)
           .applyHouseholderOnTheRight(essentialVector(actual_k), m_coeffs.coeff(actual_k), &temp.coeffRef(0));
      }
    }

    /** \internal */
    template<typename Dest> inline void applyThisOnTheLeft(Dest& dst) const
    {
      Matrix<Scalar,1,Dest::ColsAtCompileTime> temp(dst.cols());
      for(Index k = 0; k < m_length; ++k)
      {
        Index actual_k = m_trans ? k : m_length-k-1;
        dst.bottomRows(rows()-m_shift-actual_k)
           .applyHouseholderOnTheLeft(essentialVector(actual_k), m_coeffs.coeff(actual_k), &temp.coeffRef(0));
      }
    }

    template<typename OtherDerived>
    typename internal::matrix_type_times_scalar_type<Scalar, OtherDerived>::Type operator*(const MatrixBase<OtherDerived>& other) const
    {
      typename internal::matrix_type_times_scalar_type<Scalar, OtherDerived>::Type
        res(other.template cast<typename internal::matrix_type_times_scalar_type<Scalar, OtherDerived>::ResultScalar>());
      applyThisOnTheLeft(res);
      return res;
    }

    template<typename OtherDerived> friend
    typename internal::matrix_type_times_scalar_type<Scalar, OtherDerived>::Type operator*(const MatrixBase<OtherDerived>& other, const HouseholderSequence& h)
    {
      typename internal::matrix_type_times_scalar_type<Scalar, OtherDerived>::Type
        res(other.template cast<typename internal::matrix_type_times_scalar_type<Scalar, OtherDerived>::ResultScalar>());
      h.applyThisOnTheRight(res);
      return res;
    }

    template<typename _VectorsType, typename _CoeffsType, int _Side> friend struct internal::hseq_side_dependent_impl;

    HouseholderSequence& setTrans(bool trans)
    {
      m_trans = trans;
      return *this;
    }

    HouseholderSequence& setLength(Index length)
    {
      m_length = length;
      return *this;
    }

    HouseholderSequence& setShift(Index shift)
    {
      m_shift = shift;
      return *this;
    }

    bool trans() const { return m_trans; }
    Index length() const { return m_length; }
    Index shift() const { return m_shift; }

  protected:
    typename VectorsType::Nested m_vectors;
    typename CoeffsType::Nested m_coeffs;
    bool m_trans;
    Index m_length;
    Index m_shift;
};

template<typename VectorsType, typename CoeffsType>
HouseholderSequence<VectorsType,CoeffsType> householderSequence(const VectorsType& v, const CoeffsType& h)
{
  return HouseholderSequence<VectorsType,CoeffsType,OnTheLeft>(v, h);
}

template<typename VectorsType, typename CoeffsType>
HouseholderSequence<VectorsType,CoeffsType,OnTheRight> rightHouseholderSequence(const VectorsType& v, const CoeffsType& h)
{
  return HouseholderSequence<VectorsType,CoeffsType,OnTheRight>(v, h);
}

#endif // EIGEN_HOUSEHOLDER_SEQUENCE_H
