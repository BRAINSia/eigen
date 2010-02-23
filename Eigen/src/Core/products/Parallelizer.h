// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2010 Gael Guennebaud <g.gael@free.fr>
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

#ifndef EIGEN_PARALLELIZER_H
#define EIGEN_PARALLELIZER_H

template<bool Parallelize,typename Functor>
void ei_run_parallel_1d(const Functor& func, int size)
{
#ifndef EIGEN_HAS_OPENMP
  func(0,size);
#else
  if(!Parallelize)
    return func(0,size);

  int threads = omp_get_num_procs();
  int blockSize = size / threads;
  #pragma omp parallel for schedule(static,1)
  for(int i=0; i<threads; ++i)
  {
    int blockStart = i*blockSize;
    int actualBlockSize = std::min(blockSize, size - blockStart);

    func(blockStart, actualBlockSize);
  }
#endif
}

template<bool Parallelize,typename Functor>
void ei_run_parallel_2d(const Functor& func, int size1, int size2)
{
#ifndef EIGEN_HAS_OPENMP
  func(0,size1, 0,size2);
#else
  if(!Parallelize)
    return func(0,size1, 0,size2);

                                // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
  static const int divide1[17] = { 0, 1, 2, 3, 2, 5, 3, 7, 4, 3,  5, 11,  4, 13,  7,  5, 4};
  static const int divide2[17] = { 0, 1, 1, 1, 2, 1, 2, 1, 2, 3,  2,  1,  3,  1,  2,  3, 4};

  int threads = omp_get_num_procs();
  ei_assert(threads<=16 && "too many threads !");
  int blockSize1 = size1 / divide1[threads];
  int blockSize2 = size2 / divide2[threads];
  
  Matrix<int,4,Dynamic> ranges(4,threads);
  int k = 0;
  for(int i1=0; i1<divide1[threads]; ++i1)
  {
    int blockStart1 = i1*blockSize1;
    int actualBlockSize1 = std::min(blockSize1, size1 - blockStart1);
    for(int i2=0; i2<divide2[threads]; ++i2)
    {
      int blockStart2 = i2*blockSize2;
      int actualBlockSize2 = std::min(blockSize2, size2 - blockStart2);
      ranges.col(k++) << blockStart1, actualBlockSize1, blockStart2, actualBlockSize2;
    }
  }
  
  #pragma omp parallel for schedule(static,1)
  for(int i=0; i<threads; ++i)
  {
    func(ranges.col(i)[0],ranges.col(i)[1],ranges.col(i)[2],ranges.col(i)[3]);
  }
#endif
}

#endif // EIGEN_GENERAL_MATRIX_MATRIX_H
