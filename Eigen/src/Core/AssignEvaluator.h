// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2011 Benoit Jacob <jacob.benoit.1@gmail.com>
// Copyright (C) 2011 Gael Guennebaud <gael.guennebaud@inria.fr>
// Copyright (C) 2011 Jitse Niesen <jitse@maths.leeds.ac.uk>
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

#ifndef EIGEN_ASSIGN_EVALUATOR_H
#define EIGEN_ASSIGN_EVALUATOR_H

// This implementation is based on Assign.h

namespace internal {
  
/***************************************************************************
* Part 1 : the logic deciding a strategy for traversal and unrolling       *
***************************************************************************/

// copy_using_evaluator_traits is based on assign_traits
// (actually, it's identical)

template <typename Derived, typename OtherDerived>
struct copy_using_evaluator_traits
{
public:
  enum {
    DstIsAligned = Derived::Flags & AlignedBit,
    DstHasDirectAccess = Derived::Flags & DirectAccessBit,
    SrcIsAligned = OtherDerived::Flags & AlignedBit,
    JointAlignment = bool(DstIsAligned) && bool(SrcIsAligned) ? Aligned : Unaligned
  };

private:
  enum {
    InnerSize = int(Derived::IsVectorAtCompileTime) ? int(Derived::SizeAtCompileTime)
              : int(Derived::Flags)&RowMajorBit ? int(Derived::ColsAtCompileTime)
              : int(Derived::RowsAtCompileTime),
    InnerMaxSize = int(Derived::IsVectorAtCompileTime) ? int(Derived::MaxSizeAtCompileTime)
              : int(Derived::Flags)&RowMajorBit ? int(Derived::MaxColsAtCompileTime)
              : int(Derived::MaxRowsAtCompileTime),
    MaxSizeAtCompileTime = Derived::SizeAtCompileTime,
    PacketSize = packet_traits<typename Derived::Scalar>::size
  };

