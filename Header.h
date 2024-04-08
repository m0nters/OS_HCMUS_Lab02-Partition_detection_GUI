#ifndef FILESYSTEMENTITY_H
#define FILESYSTEMENTITY_H
#pragma once
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <QStandardItemModel>
#include <QFileSystemModel> 
#include <QtWidgets/QApplication>
#include <QDir>
#include <map>
#include <codecvt>
#include "Qt_GUI.h"




class Drive;
class Computer;
struct Date
{
	int day, month, year;
};
struct Time
{
	int hour, minute, second, milisecond = 0;
};
struct FAT32_BOOTSECTOR
{
	int byte_per_sector;
	int sector_per_cluster;
	int sector_before_FAT_table;
	int num_of_FAT_tables;
	int Volume_size;
	int sector_per_FAT;
	int first_cluster_of_RDET;
};

struct NTFS_VBR
{
	int byte_per_sector;
	int sector_per_cluster;
	int sum_sector_of_drive;
	int started_cluster_of_MFT;
	int started_cluster_of_extra_MFT;
	int byte_per_MFT_entry;
};

struct Header_MFT_Entry
{
	int started_attribute_offset;
	int flag;
	int byte_used;
	int byte_of_MFT_entry;
	int ID;
};

struct Header_Attribute
{
	long long type_id; // Ma loai
	int size_of_attribute;
	int flag_non_resident;
	int length_name_attribute;
	int offset_of_name;
	int flags;
	int attribute_id;
	int attribute_data_size;
	int attribute_data_offset;
};


struct NTFS_MFT
{
	Header_MFT_Entry MFT_header;
	std::vector <Header_Attribute> header_attributes;
};

struct NonResidentInfo
{
	long long offset;
	long long datasize;
};
class FileSystemEntity {
protected:
	std::string name;
	std::string attributes;
	Date date_created;
	Time time_created;
	std::vector <int> cluster_pos;
	long long total_size = 0; // in byte
	std::vector <std::vector<BYTE>>data;

	NTFS_MFT mft;
	std::string text;
	NonResidentInfo nonresidentinfo;
public:
	FileSystemEntity() {}

	std::wstring utf16_to_wstring(const std::string& str);

	void setAttributes(std::string attributes) { this->attributes = attributes; }
	std::string getAttributes() { return attributes; }

	void setName(std::string name) { this->name = name; }
	std::string getName() { return name; }

	void setTime(Time t) { this->time_created = t; }
	void setDate(Date d) { this->date_created = d; }

	std::string getTime_str() {
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << time_created.hour << ":"
			<< std::setw(2) << std::setfill('0') << time_created.minute << ":"
			<< std::setw(2) << std::setfill('0') << time_created.second << ":"
			<< std::setw(3) << std::setfill('0') << time_created.milisecond;
		return oss.str();
	}
	std::string getDate_str() {
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << date_created.day << "/"
			<< std::setw(2) << std::setfill('0') << date_created.month << "/"
			<< date_created.year; // No leading zero for year
		return oss.str();
	}

	std::string get_txt_content() {
		return text;
	}

	void add_cluster_pos(int pos) { cluster_pos.push_back(pos); }
	int get_pos_cluster(int index) { return cluster_pos[index]; }

	std::string getTotalSize_str() {
		// round 2 decimal numbers
		long long byte_size = getTotalSize();
		if (byte_size < 1024) {
			return std::to_string(byte_size) + " B";
		}
		else if (byte_size < 1024 * 1024) {
			double num = (double)byte_size / 1024;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " KB";
		}
		else if (byte_size < 1024 * 1024 * 1024) {
			double num = (double)byte_size / (1024 * 1024);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " MB";
		}
		else {
			double num = (double)byte_size / (1024 * 1024 * 1024);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " GB";
		}
	}

	void Push_Data(std::vector<BYTE>DATA) { data.push_back(DATA); }

	void FAT32_Get_Cluster_Pos(std::vector <int>& vec)
	{
		vec = std::vector<int>{};
		for (int i = 0; i < cluster_pos.size(); i++)
		{
			vec.push_back(cluster_pos[i]);
		}
	}

