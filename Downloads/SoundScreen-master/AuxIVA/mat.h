#ifndef STRUCT_H
#define STRUCT_H

#define DEBUG	// comment for actual compilation

#include <complex>

#ifdef DEBUG
	#include <iostream>
	using namespace std;
#endif

using namespace std;

template<class T>
class Matrix
{
public:	
	T **data;
	int num_rows, num_cols;
	
	Matrix() : num_rows(0), num_cols(0) {};
	Matrix(int rows, int cols);
	Matrix(const Matrix<T> &m);
	~Matrix();
	
	// 0-indexed
	Matrix<T> get_row(int row);
	Matrix<T> get_col(int col);
	Matrix<T>& set_row(int row, Matrix<T> m);
	Matrix<T>& set_col(int col, Matrix<T> m);
	
	float determinant();
	/*T at(int row, int col);
	T* at(int row);*/

	Matrix<T> inverse();
	Matrix<T>& transpose();
	Matrix<T>& hermitian();
	void print();
	
	// operator overloading
	Matrix<T>& operator+=(const Matrix<T> &m);
	Matrix<T>& operator+=(T t);

	/*Matrix<T>& operator-=(const Matrix<T> &m);
	Matrix<T>& operator-=(T t);*/

	Matrix<T>& operator*=(const Matrix<T> &m);
	Matrix<T>& operator*=(T t);
	
	Matrix<T>& operator/=(T t);
	Matrix<T>& operator/=(const Matrix<T> &m);
	
	Matrix<T>& operator=(const Matrix<T> &m);
	
	bool operator==(const Matrix<T> &m) const;
	bool operator!=(const Matrix<T> &m) const;

private:
};

template<class T>
Matrix<T>::Matrix(int rows, int cols)
{
	num_rows = rows;
	num_cols = cols;

	data = (T**)calloc(num_rows, sizeof(T));
	for (int row = 0; row < num_rows; ++row)
	{
		data[row] = (T*)calloc(num_cols, sizeof(T));
	}
}

template<class T>
Matrix<T>::Matrix(const Matrix<T> &m)
{
	num_rows = m.num_rows;
	num_cols = m.num_cols;

	data = (T**)calloc(num_rows, sizeof(T));
	for (int row = 0; row < num_rows; ++row)
	{
		data[row] = (T*)calloc(num_cols, sizeof(T));
	}
	
	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			data[row][col] = m.data[row][col];
		}
	}
}

template<class T>
Matrix<T>::~Matrix()
{
	for (int row = 0; row < num_rows; ++row)
	{
		delete[] data[row];
	}
	delete[] data;
}

/*template<class T>
T* Matrix<T>::at(int row)
{
	return data[row];
}

template<class T>
T Matrix<T>::at(int row, int col)
{
	return data[row][col];
}*/

template<class T>
Matrix<T> Matrix<T>::get_row(int row)
{
#ifdef DEBUG
	if (row < 0 || row >= num_rows)
	{
		cerr << "ERROR: get_row() index out of bounds: " << row << endl;
		return *this;
	}
#endif

	Matrix<T> r(1, num_cols);
	
	for (int col = 0; col < num_cols; ++col)
	{
		r.data[0][col] = data[row][col];
	}
	
	return r;
}

template<class T>
Matrix<T> Matrix<T>::get_col(int col)
{
#ifdef DEBUG
	if (col < 0 || col >= num_cols)
	{
		cerr << "ERROR: get_col() index out of bounds: " << col << endl;
		return *this;
	}
#endif

	Matrix<T> r(num_rows, 1);
	
	for (int row = 0; row < num_rows; ++row)
	{
		r.data[row][0] = data[row][col];
	}
	
	return r;
}

template<class T>
Matrix<T>& Matrix<T>::set_row(int row, Matrix<T> m)
{
#ifdef DEBUG
	if (row < 0 || row >= num_rows || m.num_cols != num_cols)
	{
		cerr << "ERROR: set_row() incorrect bounds" << endl;
		return *this;
	}
#endif

	for (int col = 0; col < num_cols; ++col)
	{
		data[row][col] = m.data[0][col];
	}
	
	return *this;
}

