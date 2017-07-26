
#ifndef BINARY_TO_FILE_H
#define BINARY_TO_FILE_H
#include<string>

class BinaryToFile
{
	private:
	std::string M_inputname;
	std::string M_outputname;

	public:
	BinaryToFile(std::string inputname, std::string outputname):
	M_inputname(inputname), M_outputname(outputname)
	{}
    
	void  execute();
};
#endif


