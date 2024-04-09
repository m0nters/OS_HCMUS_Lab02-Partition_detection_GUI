#include "Header.h"

std::wstring stringToWideString(const std::string& str) {
    std::wstring wideStr(str.begin(), str.end());
    return wideStr;
}

int littleEndianByteArrayToInt(const BYTE* byteArray, size_t length) {
    int result = 0;
    for (size_t i = 0; i < length; ++i) {
        result |= (static_cast<int>(byteArray[i]) << (i * 8));
    }
    return result;
}

int byteToTwosComplement(int byteValue) {
    if (byteValue & 0x80) {
        return byteValue - 256;
    }
    else {
        return byteValue;
    }
}

std::wstring FileSystemEntity::utf16_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

void File::byteArrayToString()
{
    for (const auto& cluster : this->data) {
        for (BYTE byte : cluster) {
            if (byte != 0x00)
                this->text.push_back(static_cast<char>(byte));
        }
    }
}