template<class T>
Matrix<T>& Matrix<T>::set_col(int col, Matrix<T> m)
{
#ifdef DEBUG
	if (col < 0 || col >= num_cols || m.num_rows != num_rows)
	{
		cerr << "ERROR: get_col() incorrect bounds" << endl;
		return this;
	}
#endif
	
	for (int row = 0; row < num_rows; ++row)
	{
		data[row][col] = m.data[row][0];
	}
	
	return *this;
}

template<class T>
float Matrix<T>::determinant()
{
	return (data[0][0] * data[1][1] - data[0][1] * data[1][0]); 
}

template<class T>
Matrix<T> Matrix<T>::inverse()
{
#ifdef DEBUG
	if ((num_rows != 2) && (num_cols != 2))
	{
		cerr << "ERROR: transpose wrong size: " << num_rows << "x" << num_cols << endl;
		return this;
	}
#endif

	Matrix<T> r(num_rows, num_cols);

	r.data[0][0] = data[1][1];
	r.data[0][1] = -data[0][1];
	r.data[1][0] = -data[1][0];
	r.data[1][1] = data[0][0];

	return (r / determinant());	
}

template<class T>
Matrix<T>& Matrix<T>::transpose()
{
	Matrix<T> r(num_cols, num_rows);
	
	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			r.data[col][row] = data[row][col];
		}
	}

	operator=(r);
	return *this;
}

template<class T>
Matrix<T>& Matrix<T>::hermitian()
{
	Matrix<T> r(num_cols, num_rows);
	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			r.data[col][row] = conj(data[row][col]);
		}
	}
	
	operator=(r);
	return *this;
}

template<class T>
void Matrix<T>::print()
{
	printf("[% 05.2f  % 05.2f ]\n", data[0][0], data[0][1]);
	printf("[% 05.2f  % 05.2f ]\n", data[1][0], data[1][1]);
	return;
}

// ----------------- operator overloads -----------------

template<class T>
Matrix<T>& Matrix<T>::operator+=(const Matrix<T> &m)
{
#ifdef DEBUG
	if ((m.num_rows == num_rows) && (m.num_cols != num_cols))
	{
		cerr << "ERROR: operator+=(matrix) wrong sizes" << endl;
		return m;
	}
#endif

	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			data[row][col] += m.data[row][col];
		}
	}

	return *this;
}

template<class T> inline
Matrix<T>& Matrix<T>::operator+=(T t)
{
	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			data[row][col] += t;
		}
	}

	return *this;
}

template<class T>
Matrix<T> operator+(const Matrix<T> &m1, const Matrix<T> &m2)
{
#ifdef DEBUG
	if ((m1.num_rows != m2.num_rows) && (m1.num_cols != m2.num_cols))
	{
		cerr << "ERROR: operator+(matrix, matrix) wrong sizes" << endl;
		return m1;
	}
#endif

	Matrix<T> r(m1.num_rows, m1.num_cols);

	for (int row = 0; row < r.num_rows; ++row)
	{
		for (int col = 0; col < r.num_cols; ++col)
		{
			r.data[row][col] = m1.data[row][col] + m2.data[row][col];
		}
	}

	return r;
}

template<class T>
Matrix<T> operator+(const Matrix<T> &m, T t)
{
	Matrix<T> r(m.num_rows, m.num_cols);

	for (int row = 0; row < r.num_rows; ++row)
	{
		for (int col = 0; col < r.num_cols; ++col)
		{
			r.data[row][col] = m.data[row][col] + t;
		}
	}

	return r;
}

template<class T>
Matrix<T> operator+(T t, const Matrix<T> &m)
{
	Matrix<T> r(m.num_rows, m.num_cols);

	for (int i = 0; i < r.datasize; i++)
	{
		r.data[i] = t + m.data[i];
	}

	return r;
}

