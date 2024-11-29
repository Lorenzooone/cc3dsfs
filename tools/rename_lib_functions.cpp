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
		cout << "Usage: " << argv[0] << " text_file new_start lib_file" << endl;
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

	vector<uint8_t> buffer;
	ifstream lib_in(argv[3], ios::in | ios::binary);
	if(!lib_in) {
		cout << "Couldn't open in library file!" << endl;
		return -2;
	}
	// Get length of file
	lib_in.seekg(0, ios::end);
	size_t length = lib_in.tellg();
	lib_in.seekg(0, ios::beg);

	//read file
	if (length > 0) {
		buffer.resize(length);    
		lib_in.read((char*)&buffer[0], length);
	}

	for(int u = 0; u < new_start_lines.size(); u++) {
		const uint8_t* old_data = (const uint8_t*)file_lines[u].c_str();
		const uint8_t* new_data = (const uint8_t*)new_start_lines[u].c_str();
		auto start_iterator = std::begin(buffer);
		uint8_t* inner_buffer = &buffer[0];
		size_t old_data_size = strlen((const char*)old_data);
		while(1) {
			start_iterator = search(
				start_iterator, std::end(buffer),
				std::begin(file_lines[u]), std::end(file_lines[u]));

			if (start_iterator == std::end(buffer))
				break;
			else
			{
				size_t match_pos = std::distance(std::begin(buffer), start_iterator);
				// This is technically wrong, but it works, so...
				if((match_pos > 0) && (inner_buffer[match_pos - 1] == 0)) {
					for(int i = 0; i < old_data_size; i++)
						inner_buffer[match_pos + i] = new_data[i];
				}
				start_iterator += old_data_size;
			}
		}
	}

	ofstream lib_out(argv[3], ios::out | ios::binary);
	if(!lib_out) {
		cout << "Couldn't open out library file!" << endl;
		return -2;
	}
	lib_out.write((char*)&buffer[0], buffer.size());
	lib_out.close();

	return 0;
}
