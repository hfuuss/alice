
#include "BinaryToFile.h"
#include <fstream>
#include<iostream>
#include<string>

void BinaryToFile::execute()
{
 	 std::ifstream fin(M_inputname.c_str(),std::ios::binary);

	 if (!fin.is_open())
	 {
		 std::cerr<<"Read File  Faild\n";
	 }

         std::ofstream fout(M_outputname.c_str());

	 char line[100];
	 const int line_size=100;
	 while(fin.read((char *)line, line_size) )
	 {

		 fout<<line<<"\n";

	 }


	 fin.close();
	 fout.close();

}