/*template<class T>
Matrix<T>& Matrix<T>::operator-=(const Matrix<T> &m)
{
#ifdef DEBUG
	if ((m.num_rows != num_rows) && (m.num_cols != num_cols))
	{
		cerr << "ERROR: operator-=(matrix) wrong sizes" << endl;
		return m;
	}
#endif

	int m_pos, pos = 0;

	for (int col = 0; col < num_cols; ++col)
	{
		for (int row = 0; row < num_rows; ++row)
		{
			data[pos + row] -= m.data[m_pos + row];
		}

		// next column
		m_pos += m.num_rows;
		pos += num_rows;
	}

	return *this;
}

template<class T> inline
Matrix<T>& Matrix<T>::operator-=(T t)
{
	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			data[row][col] -= t;
		}
	}

	return *this;
}

template<class T>
Matrix<T> operator-(const Matrix<T> &m1, const Matrix<T> &m2)
{
#ifdef DEBUG
	if ((m1.num_rows != m2.num_rows) && (m1.num_cols == m2.num_cols))
	{
		cerr << "ERROR: operator-(matrix, matrix) wrong sizes" << endl;
		return m1;
	}
#endif

	Matrix<T> r(m1.num_rows, m1.num_cols);
	int m1_pos, m2_pos, r_pos = 0;

	for (int col = 0; col < r.num_cols; ++col)
	{
		for (int row = 0; row < r.num_rows; ++row)
		{
			r.data[r_pos + row] = m1.data[m1_pos + row] - m2.data[m2_pos + row];
		}

		// next column
		m1_pos += m1.num_rows;
		m2_pos += m2.num_rows;
		r_pos += r.num_rows;
	}

	return r;
}

template<class T>
Matrix<T> operator-(const Matrix<T> &m, T t)
{
	Matrix<T> r(m.num_rows, m.num_cols);
	int m_pos, r_pos = 0;

	for (int col = 0; col < r.num_cols; ++col)
	{
		for (int row = 0; row < r.num_rows; ++row)
		{
			r.data[r_pos + row] = m.data[m_pos + row] - t;
		}

		// next column
		m_pos += m.num_rows;
		r_pos += r.num_rows;
	}

	return r;
}

template<class T>
Matrix<T> operator-(T t, const Matrix<T> &m)
{
	Matrix<T> r(m.num_rows, m.num_cols);

	for (int row = 0; row < r.num_rows; ++row)
	{
		for (int col = 0; col < r.num_cols; ++col)
		{
			r.data[row][col] = t - m.data[row][col];
		}
	}

	return r;
}*/

template<class T>
Matrix<T>& Matrix<T>::operator*=(const Matrix<T> &m)
{
#ifdef DEBUG
	if (num_cols != m.num_rows)
	{
		cerr << "ERROR: operator*=(matrix) wrong sizes: " << num_cols << ", " << m.num_rows << endl;
		return *this;
	}
#endif

	Matrix<T> r(num_rows, m.num_cols);
	T sum;

	for (int row = 0; row < num_rows; ++row)
	{
		for (int m_col = 0; m_col < m.num_cols; ++m_col)
		{
			sum = T(0);

			for (int m_row = 0; m_row < m.num_rows; ++m_row)
			{
				sum += data[row][m_row] * m.data[m_row][m_col];
			}

			r.data[row][m_col] = sum;
		}
	}

	operator=(r);
	return *this;	// address of *this changes
}

template<class T> inline
Matrix<T>& Matrix<T>::operator*=(T t)
{
	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			data[row][col] *= t;
		}
	}

	return *this;
}

template<class T>
Matrix<T> operator*(const Matrix<T> &m1, const Matrix<T> &m2)
{
#ifdef DEBUG
	if (m1.num_cols != m2.num_rows)
	{
		cerr << "ERROR: operator*(matrix, matrix) wrong sizes: " << m1.num_cols << ", " << m2.num_rows << endl;
		return m1;
	}
#endif

	Matrix<T> r(m1.num_rows, m2.num_cols);
	T sum;
	
	for (int m1_row = 0; m1_row < m1.num_rows; ++m1_row)
	{
		for (int m2_col = 0; m2_col < m2.num_cols; ++m2_col)
		{
			sum = T(0);

			for (int m2_row = 0; m2_row < m2.num_rows; ++m2_row)
			{
				sum += m1.data[m1_row][m2_row] * m2.data[m2_row][m2_col];
			}

			r.data[m1_row][m2_col] = sum;
		}
	}
    
	return r;
}

