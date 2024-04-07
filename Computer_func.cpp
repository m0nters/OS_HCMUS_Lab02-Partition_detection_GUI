#include "Header.h"

void Computer::readDrives()
{
    for (int i = 0; i < root_Drives.size(); i++)
    {
        std::wstring wideDriveLetter = stringToWideString(root_Drives[i]->getName());
        std::wstring drivePath = L"\\\\.\\" + wideDriveLetter;
        if (root_Drives[i]->getType() == "NTFS")
        {
            NTFS_Read_VBR(i, drivePath);
            NTFS_Read_MFT(i, drivePath);
        }
        else if (root_Drives[i]->getType() == "FAT32")
        {
            FAT32_Read_BootSector(i, drivePath);
            FAT32_Read_RDET(i, drivePath);
        }
        readData(drivePath);
        root_Drives[i]->setToTalSize();
    }
}

void Computer::detectFormat()
{
    GetRemovableDriveNames();
    HANDLE hDrive = CreateFile(L"\\\\.\\PhysicalDrive1", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDrive == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to open physical drive." << std::endl;
        return;
    }
    BYTE mbr[512]; // MBR 512 bytes
    DWORD bytesRead;
    if (!ReadFile(hDrive, mbr, sizeof(mbr), &bytesRead, NULL)) {
        std::cout << "Failed to read MBR from physical drive." << std::endl;
        CloseHandle(hDrive);
        return;
    }
    int drive_order = 0;
    for (int i = 0x01BE + 4; i < sizeof(mbr); i += 16) {// offset 0x04 bat dau tu 0x01BE, Bang mo ta 1 partition cua MBR = 16 bytes 
        if (mbr[i] == 0x07) {
            root_Drives[drive_order]->setType("NTFS");
        }
        else if (mbr[i] == 0x0C || mbr[i] == 0x0B) {
            root_Drives[drive_order]->setType("FAT32");
        }
        else if (mbr[i] == 0x00)
            break;
        else {
            root_Drives[drive_order]->setType("Unknown");
        }

        BYTE startSector[5];
        memcpy(startSector, &mbr[i + 4], 4);
        startSector[4] = '\0';
        root_Drives[drive_order++]->setStartedByte(littleEndianByteArrayToInt(startSector, 4));
    }
    CloseHandle(hDrive);
}

void Computer::GetRemovableDriveNames() {
    std::vector<std::string> driveNames;
    char LogicalDrives[MAX_PATH] = { 0 };
    DWORD dwResult = GetLogicalDriveStringsA(MAX_PATH, LogicalDrives);
    if (dwResult > 0 && dwResult <= MAX_PATH) {
        char* pszDrive = LogicalDrives;
        while (*pszDrive) {
            UINT driveType = GetDriveTypeA(pszDrive);
            if (driveType == DRIVE_REMOVABLE) {
                driveNames.push_back(pszDrive);
            }
            pszDrive += strlen(pszDrive) + 1;
        }
    }
    for (int i = 0; i < driveNames.size(); i++)
    {
        Drive* d = new Drive;
        d->setName(driveNames[i].substr(0, driveNames[i].size() - 1));
        addRootDrive(d);
    }
}

