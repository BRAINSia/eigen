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

#ifndef EIGEN_TRANSFORM_H
#define EIGEN_TRANSFORM_H

/** Represents some traits of a transformation */
enum TransformTraits {
  NoScaling,      ///< the transformation is a concatenation of translations, rotations
  NoShear,        ///< the transformation is a concatenation of translations, rotations and scalings
  GenericAffine,  ///< the transformation is affine (linear transformation + translation)
  NonAffine       ///< the transformation might not be affine
};

// Note that we have to pass Dim and HDim because it is not allowed to use a template
// parameter to define a template specialization. To be more precise, in the following
// specializations, it is not allowed to use Dim+1 instead of HDim.
template< typename Other,
          int Dim,
          int HDim,
          int OtherRows=Other::RowsAtCompileTime,
          int OtherCols=Other::ColsAtCompileTime>
struct ei_transform_product_impl;

/** \geometry_module \ingroup GeometryModule
  *
  * \class Transform
  *
  * \brief Represents an homogeneous transformation in a N dimensional space
  *
  * \param _Scalar the scalar type, i.e., the type of the coefficients
  * \param _Dim the dimension of the space
  *
  * The homography is internally represented and stored as a (Dim+1)^2 matrix which
  * is available through the matrix() method.
  *
  * Conversion methods from/to Qt's QMatrix and QTransform are available if the
  * preprocessor token EIGEN_QT_SUPPORT is defined.
  *
  * \sa class Matrix, class Quaternion
  */
template<typename _Scalar, int _Dim>
class Transform
{
public:

  enum {
    Dim = _Dim,     ///< space dimension in which the transformation holds
    HDim = _Dim+1   ///< size of a respective homogeneous vector
  };
  /** the scalar type of the coefficients */
  typedef _Scalar Scalar;
  /** type of the matrix used to represent the transformation */
  typedef Matrix<Scalar,HDim,HDim> MatrixType;
  /** type of the matrix used to represent the linear part of the transformation */
  typedef Matrix<Scalar,Dim,Dim> LinearMatrixType;
  /** type of read/write reference to the linear part of the transformation */
  typedef Block<MatrixType,Dim,Dim> LinearPart;
  /** type of a vector */
  typedef Matrix<Scalar,Dim,1> VectorType;
  /** type of a read/write reference to the translation part of the rotation */
  typedef Block<MatrixType,Dim,1> TranslationPart;
  /** corresponding translation type */
  typedef Translation<Scalar,Dim> TranslationType;
  /** corresponding scaling transformation type */
  typedef Scaling<Scalar,Dim> ScalingType;

protected:

  MatrixType m_matrix;

public:

  /** Default constructor without initialization of the coefficients. */
  inline Transform() { }

  inline Transform(const Transform& other)
  { m_matrix = other.m_matrix; }

  inline Transform& operator=(const Transform& other)
  { m_matrix = other.m_matrix; return *this; }

  template<typename OtherDerived, bool select = OtherDerived::RowsAtCompileTime == Dim>
  struct construct_from_matrix
  {
    static inline void run(Transform *transform, const MatrixBase<OtherDerived>& other)
    {
      transform->matrix() = other;
    }
  };

  template<typename OtherDerived> struct construct_from_matrix<OtherDerived, true>
  {
    static inline void run(Transform *transform, const MatrixBase<OtherDerived>& other)
    {
      transform->linear() = other;
      transform->translation().setZero();
      transform->matrix()(Dim,Dim) = Scalar(1);
      transform->matrix().template block<1,Dim>(Dim,0).setZero();
    }
  };

  /** Constructs and initializes a transformation from a Dim^2 or a (Dim+1)^2 matrix. */
  template<typename OtherDerived>
  inline explicit Transform(const MatrixBase<OtherDerived>& other)
  {
    construct_from_matrix<OtherDerived>::run(this, other);
  }

  /** Set \c *this from a (Dim+1)^2 matrix. */
  template<typename OtherDerived>
  inline Transform& operator=(const MatrixBase<OtherDerived>& other)
  { m_matrix = other; return *this; }

  #ifdef EIGEN_QT_SUPPORT
  inline Transform(const QMatrix& other);
  inline Transform& operator=(const QMatrix& other);
  inline QMatrix toQMatrix(void) const;
  inline Transform(const QTransform& other);
  inline Transform& operator=(const QTransform& other);
  inline QTransform toQTransform(void) const;
  #endif

