// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2006-2008 Benoit Jacob <jacob@math.jussieu.fr>
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

#ifndef EIGEN_DOT_H
#define EIGEN_DOT_H

template<int Index, int Size, typename Derived1, typename Derived2>
struct DotUnroller
{
  static void run(const Derived1 &v1, const Derived2& v2, typename Derived1::Scalar &dot)
  {
    DotUnroller<Index-1, Size, Derived1, Derived2>::run(v1, v2, dot);
    dot += v1.coeff(Index) * ei_conj(v2.coeff(Index));
  }
};

template<int Size, typename Derived1, typename Derived2>
struct DotUnroller<0, Size, Derived1, Derived2>
{
  static void run(const Derived1 &v1, const Derived2& v2, typename Derived1::Scalar &dot)
  {
    dot = v1.coeff(0) * ei_conj(v2.coeff(0));
  }
};

template<int Index, typename Derived1, typename Derived2>
struct DotUnroller<Index, Dynamic, Derived1, Derived2>
{
  static void run(const Derived1&, const Derived2&, typename Derived1::Scalar&) {}
};

// prevent buggy user code from causing an infinite recursion
template<int Index, typename Derived1, typename Derived2>
struct DotUnroller<Index, 0, Derived1, Derived2>
{
  static void run(const Derived1&, const Derived2&, typename Derived1::Scalar&) {}
};

/** \returns the dot product of *this with other.
  *
  * \only_for_vectors
  *
  * \note If the scalar type is complex numbers, then this function returns the hermitian
  * (sesquilinear) dot product, linear in the first variable and anti-linear in the
  * second variable.
  *
  * \sa norm2(), norm()
  */
template<typename Derived>
template<typename OtherDerived>
typename ei_traits<Derived>::Scalar
MatrixBase<Derived>::dot(const MatrixBase<OtherDerived>& other) const
{
  assert(IsVectorAtCompileTime
      && OtherDerived::IsVectorAtCompileTime
      && size() == other.size());
  Scalar res;
  if(EIGEN_UNROLLED_LOOPS
  && SizeAtCompileTime != Dynamic
  && SizeAtCompileTime <= EIGEN_UNROLLING_LIMIT_PRODUCT)
    DotUnroller<SizeAtCompileTime-1,
                SizeAtCompileTime <= EIGEN_UNROLLING_LIMIT_PRODUCT ? SizeAtCompileTime : Dynamic,
                Derived, MatrixBase<OtherDerived> >
      ::run(*static_cast<const Derived*>(this), other, res);
  else
  {
    res = (*this).coeff(0) * ei_conj(other.coeff(0));
    for(int i = 1; i < size(); i++)
      res += (*this).coeff(i)* ei_conj(other.coeff(i));
  }
  return res;
}

/** \returns the squared norm of *this, i.e. the dot product of *this with itself.
  *
  * \only_for_vectors
  *
  * \sa dot(), norm()
  */
template<typename Derived>
typename NumTraits<typename ei_traits<Derived>::Scalar>::Real MatrixBase<Derived>::norm2() const
{
  return ei_real(dot(*this));
}

/** \returns the norm of *this, i.e. the square root of the dot product of *this with itself.
  *
  * \only_for_vectors
  *
  * \sa dot(), norm2()
  */
template<typename Derived>
typename NumTraits<typename ei_traits<Derived>::Scalar>::Real MatrixBase<Derived>::norm() const
{
  return ei_sqrt(norm2());
}

/** \returns an expression of the quotient of *this by its own norm.
  *
  * \only_for_vectors
  *
  * \sa norm()
  */
template<typename Derived>
const CwiseUnaryOp<ScalarMultipleOp<typename ei_traits<Derived>::Scalar>, Derived>
MatrixBase<Derived>::normalized() const
{
  return (*this) / norm();
}

/** \returns true if *this is approximately orthogonal to \a other,
  *          within the precision given by \a prec.
  *
  * Example: \include MatrixBase_isOrtho_vector.cpp
  * Output: \verbinclude MatrixBase_isOrtho_vector.out
  */
template<typename Derived>
template<typename OtherDerived>
bool MatrixBase<Derived>::isOrtho
(const MatrixBase<OtherDerived>& other, RealScalar prec) const
{
  return ei_abs2(dot(other)) <= prec * prec * norm2() * other.norm2();
}

/** \returns true if *this is approximately an unitary matrix,
  *          within the precision given by \a prec. In the case where the \a Scalar
  *          type is real numbers, a unitary matrix is an orthogonal matrix, whence the name.
  *
  * \note This can be used to check whether a family of vectors forms an orthonormal basis.
  *       Indeed, \c m.isOrtho() returns true if and only if the columns of m form an
  *       orthonormal basis.
  *
  * Example: \include MatrixBase_isOrtho_matrix.cpp
  * Output: \verbinclude MatrixBase_isOrtho_matrix.out
  */
template<typename Derived>
bool MatrixBase<Derived>::isOrtho(RealScalar prec) const
{
  for(int i = 0; i < cols(); i++)
  {
    if(!ei_isApprox(col(i).norm2(), static_cast<Scalar>(1), prec))
      return false;
    for(int j = 0; j < i; j++)
      if(!ei_isMuchSmallerThan(col(i).dot(col(j)), static_cast<Scalar>(1), prec))
        return false;
  }
  return true;
}
#endif // EIGEN_DOT_H
