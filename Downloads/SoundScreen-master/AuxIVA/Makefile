cc = g++ -g
cc_o = $(cc) -c -std=c++11 -Wall

aux_iva : main.o auxiva.o
	$(cc) main.o auxiva.o -o aux_iva


main.o : main.cpp auxiva.h
	$(cc_o) main.cpp

auxiva.o : auxiva.cpp auxiva.h fft.h mat.h
	$(cc_o) auxiva.cpp


clean : 
	rm aux_iva *.o