	virtual void NTFS_Set_MFT(Header_MFT_Entry mft_header)
	{
		this->mft.MFT_header = mft_header;
	}
	NTFS_MFT getMFT() { return mft; }
	int NTFS_Get_ID() { return mft.MFT_header.ID; }
	void pushHeaderAttribute(Header_Attribute h) { mft.header_attributes.push_back(h); }

	void setTotalSize(long long size) { this->total_size = size; }
	long long getTotalSize() { return total_size; }
	void FAT32_Read_Next_Sector(Drive* dr, std::wstring drivePath);

	virtual void displayInfo() {};
	virtual ~FileSystemEntity() {};
};


class File : public FileSystemEntity {
private:
public:
	File() = default;
	void displayInfo() override {
		std::cout << "File: " << getName() << "; ";
		std::cout << "Date created: " << date_created.day << "/" << date_created.month << "/" << date_created.year << "; ";
		std::cout << "Time created: " << time_created.hour << ":" << time_created.minute << ":" << time_created.second << ":" << time_created.milisecond << "; ";
		std::cout << "Total size: " << total_size << "; ";
		for (int i = 0; i < cluster_pos.size(); i++)
		{
			std::cout << cluster_pos[i] << "; ";
		}
		std::cout << std::endl;
		std::cout << text << std::endl;
	}
	void setTotalSize(long long size) { this->total_size = size; }
	long long getTotalSize() { return total_size; }

	void FAT32_Read_Next_Sector(Drive* dr, std::wstring drivePath)
	{
		FileSystemEntity::FAT32_Read_Next_Sector(dr, drivePath);
	}
	void FAT32_Read_Data(Drive* dr, std::wstring drivePath);
	void byteArrayToString();
	void NTFS_Set_MFT(Header_MFT_Entry mft_header)
	{
		FileSystemEntity::NTFS_Set_MFT(mft_header);
	}

	void NTFS_Read_Resident_Data(BYTE* attr, Header_Attribute h); // Su dung trong Computer::NTFS_Read_MFT()
	void NTFS_Read_Non_Resident_Info(BYTE* attr, NTFS_VBR vbr, int datarun_offset, std::wstring drivePath, long long datasize);
	void NTFS_Nonresident_Read_Data(std::wstring drivePath, NTFS_VBR vbr); // Point den cluster chua data o ngoai vung MFT va xu ly du lieu
	void NTFS_read_data(std::wstring drivePath, NTFS_VBR vbr) // Doc nhung du lieu chua duoc doc trong MFT (Nonresident)
	{
		for (int i = 0; i < mft.header_attributes.size(); i++)
		{

			if (mft.header_attributes[i].type_id == 128 && mft.header_attributes[i].flag_non_resident == 1)
			{
				NTFS_Nonresident_Read_Data(drivePath, vbr);
				byteArrayToString(); // bo du lieu vo string text
			}
		}
	}
};


class Directory : public FileSystemEntity {
private:
	std::vector<FileSystemEntity*> contents;
public:
	Directory() = default;


