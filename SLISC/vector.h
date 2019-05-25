// vector container
#pragma once
#include "meta.h"

#ifndef NDEBUG
#define SLS_CHECK_BOUNDS
#define SLS_CHECK_SHAPE
#endif

namespace slisc {

// memory set and copy
template<class T>
inline void vecset(T *dest, const T &val, Long_I n)
{
	for (Long i = 0; i < n; ++i)
		dest[i] = val;
}

template<class T>
inline void veccpy(T *dest, const T *src, Long_I n)
{
	memcpy(dest, src, n * sizeof(T));
}

template<class T, class T1, SLS_IF(is_promo<T,T1>())>
inline void veccpy(T *dest, const T1 *src, Long_I n)
{
	for (Long i = 0; i < n; ++i)
		dest[i] = src[i];
}

template <class Tv, class Tv1, SLS_IF(
	is_dense<Tv>() && is_dense<Tv1>())>
inline void copy(Tv &v, const Tv1 &v1)
{
#ifdef SLS_CHECK_SHAPE
	if (!shape_cmp(v, v1))
		SLS_ERR("wrong shape!");
#endif
	veccpy(v.ptr(), v1.ptr(), v.size());
}

// Base Class for vector/matrix
template <class T>
class Vbase
{
protected:
	T *m_p; // pointer to the first element
	Long m_N; // number of elements
	Vbase(); // default constructor, everything is uninitialized
public:
	typedef T value_type;
	// constructors
	explicit Vbase(Long_I N);
	Vbase(const Vector<T> &rhs); // copy constructor

	// get properties
	T* ptr(); // get pointer
	const T* ptr() const;
	Long size() const;
	void resize(Long_I N);
	// resize and copy old data, new elements are uninitialized
	void resize_cpy(Long_I N);
	T & operator[](Long_I i);
	const T & operator[](Long_I i) const;
	T & operator()(Long_I i);
	const T & operator()(Long_I i) const;
	T& end();
	const T& end() const;
	T& end(Long_I i);
	const T& end(Long_I i) const;
	Vbase & operator=(const Vbase &rhs);
	template <class Tv, SLS_IF(is_dense_vec<Tv>())>
	Vbase & operator=(const Tv &rhs);
	Vbase & operator=(const T &rhs); // for scalar
	void operator<<(Vbase &rhs); // move data
	~Vbase();
};

template<class T>
inline Vbase<T>::Vbase() {}

template<class T>
inline Vbase<T>::Vbase(Long_I N) : m_N(N), m_p(new T[N]) {}

template <class T>
Vbase<T>::Vbase(const Vector<T> &rhs)
{
#ifndef SLS_ALLOW_COPY_CONSTRUCTOR
	SLS_ERR("Copy constructor or move constructor is forbidden, use reference "
		"argument for function input or output, and use \"=\" to copy!");
#endif
	m_N = rhs.m_N;
	m_p = new T[rhs.m_N];
	veccpy(m_p, rhs.ptr(), m_N);
}

template<class T>
inline T * Vbase<T>::ptr()
{
#ifdef SLS_CHECK_BOUNDS
	if (m_N == 0)
		SLS_ERR("using ptr() for empty container!");
#endif
	return m_p;
}

template<class T>
inline const T * Vbase<T>::ptr() const
{
	return m_p;
}

template<class T>
inline Long Vbase<T>::size() const
{
	return m_N;
}

template <class T>
inline void Vbase<T>::resize(Long_I N)
{
	if (N != m_N) {
		if (m_N == 0) {
			m_N = N; m_p = new T[N];
		}
		else {
			delete[] m_p;
			if (N == 0)
				m_N = 0;
			else {
				m_N = N;
				m_p = new T[N];
			}
		}
	}
}

template <class T>
inline void Vbase<T>::resize_cpy(Long_I N)
{
	if (N != m_N) {
		if (m_N == 0 || N == 0) {
			resize(N);
		}
		else {
			m_N = N;
			T *old_p = m_p;
			m_p = new T[N];
			veccpy(m_p, old_p, N);
			delete[] old_p;
		}
	}
}

template <class T>
inline void Vbase<T>::operator<<(Vbase &rhs)
{
	if (this == &rhs)
		SLS_ERR("self move is forbidden!");
	if (m_N != 0)
		delete[] m_p;
	m_N = rhs.m_N; rhs.m_N = 0;
	m_p = rhs.m_p;
}

template <class T>
inline T & Vbase<T>::operator[](Long_I i)
{
#ifdef SLS_CHECK_BOUNDS
if (i<0 || i>=m_N)
	SLS_ERR("Vbase subscript out of bounds");
#endif
	return m_p[i];
}

template <class T>
inline const T & Vbase<T>::operator[](Long_I i) const
{
#ifdef SLS_CHECK_BOUNDS
	if (i<0 || i>=m_N)
		SLS_ERR("Vbase subscript out of bounds");
#endif
	return m_p[i];
}

template <class T>
inline T & Vbase<T>::operator()(Long_I i)
{ return (*this)[i]; }

template <class T>
inline const T & Vbase<T>::operator()(Long_I i) const
{ return (*this)[i]; }

template <class T>
inline Vbase<T> & Vbase<T>::operator=(const Vbase<T> &rhs)
{
	veccpy(m_p, rhs.ptr(), m_N);
	return *this;
}

template <class T> template <class Tv, SLS_IF0(is_dense_vec<Tv>())>
inline Vbase<T> & Vbase<T>::operator=(const Tv &rhs)
{
	veccpy(m_p, rhs.ptr(), m_N);
	return *this;
}

template <class T>
inline Vbase<T> & Vbase<T>::operator=(const T &rhs)
{
	vecset(m_p, rhs, m_N);
	return *this;
}

template <class T>
inline T & Vbase<T>::end()
{
#ifdef SLS_CHECK_BOUNDS
	if (m_N == 0)
		SLS_ERR("tring to use end() on empty vector!");
#endif
	return m_p[m_N - 1];
}

template <class T>
inline const T & Vbase<T>::end() const
{
#ifdef SLS_CHECK_BOUNDS
	if (m_N == 0)
		SLS_ERR("tring to use end() on empty vector!");
#endif
	return m_p[m_N - 1];
}

template <class T>
inline T & Vbase<T>::end(Long_I i)
{
#ifdef SLS_CHECK_BOUNDS
	if (i <= 0 || i > m_N)
		SLS_ERR("index out of bound");
#endif
	return m_p[m_N - i];
}

template <class T>
inline const T & Vbase<T>::end(Long_I i) const
{
#ifdef SLS_CHECK_BOUNDS
	if (i <= 0 || i > m_N)
		SLS_ERR("index out of bound");
#endif
	return m_p[m_N - i];
}

template<class T>
inline Vbase<T>::~Vbase()
{
	if (m_N != 0)
		delete[] m_p;
}

template <class T>
class Vector : public Vbase<T>
{
protected:
	typedef Vbase<T> Base;
	using Base::m_p;
	using Base::m_N;
	Vector();
public:
	using Base::resize;
	using Base::resize_cpy;
	using Base::operator=;