  enum {
    StorageOrdersAgree = (int(Derived::IsRowMajor) == int(OtherDerived::IsRowMajor)),
    MightVectorize = StorageOrdersAgree
                  && (int(Derived::Flags) & int(OtherDerived::Flags) & ActualPacketAccessBit),
    MayInnerVectorize  = MightVectorize && int(InnerSize)!=Dynamic && int(InnerSize)%int(PacketSize)==0
                       && int(DstIsAligned) && int(SrcIsAligned),
    MayLinearize = StorageOrdersAgree && (int(Derived::Flags) & int(OtherDerived::Flags) & LinearAccessBit),
    MayLinearVectorize = MightVectorize && MayLinearize && DstHasDirectAccess
                       && (DstIsAligned || MaxSizeAtCompileTime == Dynamic),
      /* If the destination isn't aligned, we have to do runtime checks and we don't unroll,
         so it's only good for large enough sizes. */
    MaySliceVectorize  = MightVectorize && DstHasDirectAccess
                       && (int(InnerMaxSize)==Dynamic || int(InnerMaxSize)>=3*PacketSize)
      /* slice vectorization can be slow, so we only want it if the slices are big, which is
         indicated by InnerMaxSize rather than InnerSize, think of the case of a dynamic block
         in a fixed-size matrix */
  };

public:
  enum {
    Traversal = int(MayInnerVectorize)  ? int(InnerVectorizedTraversal)
              : int(MayLinearVectorize) ? int(LinearVectorizedTraversal)
              : int(MaySliceVectorize)  ? int(SliceVectorizedTraversal)
              : int(MayLinearize)       ? int(LinearTraversal)
                                        : int(DefaultTraversal),
    Vectorized = int(Traversal) == InnerVectorizedTraversal
              || int(Traversal) == LinearVectorizedTraversal
              || int(Traversal) == SliceVectorizedTraversal
  };

private:
  enum {
    UnrollingLimit      = EIGEN_UNROLLING_LIMIT * (Vectorized ? int(PacketSize) : 1),
    MayUnrollCompletely = int(Derived::SizeAtCompileTime) != Dynamic
                       && int(OtherDerived::CoeffReadCost) != Dynamic
                       && int(Derived::SizeAtCompileTime) * int(OtherDerived::CoeffReadCost) <= int(UnrollingLimit),
    MayUnrollInner      = int(InnerSize) != Dynamic
                       && int(OtherDerived::CoeffReadCost) != Dynamic
                       && int(InnerSize) * int(OtherDerived::CoeffReadCost) <= int(UnrollingLimit)
  };

public:
  enum {
    Unrolling = (int(Traversal) == int(InnerVectorizedTraversal) || int(Traversal) == int(DefaultTraversal))
                ? (
 		    int(MayUnrollCompletely) ? int(CompleteUnrolling)
                  : int(MayUnrollInner)      ? int(InnerUnrolling)
                                             : int(NoUnrolling)
                  )
              : int(Traversal) == int(LinearVectorizedTraversal)
                ? ( bool(MayUnrollCompletely) && bool(DstIsAligned) ? int(CompleteUnrolling) 
                                                                    : int(NoUnrolling) )
              : int(Traversal) == int(LinearTraversal)
                ? ( bool(MayUnrollCompletely) ? int(CompleteUnrolling) 
                                              : int(NoUnrolling) )
              : int(NoUnrolling)
  };

#ifdef EIGEN_DEBUG_ASSIGN
  static void debug()
  {
    EIGEN_DEBUG_VAR(DstIsAligned)
    EIGEN_DEBUG_VAR(SrcIsAligned)
    EIGEN_DEBUG_VAR(JointAlignment)
    EIGEN_DEBUG_VAR(InnerSize)
    EIGEN_DEBUG_VAR(InnerMaxSize)
    EIGEN_DEBUG_VAR(PacketSize)
    EIGEN_DEBUG_VAR(StorageOrdersAgree)
    EIGEN_DEBUG_VAR(MightVectorize)
    EIGEN_DEBUG_VAR(MayLinearize)
    EIGEN_DEBUG_VAR(MayInnerVectorize)
    EIGEN_DEBUG_VAR(MayLinearVectorize)
    EIGEN_DEBUG_VAR(MaySliceVectorize)
    EIGEN_DEBUG_VAR(Traversal)
    EIGEN_DEBUG_VAR(UnrollingLimit)
    EIGEN_DEBUG_VAR(MayUnrollCompletely)
    EIGEN_DEBUG_VAR(MayUnrollInner)
    EIGEN_DEBUG_VAR(Unrolling)
  }
#endif
};

/***************************************************************************
* Part 2 : meta-unrollers
***************************************************************************/

// TODO:`Ideally, we want to use only the evaluator objects here, not the expression objects
//       However, we need to access .rowIndexByOuterInner() which is in the expression object

/************************
*** Default traversal ***
************************/

template<typename DstXprType, typename SrcXprType, int Index, int Stop>
struct copy_using_evaluator_DefaultTraversal_CompleteUnrolling
{
  enum {
    outer = Index / DstXprType::InnerSizeAtCompileTime,
    inner = Index % DstXprType::InnerSizeAtCompileTime
  };

  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

  EIGEN_STRONG_INLINE static void run(DstEvaluatorType &dstEvaluator, 
				      SrcEvaluatorType &srcEvaluator, 
				      const DstXprType &dst)
  {
    // TODO: Use copyCoeffByOuterInner ?
    typename DstXprType::Index row = dst.rowIndexByOuterInner(outer, inner);
    typename DstXprType::Index col = dst.colIndexByOuterInner(outer, inner);
    dstEvaluator.coeffRef(row, col) = srcEvaluator.coeff(row, col);
    copy_using_evaluator_DefaultTraversal_CompleteUnrolling<DstXprType, SrcXprType, Index+1, Stop>
      ::run(dstEvaluator, srcEvaluator, dst);
  }
};

template<typename DstXprType, typename SrcXprType, int Stop>
struct copy_using_evaluator_DefaultTraversal_CompleteUnrolling<DstXprType, SrcXprType, Stop, Stop>
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
  EIGEN_STRONG_INLINE static void run(DstEvaluatorType&, SrcEvaluatorType&, const DstXprType&) { }
};