	void displayInfo() override {
		std::cout << "Directory: " << getName() << "; ";
		std::cout << "Date created: " << date_created.day << "/" << date_created.month << "/" << date_created.year << "; ";
		std::cout << "Time created: " << time_created.hour << ":" << time_created.minute << ":" << time_created.second << ":" << time_created.milisecond << "; ";
		std::cout << "Total size: " << total_size << "; ";
		for (int i = 0; i < cluster_pos.size(); i++)
		{
			std::cout << cluster_pos[i] << ", ";
		}
		std::cout << std::endl;
		std::cout << contents.size() << std::endl;
		for (int i = 0; i < contents.size(); i++)
		{
			contents[i]->displayInfo();
		}
	}
	void makeGUI(Qt_GUI& w, QTreeWidgetItem*& folder)
	{
		int ith_drive = folder->data(0, Qt::UserRole + 1).toInt(); // call out the ith_drive data of the parent folder
		for (int i = 0; i < contents.size(); i++)
		{
			Directory* dir_entity = dynamic_cast<Directory*>(contents[i]);
			if (dir_entity)
			{
				QTreeWidgetItem* dir_item = new QTreeWidgetItem();
				w.QTreeWidgetItem_populate_info(
					dir_item,
					QString::fromStdString(dir_entity->getName()),
					QString::fromStdString(dir_entity->getDate_str()),
					QString::fromStdString(dir_entity->getTime_str()),
					QString::fromStdString(dir_entity->getTotalSize_str())
				); // folder ko co size
				folder->addChild(dir_item);
				dir_item->setIcon(0, QIcon(".\\folder_icon.png"));
				// ========================================================================================
				dir_item->setData(0, Qt::UserRole + 1, QVariant(ith_drive)); // save ith_drive where the folder belongs to
				dir_item->setData(0, Qt::UserRole + 2, QVariant(QString::fromStdString(dir_entity->getName()))); // save name_file
				// ========================================================================================
				dir_entity->makeGUI(w, dir_item);
			}
			else
			{
				File* file_entity = dynamic_cast<File*>(contents[i]);
				if (file_entity)
				{
					QTreeWidgetItem* file_item = new QTreeWidgetItem();
					w.QTreeWidgetItem_populate_info(
						file_item,
						QString::fromStdString(file_entity->getName()),
						QString::fromStdString(file_entity->getDate_str()),
						QString::fromStdString(file_entity->getTime_str()),
						QString::fromStdString(file_entity->getTotalSize_str())
					);
					folder->addChild(file_item);
					if (file_entity->get_txt_content() != "")
						file_item->setData(0, Qt::UserRole, QVariant(QString::fromStdString(file_entity->get_txt_content())));
					// ========================================================================================
					file_item->setData(0, Qt::UserRole + 1, QVariant(ith_drive)); // save ith_drive where the file belongs to
					file_item->setData(0, Qt::UserRole + 2, QVariant(QString::fromStdString(file_entity->getName()))); // save name_file
					// ========================================================================================
					file_item->setIcon(0, QIcon(".\\txt_icon.png"));
				}
			}
		}
	}

	long long getTotalSize() {
		long long totalsize = 0; // because folder_entity can be called many times, so we need to reset totalsize as 0 for each new folder call
		for (int i = 0; i < contents.size(); i++)
		{
			if (dynamic_cast<Directory*>(contents[i]))
			{
				totalsize += dynamic_cast<Directory*>(contents[i])->getTotalSize();
			}
			else
			{
				totalsize += dynamic_cast<File*>(contents[i])->getTotalSize();
			}
		}
		total_size = totalsize;
		return total_size;
	}

	std::string getTotalSize_str() {
		// round 2 decimal numbers
		long long byte_size = getTotalSize();
		if (byte_size < 1024) {
			return std::to_string(byte_size) + " B";
		}
		else if (byte_size < 1024 * 1024) {
			double num = (double)byte_size / 1024;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " KB";
		}
		else if (byte_size < 1024 * 1024 * 1024) {
			double num = (double)byte_size / (1024 * 1024);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " MB";
		}
		else {
			double num = (double)byte_size / (1024 * 1024 * 1024);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " GB";
		}
	}

	void addNewFile_Directory(FileSystemEntity* f)
	{
		contents.push_back(f);
	}

	void FAT32_Read_Next_Sector(Drive* dr, std::wstring drivePath)
	{
		FileSystemEntity::FAT32_Read_Next_Sector(dr, drivePath);
	}

	void FAT32_Read_Data(Drive* dr, std::wstring drivePath)
	{
		FAT32_ReadDirectoryData(dr, drivePath);
		for (int i = 0; i < contents.size(); i++)
		{
			if (dynamic_cast<Directory*>(contents[i]))
			{
				static_cast<Directory*>(contents[i])->FAT32_Read_Data(dr, drivePath);
			}
			else
			{
				static_cast<File*>(contents[i])->FAT32_Read_Data(dr, drivePath);
			}
		}
	}
	void FAT32_ReadDirectoryData(Drive* dr, std::wstring drivePath);
	void NTFS_Set_MFT(Header_MFT_Entry mft_header)
	{
		FileSystemEntity::NTFS_Set_MFT(mft_header);
	}
	Directory* NTFS_Find_Parent_Directory(int parent_id);
	void NTFS_Read_Data(std::wstring drivePath, NTFS_VBR vbr)
	{
		for (int i = 0; i < contents.size(); i++)
		{
			if (dynamic_cast<File*>(contents[i]))
			{
				static_cast<File*>(contents[i])->NTFS_read_data(drivePath, vbr);
			}
		}
	}

	bool FAT32_Remove_File(std::wstring drivePath, std::string name_file, FAT32_BOOTSECTOR bs, Computer& MyPC);