	explicit Vector(Long_I N);
	Vector(Long_I N, const T &s); // initialize to constant value
	Vector(Long_I N, const T *a); // copy from existing memory

	Vector(const Vector &rhs);	// copy constructor
	Vector &operator=(const Vector &rhs);
	/*template <class Tv, SLS_IF(is_dense_vec<Tv>())>
	Vector &operator=(const Tv &rhs);*/
	void operator<<(Vector &rhs); // move data and rhs.resize(0)
	template <class T1>
	void resize(const Vector<T1> &v);
#ifdef SLS_CUSLISC
	Vector & operator=(const Gvector<T> &rhs) // copy from GPU vector
	{ rhs.get(*this); return *this; }
#endif
};

template<class T>
inline Vector<T>::Vector() {}

template<class T>
inline Vector<T>::Vector(Long_I N) : Base(N) {}

template<class T>
inline Vector<T>::Vector(Long_I N, const T & s) : Vector(N)
{
	*this = s;
}

template<class T>
inline Vector<T>::Vector(Long_I N, const T * a) : Vector(N)
{
	veccpy(m_p, a, N);
}

template <class T>
Vector<T>::Vector(const Vector<T> &rhs) : Base(rhs)
{}

template <class T>
Vector<T> &Vector<T>::operator=(const Vector<T> &rhs)
{
	copy(*this, rhs);
	return *this;
}

template<class T>
template<class T1>
inline void Vector<T>::resize(const Vector<T1>& v)
{
	resize(v.size());
}

template <class T>
inline void Vector<T>::operator<<(Vector<T> &rhs)
{
	Base::operator<<(rhs);
}

} // namespace slisc
