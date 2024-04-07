#include "Header.h"

void Computer::NTFS_Read_VBR(int ith_drive, std::wstring drivePath)
{
    HANDLE hDrive = CreateFile(drivePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDrive == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to open physical drive." << std::endl;
        return;
    }
    SetFilePointer(hDrive, 0, NULL, FILE_BEGIN);
    DWORD bytesRead;
    BYTE vbr[512];
    if (!ReadFile(hDrive, vbr, sizeof(vbr), &bytesRead, NULL)) {
        std::wcerr << "Failed to read boot sector from physical drive." << std::endl;
        CloseHandle(hDrive);
        return;
    }
    int byte_per_sector = vbr[0x0B] | (vbr[0x0B + 1] << 8);
    int sector_per_cluster = vbr[0x0D];
    int sum_sector_of_drive = vbr[0x28] | (vbr[0x28 + 1] << 8) | (vbr[0x28 + 2] << 16) | (vbr[0x28 + 3] << 24) | (vbr[0x28 + 4] << 32) | (vbr[0x28 + 5] << 40) | (vbr[0x28 + 6] << 48) | (vbr[0x28 + 7] << 56);
    int started_cluster_of_MFT = vbr[0x30] | (vbr[0x30 + 1] << 8) | (vbr[0x30 + 2] << 16) | (vbr[0x30 + 3] << 24) | (vbr[0x30 + 4] << 32) | (vbr[0x30 + 5] << 40) | (vbr[0x30 + 6] << 48) | (vbr[0x30 + 7] << 56);
    int started_cluster_of_extra_MFT = vbr[0x38] | (vbr[0x38 + 1] << 8) | (vbr[0x38 + 2] << 16) | (vbr[0x38 + 3] << 24) | (vbr[0x38 + 4] << 32) | (vbr[0x38 + 5] << 40) | (vbr[0x38 + 6] << 48) | (vbr[0x38 + 7] << 56);
    int byte_per_MFT_entry = pow(2, abs(byteToTwosComplement(vbr[0x40])));
    std::cout << "Byte per sector: " << byte_per_sector << std::endl;
    std::cout << "Sector per cluster: " << sector_per_cluster << std::endl;
    std::cout << "Total sectors of drive: " << sum_sector_of_drive << std::endl;
    std::cout << "Started cluster of MFT: " << started_cluster_of_MFT << std::endl;
    std::cout << "Started cluster of extra MFT: " << started_cluster_of_extra_MFT << std::endl;
    std::cout << "Byte per MFT entry: " << byte_per_MFT_entry << std::endl;
    root_Drives[ith_drive]->set_ntfs_vbr(byte_per_sector, sector_per_cluster, sum_sector_of_drive, started_cluster_of_MFT, started_cluster_of_extra_MFT, byte_per_MFT_entry);
    CloseHandle(hDrive);
}