template<typename DstXprType, typename SrcXprType, int Index, int Stop>
struct copy_using_evaluator_DefaultTraversal_InnerUnrolling
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

  EIGEN_STRONG_INLINE static void run(DstEvaluatorType &dstEvaluator, 
				      SrcEvaluatorType &srcEvaluator, 
				      const DstXprType &dst,
				      int outer)
  {
    // TODO: Use copyCoeffByOuterInner ?
    typename DstXprType::Index row = dst.rowIndexByOuterInner(outer, Index);
    typename DstXprType::Index col = dst.colIndexByOuterInner(outer, Index);
    dstEvaluator.coeffRef(row, col) = srcEvaluator.coeff(row, col);
    copy_using_evaluator_DefaultTraversal_InnerUnrolling<DstXprType, SrcXprType, Index+1, Stop>
      ::run(dstEvaluator, srcEvaluator, dst, outer);
  }
};

template<typename DstXprType, typename SrcXprType, int Stop>
struct copy_using_evaluator_DefaultTraversal_InnerUnrolling<DstXprType, SrcXprType, Stop, Stop>
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
  EIGEN_STRONG_INLINE static void run(DstEvaluatorType&, SrcEvaluatorType&, const DstXprType&, int) { }
};

/***********************
*** Linear traversal ***
***********************/

template<typename DstXprType, typename SrcXprType, int Index, int Stop>
struct copy_using_evaluator_LinearTraversal_CompleteUnrolling
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

  EIGEN_STRONG_INLINE static void run(DstEvaluatorType &dstEvaluator, 
				      SrcEvaluatorType &srcEvaluator, 
				      const DstXprType &dst)
  {
    // use copyCoeff ?
    dstEvaluator.coeffRef(Index) = srcEvaluator.coeff(Index);
    copy_using_evaluator_LinearTraversal_CompleteUnrolling<DstXprType, SrcXprType, Index+1, Stop>
      ::run(dstEvaluator, srcEvaluator, dst);
  }
};

template<typename DstXprType, typename SrcXprType, int Stop>
struct copy_using_evaluator_LinearTraversal_CompleteUnrolling<DstXprType, SrcXprType, Stop, Stop>
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
  EIGEN_STRONG_INLINE static void run(DstEvaluatorType&, SrcEvaluatorType&, const DstXprType&) { }
};

/**************************
*** Inner vectorization ***
**************************/

template<typename DstXprType, typename SrcXprType, int Index, int Stop>
struct copy_using_evaluator_innervec_CompleteUnrolling
{
  enum {
    outer = Index / DstXprType::InnerSizeAtCompileTime,
    inner = Index % DstXprType::InnerSizeAtCompileTime,
    JointAlignment = copy_using_evaluator_traits<DstXprType,SrcXprType>::JointAlignment
  };

  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

  EIGEN_STRONG_INLINE static void run(DstEvaluatorType &dstEvaluator, 
				      SrcEvaluatorType &srcEvaluator, 
				      const DstXprType &dst)
  {
    // TODO: Use copyPacketByOuterInner ?
    typename DstXprType::Index row = dst.rowIndexByOuterInner(outer, inner);
    typename DstXprType::Index col = dst.colIndexByOuterInner(outer, inner);
    dstEvaluator.template writePacket<Aligned>(row, col, srcEvaluator.template packet<JointAlignment>(row, col));
    copy_using_evaluator_innervec_CompleteUnrolling<DstXprType, SrcXprType,
      Index+packet_traits<typename DstXprType::Scalar>::size, Stop>::run(dstEvaluator, srcEvaluator, dst);
  }
};

template<typename DstXprType, typename SrcXprType, int Stop>
struct copy_using_evaluator_innervec_CompleteUnrolling<DstXprType, SrcXprType, Stop, Stop>
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
  EIGEN_STRONG_INLINE static void run(DstEvaluatorType&, SrcEvaluatorType&, const DstXprType&) { }
};

