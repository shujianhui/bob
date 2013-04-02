/**
 * @file bob/math/lu.h
 * @date Fri Jan 27 14:10:23 2012 +0100
 * @author Laurent El Shafey <Laurent.El-Shafey@idiap.ch>
 *
 * Copyright (C) 2011-2013 Idiap Research Institute, Martigny, Switzerland
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BOB_MATH_LU_H
#define BOB_MATH_LU_H

#include <blitz/array.h>

namespace bob { namespace math {
/**
 * @ingroup MATH
 * @{
 *
 */

  /**
   * @brief Function which performs a LU decomposition of a real
   *   matrix, using the dgetrf LAPACK function: \f$A = P*L*U\f$
   * @param A The A matrix to decompose (size MxN)
   * @param L The L lower-triangular matrix of the decomposition (size Mxmin(M,N)) 
   * @param U The U upper-triangular matrix of the decomposition (size min(M,N)xN) 
   * @param P The P permutation matrix of the decomposition (size min(M,N)xmin(M,N))
   */
  void lu(const blitz::Array<double,2>& A, blitz::Array<double,2>& L,
    blitz::Array<double,2>& U, blitz::Array<double,2>& P);
  void lu_(const blitz::Array<double,2>& A, blitz::Array<double,2>& L,
    blitz::Array<double,2>& U, blitz::Array<double,2>& P);

  /**
   * @brief Performs the Cholesky decomposition of a real symmetric 
   *   positive-definite matrix into the product of a lower triangular matrix
   *   and its transpose. When it is applicable, this is much more efficient
   *   than the LU decomposition. It uses the dpotrf LAPACK function: 
   *     \f$A = L*L^{T}\f$
   * @param A The A matrix to decompose (size NxN)
   * @param L The L lower-triangular matrix of the decomposition
   */
  void chol(const blitz::Array<double,2>& A, blitz::Array<double,2>& L);
  void chol_(const blitz::Array<double,2>& A, blitz::Array<double,2>& L);
/**
 * @}
 */
}}

#endif /* BOB_MATH_LU_H */
