#include "mat.h"
#include <iostream>

using namespace std;

// only works for floats & 2x2 matrices
void printMatrix(Matrix<float> m)
{
	printf("[% 05.2f  % 05.2f ]\n", m.data[0][0], m.data[0][1]);
	printf("[% 05.2f  % 05.2f ]\n", m.data[1][0], m.data[1][1]);
	return;
}

int main()
{
	Matrix<int> i_1(2,2);
	Matrix<int> i_2(2,2);
	Matrix<int> i_3(2,3);

	Matrix<int> i_v[2];
	i_v[0] = Matrix<int>(2,2);
	i_v[0].data[0][0] = 1;
	
	Matrix<float> f_1(2,2);
	Matrix<float> f_2(2,2);
	Matrix<float> f_3(2,3);

	int count = 0;
	float count_f = 0;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++, count++, count_f += 0.5)
		{
			i_1.data[i][j] = count;
			i_2.data[i][j] = count;
			
			f_1.data[i][j] = count_f;
			f_2.data[i][j] = count_f;
		}
	}

	count = 0;
	count_f = 0;
	
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++, count++, count_f += 0.5)
		{
			i_3.data[i][j] = count;
			f_3.data[i][j] = count_f;
		}
	}

	// test * with matrices of different sizes
	Matrix<int> i_4 = i_1 * i_3;
	Matrix<float> f_4 = f_1 * f_3;

	// test +
	Matrix<int> i_5 = i_1 + i_2;
	Matrix<float> f_5 = f_1 + f_2;

	// test * with matrices of the same size
	Matrix<int> i_6 = i_1 * i_2;
	Matrix<float> f_6 = f_1 * f_2;
	
	// test transpose()
	i_3.transpose();
	f_3.transpose();

	// test *=
	i_1 *= i_2;
	f_1 *= f_2;
	
	if (i_1 != i_6 || f_1 != f_6)
	{
		cerr << "ERROR: either operator!=(), operator*(), or operator*=() broken" << endl;
	}
	
	if (!(i_1 == i_6) || !(f_1 == f_6))
	{
		cerr << "ERROR: either operator==(), operator*(), or operator*=() broken" << endl;
	}
	
	// test get/set row
	Matrix<int> i_7(1, 2);
	i_7 = i_2.get_row(0);
	i_7 = i_2.get_row(1);
	
	i_7.data[0][0] = 10;
	i_7.data[0][1] = 11;
	
	i_1.set_row(0, i_2.get_row(0));
	i_1.set_row(1, i_7);
	
	return 0;
}