template<typename DstXprType, typename SrcXprType, int Index, int Stop>
struct copy_using_evaluator_innervec_InnerUnrolling
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

  EIGEN_STRONG_INLINE static void run(DstEvaluatorType &dstEvaluator, 
				      SrcEvaluatorType &srcEvaluator, 
				      const DstXprType &dst,
				      int outer)
  {
    // TODO: Use copyPacketByOuterInner ?
    typename DstXprType::Index row = dst.rowIndexByOuterInner(outer, Index);
    typename DstXprType::Index col = dst.colIndexByOuterInner(outer, Index);
    dstEvaluator.template writePacket<Aligned>(row, col, srcEvaluator.template packet<Aligned>(row, col));
    copy_using_evaluator_innervec_InnerUnrolling<DstXprType, SrcXprType,
      Index+packet_traits<typename DstXprType::Scalar>::size, Stop>::run(dstEvaluator, srcEvaluator, dst, outer);
  }
};

template<typename DstXprType, typename SrcXprType, int Stop>
struct copy_using_evaluator_innervec_InnerUnrolling<DstXprType, SrcXprType, Stop, Stop>
{
  typedef typename evaluator<DstXprType>::type DstEvaluatorType;
  typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
  EIGEN_STRONG_INLINE static void run(DstEvaluatorType&, SrcEvaluatorType&, const DstXprType&, int) { }
};

/***************************************************************************
* Part 3 : implementation of all cases
***************************************************************************/

// copy_using_evaluator_impl is based on assign_impl

template<typename DstXprType, typename SrcXprType,
         int Traversal = copy_using_evaluator_traits<DstXprType, SrcXprType>::Traversal,
         int Unrolling = copy_using_evaluator_traits<DstXprType, SrcXprType>::Unrolling>
struct copy_using_evaluator_impl;

/************************
*** Default traversal ***
************************/

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, DefaultTraversal, NoUnrolling>
{
  static void run(const DstXprType& dst, const SrcXprType& src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
    typedef typename DstXprType::Index Index;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    for(Index outer = 0; outer < dst.outerSize(); ++outer) {
      for(Index inner = 0; inner < dst.innerSize(); ++inner) {
	Index row = dst.rowIndexByOuterInner(outer, inner);
	Index col = dst.colIndexByOuterInner(outer, inner);
	dstEvaluator.coeffRef(row, col) = srcEvaluator.coeff(row, col); // TODO: use copyCoeff ?
      }
    }
  }
};

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, DefaultTraversal, CompleteUnrolling>
{
  EIGEN_STRONG_INLINE static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    copy_using_evaluator_DefaultTraversal_CompleteUnrolling<DstXprType, SrcXprType, 0, DstXprType::SizeAtCompileTime>
      ::run(dstEvaluator, srcEvaluator, dst);
  }
};

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, DefaultTraversal, InnerUnrolling>
{
  typedef typename DstXprType::Index Index;
  EIGEN_STRONG_INLINE static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    const Index outerSize = dst.outerSize();
    for(Index outer = 0; outer < outerSize; ++outer)
      copy_using_evaluator_DefaultTraversal_InnerUnrolling<DstXprType, SrcXprType, 0, DstXprType::InnerSizeAtCompileTime>
        ::run(dstEvaluator, srcEvaluator, dst, outer);
  }
};

/***************************
*** Linear vectorization ***
***************************/

template <bool IsAligned = false>
struct unaligned_copy_using_evaluator_impl
{
  template <typename SrcEvaluatorType, typename DstEvaluatorType>
  static EIGEN_STRONG_INLINE void run(const SrcEvaluatorType&, DstEvaluatorType&, 
				      typename SrcEvaluatorType::Index, typename SrcEvaluatorType::Index) {}
};

// TODO: check why no ...<true> ????