  /** shortcut for m_matrix(row,col);
    * \sa MatrixBase::operaror(int,int) const */
  inline Scalar operator() (int row, int col) const { return m_matrix(row,col); }
  /** shortcut for m_matrix(row,col);
    * \sa MatrixBase::operaror(int,int) */
  inline Scalar& operator() (int row, int col) { return m_matrix(row,col); }

  /** \returns a read-only expression of the transformation matrix */
  inline const MatrixType& matrix() const { return m_matrix; }
  /** \returns a writable expression of the transformation matrix */
  inline MatrixType& matrix() { return m_matrix; }

  /** \returns a read-only expression of the linear (linear) part of the transformation */
  inline const LinearPart linear() const { return m_matrix.template block<Dim,Dim>(0,0); }
  /** \returns a writable expression of the linear (linear) part of the transformation */
  inline LinearPart linear() { return m_matrix.template block<Dim,Dim>(0,0); }

  /** \returns a read-only expression of the translation vector of the transformation */
  inline const TranslationPart translation() const { return m_matrix.template block<Dim,1>(0,Dim); }
  /** \returns a writable expression of the translation vector of the transformation */
  inline TranslationPart translation() { return m_matrix.template block<Dim,1>(0,Dim); }

  /** \returns an expression of the product between the transform \c *this and a matrix expression \a other
  *
  * The right hand side \a other might be either:
  * \li a vector of size Dim,
  * \li an homogeneous vector of size Dim+1,
  * \li a transformation matrix of size Dim+1 x Dim+1.
  */
  // note: this function is defined here because some compilers cannot find the respective declaration
  template<typename OtherDerived>
  inline const typename ei_transform_product_impl<OtherDerived,_Dim,_Dim+1>::ResultType
  operator * (const MatrixBase<OtherDerived> &other) const
  { return ei_transform_product_impl<OtherDerived,Dim,HDim>::run(*this,other.derived()); }

  /** Contatenates two transformations */
  inline const typename ProductReturnType<MatrixType,MatrixType>::Type
  operator * (const Transform& other) const
  { return m_matrix * other.matrix(); }

  /** \sa MatrixBase::setIdentity() */
  void setIdentity() { m_matrix.setIdentity(); }

  template<typename OtherDerived>
  inline Transform& scale(const MatrixBase<OtherDerived> &other);

  template<typename OtherDerived>
  inline Transform& prescale(const MatrixBase<OtherDerived> &other);

  inline Transform& scale(Scalar s);
  inline Transform& prescale(Scalar s);

  template<typename OtherDerived>
  inline Transform& translate(const MatrixBase<OtherDerived> &other);

  template<typename OtherDerived>
  inline Transform& pretranslate(const MatrixBase<OtherDerived> &other);

  template<typename RotationType>
  inline Transform& rotate(const RotationType& rotation);

  template<typename RotationType>
  inline Transform& prerotate(const RotationType& rotation);

  Transform& shear(Scalar sx, Scalar sy);
  Transform& preshear(Scalar sx, Scalar sy);

  inline Transform& operator=(const TranslationType& t);
  inline Transform& operator*=(const TranslationType& t) { return translate(t.vector()); }
  inline Transform operator*(const TranslationType& t) const;

  inline Transform& operator=(const ScalingType& t);
  inline Transform& operator*=(const ScalingType& s) { return scale(s.coeffs()); }
  inline Transform operator*(const ScalingType& s) const;
  friend inline Transform operator*(const LinearMatrixType& mat, const Transform& t)
  {
    Transform res = t;
    res.matrix().row(Dim) = t.matrix().row(Dim);
    res.matrix().template block<Dim,HDim>(0,0) = (mat * t.matrix().template block<Dim,HDim>(0,0)).lazy();
    return res;
  }

//   template<typename Derived>
//   inline Transform& operator=(const Rotation<Derived,Dim>& t);
  template<typename Derived>
  inline Transform& operator*=(const RotationBase<Derived,Dim>& r) { return rotate(r.toRotationMatrix()); }
  template<typename Derived>
  inline Transform operator*(const RotationBase<Derived,Dim>& r) const;

  LinearMatrixType extractRotation(TransformTraits traits = GenericAffine) const;

  template<typename PositionDerived, typename OrientationType, typename ScaleDerived>
  Transform& fromPositionOrientationScale(const MatrixBase<PositionDerived> &position,
    const OrientationType& orientation, const MatrixBase<ScaleDerived> &scale);

