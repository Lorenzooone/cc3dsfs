#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char *argv[]) {
	if(argc < 5) {
		cout << "Usage: " << argv[0] << " binary_file output_folder base_output_file_name base_array_name" << endl;
		return -1;
	}

	ifstream input(argv[1], ios::in | ios::binary);
	if(!input) {
		cout << "Couldn't open input file!" << endl;
		return -2;
	}
	vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});

	input.close();
	
	string output_folder(argv[2]);
	string output_base_filename(argv[3]);
	string base_array_name(argv[4]);

	string name_array = base_array_name;
	string name_len_array = base_array_name + "_len";
	int len = buffer.size();

	ofstream output(output_folder + "/" + output_base_filename + ".cpp", ios::out);
	if(!output) {
		cout << "Couldn't open output cpp file!" << endl;
		return -2;
	}
	output << "#include \"" << output_base_filename << ".h\"" << endl;
	output << "unsigned char " << name_array << "[" << name_len_array << "] = {";

	for(int i = 0; i < len; i+= 16) {
		output << endl;
		int curr_line_todo = 16;
		if((len - i) < curr_line_todo)
			curr_line_todo = len - i;
		for(int j = 0; j < curr_line_todo; j++) {
			output << "0x";
			int value = ((int)buffer[i + j]);
			if(value < 16)
				output << "0";
			output << std::hex << value;
			if((i + j) < (len - 1))
				output << ",";
			if(j < (curr_line_todo - 1))
				output << " ";
		}
	}
	output << "};";

	output.close();

	ofstream output2(output_folder + "/" + output_base_filename + ".h", ios::out);
	if(!output2) {
		cout << "Couldn't open output h file!" << endl;
		return -3;
	}
	output2 << "#ifndef __" << base_array_name << "_H" << endl << "#define __" << base_array_name << "_H" << endl;
	output2 << "extern unsigned char " << name_array << "[];" << endl;
	output2 << "#define " << name_len_array << " " << len << endl;
	output2 << "#endif" << endl;
	output2.close();

	return 0;
}
