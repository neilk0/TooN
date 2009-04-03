/*                       
	 Copyright (C) 2005,2009 Tom Drummond, G. Reitmayr

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc.
     51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#ifndef TOON_INCLUDE_SL_H
#define TOON_INCLUDE_SL_H

#include <TooN/TooN.h>
#include <TooN/helpers.h>
#include <TooN/LU.h>

namespace TooN {

template <int N, typename P> class SL;
template <int N, typename P> std::istream & operator>>(std::istream &, SL<N, P> &);

/// represents an element from the group SL(n), the nxn matrices M with det(M) = 1.
/// This can be used to conveniently estimate homographies on n-1 dimentional spaces.
/// The implementation uses the matrix exponential function @ref exp for
/// exponentiation from an element in the Lie algebra and LU to compute an inverse.
/// 
/// The Lie algebra are the nxn matrices M with trace(M) = 0. The generators used
/// to represent this vector space are the following:
/// @item n-1 diag(...,1,-1,...), along the diagonal
/// @item symmetric generators for every pair of off-diagonal elements
/// @item anti-symmetric generators for every pair of off-diagonal elements
/// This choice represents the fact that SL(n) can be interpreted as the product
/// of all symmetric matrices with det() = 1 times SO(n).
template <int N, typename Precision = double>
class SL {
	friend std::istream & operator>> <N,Precision>(std::istream &, SL &);
public:
	static const int size = N;
	static const int dim = N*N - 1;

	SL() : my_matrix(Identity) {}
	template <int S, typename P, typename B>
	SL( const Vector<S,P,B> & v ) { *this = exp(v); }

	template <int R, int C, typename P, typename A>
	SL(Matrix<R,C,P,A>& M) : my_matrix(M) {
		coerce(M);
	}

	const Matrix<N,N,Precision> & get_matrix() const { return my_matrix; }
	SL inverse() const { return SL(*this, Invert()); }

	SL operator*( const SL & rhs) const { return SL(*this, rhs); }
	SL operator*=( const SL & rhs) { *this = *this*rhs; return *this; }

	template <int S, typename P, typename B>
	static inline SL exp( const Vector<S,P,B> &);

	static inline Matrix<N,N,Precision> generator(int);

	template <int R, int C, typename P, typename A>
	static void coerce(Matrix<R,C,P,A>& M){
		using std::abs;
		SizeMismatch<N,R>::test(N, M.num_rows());
		SizeMismatch<N,C>::test(N, M.num_cols());
		P det = LU<N>(M).determinant();
		assert(abs(det) > 0);
		M /= det;
	}

private:
	struct Invert {};
	SL( const SL & from, struct Invert ) : my_matrix(LU<N>(from.get_matrix()).get_inverse()) {}
	SL( const SL & a, const SL & b) : my_matrix(a.get_matrix() * b.get_matrix()) {}

	/// these constants indicate which parts of the parameter vector 
	/// map to which generators
	///{
	static const int COUNT_DIAG = N - 1;
	static const int COUNT_SYMM = (dim - COUNT_DIAG)/2;
	static const int COUNT_ASYMM = COUNT_SYMM;
	static const int DIAG_LIMIT = COUNT_DIAG;
	static const int SYMM_LIMIT = COUNT_SYMM + DIAG_LIMIT;
	///}

	Matrix<N,N,Precision> my_matrix;
};

template <int N, typename Precision>
template <int S, typename P, typename B>
inline SL<N, Precision> SL<N, Precision>::exp( const Vector<S,P,B> & v){
	SizeMismatch<S,dim>::test(v.size(), dim);
	Matrix<N,N,Precision> t = Zero;
	for(int i = 0; i < dim; ++i)
		t += generator(i) * v[i];
	SL<N, Precision> result;
	result.my_matrix = TooN::exp(t);
	return result;
}

template <int N, typename Precision>
inline Matrix<N,N,Precision> SL<N, Precision>::generator(int i){
	assert( i > -1 && i < dim );
	Matrix<N,N,Precision> result = Zero;
	if(i < DIAG_LIMIT) { 				// first ones are the diagonal ones
		result(i,i) = 1;
		result(i+1,i+1) = -1;
	} else if(i < SYMM_LIMIT){			// then the symmetric ones
		int row = 0, col = i - DIAG_LIMIT + 1;
		while(col > (N - row - 1)){
			col -= (N - row - 1); 
			++row;
		}
		col += row;
		result(row, col) = result(col, row) = 1;
	} else {							// finally the antisymmetric ones
		int row = 0, col = i - SYMM_LIMIT + 1;
		while(col > N - row - 1){
			col -= N - row - 1; 
			++row;
		}
		col += row;
		result(row, col) = -1;
		result(col, row) = 1;
	}
	return result;
}

template <int S, typename PV, typename B, int N, typename P>
Vector<N, P> operator*( const SL<N, P> & lhs, const Vector<S,PV,B> & rhs ){
	return lhs.get_matrix() * rhs;
}

template <int S, typename PV, typename B, int N, typename P>
Vector<N, P> operator*( const Vector<S,PV,B> & lhs, const SL<N,P> & rhs ){
	return lhs * rhs.get_matrix();
}

template <int N, typename P>
std::ostream & operator<<(std::ostream & out, const SL<N, P> & h){
	out << h.get_matrix();
	return out;
}

template <int N, typename P>
std::istream & operator>>(std::istream & in, SL<N, P> & h){
	in >> h.my_matrix;
	SL<N,P>::coerce(h.my_matrix);
	return in;
}

};

#endif