  inline const MatrixType inverse(TransformTraits traits = GenericAffine) const;

  const Scalar* data() const { return m_matrix.data(); }
  Scalar* data() { return m_matrix.data(); }

protected:

};

/** \ingroup GeometryModule */
typedef Transform<float,2> Transform2f;
/** \ingroup GeometryModule */
typedef Transform<float,3> Transform3f;
/** \ingroup GeometryModule */
typedef Transform<double,2> Transform2d;
/** \ingroup GeometryModule */
typedef Transform<double,3> Transform3d;

/**************************
*** Optional QT support ***
**************************/

#ifdef EIGEN_QT_SUPPORT
/** Initialises \c *this from a QMatrix assuming the dimension is 2.
  *
  * This function is available only if the token EIGEN_QT_SUPPORT is defined.
  */
template<typename Scalar, int Dim>
Transform<Scalar,Dim>::Transform(const QMatrix& other)
{
  *this = other;
}

/** Set \c *this from a QMatrix assuming the dimension is 2.
  *
  * This function is available only if the token EIGEN_QT_SUPPORT is defined.
  */
template<typename Scalar, int Dim>
Transform<Scalar,Dim>& Transform<Scalar,Dim>::operator=(const QMatrix& other)
{
  EIGEN_STATIC_ASSERT(Dim==2, you_did_a_programming_error);
  m_matrix << other.m11(), other.m21(), other.dx(),
              other.m12(), other.m22(), other.dy(),
              0, 0, 1;
   return *this;
}

/** \returns a QMatrix from \c *this assuming the dimension is 2.
  *
  * \warning this convertion might loss data if \c *this is not affine
  *
  * This function is available only if the token EIGEN_QT_SUPPORT is defined.
  */
template<typename Scalar, int Dim>
QMatrix Transform<Scalar,Dim>::toQMatrix(void) const
{
  EIGEN_STATIC_ASSERT(Dim==2, you_did_a_programming_error);
  return QMatrix(other.coeffRef(0,0), other.coeffRef(1,0),
                 other.coeffRef(0,1), other.coeffRef(1,1),
                 other.coeffRef(0,2), other.coeffRef(1,2));
}

/** Initialises \c *this from a QTransform assuming the dimension is 2.
  *
  * This function is available only if the token EIGEN_QT_SUPPORT is defined.
  */
template<typename Scalar, int Dim>
Transform<Scalar,Dim>::Transform(const QTransform& other)
{
  *this = other;
}

/** Set \c *this from a QTransform assuming the dimension is 2.
  *
  * This function is available only if the token EIGEN_QT_SUPPORT is defined.
  */
template<typename Scalar, int Dim>
Transform<Scalar,Dim>& Transform<Scalar,Dim>::operator=(const QTransform& other)
{
  EIGEN_STATIC_ASSERT(Dim==2, you_did_a_programming_error);
  m_matrix << other.m11(), other.m21(), other.dx(),
              other.m12(), other.m22(), other.dy(),
              other.m13(), other.m23(), other.m33();
   return *this;
}

/** \returns a QTransform from \c *this assuming the dimension is 2.
  *
  * This function is available only if the token EIGEN_QT_SUPPORT is defined.
  */