	~Directory()
	{
		for (int i = 0; i < contents.size(); i++)
			delete contents[i];
	}
};



class Drive {
private:
	std::string name;
	std::string type;
	int started_byte_bootsector;
	int started_byte_rdet;
	std::vector<FileSystemEntity*> rootDirectories_Files;
	FAT32_BOOTSECTOR fat32bs;
	NTFS_VBR vbr;
	long long total_size = 0; // in byte
	std::wstring drivePath;

public:
	void set_fat32_bootsector(int byte_per_sector, int sector_per_cluster, int sector_before_FAT_table, int num_of_FAT_tables, int Volume_size, int sector_per_FAT, int first_cluster_of_RDET)
	{
		this->fat32bs.byte_per_sector = byte_per_sector;
		this->fat32bs.sector_per_cluster = sector_per_cluster;
		this->fat32bs.sector_before_FAT_table = sector_before_FAT_table;
		this->fat32bs.num_of_FAT_tables = num_of_FAT_tables;
		this->fat32bs.Volume_size = Volume_size;
		this->fat32bs.sector_per_FAT = sector_per_FAT;
		this->fat32bs.first_cluster_of_RDET = first_cluster_of_RDET;
		this->started_byte_rdet = (sector_per_FAT * num_of_FAT_tables + sector_before_FAT_table) * byte_per_sector;
	}

	void set_ntfs_vbr(int byte_per_sector, int sector_per_cluster, int sum_sector_of_drive, int started_cluster_of_MFT, int started_cluster_of_extra_MFT, int byte_per_MFT_entry) {
		this->vbr.byte_per_sector = byte_per_sector;
		this->vbr.sector_per_cluster = sector_per_cluster;
		this->vbr.sum_sector_of_drive = sum_sector_of_drive;
		this->vbr.started_cluster_of_MFT = started_cluster_of_MFT;
		this->vbr.started_cluster_of_extra_MFT = started_cluster_of_extra_MFT;
		this->vbr.byte_per_MFT_entry = byte_per_MFT_entry;
	}

	NTFS_VBR getVBRIn4() { return vbr; }

	FAT32_BOOTSECTOR getBootSectorIn4() { return fat32bs; }

	void setName(std::string name)
	{
		this->name = name;
	}
	std::string getName() { return this->name; }
	void setType(std::string type)
	{
		this->type = type;
	}
	std::string getType() { return this->type; }
	void setStartedByte(int started_byte_bootsector)
	{
		this->started_byte_bootsector = started_byte_bootsector;
	}
	int getStartedByte() { return started_byte_bootsector; }
	int getStartedByteRDET() { return started_byte_rdet; }
	void addNewFile_Directory(FileSystemEntity* f)
	{
		rootDirectories_Files.push_back(f);
	}
	void DisplayInfo() // for console debug only
	{
		std::cout << "Drive " << name << " " << type << std::endl;
		for (int i = 0; i < rootDirectories_Files.size(); i++)
		{
			Directory* dir = dynamic_cast<Directory*>(rootDirectories_Files[i]);
			if (dir)
			{
				dir->displayInfo();
			}
			else
			{
				File* file = dynamic_cast<File*>(rootDirectories_Files[i]);
				if (file)
				{
					file->displayInfo();
				}
			}
		}
	}
	void make_GUI(Qt_GUI& w, QTreeWidgetItem*& Drive)
	{
		int ith_dirve = QVariant(Drive->data(0, Qt::UserRole + 1)).toInt(); // call out the ith_drive
		for (int i = 0; i < rootDirectories_Files.size(); i++)
		{
			Directory* dir_entity = dynamic_cast<Directory*>(rootDirectories_Files[i]);
			if (dir_entity) // is folder
			{
				QTreeWidgetItem* dir_item = new QTreeWidgetItem();
				w.QTreeWidgetItem_populate_info(
					dir_item,
					QString::fromStdString(dir_entity->getName()),
					QString::fromStdString(dir_entity->getDate_str()),
					QString::fromStdString(dir_entity->getTime_str()),
					QString::fromStdString(dir_entity->getTotalSize_str())
				);  // folder ko co size
				Drive->addChild(dir_item);
				dir_item->setIcon(0, QIcon(".\\folder_icon.png"));
				// ========================================================================================
				dir_item->setData(0, Qt::UserRole + 1, QVariant(ith_dirve)); // save ith_drive where the folder belongs to
				dir_item->setData(0, Qt::UserRole + 2, QVariant(QString::fromStdString(dir_entity->getName()))); // save name_file
				// ========================================================================================
				dir_entity->makeGUI(w, dir_item);
			}
			else // is file
			{
				File* file_entity = dynamic_cast<File*>(rootDirectories_Files[i]);
				if (file_entity)
				{
					QTreeWidgetItem* file_item = new QTreeWidgetItem();
					w.QTreeWidgetItem_populate_info(
						file_item,
						QString::fromStdString(file_entity->getName()),
						QString::fromStdString(file_entity->getDate_str()),
						QString::fromStdString(file_entity->getTime_str()),
						QString::fromStdString(file_entity->getTotalSize_str())
					);
					Drive->addChild(file_item);
					if (file_entity->get_txt_content() != "")
						file_item->setData(0, Qt::UserRole, QVariant(QString::fromStdString(file_entity->get_txt_content())));
					// ========================================================================================
					file_item->setData(0, Qt::UserRole + 1, QVariant(ith_dirve)); // save ith_drive where the file belongs to
					file_item->setData(0, Qt::UserRole + 2, QVariant(QString::fromStdString(file_entity->getName()))); // save name_file
					// ========================================================================================
					file_item->setIcon(0, QIcon(".\\txt_icon.png"));
				}
			}
		}
	}


