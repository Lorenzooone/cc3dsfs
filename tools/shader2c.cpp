#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

#if (!defined(_MSC_VER)) || (_MSC_VER > 1916)
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

#define ENUM_NAME_STR_IDENTIFIER_POS 0
#define OPERATION_POS 1
#define MIN_DOT_SEPARATED_STRINGS 2

using namespace std;

enum shader_operations { NO_OPERATION, POWER_OF_2_OPERATION };

vector<string> split_str_by_char(string str_to_split, char split_char) {
	stringstream str_stream(str_to_split);
	vector<string> result;

	while(str_stream.good())
	{
		string substr;
		getline(str_stream, substr, split_char);
		result.push_back(substr);
	}
	return result;
}

inline bool str_ends_with(string const &value, string const &ending)
{
    if(ending.size() > value.size()) return false;
    return equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int process_file(filesystem::path input_path, ofstream &output) {
	string final_path = input_path.filename().string();
	int first = -1;
	int last = -1;
	shader_operations loaded_operation = NO_OPERATION;

	vector<string> dot_separated_names = split_str_by_char(final_path, '.');
	if(dot_separated_names.size() < MIN_DOT_SEPARATED_STRINGS) {
		cout << "Badly formatted input name!" << endl;
		return -3;
	}

	transform(dot_separated_names[ENUM_NAME_STR_IDENTIFIER_POS].begin(), dot_separated_names[ENUM_NAME_STR_IDENTIFIER_POS].end(), dot_separated_names[ENUM_NAME_STR_IDENTIFIER_POS].begin(), ::toupper);

	ifstream input(input_path, ios::in);
	if(!input) {
		cout << "Couldn't open input file!" << endl;
		return -2;
	}
	vector<string> file_lines;
	string single_str = "";
	while (getline(input, single_str))
		file_lines.push_back(single_str);

	input.close();

	if(dot_separated_names[OPERATION_POS].rfind("2_to_x", 0) == 0) {
		vector<string> underscore_separated_values = split_str_by_char(final_path, '_');
		first = stoi(underscore_separated_values[underscore_separated_values.size()-2]);
		last = stoi(underscore_separated_values[underscore_separated_values.size()-1]);
		loaded_operation = POWER_OF_2_OPERATION;
	}

	bool first_pass = true;
	do {
		if(!first_pass)
			first += 1;
		first_pass = false;
		string enum_str = dot_separated_names[ENUM_NAME_STR_IDENTIFIER_POS];
		output << "shader_strings[" << dot_separated_names[ENUM_NAME_STR_IDENTIFIER_POS];
		if(first != -1)
			output << "_" << first;
		output << "] = \\" << endl;

		for(int i = 0; i < file_lines.size(); i++) {
			string line = file_lines[i];
			if((loaded_operation == POWER_OF_2_OPERATION) && (str_ends_with(line, "= x;"))) {
				vector<string> x_split_line = split_str_by_char(line, 'x');
				string new_line = "";
				for(int j = 0; j < x_split_line.size() - 1; j++) {
					new_line += x_split_line[j];
					if(j < x_split_line.size() - 2)
						new_line += "x";
				}
				new_line += to_string((float)(1 << first)) + ";";
				line = new_line;
			}
			output << "\"" << line << "\" \\" <<endl;
		}
		output << "\"\";" << endl;
	} while(first != last);
	return 0;
}

int main(int argc, char *argv[]) {
	if(argc < 4) {
		cout << "Usage: " << argv[0] << " shaders_folder cpp_template_file output_file_name" << endl;
		return -1;
	}

	ifstream input(argv[2], ios::in);
	if(!input) {
		cout << "Couldn't open template file!" << endl;
		return -4;
	}
	vector<string> file_lines;
	string single_str = "";
	while (getline(input, single_str))
		file_lines.push_back(single_str);

	input.close();

	ofstream output(argv[3], ios::out);
	if(!output) {
		cout << "Couldn't open output cpp file!" << endl;
		return -5;
	}
	for(int i = 0; i < file_lines.size(); i++)
		output << file_lines[i] << endl;

	for(const auto & entry : filesystem::directory_iterator(argv[1])) {
		int result = process_file(entry.path(), output);
		if(result != 0)
			return result;
	}
	output<<"}"<<endl;

	output.close();

	return 0;
}
