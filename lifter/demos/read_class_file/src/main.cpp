#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdint>

#include "jbc/Opcodes.hpp"
#include "jbc/ClassFile.hpp"
#include "ClassViewer.hpp"

std::vector<uint8_t> read_class_file(const std::filesystem::path& path) {
	// Open at the end (ate) to get size immediately
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open .class file");
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(size);
	if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
		return buffer;
	}

	return {};
}


int main() {
	try {
		std::cout << "Current working directory: "
			<< std::filesystem::current_path()
			<< '\n';
	}
	catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Error getting current path: " << e.what() << '\n';
	}

	std::cout << "Reading LoopTest.class\n";

	const auto bytes = read_class_file("resources/LoopTest.class");
	const auto classFile = jbc::parse_class_file(bytes);


	ClassViewerApp app{ classFile };
	app.loop();

	return EXIT_SUCCESS;

}