	void setToTalSize()
	{
		for (int i = 0; i < rootDirectories_Files.size(); i++)
		{
			if (dynamic_cast<Directory*>(rootDirectories_Files[i]))
			{
				total_size += dynamic_cast<Directory*>(rootDirectories_Files[i])->getTotalSize();
			}
			else
			{
				total_size += dynamic_cast<File*>(rootDirectories_Files[i])->getTotalSize();
			}
		}
	}
	long long getTotalSize() { return total_size; }

	std::string getTotalSize_str() {
		// round 2 decimal numbers
		long long byte_size = getTotalSize();
		if (byte_size < 1024) {
			return std::to_string(byte_size) + " B";
		}
		else if (byte_size < 1024 * 1024) {
			double num = (double)byte_size / 1024;
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " KB";
		}
		else if (byte_size < 1024 * 1024 * 1024) {
			double num = (double)byte_size / (1024 * 1024);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " MB";
		}
		else {
			double num = (double)byte_size / (1024 * 1024 * 1024);
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << num;
			return ss.str() + " GB";
		}
	}


	void FAT32_Read_Data(std::wstring drivePath)
	{
		for (int i = 0; i < rootDirectories_Files.size(); i++)
		{
			if (dynamic_cast<Directory*>(rootDirectories_Files[i]))
			{
				static_cast<Directory*>(rootDirectories_Files[i])->FAT32_Read_Data(this, drivePath);
			}
			else
			{
				static_cast<File*>(rootDirectories_Files[i])->FAT32_Read_Data(this, drivePath);
				static_cast<File*>(rootDirectories_Files[i])->byteArrayToString();
			}
		}
	}
	void NTFS_Read_Data(std::wstring drivePath)
	{

		for (int i = 0; i < rootDirectories_Files.size(); i++)
		{
			if (dynamic_cast<Directory*>(rootDirectories_Files[i]))
			{
				static_cast<Directory*>(rootDirectories_Files[i])->NTFS_Read_Data(drivePath, vbr);
			}
			else
			{
				static_cast<File*>(rootDirectories_Files[i])->NTFS_read_data(drivePath, vbr);
			}
		}
	}
	Directory* NTFS_Find_Parent_Directory(int parent_id);

	bool FAT32_Remove_File(std::wstring drivePath, std::string name_file, Computer& MyPC);
	void setDrivePath(std::wstring drivePath) { this->drivePath = drivePath; };
	std::wstring getDrivePath() { return drivePath; }
	~Drive()
	{
		for (int i = 0; i < rootDirectories_Files.size(); i++)
			delete rootDirectories_Files[i];
	}
};