template<typename Scalar, int Dim>
QMatrix Transform<Scalar,Dim>::toQTransform(void) const
{
  EIGEN_STATIC_ASSERT(Dim==2, you_did_a_programming_error);
  return QTransform(other.coeffRef(0,0), other.coeffRef(1,0), other.coeffRef(2,0)
                    other.coeffRef(0,1), other.coeffRef(1,1), other.coeffRef(2,1)
                    other.coeffRef(0,2), other.coeffRef(1,2), other.coeffRef(2,2);
}
#endif

/*********************
*** Procedural API ***
*********************/

/** Applies on the right the non uniform scale transformation represented
  * by the vector \a other to \c *this and returns a reference to \c *this.
  * \sa prescale()
  */
template<typename Scalar, int Dim>
template<typename OtherDerived>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::scale(const MatrixBase<OtherDerived> &other)
{
  EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(OtherDerived,int(Dim));
  linear() = (linear() * other.asDiagonal()).lazy();
  return *this;
}

/** Applies on the right a uniform scale of a factor \a c to \c *this
  * and returns a reference to \c *this.
  * \sa prescale(Scalar)
  */
template<typename Scalar, int Dim>
inline Transform<Scalar,Dim>& Transform<Scalar,Dim>::scale(Scalar s)
{
  linear() *= s;
  return *this;
}

/** Applies on the left the non uniform scale transformation represented
  * by the vector \a other to \c *this and returns a reference to \c *this.
  * \sa scale()
  */
template<typename Scalar, int Dim>
template<typename OtherDerived>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::prescale(const MatrixBase<OtherDerived> &other)
{
  EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(OtherDerived,int(Dim));
  m_matrix.template block<Dim,HDim>(0,0) = (other.asDiagonal() * m_matrix.template block<Dim,HDim>(0,0)).lazy();
  return *this;
}

/** Applies on the left a uniform scale of a factor \a c to \c *this
  * and returns a reference to \c *this.
  * \sa scale(Scalar)
  */
template<typename Scalar, int Dim>
inline Transform<Scalar,Dim>& Transform<Scalar,Dim>::prescale(Scalar s)
{
  m_matrix.template corner<Dim,HDim>(TopLeft) *= s;
  return *this;
}

/** Applies on the right the translation matrix represented by the vector \a other
  * to \c *this and returns a reference to \c *this.
  * \sa pretranslate()
  */
template<typename Scalar, int Dim>
template<typename OtherDerived>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::translate(const MatrixBase<OtherDerived> &other)
{
  EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(OtherDerived,int(Dim));
  translation() += linear() * other;
  return *this;
}

/** Applies on the left the translation matrix represented by the vector \a other
  * to \c *this and returns a reference to \c *this.
  * \sa translate()
  */
template<typename Scalar, int Dim>
template<typename OtherDerived>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::pretranslate(const MatrixBase<OtherDerived> &other)
{
  EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(OtherDerived,int(Dim));
  translation() += other;
  return *this;
}

/** Applies on the right the rotation represented by the rotation \a rotation
  * to \c *this and returns a reference to \c *this.
  *
  * The template parameter \a RotationType is the type of the rotation which
  * must be known by ei_toRotationMatrix<>.
  *
  * Natively supported types includes:
  *   - any scalar (2D),
  *   - a Dim x Dim matrix expression,
  *   - a Quaternion (3D),
  *   - a AngleAxis (3D)
  *
  * This mechanism is easily extendable to support user types such as Euler angles,
  * or a pair of Quaternion for 4D rotations.
  *
  * \sa rotate(Scalar), class Quaternion, class AngleAxis, prerotate(RotationType)
  */
template<typename Scalar, int Dim>
template<typename RotationType>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::rotate(const RotationType& rotation)
{
  linear() *= ei_toRotationMatrix<Scalar,Dim>(rotation);
  return *this;
}

/** Applies on the left the rotation represented by the rotation \a rotation
  * to \c *this and returns a reference to \c *this.
  *
  * See rotate() for further details.
  *
  * \sa rotate()
  */
template<typename Scalar, int Dim>
template<typename RotationType>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::prerotate(const RotationType& rotation)
{
  m_matrix.template block<Dim,HDim>(0,0) = ei_toRotationMatrix<Scalar,Dim>(rotation)
                                         * m_matrix.template block<Dim,HDim>(0,0);
  return *this;
}

/** Applies on the right the shear transformation represented
  * by the vector \a other to \c *this and returns a reference to \c *this.
  * \warning 2D only.
  * \sa preshear()
  */
template<typename Scalar, int Dim>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::shear(Scalar sx, Scalar sy)
{
  EIGEN_STATIC_ASSERT(int(Dim)==2, you_did_a_programming_error);
  VectorType tmp = linear().col(0)*sy + linear().col(1);
  linear() << linear().col(0) + linear().col(1)*sx, tmp;
  return *this;
}

/** Applies on the left the shear transformation represented
  * by the vector \a other to \c *this and returns a reference to \c *this.
  * \warning 2D only.
  * \sa shear()
  */
template<typename Scalar, int Dim>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::preshear(Scalar sx, Scalar sy)
{
  EIGEN_STATIC_ASSERT(int(Dim)==2, you_did_a_programming_error);
  m_matrix.template block<Dim,HDim>(0,0) = LinearMatrixType(1, sx, sy, 1) * m_matrix.template block<Dim,HDim>(0,0);
  return *this;
}

/******************************************************
*** Scaling, Translation and Rotation compatibility ***
******************************************************/

template<typename Scalar, int Dim>
inline Transform<Scalar,Dim>& Transform<Scalar,Dim>::operator=(const TranslationType& t)
{
  setIdentity();
  translation() = t.vector();
  return *this;
}

template<typename Scalar, int Dim>
inline Transform<Scalar,Dim> Transform<Scalar,Dim>::operator*(const TranslationType& t) const
{
  Transform res = *this;
  res.translate(t.vector());
  return res;
}

template<typename Scalar, int Dim>
inline Transform<Scalar,Dim>& Transform<Scalar,Dim>::operator=(const ScalingType& s)
{
  m_matrix.setZero();
  linear().diagonal() = s.coeffs();
  m_matrix(Dim,Dim) = Scalar(1);
  return *this;
}

template<typename Scalar, int Dim>
inline Transform<Scalar,Dim> Transform<Scalar,Dim>::operator*(const ScalingType& s) const
{
  Transform res = *this;
  res.scale(s.coeffs());
  return res;
}

template<typename Scalar, int Dim>
template<typename Derived>
inline Transform<Scalar,Dim> Transform<Scalar,Dim>::operator*(const RotationBase<Derived,Dim>& r) const
{
  Transform res = *this;
  res.rotate(r.derived());
  return res;
}

/***************************
*** Specialial functions ***
***************************/

/** \returns the rotation part of the transformation
  *
  * \param traits allows to optimize the extraction process when the transformion
  * is known to be not a general aafine transformation. The possible values are:
  *  - GenericAffine which use a QR decomposition (default),
  *  - NoShear which is the most probable case and very fast,
  *  - NoScaling which simply returns the linear part !
  *
  * \warning this function consider the scaling is positive
  *
  * \warning to use this method in the general case (traits==GenericAffine), you need
  * to include the QR module.
  *
  * \sa inverse(), class QR
  */
template<typename Scalar, int Dim>
typename Transform<Scalar,Dim>::LinearMatrixType
Transform<Scalar,Dim>::extractRotation(TransformTraits traits) const
{
  ei_assert(traits!=NonAffine && "you cannot extract a rotation from a non affine transformation");
  if (traits == GenericAffine)
  {
    // FIXME maybe QR should be fixed to return a R matrix with a positive diagonal ??
    QR<LinearMatrixType> qr(linear());
    LinearMatrixType matQ = qr.matrixQ();
    LinearMatrixType matR = qr.matrixR();
    for (int i=0 ; i<Dim; ++i)
      if (matR(i,i)<0)
        matQ.col(i) = -matQ.col(i);
    return matQ;
  }
  else if (traits == NoShear)
  {
    // extract linear = rotation * scaling
    //  => rotation = linear * inv(Scaling)
    VectorType invScaling = linear().colwise().norm().cwise().inverse();
    return linear() * invScaling.asDiagonal();
  }
  else if (traits == NoScaling) // though that's stupid let's handle it !
    return linear();
  else
  {
    ei_assert("invalid traits value in Transform::inverse()");
    return LinearMatrixType();
  }
}

/** Convenient method to set \c *this from a position, orientation and scale
  * of a 3D object.
  */
template<typename Scalar, int Dim>
template<typename PositionDerived, typename OrientationType, typename ScaleDerived>
Transform<Scalar,Dim>&
Transform<Scalar,Dim>::fromPositionOrientationScale(const MatrixBase<PositionDerived> &position,
  const OrientationType& orientation, const MatrixBase<ScaleDerived> &scale)
{
  linear() = ei_toRotationMatrix<Scalar,Dim>(orientation);
  linear() *= scale.asDiagonal();
  translation() = position;
  m_matrix(Dim,Dim) = 1.;
  m_matrix.template block<1,Dim>(Dim,0).setZero();
  return *this;
}

/** \returns the inverse transformation matrix according to some given knowledge
  * on \c *this.
  *
  * \param traits allows to optimize the inversion process when the transformion
  * is known to be not a general transformation. The possible values are:
  *  - NonAffine if the transformation is not necessarily affines, i.e., if the
  *    last row is not guaranteed to be [0 ... 0 1]
  *  - GenericAffine is the default, the last row is assumed to be [0 ... 0 1]
  *  - NoShear if the transformation is only a concatenations of translations,
  *    rotations, and scalings.
  *  - NoScaling if the transformation is only a concatenations of translations
  *    and rotations.
  *
  * \warning unless \a traits is always set to NoShear or NoScaling, this function
  * requires the generic inverse method of MatrixBase defined in the LU module. If
  * you forget to include this module, then you will get hard to debug linking errors.
  *
  * \sa MatrixBase::inverse()
  */
template<typename Scalar, int Dim>
inline const typename Transform<Scalar,Dim>::MatrixType
Transform<Scalar,Dim>::inverse(TransformTraits traits) const
{
  if (traits == NonAffine)
  {
    return m_matrix.inverse();
  }
  else
  {
    MatrixType res;
    if (traits == GenericAffine)
    {
      res.template corner<Dim,Dim>(TopLeft) = linear().inverse();
    }
    else if (traits == NoShear)
    {
      // extract linear = rotation * scaling
      // then inv(linear) = inv(scaling) * inv(rotation)
      //                  = inv(scaling) * trans(rotation)
      //                  = inv(scaling) * trans(inv(scaling)) * trans(A)
      //                  = inv(scaling) * inv(scaling) * trans(A)
      //                  = inv(scaling)^2 * trans(A)
      //                  = scaling^-2 * trans(A)
      // with scaling[i] = A.col(i).norm()
      VectorType invScaling2 = linear().colwise().norm2().cwise().inverse();
      res.template corner<Dim,Dim>(TopLeft) = (invScaling2.asDiagonal() * linear().transpose()).lazy();
    }
    else if (traits == NoScaling)
    {
      res.template corner<Dim,Dim>(TopLeft) = linear().transpose();
    }
    else
    {
      ei_assert("invalid traits value in Transform::inverse()");
    }
    // translation and remaining parts
    res.template corner<Dim,1>(TopRight) = - res.template corner<Dim,Dim>(TopLeft) * translation();
    res.template corner<1,Dim>(BottomLeft).setZero();
    res.coeffRef(Dim,Dim) = Scalar(1);
    return res;
  }
}

/*****************************************************
*** Specializations of operator* with a MatrixBase ***
*****************************************************/

template<typename Other, int Dim, int HDim>
struct ei_transform_product_impl<Other,Dim,HDim, HDim,HDim>
{
  typedef Transform<typename Other::Scalar,Dim> TransformType;
  typedef typename TransformType::MatrixType MatrixType;
  typedef typename ProductReturnType<MatrixType,Other>::Type ResultType;
  static ResultType run(const TransformType& tr, const Other& other)
  { return tr.matrix() * other; }
};

template<typename Other, int Dim, int HDim>
struct ei_transform_product_impl<Other,Dim,HDim, Dim,Dim>
{
  typedef Transform<typename Other::Scalar,Dim> TransformType;
  typedef typename TransformType::MatrixType MatrixType;
  typedef TransformType ResultType;
  static ResultType run(const TransformType& tr, const Other& other)
  {
    TransformType res;
    res.translation() = tr.translation();
    res.matrix().row(Dim) = tr.matrix().row(Dim);
    res.linear() = (tr.linear() * other).lazy();
    return res;
  }
};

template<typename Other, int Dim, int HDim>
struct ei_transform_product_impl<Other,Dim,HDim, HDim,1>
{
  typedef Transform<typename Other::Scalar,Dim> TransformType;
  typedef typename TransformType::MatrixType MatrixType;
  typedef typename ProductReturnType<MatrixType,Other>::Type ResultType;
  static ResultType run(const TransformType& tr, const Other& other)
  { return tr.matrix() * other; }
};

template<typename Other, int Dim, int HDim>
struct ei_transform_product_impl<Other,Dim,HDim, Dim,1>
{
  typedef typename Other::Scalar Scalar;
  typedef Transform<Scalar,Dim> TransformType;
  typedef typename TransformType::LinearPart MatrixType;
  typedef const CwiseUnaryOp<
      ei_scalar_multiple_op<Scalar>,
      NestByValue<CwiseBinaryOp<
        ei_scalar_sum_op<Scalar>,
        NestByValue<typename ProductReturnType<NestByValue<MatrixType>,Other>::Type >,
        NestByValue<typename TransformType::TranslationPart> > >
      > ResultType;
  // FIXME should we offer an optimized version when the last row is known to be 0,0...,0,1 ?
  static ResultType run(const TransformType& tr, const Other& other)
  { return ((tr.linear().nestByValue() * other).nestByValue() + tr.translation().nestByValue()).nestByValue()
          * (Scalar(1) / ( (tr.matrix().template block<1,Dim>(Dim,0) * other).coeff(0) + tr.matrix().coeff(Dim,Dim))); }
};

#endif // EIGEN_TRANSFORM_H