template <>
struct unaligned_copy_using_evaluator_impl<false>
{
  // MSVC must not inline this functions. If it does, it fails to optimize the
  // packet access path.
#ifdef _MSC_VER
  template <typename SrcEvaluatorType, typename DstEvaluatorType>
  static EIGEN_DONT_INLINE void run(const SrcEvaluatorType& src, DstEvaluatorType& dst, 
				    typename SrcEvaluatorType::Index start, typename SrcEvaluatorType::Index end)
#else
  template <typename SrcEvaluatorType, typename DstEvaluatorType>
  static EIGEN_STRONG_INLINE void run(const SrcEvaluatorType& src, DstEvaluatorType& dst, 
				      typename SrcEvaluatorType::Index start, typename SrcEvaluatorType::Index end)
#endif
  {
    for (typename SrcEvaluatorType::Index index = start; index < end; ++index)
      dst.copyCoeff(index, src);
  }
};

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, LinearVectorizedTraversal, NoUnrolling>
{
  EIGEN_STRONG_INLINE static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
    typedef typename DstXprType::Index Index;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    const Index size = dst.size();
    typedef packet_traits<typename DstXprType::Scalar> PacketTraits;
    enum {
      packetSize = PacketTraits::size,
      dstIsAligned = int(copy_using_evaluator_traits<DstXprType,SrcXprType>::DstIsAligned),
      dstAlignment = PacketTraits::AlignedOnScalar ? Aligned : dstIsAligned,
      srcAlignment = copy_using_evaluator_traits<DstXprType,SrcXprType>::JointAlignment
    };
    const Index alignedStart = dstIsAligned ? 0 : first_aligned(&dst.coeffRef(0), size);
    const Index alignedEnd = alignedStart + ((size-alignedStart)/packetSize)*packetSize;

    unaligned_copy_using_evaluator_impl<dstIsAligned!=0>::run(src,dst.const_cast_derived(),0,alignedStart);

    for(Index index = alignedStart; index < alignedEnd; index += packetSize)
    {
      dstEvaluator.template writePacket<dstAlignment>(index, srcEvaluator.template packet<srcAlignment>(index));
    }

    unaligned_copy_using_evaluator_impl<>::run(src,dst.const_cast_derived(),alignedEnd,size);
  }
};

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, LinearVectorizedTraversal, CompleteUnrolling>
{
  typedef typename DstXprType::Index Index;
  EIGEN_STRONG_INLINE static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    enum { size = DstXprType::SizeAtCompileTime,
           packetSize = packet_traits<typename DstXprType::Scalar>::size,
           alignedSize = (size/packetSize)*packetSize };

    copy_using_evaluator_innervec_CompleteUnrolling<DstXprType, SrcXprType, 0, alignedSize>
      ::run(dstEvaluator, srcEvaluator, dst);
    copy_using_evaluator_DefaultTraversal_CompleteUnrolling<DstXprType, SrcXprType, alignedSize, size>
      ::run(dstEvaluator, srcEvaluator, dst);
  }
};

/**************************
*** Inner vectorization ***
**************************/

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, InnerVectorizedTraversal, NoUnrolling>
{
  inline static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
    typedef typename DstXprType::Index Index;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    const Index innerSize = dst.innerSize();
    const Index outerSize = dst.outerSize();
    const Index packetSize = packet_traits<typename DstXprType::Scalar>::size;
    for(Index outer = 0; outer < outerSize; ++outer)
      for(Index inner = 0; inner < innerSize; inner+=packetSize) {
	// TODO: Use copyPacketByOuterInner ?
	Index row = dst.rowIndexByOuterInner(outer, inner);
	Index col = dst.colIndexByOuterInner(outer, inner);
	dstEvaluator.template writePacket<Aligned>(row, col, srcEvaluator.template packet<Aligned>(row, col));
      }
  }
};

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, InnerVectorizedTraversal, CompleteUnrolling>
{
  EIGEN_STRONG_INLINE static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    copy_using_evaluator_innervec_CompleteUnrolling<DstXprType, SrcXprType, 0, DstXprType::SizeAtCompileTime>
      ::run(dstEvaluator, srcEvaluator, dst);
  }
};

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, InnerVectorizedTraversal, InnerUnrolling>
{
  typedef typename DstXprType::Index Index;
  EIGEN_STRONG_INLINE static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    const Index outerSize = dst.outerSize();
    for(Index outer = 0; outer < outerSize; ++outer)
      copy_using_evaluator_innervec_InnerUnrolling<DstXprType, SrcXprType, 0, DstXprType::InnerSizeAtCompileTime>
        ::run(dstEvaluator, srcEvaluator, dst, outer);
  }
};