class Computer {
private:
	std::map<std::string, std::pair<long long, int>> offset_recover_started_rdet_entry;
	std::map<std::string, std::pair<int, int>> offset_recover_started_sdet_entry; // pair: .firt - offset cua file bi xoa trong sdet, .second - offset trong sdet
	std::map<std::string, std::vector<BYTE>> started_byte_rdet_sdet;
	std::vector<Drive*> root_Drives;
	std::map<std::string, int> drive_to_order_map;
public:
	std::string capitalizeString(const std::string& str) {
		std::string result = str;
		for (char& c : result) {
			c = toupper(c);
		}
		return result;
	}
	int getOrderDrive(std::string target) {
		std::string str_to_find = capitalizeString(target);
		auto it = drive_to_order_map.find(str_to_find);
		if (it != drive_to_order_map.end()) {
			return drive_to_order_map[str_to_find];
		}
		else {
			return -1; // in default, if the key is not in the map, the map will automatically return 0, but now we change it to -1
		}
	}
	void setMapRDET(std::vector<BYTE> started_byte, long long rdet_offset, int first_entry_offset, std::string name_file)
	{
		for (int i = 0; i < started_byte.size(); i++)
		{
			this->started_byte_rdet_sdet[name_file].push_back(started_byte[i]);
		}
		offset_recover_started_rdet_entry[name_file].first = rdet_offset;
		offset_recover_started_rdet_entry[name_file].second = first_entry_offset;
	}
	void setMapSDET(std::vector<BYTE> started_byte, int total_offset, int sdet_offset, std::string name_file)
	{
		for (int i = 0; i < started_byte.size(); i++)
		{
			this->started_byte_rdet_sdet[name_file].push_back(started_byte[i]);
		}
		offset_recover_started_sdet_entry[name_file].first = total_offset;
		offset_recover_started_sdet_entry[name_file].second = sdet_offset;
	}

	void addRootDrive(Drive*& d) {
		root_Drives.push_back(d);
	}
	void readDrives();
	void FAT32_Read_BootSector(int ith_drive, std::wstring drivePath);
	void FAT32_Read_RDET(int ith_drive, std::wstring drivePath);

	void NTFS_Read_VBR(int ith_drive, std::wstring drivePath);
	void NTFS_Read_MFT(int ith_drive, std::wstring drivePath);

	void detectFormat();
	void GetRemovableDriveNames();
	void DisplayInfo()
	{
		for (int i = 0; i < root_Drives.size(); i++)
		{
			root_Drives[i]->DisplayInfo();
		}
	}
	void readData(std::wstring drivePath)
	{
		for (int i = 0; i < root_Drives.size(); i++)
		{

			if (root_Drives[i]->getType() == "FAT32")
			{
				root_Drives[i]->FAT32_Read_Data(drivePath);
			}
			else
			{
				root_Drives[i]->NTFS_Read_Data(drivePath);
			}
		}
	}
	void make_GUI(Qt_GUI& w)
	{
		for (int i = 0; i < root_Drives.size(); i++)
		{
			QTreeWidgetItem* drive_item = new QTreeWidgetItem;
			w.QTreeWidgetItem_populate_info(
				drive_item,
				QString::fromStdString(root_Drives[i]->getName()),
				"",
				"",
				QString::fromStdString(root_Drives[i]->getTotalSize_str())
			);
			w.get_current_treeWidget()->addTopLevelItem(drive_item);
			drive_item->setIcon(0, QIcon(".\\drive_icon.png"));
			drive_item->setData(0, Qt::UserRole + 1, QVariant(i)); // save the ith_drive data for each drive
			root_Drives[i]->make_GUI(w, drive_item);
		}
	}



	bool FAT32_Remove_File(int ith_drive, std::string name_file);
	bool FAT32_Recover_File(int ith_drive, std::string name_file);
};


int littleEndianByteArrayToInt(const BYTE* byteArray, size_t length);
std::wstring stringToWideString(const std::string& str);
std::string FAT32_Create_Name(std::vector <std::vector <BYTE>> extra_entry, std::vector <BYTE> main_entry);
std::string NTFS_Create_Name(BYTE* attr, Header_Attribute h);
Date FAT32_Create_Date(std::vector <BYTE> main_entry);
Time FAT32_Create_Time(std::vector <BYTE> main_entry);
void NTFS_Create_Date_Time(BYTE* attr, Header_Attribute h, Date& d, Time& t);
int byteToTwosComplement(int byteValue);
int NTFS_Read_BITMAP(BYTE* attr, Header_Attribute h);
#endif