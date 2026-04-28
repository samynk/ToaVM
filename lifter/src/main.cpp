#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdint>

#include "jbc/Opcodes.hpp"

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


int main(){
	std::cout << "Reading LoopTest.class\n";
    auto classFile = read_class_file("../resources/jbc/LoopTest.class");

    std::cout << "Read " << classFile.size() << " bytes." << "\n";
}