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

#ifndef EIGEN_SCALING_H
#define EIGEN_SCALING_H

/** \geometry_module \ingroup GeometryModule
  *
  * \class Scaling
  *
  * \brief Represents a possibly non uniform scaling transformation
  *
  * \param _Scalar the scalar type, i.e., the type of the coefficients.
  * \param _Dim the  dimension of the space, can be a compile time value or Dynamic
  *
  *
  * \sa class Translation, class Transform
  */
template<typename _Scalar, int _Dim>
class Scaling
{
public:
  /** dimension of the space */
  enum { Dim = _Dim };
  /** the scalar type of the coefficients */
  typedef _Scalar Scalar;
  /** corresponding vector type */
  typedef Matrix<Scalar,Dim,1> VectorType;
  /** corresponding linear transformation matrix type */
  typedef Matrix<Scalar,Dim,Dim> LinearMatrixType;
  /** corresponding translation type */
  typedef Translation<Scalar,Dim> TranslationType;
  /** corresponding affine transformation type */
  typedef Transform<Scalar,Dim> TransformType;

protected:

  VectorType m_coeffs;

public:

  /** Default constructor without initialization. */
  Scaling() {}
  /** Constructs and initialize a uniform scaling transformation */
  explicit inline Scaling(const Scalar& s) { m_coeffs.setConstant(s); }
  /** 2D only */
  inline Scaling(const Scalar& sx, const Scalar& sy)
  {
    ei_assert(Dim==2);
    m_coeffs.x() = sx;
    m_coeffs.y() = sy;
  }
  /** 3D only */
  inline Scaling(const Scalar& sx, const Scalar& sy, const Scalar& sz)
  {
    ei_assert(Dim==3);
    m_coeffs.x() = sx;
    m_coeffs.y() = sy;
    m_coeffs.z() = sz;
  }
  /** Constructs and initialize the scaling transformation from a vector of scaling coefficients */
  explicit inline Scaling(const VectorType& coeffs) : m_coeffs(coeffs) {}

  const VectorType& coeffs() const { return m_coeffs; }
  VectorType& coeffs() { return m_coeffs; }

  /** Concatenates two scaling */
  inline Scaling operator* (const Scaling& other) const
  { return Scaling(coeffs().cwise() * other.coeffs()); }

  /** Concatenates a scaling and a translation */
  inline TransformType operator* (const TranslationType& t) const;

  /** Concatenates a scaling and an affine transformation */
  inline TransformType operator* (const TransformType& t) const;

  /** Concatenates a scaling and a linear transformation matrix */
  // TODO returns an expression
  inline LinearMatrixType operator* (const LinearMatrixType& other) const
  { return coeffs().asDiagonal() * other; }

  /** Concatenates a linear transformation matrix and a scaling */
  // TODO returns an expression
  friend inline LinearMatrixType operator* (const LinearMatrixType& other, const Scaling& s)
  { return other * s.coeffs().asDiagonal(); }

  /** Applies scaling to vector */
  inline VectorType operator* (const VectorType& other) const
  { return coeffs().asDiagonal() * other; }

  /** \returns the inverse scaling */
  inline Scaling inverse() const
  { return Scaling(coeffs.cwise().inverse()); }

  inline Scaling& operator=(const Scaling& other)
  {
    m_coeffs = other.m_coeffs;
    return *this;
  }

};

/** \addtogroup GeometryModule */
//@{
typedef Scaling<float, 2> Scaling2f;
typedef Scaling<double,2> Scaling2d;
typedef Scaling<float, 3> Scaling3f;
typedef Scaling<double,3> Scaling3d;
//@}

template<typename Scalar, int Dim>
inline typename Scaling<Scalar,Dim>::TransformType
Scaling<Scalar,Dim>::operator* (const TranslationType& t) const
{
  TransformType res;
  res.matrix().setZero();
  res.linear().diagonal() = coeffs();
  res.translation() = m_coeffs.cwise() * t.vector();
  res(Dim,Dim) = Scalar(1);
  return res;
}

template<typename Scalar, int Dim>
inline typename Scaling<Scalar,Dim>::TransformType
Scaling<Scalar,Dim>::operator* (const TransformType& t) const
{
  TransformType res = t;
  res.prescale(m_coeffs);
  return res;
}

#endif // EIGEN_SCALING_H
