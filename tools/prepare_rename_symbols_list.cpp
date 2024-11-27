#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

using namespace std;

int main(int argc, char *argv[]) {
	if(argc < 4) {
		cout << "Usage: " << argv[0] << " text_file prefix new_file define_file" << endl;
		return -1;
	}

	ifstream input(argv[1], ios::in);
	if(!input) {
		cout << "Couldn't open input file!" << endl;
		return -2;
	}
	vector<string> file_lines;
	string single_str = "";
	while (getline(input, single_str))
		file_lines.push_back(single_str);

	input.close();
	
	string prefix(argv[2]);

	ofstream output(argv[3], ios::out);
	if(!output) {
		cout << "Couldn't open output file!" << endl;
		return -2;
	}
	for(int i = 0; i < file_lines.size(); i++)
		output << file_lines[i] << " " << prefix << file_lines[i] << endl;

	output.close();

	if(argc > 4) {
		ofstream output_define(argv[4], ios::out);
		if(!output_define) {
			cout << "Couldn't open output define file!" << endl;
			return -2;
		}
		for(int i = 0; i < file_lines.size(); i++)
			output_define << "#define " << file_lines[i] << " " << prefix << file_lines[i] << endl;

		output_define.close();
	}

	return 0;
}