void Computer::NTFS_Read_MFT(int ith_drive, std::wstring drivePath) {
    int drive_id = 5;
    NTFS_VBR vbr = root_Drives[ith_drive]->getVBRIn4();
    long long main_mft_offset_byte = (long long)vbr.byte_per_sector * vbr.sector_per_cluster * vbr.started_cluster_of_MFT;
    HANDLE hDrive = CreateFile(drivePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDrive == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to open physical drive." << std::endl;
        return;
    }
    std::vector <int> avalable_father_folder(100, 0);
    int end = 0;
    int cnt = 0;
    while (end == 0 || (end != 0 && cnt <= end))
    {
        DWORD lowOffset = static_cast<DWORD>(main_mft_offset_byte & 0xFFFFFFFF);
        DWORD highOffset = static_cast<DWORD>((main_mft_offset_byte >> 32) & 0xFFFFFFFF);
        DWORD result = SetFilePointer(hDrive, lowOffset, reinterpret_cast<PLONG>(&highOffset), FILE_BEGIN);
        if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
            std::cerr << "Failed to set file pointer." << std::endl;
            CloseHandle(hDrive);
            return;
        }
        DWORD bytesRead;
        BYTE* mft = new BYTE[vbr.byte_per_MFT_entry];
        if (!ReadFile(hDrive, mft, vbr.byte_per_MFT_entry, &bytesRead, NULL)) {
            std::wcerr << "Failed to read cluster from physical drive." << std::endl;
            CloseHandle(hDrive);
            delete[] mft;
            return;
        }
        int flag_file_directory_used = mft[0x16] | (mft[0x17] << 8);
        if (mft[0x00] == 70 && (flag_file_directory_used == 0x01 || flag_file_directory_used == 0x03) || cnt == 0) // FILE || BAAD, 0x01: File dang dung, 0x03: Thu muc dang dung
        {
            if (flag_file_directory_used == 0x01)
            {
                File* f = new File;
                f->setAttributes("File");
                Header_MFT_Entry mft_header;
                mft_header.started_attribute_offset = mft[0x14] | mft[0x15] << 8;
                mft_header.flag = mft[0x16] | mft[0x17] << 8;
                mft_header.byte_used = mft[0x18] | (mft[0x19] << 8) | (mft[0x20] << 16) | (mft[0x21] << 24);
                mft_header.byte_of_MFT_entry = mft[0x1C] | (mft[0x1D] << 8) | (mft[0x1E] << 16) | (mft[0x1F] << 24);
                mft_header.ID = mft[0x2C] | (mft[0x2D] << 8) | (mft[0x2E] << 16) | (mft[0x2F] << 21);
                f->NTFS_Set_MFT(mft_header);

                //Doc attributes
                int started_byte = mft_header.started_attribute_offset;
                while (started_byte < mft_header.byte_used)
                {

                    //Kich thuoc 1 attribute 
                    int size = mft[started_byte + 0x04] | (mft[started_byte + 0x05] << 8) | (mft[started_byte + 0x06] << 16) | (mft[started_byte + 0x07] << 24);
                    if (started_byte + size > mft_header.byte_used || size == 0)
                    {
                        break;
                    }
                    // Doc header cua attribute
                    BYTE* attr = new BYTE[size];
                    std::copy(mft + started_byte, mft + started_byte + size, attr);
                    Header_Attribute h;
                    h.type_id = (long long)attr[0x00] | (attr[0x01] << 8) | (attr[0x02] << 16) | (attr[0x03] << 24);
                    h.size_of_attribute = attr[0x04] | (attr[0x05] << 8) | (attr[0x06] << 16) | (attr[0x07] << 24);
                    h.flag_non_resident = attr[0x08];
                    h.length_name_attribute = attr[9];
                    h.offset_of_name = attr[10] | (attr[11] << 8);
                    h.flags = attr[12] | (attr[13] << 8);
                    h.attribute_id = attr[14] | (attr[15] << 8);
                    h.attribute_data_offset = attr[20] | (attr[21] << 8);
                    h.attribute_data_size = attr[16] | (attr[17] << 8) | (attr[18] << 16) | (attr[19] << 24);
                    f->pushHeaderAttribute(h);
                    if (h.type_id == 16)
                    {
                        Date d; Time t;
                        NTFS_Create_Date_Time(attr, h, d, t);
                        f->setDate(d);
                        f->setTime(t);
                    }
                    else if (h.type_id == 48)
                    {
                        int x = h.attribute_data_offset;
                        int flag = attr[x + 56] | (attr[x + 57] << 8) | (attr[x + 58] << 16) | (attr[x + 59] << 24);
                        int mask = 2;
                        bool is_Hiden = (flag & mask) == 2;
                        if (((is_Hiden) || h.attribute_data_size == 0) && cnt != 0)
                        {
                            delete[] attr;
                            delete f;
                            break;
                        }
                        if (cnt != 0)
                        {
                            f->setName(NTFS_Create_Name(attr, h));
                            int parentID = attr[x] | (attr[x + 1] << 8) | (attr[x + 2] << 16) | (attr[x + 3] << 24) | (attr[x + 4] << 32) | (attr[x + 5] << 40);
                            if (parentID == drive_id)
                            {
                                root_Drives[ith_drive]->addNewFile_Directory(f);

                            }
                            else
                            {
                                if (avalable_father_folder[parentID] == 1)
                                {
                                    Directory* d = root_Drives[ith_drive]->NTFS_Find_Parent_Directory(parentID);
                                    if (d != NULL)
                                    {
                                        d->addNewFile_Directory(f);
                                        std::cout << "Started byte: " << started_byte << std::endl;
                                        std::cout << "Byte used: " << mft_header.byte_used << std::endl;
                                    }
                                    else
                                    {
                                        delete f;
                                        delete[] attr;
                                        break;
                                    }
                                }
                            }

                        }
                    }
                    else if (h.type_id == 128)
                    {
                        if (h.flag_non_resident == 0)
                        {
                            f->setTotalSize(h.attribute_data_size);
                            f->NTFS_Read_Resident_Data(attr, h);
                        }
                        if (h.flag_non_resident)
                        {
                            int datarun_offset = attr[32];
                            long long datasize = 0;
                            for (int i = 0; i < 8; i++)
                            {
                                datasize |= (long long)(attr[48 + i] << (i * 8));
                            }
                            f->setTotalSize(datasize);
                            f->NTFS_Read_Non_Resident_Info(attr, vbr, datarun_offset, drivePath, datasize);

                        }
                    }
                    else if (h.type_id == 176 && cnt == 0)
                    {
                        end = NTFS_Read_BITMAP(attr, h);
                    }
                    started_byte += size;
                    delete[] attr;
                }
            }
            else if (flag_file_directory_used == 0x03)
            {
                Directory* f = new Directory;
                f->setAttributes("Folder");
                Header_MFT_Entry mft_header;
                mft_header.started_attribute_offset = mft[0x14] | mft[0x15] << 8;
                mft_header.flag = mft[0x16] | mft[0x17] << 8;
                mft_header.byte_used = mft[0x18] | (mft[0x19] << 8) | (mft[0x20] << 16) | (mft[0x21] << 24);
                mft_header.byte_of_MFT_entry = mft[0x1C] | (mft[0x1D] << 8) | (mft[0x1E] << 16) | (mft[0x1F] << 24);
                mft_header.ID = mft[0x2C] | (mft[0x2D] << 8) | (mft[0x2E] << 16) | (mft[0x2F] << 21);
                f->NTFS_Set_MFT(mft_header);

                //Doc attributes
                int started_byte = mft_header.started_attribute_offset;
                while (started_byte < mft_header.byte_used)
                {

                    //Kich thuoc 1 attribute 
                    int size = mft[started_byte + 0x04] | (mft[started_byte + 0x05] << 8) | (mft[started_byte + 0x06] << 16) | (mft[started_byte + 0x07] << 24);
                    if (started_byte + size > mft_header.byte_used || size == 0)
                    {
                        break;
                    }

                    // Doc header cua attribute
                    BYTE* attr = new BYTE[size];
                    std::copy(mft + started_byte, mft + started_byte + size, attr);
                    Header_Attribute h;
                    h.type_id = (long long)attr[0x00] | (attr[0x01] << 8) | (attr[0x02] << 16) | (attr[0x03] << 24);
                    if (h.type_id != 16 && h.type_id != 48 && h.type_id != 128 && h.type_id != 176)
                    {
                        started_byte += size;
                        delete[] attr;
                        break;
                    }
                    h.size_of_attribute = attr[0x04] | (attr[0x05] << 8) | (attr[0x06] << 16) | (attr[0x07] << 24);
                    h.flag_non_resident = attr[0x08];
                    h.length_name_attribute = attr[9];
                    h.offset_of_name = attr[10] | (attr[11] << 8);
                    h.flags = attr[12] | (attr[13] << 8);
                    h.attribute_id = attr[14] | (attr[15] << 8);
                    h.attribute_data_offset = attr[20] | (attr[21] << 8);
                    h.attribute_data_size = attr[16] | (attr[17] << 8) | (attr[18] << 16) | (attr[19] << 24);
                    f->pushHeaderAttribute(h);

                    if (h.type_id == 16)
                    {
                        Date d; Time t;
                        NTFS_Create_Date_Time(attr, h, d, t);
                        f->setDate(d);
                        f->setTime(t);
                    }
                    else if (h.type_id == 48)
                    {
                        int x = h.attribute_data_offset;
                        int flag = attr[x + 56] | (attr[x + 57] << 8) | (attr[x + 58] << 16) | (attr[x + 59] << 24);
                        int mask = 2;
                        bool is_Hiden = (flag & mask) == 2;
                        if (((is_Hiden) || h.attribute_data_size == 0) && cnt != 0)
                        {
                            delete[] attr;
                            delete f;
                            break;
                        }
                        if (cnt != 0)
                        {
                            f->setName(NTFS_Create_Name(attr, h));
                            int parentID = attr[x] | (attr[x + 1] << 8) | (attr[x + 2] << 16) | (attr[x + 3] << 24) | (attr[x + 4] << 32) | (attr[x + 5] << 40);
                            if (parentID == drive_id)
                            {
                                root_Drives[ith_drive]->addNewFile_Directory(f);
                                avalable_father_folder[f->NTFS_Get_ID()] = 1;
                            }
                            else
                            {
                                if (avalable_father_folder[parentID] == 1)
                                {
                                    Directory* d = root_Drives[ith_drive]->NTFS_Find_Parent_Directory(parentID);
                                    if (d != NULL)
                                    {
                                        d->addNewFile_Directory(f);
                                        avalable_father_folder[f->NTFS_Get_ID()] = 1;
                                    }
                                }
                                else
                                {
                                    delete f;
                                    delete[] attr;
                                    break;
                                }
                            }

                        }
                    }
                    else if (h.type_id == 176 && cnt == 0)
                    {
                        end = NTFS_Read_BITMAP(attr, h);
                    }
                    started_byte += size;
                    delete[] attr;
                }
            }

        }
        main_mft_offset_byte += (long long)vbr.byte_per_MFT_entry;
        cnt++;
        delete[] mft;
    }
    CloseHandle(hDrive);
}

