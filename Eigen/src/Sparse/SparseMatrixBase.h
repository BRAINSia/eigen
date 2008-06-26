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

#ifndef EIGEN_SPARSEMATRIXBASE_H
#define EIGEN_SPARSEMATRIXBASE_H

template<typename Derived>
class SparseMatrixBase : public MatrixBase<Derived>
{
  public:

    typedef MatrixBase<Derived> Base;
    typedef typename Base::Scalar Scalar;
    enum {
      Flags = Base::Flags,
      RowMajor = ei_traits<Derived>::Flags&RowMajorBit ? 1 : 0
    };

    inline const Derived& derived() const { return *static_cast<const Derived*>(this); }
    inline Derived& derived() { return *static_cast<Derived*>(this); }
    inline Derived& const_cast_derived() const
    { return *static_cast<Derived*>(const_cast<SparseMatrixBase*>(this)); }

    SparseMatrixBase()
      : m_isRValue(false), m_hasBeenCopied(false)
    {}

    bool isRValue() const { return m_isRValue; }
    Derived& temporary() { m_isRValue = true; return derived(); }

    int outerSize() const { return RowMajor ? this->rows() : this->cols(); }
    int innerSize() const { return RowMajor ? this->cols() : this->rows(); }

    inline Derived& operator=(const Derived& other)
    {
      if (other.isRValue())
      {
        m_hasBeenCopied = true;
        derived().shallowCopy(other);
      }
      else
        this->operator=<Derived>(other);
      return derived();
    }



    template<typename OtherDerived>
    inline Derived& operator=(const MatrixBase<OtherDerived>& other)
    {
      const bool transpose = (Flags & RowMajorBit) != (OtherDerived::Flags & RowMajorBit);
      const int outerSize = (int(OtherDerived::Flags) & RowMajorBit) ? other.rows() : other.cols();
      // eval to a temporary and then do a shallow copy
      typename ei_meta_if<transpose, LinkedVectorMatrix<Scalar,Flags&RowMajorBit>, Derived>::ret temp(other.rows(), other.cols());

      temp.startFill(std::max(this->rows(),this->cols())*2);
//       std::cout << other.rows() << " xm " << other.cols() << "\n";
      for (int j=0; j<outerSize; ++j)
      {
        for (typename OtherDerived::InnerIterator it(other.derived(), j); it; ++it)
        {
//           std::cout << other.rows() << " x " << other.cols() << "\n";
//           std::cout << it.m_matrix.rows() << "\n";
          Scalar v = it.value();
          if (v!=Scalar(0))
            if (RowMajor) temp.fill(j,it.index()) = v;
            else temp.fill(it.index(),j) = v;
        }
      }
      temp.endFill();
      derived() = temp.temporary();
      return derived();
    }

    template<typename OtherDerived>
    inline Derived& operator=(const SparseMatrixBase<OtherDerived>& other)
    {
      const bool transpose = (Flags & RowMajorBit) != (OtherDerived::Flags & RowMajorBit);
      const int outerSize = (int(OtherDerived::Flags) & RowMajorBit) ? other.rows() : other.cols();
      if ((!transpose) && other.isRValue())
      {
        // eval without temporary
        derived().resize(other.rows(), other.cols());
        derived().startFill(std::max(this->rows(),this->cols())*2);
        for (int j=0; j<outerSize; ++j)
        {
          for (typename OtherDerived::InnerIterator it(other.derived(), j); it; ++it)
          {
            Scalar v = it.value();
            if (v!=Scalar(0))
              if (RowMajor) derived().fill(j,it.index()) = v;
              else derived().fill(it.index(),j) = v;
          }
        }
        derived().endFill();
      }
      else
        this->operator=<OtherDerived>(static_cast<const MatrixBase<OtherDerived>&>(other));
      return derived();
    }

    friend std::ostream & operator << (std::ostream & s, const SparseMatrixBase& m)
    {

      if (Flags&RowMajorBit)
      {
        for (int row=0; row<m.rows(); ++row)
        {
          int col = 0;
          for (typename Derived::InnerIterator it(m.derived(), row); it; ++it)
          {
            for ( ; col<it.index(); ++col)
              s << "0 ";
            std::cout << it.value() << " ";
          }
          for ( ; col<m.cols(); ++col)
            s << "0 ";
          s << std::endl;
        }
      }
      else
      {
        LinkedVectorMatrix<Scalar, RowMajorBit> trans = m.derived();
        s << trans;
      }
      return s;
    }

  protected:

    bool isNotShared() { return !m_hasBeenCopied; }
    void markAsCopied() const { m_hasBeenCopied = true; }

  protected:

    bool m_isRValue;
    mutable bool m_hasBeenCopied;
};



#endif // EIGEN_SPARSEMATRIXBASE_H