/***********************
*** Linear traversal ***
***********************/

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, LinearTraversal, NoUnrolling>
{
  inline static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
    typedef typename DstXprType::Index Index;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    const Index size = dst.size();
    for(Index i = 0; i < size; ++i)
      dstEvaluator.coeffRef(i) = srcEvaluator.coeff(i); // TODO: use copyCoeff ?
  }
};

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, LinearTraversal, CompleteUnrolling>
{
  EIGEN_STRONG_INLINE static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    copy_using_evaluator_LinearTraversal_CompleteUnrolling<DstXprType, SrcXprType, 0, DstXprType::SizeAtCompileTime>
      ::run(dstEvaluator, srcEvaluator, dst);
  }
};

/**************************
*** Slice vectorization ***
***************************/

template<typename DstXprType, typename SrcXprType>
struct copy_using_evaluator_impl<DstXprType, SrcXprType, SliceVectorizedTraversal, NoUnrolling>
{
  inline static void run(const DstXprType &dst, const SrcXprType &src)
  {
    typedef typename evaluator<DstXprType>::type DstEvaluatorType;
    typedef typename evaluator<SrcXprType>::type SrcEvaluatorType;
    typedef typename DstXprType::Index Index;

    DstEvaluatorType dstEvaluator(dst.const_cast_derived());
    SrcEvaluatorType srcEvaluator(src);

    typedef packet_traits<typename DstXprType::Scalar> PacketTraits;
    enum {
      packetSize = PacketTraits::size,
      alignable = PacketTraits::AlignedOnScalar,
      dstAlignment = alignable ? Aligned : int(copy_using_evaluator_traits<DstXprType,SrcXprType>::DstIsAligned) ,
      srcAlignment = copy_using_evaluator_traits<DstXprType,SrcXprType>::JointAlignment
    };
    const Index packetAlignedMask = packetSize - 1;
    const Index innerSize = dst.innerSize();
    const Index outerSize = dst.outerSize();
    const Index alignedStep = alignable ? (packetSize - dst.outerStride() % packetSize) & packetAlignedMask : 0;
    Index alignedStart = ((!alignable) || copy_using_evaluator_traits<DstXprType,SrcXprType>::DstIsAligned) ? 0
                       : first_aligned(&dstEvaluator.coeffRef(0,0), innerSize);

    for(Index outer = 0; outer < outerSize; ++outer)
    {
      const Index alignedEnd = alignedStart + ((innerSize-alignedStart) & ~packetAlignedMask);
      // do the non-vectorizable part of the assignment
      for(Index inner = 0; inner<alignedStart ; ++inner) {
	Index row = dst.rowIndexByOuterInner(outer, inner);
	Index col = dst.colIndexByOuterInner(outer, inner);
        dstEvaluator.coeffRef(row, col) = srcEvaluator.coeff(row, col);
      }

      // do the vectorizable part of the assignment
      for(Index inner = alignedStart; inner<alignedEnd; inner+=packetSize) {
	Index row = dst.rowIndexByOuterInner(outer, inner);
	Index col = dst.colIndexByOuterInner(outer, inner);
        dstEvaluator.template writePacket<dstAlignment>(row, col, srcEvaluator.template packet<srcAlignment>(row, col));
      }

      // do the non-vectorizable part of the assignment
      for(Index inner = alignedEnd; inner<innerSize ; ++inner) {
	Index row = dst.rowIndexByOuterInner(outer, inner);
	Index col = dst.colIndexByOuterInner(outer, inner);
        dstEvaluator.coeffRef(row, col) = srcEvaluator.coeff(row, col);
      }

      alignedStart = std::min<Index>((alignedStart+alignedStep)%packetSize, innerSize);
    }
  }
};

/***************************************************************************
* Part 4 : Entry points
***************************************************************************/

// Based on DenseBase::LazyAssign()

template<typename DstXprType, typename SrcXprType>
const DstXprType& copy_using_evaluator(const DstXprType& dst, const SrcXprType& src)
{
#ifdef EIGEN_DEBUG_ASSIGN
  internal::copy_using_evaluator_traits<DstXprType, SrcXprType>::debug();
#endif
  copy_using_evaluator_impl<DstXprType, SrcXprType>::run(dst, src);
  return dst;
}

} // namespace internal

#endif // EIGEN_ASSIGN_EVALUATOR_H