void File::NTFS_Nonresident_Read_Data(std::wstring drivePath, NTFS_VBR vbr)
{
    HANDLE hDrive = CreateFile(drivePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDrive == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to open physical drive." << std::endl;
        return;
    }

    while (nonresidentinfo.datasize > 0)
    {
        std::vector <BYTE> Data;
        DWORD lowOffset = static_cast<DWORD>(nonresidentinfo.offset & 0xFFFFFFFF);
        DWORD highOffset = static_cast<DWORD>((nonresidentinfo.offset >> 32) & 0xFFFFFFFF);
        DWORD result = SetFilePointer(hDrive, lowOffset, reinterpret_cast<PLONG>(&highOffset), FILE_BEGIN);
        if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
            std::cerr << "Failed to set file pointer." << std::endl;
            CloseHandle(hDrive);
            return;
        }
        DWORD bytesRead;
        BYTE* DATA = new BYTE[vbr.byte_per_sector * vbr.sector_per_cluster];
        if (!ReadFile(hDrive, DATA, vbr.byte_per_sector * vbr.sector_per_cluster, &bytesRead, NULL)) {
            std::wcerr << "Failed to read cluster from physical drive." << std::endl;
            CloseHandle(hDrive);
            delete[] DATA;
            return;
        }
        int cnt = 0;
        while (cnt < vbr.byte_per_sector * vbr.sector_per_cluster && cnt < nonresidentinfo.datasize)
        {
            if (DATA[cnt] == 0xFF)
            {
                if (DATA[cnt + 1] == 0xFF && DATA[cnt + 2] == 0xFF && DATA[cnt + 3] == 0xFF)
                {
                    Push_Data(Data);
                    delete[] DATA;
                    return;
                }
            }
            Data.push_back(DATA[cnt]);
            cnt++;
        }
        Push_Data(Data);
        nonresidentinfo.offset += (long long)(vbr.byte_per_sector * vbr.sector_per_cluster);
        nonresidentinfo.datasize -= (long long)(vbr.byte_per_sector * vbr.sector_per_cluster);
        delete[] DATA;
    }
    CloseHandle(hDrive);
}

