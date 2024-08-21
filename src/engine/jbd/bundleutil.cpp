//
// Created by Ethan Lee on 8/19/24.
//

#include "bundleutil.h"
#include <fstream>
#include <unordered_map>
#include <cstring>

struct JEBundledFileInfo {
    char path[64];
    unsigned long fileStartOffset;
    unsigned long fileLength;
};

struct JEFileHeader {
    unsigned int magicNum = 0x0B1A11A7;
    unsigned int fileCount = 0;
};

std::unordered_map<std::string, std::vector<unsigned char>> bundleCache{};

std::vector<unsigned char> getFileChars(std::string bundleFileName) {
    if (bundleCache.contains(bundleFileName)) {
        return bundleCache.at(bundleFileName);
    } else {
        std::vector<unsigned char> chars;
        std::ifstream file(bundleFileName, std::ios::binary);
        while (!file.eof()) {
            char c{};
            file.get(c);
            chars.push_back(c);
        }
        chars.resize(chars.size()-1); // TODO this is bad, probably
        bundleCache.insert({bundleFileName, chars});
        return chars;
    }
}

std::vector<unsigned char> getFileCharVec(const std::string& extractFileName, const std::string& bundleFileName) {
    std::vector<unsigned char> chars = getFileChars(bundleFileName);
    JEFileHeader head = *reinterpret_cast<JEFileHeader*>(&chars[0]);
    if (head.magicNum != 0x0B1A11A7) {
        throw std::runtime_error("Couldn't read bundle file \"" + bundleFileName + "\"!");
    }
    JEBundledFileInfo* correctBfiPtr = nullptr;
    for (int i = 0; i < head.fileCount; ++i) {
        JEBundledFileInfo* current = reinterpret_cast<JEBundledFileInfo*>(&chars[sizeof(JEFileHeader) + i * sizeof(JEBundledFileInfo)]);
        if (extractFileName.starts_with(current->path)) {
            correctBfiPtr = current;
            break;
        }
    }
    if (correctBfiPtr != nullptr) {
        JEBundledFileInfo correctBfi{};
        memcpy(&correctBfi.path, &correctBfiPtr->path, sizeof(correctBfi.path)); // i know it's 64 bytes but codacy hates me and makes me verify it
        correctBfi.fileLength = correctBfiPtr->fileLength;
        correctBfi.fileStartOffset = correctBfiPtr->fileStartOffset;
        chars.erase(chars.begin(), chars.begin()+correctBfi.fileStartOffset);
        chars.resize(correctBfi.fileLength);
        return chars;
    }
    else {
        throw std::runtime_error("Couldn't read file \"" + extractFileName + "\" from bundle \"" + bundleFileName + "\"!");
    }
}