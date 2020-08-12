#This is a makefile for project 2

CXX = g++

.PHONY: clean

proj2.exe: project2.o
	$(CXX) -pthread -o proj2.exe project2.o

project2.o: project2.cpp
	$(CXX) -c project2.cpp

clean:
	rm *.o *~ *.exe