int NTFS_Read_BITMAP(BYTE* attr, Header_Attribute h)
{
    BYTE* data_attr = new BYTE[h.size_of_attribute];
    std::copy(attr + h.attribute_data_offset, attr + h.attribute_data_offset + h.size_of_attribute, data_attr);
    for (long long i = h.size_of_attribute - 1; i >= 0; i--)
    {
        if (data_attr[i] == 0x01)
        {
            return i;
        }
    }
}
void NTFS_Create_Date_Time(BYTE* attr, Header_Attribute h, Date& d, Time& t)
{
    BYTE* data_attr = new BYTE[h.attribute_data_size];
    std::copy(attr + h.attribute_data_offset, attr + h.attribute_data_offset + h.attribute_data_size, data_attr);
    unsigned long long hexValue = 0;
    for (int i = 0; i < 8; ++i) {
        hexValue |= static_cast<unsigned long long>(data_attr[i]) << (i * 8);
    }
    unsigned long long nanoSeconds = hexValue * 100;

    long long epochTime1970 = 11644473600ULL;
    long long secondsSince1970 = static_cast<long long>((nanoSeconds) / 1000000000LL) - epochTime1970;

    time_t timeSince1970 = static_cast<time_t>(secondsSince1970);

    struct tm timeinfo;
    localtime_s(&timeinfo, &timeSince1970);

    d.day = timeinfo.tm_mday;
    d.month = timeinfo.tm_mon + 1;
    d.year = timeinfo.tm_year + 1900;
    t.hour = timeinfo.tm_hour;
    t.minute = timeinfo.tm_min;
    t.second = timeinfo.tm_sec;
    delete[] data_attr;

}

