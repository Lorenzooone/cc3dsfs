#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

using namespace std;

int main(int argc, char *argv[]) {
	if(argc < 4) {
		cout << "Usage: " << argv[0] << " text_file new_start define_file" << endl;
		return -1;
	}

	string new_start(argv[2]);
	ifstream input(argv[1], ios::in);
	if(!input) {
		cout << "Couldn't open input file!" << endl;
		return -2;
	}
	vector<string> file_lines;
	vector<string> new_start_lines;
	string single_str = "";
	while (getline(input, single_str)) {
		file_lines.push_back(single_str);
		string new_string = single_str;
		for(int i = 0; i < new_start.length(); i++)
			new_string[i] = new_start[i];
		new_start_lines.push_back(new_string);
	}

	input.close();

	ofstream output_define(argv[3], ios::out);
	if(!output_define) {
		cout << "Couldn't open output define file!" << endl;
		return -2;
	}
	for(int i = 0; i < file_lines.size(); i++)
		output_define << "#define " << file_lines[i] << " " << new_start_lines[i] << endl;

	output_define.close();

	return 0;
}