template<class T>
T* operator*(const Matrix<T> &m, const T* v)
{
#ifdef DEBUG
	if (m.num_cols != (sizeof(v) / sizeof(v[0])) / sizeof(T))
	{
		cerr << "ERROR: operator*(matrix, vector) wrong sizes: " << m.num_cols << ", " <<
			(sizeof(v) / sizeof(v[0])) / sizeof(T) << endl;
		return v;
	}
#endif

	T* r[m.num_rows];
	int m_pos;

	for (int row = 0; row < m.rows(); ++row)
	{
		r[row] = T(0);
		m_pos = 0;

		for (int col = 0; col < m.cols(); ++col)
		{
			r[row] += m.data[m_pos + row] * v[col];
			m_pos += m.num_rows;
		}
	}

	return r;
}

template<class T>
Matrix<T> operator*(const Matrix<T> &m, T t)
{
	Matrix<T> r(m.num_rows, m.num_cols);

	for (int row = 0; row < r.num_rows; ++row)
	{
		for (int col = 0; col < r.num_cols; ++col)
		{
			r.data[row][col] = m.data[row][col] * t;
		}
	}

	return r;
}

template<class T> inline
Matrix<T> operator*(T t, const Matrix<T> &m)
{
	return operator*(m, t);
}

template<class T> inline
Matrix<T>& Matrix<T>::operator/=(T t)
{
	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			data[row][col] /= t;
		}
	}

	return *this;
}

template<class T> inline
Matrix<T>& Matrix<T>::operator/=(const Matrix<T> &m)
{
#ifdef DEBUG
	if ((m.num_rows != num_rows) && (m.num_cols != num_cols))
	{
		cerr << "ERROR: operator/=(matrix) wrong sizes" << endl;

		return m;
	}
#endif

	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			data[row][col] /= m.data[row][col];
		}
	}

	return *this;
}

template<class T>
Matrix<T> operator/(const Matrix<T> &m, T t)
{
	Matrix<T> r(m.num_rows, m.num_cols);

	for (int row = 0; row < r.num_rows; ++row)
	{
		for (int col = 0; col < r.num_cols; ++col)
		{
			r.data[row][col] = m.data[row][col] / t;
		}
	}

	return r;
}

template<class T>
Matrix<T> operator/(T t, const Matrix<T> &m)
{
	Matrix<T> r(m.num_rows, m.num_cols);

	for (int row = 0; row < r.num_rows; ++row)
	{
		for (int col = 0; col < r.num_cols; ++col)
		{
			r.data[row][col] = t / m.data[row][col];
		}
	}

	return r;
}

template<class T> inline
Matrix<T>& Matrix<T>::operator=(const Matrix<T> &m)
{
	for (int row = 0; row < num_rows; ++row)
	{
		delete[] data[row];
	}
	
	num_rows = m.num_rows;
	num_cols = m.num_cols;

	data = (T**)calloc(num_rows, sizeof(T));
	for (int row = 0; row < num_rows; ++row)
	{
		data[row] = (T*)calloc(num_cols, sizeof(T));
	}

	if (m.data != 0)
	{
		for (int row = 0; row < num_rows; ++row)
		{
			for (int col = 0; col < num_cols; ++col)
			{
				data[row][col] = m.data[row][col];
			}
		}
	}
	
	return *this;
}

template<class T>
bool Matrix<T>::operator==(const Matrix<T> &m) const
{
	if (num_rows != m.num_rows || num_cols != m.num_cols)
	{
		return false;
	}

	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			if (data[row][col] != m.data[row][col])
			{
				return false;
			}
		}
	}

	return true;
}

template<class T>
bool Matrix<T>::operator!=(const Matrix<T> &m) const
{
	if (num_rows != m.num_rows || num_cols != m.num_cols)
	{
		return true;
	}

	for (int row = 0; row < num_rows; ++row)
	{
		for (int col = 0; col < num_cols; ++col)
		{
			if (data[row][col] != m.data[row][col])
			{
				return true;
			}
		}
	}

	return false;
}

#endif	// MAT_H