std::string NTFS_Create_Name(BYTE* attr, Header_Attribute h)
{
    std::ostringstream oss;
    BYTE* data_attr = new BYTE[h.attribute_data_size];
    std::copy(attr + h.attribute_data_offset, attr + h.attribute_data_offset + h.attribute_data_size, data_attr);
    int length_name = data_attr[64];
    for (int i = 0; i < length_name * 2 - 1; i++)
    {
        if (data_attr[66 + i] != 0x00)
            oss << std::hex << data_attr[66 + i];
    }
    std::string str = oss.str();
    delete[] data_attr;
    return oss.str();
}

Directory* Drive::NTFS_Find_Parent_Directory(int parent_id)
{
    for (int i = 0; i < rootDirectories_Files.size(); i++)
    {
        if (dynamic_cast<Directory*>(rootDirectories_Files[i]))
        {
            if (static_cast<Directory*>(rootDirectories_Files[i])->NTFS_Get_ID() == parent_id)
                return static_cast<Directory*>(rootDirectories_Files[i]);
            Directory* d = static_cast<Directory*>(rootDirectories_Files[i])->NTFS_Find_Parent_Directory(parent_id);
            if (d)
                return d;
        }
    }
    return NULL;
}

Directory* Directory::NTFS_Find_Parent_Directory(int parent_id)
{
    if (NTFS_Get_ID() == parent_id)
    {
        return this;
    }

    for (int i = 0; i < contents.size(); i++)
    {
        if (dynamic_cast<Directory*>(contents[i]))
        {
            return dynamic_cast<Directory*>(contents[i])->NTFS_Find_Parent_Directory(parent_id);
        }
    }
    return NULL;
}

void File::NTFS_Read_Non_Resident_Info(BYTE* attr, NTFS_VBR vbr, int datarun_offset, std::wstring drivePath, long long datasize)
{
    BYTE* data_attr = new BYTE[10];
    std::copy(attr + datarun_offset, attr + datarun_offset + 10, data_attr);
    // Doc datarun offset
    int firstDigit = (data_attr[0] >> 4) & 0xF;
    int secondDigit = data_attr[0] & 0xF;
    int x = 0, first_cluster = 0;
    for (int i = 0; i < secondDigit; i++)
    {
        x |= static_cast<int>(data_attr[1 + i + i] << (i * 8));
    }
    for (int i = 0; i < firstDigit; i++)
    {
        first_cluster |= static_cast<int>(data_attr[1 + secondDigit + i] << (i * 8));
    }
    long long offset = (long long)vbr.byte_per_sector * vbr.sector_per_cluster * first_cluster;
    std::vector <BYTE> data;
    nonresidentinfo.datasize = datasize;
    nonresidentinfo.offset = offset;
}

void File::NTFS_Read_Resident_Data(BYTE* attr, Header_Attribute h)
{
    BYTE* data_attr = new BYTE[h.attribute_data_size];
    std::copy(attr + h.attribute_data_offset, attr + h.attribute_data_offset + h.attribute_data_size, data_attr);
    int size = h.attribute_data_size;
    std::vector <BYTE> data;
    for (int i = 0; i < size; i++)
    {
        data.push_back(data_attr[i]);
    }
    Push_Data(data);
    byteArrayToString();
}