void Computer::FAT32_Remove_File(int ith_drive, std::wstring drivePath, std::string name_file)
{
    HANDLE hDrive = CreateFileW(drivePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (hDrive == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open drive: " << GetLastError() << std::endl;
    }
    DWORD bytesRead;
    int bytes_read = 0;
    bool is_end = 0;
    while (!is_end)
    {
        SetFilePointer(hDrive, root_Drives[ith_drive]->getStartedByteRDET() + bytes_read, NULL, FILE_BEGIN);
        bytes_read += 512;
        BYTE rdet[512];
        if (!ReadFile(hDrive, rdet, sizeof(rdet), &bytesRead, NULL)) {
            std::wcerr << "Failed to read boot sector from physical drive." << std::endl;
            CloseHandle(hDrive);
            return;
        }

        int count_extra_entry = 0; // Dem entry phu
        int start_byte = 0; // Cho vong lap chay 32 byte cho moi vong lap
        std::vector <std::vector <BYTE>> extra_entry;
        std::vector <BYTE> main_entry;
        while (start_byte < 512)
        {
            int attribute = rdet[start_byte + 0x0B]; //0-0-A-D-V-S-H-R
            if (attribute == 0x0F)
            {
                count_extra_entry++;
                std::vector<BYTE> temp_vec;
                std::copy(&rdet[start_byte], &rdet[start_byte + 32], back_inserter(temp_vec)); // Doc 32 byte vao entry phu
                extra_entry.push_back(temp_vec);
            }
            else if ((attribute == 0x10 || attribute == 0x20) && rdet[start_byte] != 0xE5)
            {
                copy(&rdet[start_byte], &rdet[start_byte + 32], back_inserter(main_entry)); // Doc 32 byte vao entry chinh
                std::string name = FAT32_Create_Name(extra_entry, main_entry);
                name.erase(std::remove(name.begin(), name.end(), 0x00), name.end());
                name.erase(std::remove(name.begin(), name.end(), '\0'), name.end());

                if (name == name_file)
                {
                    int started_file_offset = start_byte - count_extra_entry * 32;
                    std::vector <BYTE> started_byte; // Luu vao de recover lai
                    for (int i = 0; i < (count_extra_entry + 1) * 32; i += 32)
                    {
                        started_byte.push_back(rdet[started_file_offset + i]);
                        rdet[started_file_offset + i] = 0xE5;
                    }
                    this->setMapRDET(started_byte, started_file_offset, name_file);
                    uint64_t offset = root_Drives[ith_drive]->getStartedByteRDET() + bytes_read - 512;
                    LARGE_INTEGER liOffset;
                    liOffset.QuadPart = offset;
                    if (!SetFilePointerEx(hDrive, liOffset, NULL, FILE_BEGIN))
                    {
                        std::cout << "Failed to point to offset\n";
                    }
                    DWORD bytesReturn;
                    if (!DeviceIoControl(hDrive, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturn, NULL))
                    {
                        std::cout << "Failed to lock Drive\n";
                        DWORD error = GetLastError();
                        std::cout << "Error: " << error << std::endl;
                    }

                    DWORD bytesWritten;
                    if (!WriteFile(hDrive, rdet, 512, &bytesWritten, NULL)) {
                        DWORD lastError = GetLastError();
                        std::wcerr << "Failed to write to physical drive." << lastError << std::endl;
                        CloseHandle(hDrive);
                        return;
                    }

                    FlushFileBuffers(hDrive);
                    CloseHandle(hDrive);
                    return;
                }

                extra_entry.clear();
                main_entry.clear();
            }
            else if ((attribute == 0x10 || attribute == 0x20) && rdet[start_byte] == 0xE5)
            {
                count_extra_entry = 0;
                extra_entry.clear();
                main_entry.clear();
            }
            else if (attribute == 0x00)
            {
                is_end = 1;
                break;
            }
            else
            {
                count_extra_entry = 0;
                extra_entry.clear();
                main_entry.clear();
            }
            start_byte += 32;
        }
    }
    CloseHandle(hDrive);
    this->root_Drives[ith_drive]->FAT32_Remove_File(drivePath, name_file, *this);
}
void Computer::FAT32_Recover_File(int ith_drive, std::wstring drivePath, std::string name_file)
{
    auto it = this->offset_recover_started_rdet_entry.find(name_file);
    if (it != this->offset_recover_started_rdet_entry.end())

    {
        HANDLE hDrive = CreateFileW(drivePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (hDrive == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open drive: " << GetLastError() << std::endl;
        }
        DWORD bytesRead;
        int offset = 0;
        if (512 < this->offset_recover_started_rdet_entry[name_file])
        {
            offset = root_Drives[ith_drive]->getStartedByteRDET() + (int)(this->offset_recover_started_rdet_entry[name_file] / 512);
        }
        else offset = root_Drives[ith_drive]->getStartedByteRDET();
        SetFilePointer(hDrive, offset, NULL, FILE_BEGIN);
        BYTE rdet[512];
        if (!ReadFile(hDrive, rdet, sizeof(rdet), &bytesRead, NULL)) {
            std::wcerr << "Failed to read boot sector from physical drive." << std::endl;
            CloseHandle(hDrive);
            return;
        }
        int started_offset = this->offset_recover_started_rdet_entry[name_file];
        for (int i = 0; i < this->started_byte_rdet_sdet[name_file].size(); i++)
        {
            rdet[started_offset + i * 32] = this->started_byte_rdet_sdet[name_file][i];
        }
        uint64_t u_offset = offset;
        LARGE_INTEGER liOffset;
        liOffset.QuadPart = u_offset;
        if (!SetFilePointerEx(hDrive, liOffset, NULL, FILE_BEGIN))
        {
            std::cout << "Failed to point to offset\n";
        }
        DWORD bytesReturn;
        if (!DeviceIoControl(hDrive, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturn, NULL))
        {
            std::cout << "Failed to lock Drive\n";
            DWORD error = GetLastError();
            std::cout << "Error: " << error << std::endl;
        }

        DWORD bytesWritten;
        if (!WriteFile(hDrive, rdet, 512, &bytesWritten, NULL)) {
            DWORD lastError = GetLastError();
            std::wcerr << "Failed to write to physical drive." << lastError << std::endl;
            CloseHandle(hDrive);
            return;
        }

        FlushFileBuffers(hDrive);
        CloseHandle(hDrive);
    }
    else
    {
        HANDLE hDrive = CreateFileW(drivePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (hDrive == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open drive: " << GetLastError() << std::endl;
        }
        DWORD bytesRead;
        int offset = this->offset_recover_started_sdet_entry[name_file].first;
        SetFilePointer(hDrive, offset, NULL, FILE_BEGIN);
        BYTE sdet[512];
        if (!ReadFile(hDrive, sdet, sizeof(sdet), &bytesRead, NULL)) {
            std::wcerr << "Failed to read boot sector from physical drive." << std::endl;
            CloseHandle(hDrive);
            return;
        }
        int started_offset = this->offset_recover_started_sdet_entry[name_file].second;
        for (int i = 0; i < this->started_byte_rdet_sdet[name_file].size(); i++)
        {
            std::cout << std::hex << (int)sdet[started_offset + i * 32] << " ";
            sdet[started_offset + i * 32] = this->started_byte_rdet_sdet[name_file][i];
        }
        uint64_t u_offset = offset;
        LARGE_INTEGER liOffset;
        liOffset.QuadPart = u_offset;
        if (!SetFilePointerEx(hDrive, liOffset, NULL, FILE_BEGIN))
        {
            std::cout << "Failed to point to offset\n";
        }
        DWORD bytesReturn;
        if (!DeviceIoControl(hDrive, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturn, NULL))
        {
            std::cout << "Failed to lock Drive\n";
            DWORD error = GetLastError();
            std::cout << "Error: " << error << std::endl;
        }

        DWORD bytesWritten;
        if (!WriteFile(hDrive, sdet, 512, &bytesWritten, NULL)) {
            DWORD lastError = GetLastError();
            std::wcerr << "Failed to write to physical drive." << lastError << std::endl;
            CloseHandle(hDrive);
            return;
        }

        FlushFileBuffers(hDrive);
        CloseHandle(hDrive);
    }
}