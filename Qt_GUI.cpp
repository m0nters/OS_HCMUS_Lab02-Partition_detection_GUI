#include "Header.h"
#include <QDesktopServices>
#include <regex>
#include <qmessagebox.h>
#include <unordered_set>
#include <qcheckbox.h>

std::shared_ptr<Computer> clone_pc = std::make_shared<Computer>(); // mimic the backend data of current Computer instance in main() to alter the GUI

const std::unordered_set<std::string> system_files_names = {
		"System Volume Information",
		"$MFT",
		"$MFTMirr",
		"$LogFile",
		"$Volume",
		"$AttrDef",
		".",
		"$Bitmap",
		"$Boot",
		"$BadClus",
		"$UpCase",
		"$Extend",
		"$Volume"
};

Qt_GUI::Qt_GUI(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.treeWidget->setHeaderLabels({ "Name", "Date created\n(dd/mm/yyyy)", "Time created\n(hh:mm:ss:ms)", "Total Size",  "Other attributes" });
	ui.splitter->setSizes({ 300,25 });

	clone_pc->detectFormat();
	clone_pc->readDrives();

	// left click to .txt file to show its content
	connect(ui.treeWidget, &QTreeWidget::itemClicked, this, &Qt_GUI::onTreeItemClicked);

	// Connect signals to slots for auto resizing columns
	connect(ui.treeWidget, &QTreeWidget::itemExpanded, this, &Qt_GUI::resizeColumnsToContents);
	connect(ui.treeWidget, &QTreeWidget::itemCollapsed, this, &Qt_GUI::resizeColumnsToContents);

	// Connect textChanged signals of drive_input and name_input to onDriveOrFileNameChanged slot
	connect(ui.drive_input, &QLineEdit::textChanged, this, &Qt_GUI::onDriveOrFileNameChanged);
	connect(ui.name_input, &QLineEdit::textChanged, this, &Qt_GUI::onDriveOrFileNameChanged);

	// for event show system files checkbox tick/untick
	connect(ui.allow_system_files_checkbox, &QCheckBox::stateChanged, this, &Qt_GUI::onShowSystemFilesStateChanged);
}

Qt_GUI::~Qt_GUI()
{
}

void Qt_GUI::QTreeWidgetItem_populate_info(QTreeWidgetItem*& node, QString name, QString date_created, QString time_created, QString total_size, QString other_attributes) {
	node->setText(0, name);
	node->setText(1, date_created);
	node->setText(2, time_created);
	node->setText(3, total_size);
	node->setText(4, other_attributes);
}

void Qt_GUI::onTreeItemClicked(QTreeWidgetItem* item, int column) {
	selectedItem = item; // Store the selected item
	ui.delete_button->setEnabled(selectedItem);
	QVariant raw_data = item->data(0, Qt::UserRole);
	QString data = raw_data.toString();
	if (!data.isEmpty()) {
		// add "please wait" in the file_content_box and during waiting if user press esc then stop the process, if stop the process, add "can't load the content" and never load the code below
		ui.file_content_box->setPlainText(data);
	}
	else
		ui.file_content_box->setPlainText("");
}

void Qt_GUI::resizeColumnsToContents() {
	for (int i = 0; i < ui.treeWidget->columnCount(); ++i) {
		ui.treeWidget->resizeColumnToContents(i);
		ui.treeWidget->setColumnWidth(i, ui.treeWidget->columnWidth(i) + 10); // Add 10 pixels for spacing aesthetic
	}
}

void Qt_GUI::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Delete)
	{
		on_delete_button_clicked(); // Call your delete button click event handler
	}
	else
	{
		// Call base class implementation for other key events
		QMainWindow::keyPressEvent(event);
	}
}

void Qt_GUI::on_delete_button_clicked() {
	if (!selectedItem) return; // if no item is selected, do nothing
	parent_selectedItem = selectedItem->parent();

	int ith_drive = selectedItem->data(0, Qt::UserRole + 1).toInt();
	std::string name_file = selectedItem->data(0, Qt::UserRole + 2).toString().toStdString(); // return empty if it's a drive item, which is fine since there is no way user can set foler/file's name empty
	if (deleted_map.find({ ith_drive, name_file }) != deleted_map.end()) // IN CASE OF SPAMMING DELETE BUTTON FOR 1 ITEM
		return;

	// change the real data
	bool success = clone_pc->FAT32_Remove_File(ith_drive, name_file);
	if (!success) { // this should only appear when you try to delete the drive item
		QMessageBox::critical(this, "Không thể xóa item đã chọn", "Không thể xóa bản thân partition bởi nó là một phần của ổ đĩa.");
		return;
	}

	// change the GUI
	// // update the deleted_list_box in GUI
	deleted_map[{ith_drive, name_file}] = { selectedItem, parent_selectedItem }; // create an identity mapping for each deleted node in case mass recovery in the future
	std::string previous_str = ui.deleted_list_box->toPlainText().toStdString();
	std::string str_to_add = "Drive " + std::to_string(ith_drive) + ": " + name_file + "\n";
	std::string current_str = previous_str + str_to_add;
	ui.deleted_list_box->setPlainText(QString::fromStdString(current_str));
	if (parent_selectedItem) {
		int childIndex = parent_selectedItem->indexOfChild(selectedItem);
		parent_selectedItem->takeChild(childIndex);
	}
	ui.file_content_box->clear();
}

bool is_nonneg_num(const std::string& s) {
	if (s.empty() || (!isdigit(s[0]))) {
		return false;
	}

	for (char c : s) {
		if (!isdigit(c)) {
			return false;
		}
	}

	return true;
}

void Qt_GUI::on_restore_button_clicked() {
	int ith_drive = 0;
	if (is_nonneg_num(ui.drive_input->text().toStdString())) // if the user inputs number, take that number
		ith_drive = ui.drive_input->text().toInt();
	else // if it's name (string), convert to the corresponding number using map
		ith_drive = clone_pc->getOrderDrive(ui.drive_input->text().toStdString());
	std::string name_file = ui.name_input->text().toStdString();

	// change real data
	bool success = clone_pc->FAT32_Recover_File(ith_drive, name_file);
	if (!success) {
		QMessageBox::critical(this, "Không thể restore item đã chọn", "Thông tin item chọn restore không khớp danh sách các thông tin tương ứng của các item đã xóa ở phiên hiện tại.\nHoặc có thể là item đã được restore trước đó rồi, hoặc vui lòng kiểm tra kĩ lại nhập liệu tên (hoặc số thứ tự) drive cần restore hoặc tên item cần restore");
		return;
	}

	// change GUI
	auto pair = deleted_map[{ith_drive, name_file}]; // take out pair <node, parent>, it exists since the backend code above ran successfully
	deleted_map.erase({ ith_drive, name_file }); // remove that pair out of the deleted_map, only to have a proper map to look up for the next time, but also make the map lighter
	QTreeWidgetItem* node = pair.first;
	QTreeWidgetItem* parent = pair.second;
	parent->addChild(node);

	// // update the deleted_list_box in GUI
	std::string current_str = ui.deleted_list_box->toPlainText().toStdString();
	std::string deleted_file_str = "Drive " + std::to_string(ith_drive) + ": " + name_file + "\n";
	size_t pos = current_str.find(deleted_file_str); // this should never fail since we have checked the existence condtion above
	current_str.erase(pos, deleted_file_str.length());
	ui.deleted_list_box->setPlainText(QString::fromStdString(current_str));
}

void Qt_GUI::onDriveOrFileNameChanged()
{
	if (!ui.drive_input->text().isEmpty() && !ui.name_input->text().isEmpty())
		ui.restore_button->setEnabled(true);
	else
		ui.restore_button->setEnabled(false);
}

void Qt_GUI::mousePressEvent(QMouseEvent* event) {
	// Check if the mouse press event occurs outside the tree widget
	if (!ui.treeWidget->geometry().contains(event->pos())) {
		selectedItem = nullptr;
		ui.delete_button->setEnabled(false);
		ui.treeWidget->clearSelection();
		ui.file_content_box->clear();
	}
}

void Qt_GUI::onShowSystemFilesStateChanged(int state) {
	static int run_time = 0;
	if (!run_time++) // because we can't put this code into constructor, fill the system_files vector for only 1 time to save resource
		for (int drive_idx = 0; drive_idx < ui.treeWidget->topLevelItemCount(); ++drive_idx) {
			system_files.push_back(std::vector<std::pair<QTreeWidgetItem*, QTreeWidgetItem*>>());
			QTreeWidgetItem* drive_node = ui.treeWidget->topLevelItem(drive_idx); // for each drive
			for (int item_idx = 0; item_idx < drive_node->childCount(); ++item_idx) { // take out each of its items (child)
				QTreeWidgetItem* item = drive_node->child(item_idx);
				if (system_files_names.find(item->text(0).toStdString()) != system_files_names.end()) { // if the item is a system file (match one of system files names)
					system_files[drive_idx].push_back({ item, drive_node });
				}
			}
		}

	if (state == Qt::Checked) {
		for (int drive_idx = 0; drive_idx < system_files.size(); ++drive_idx) {
			for (int pair_idx = 0; pair_idx < system_files[drive_idx].size(); ++pair_idx) {
				system_files[drive_idx][pair_idx].second->addChild(system_files[drive_idx][pair_idx].first);
			}
		}
	}
	else {
		for (int drive_idx = 0; drive_idx < system_files.size(); ++drive_idx) {
			for (int pair_idx = 0; pair_idx < system_files[drive_idx].size(); ++pair_idx) {
				int childIndex = system_files[drive_idx][pair_idx].second->indexOfChild(system_files[drive_idx][pair_idx].first);
				system_files[drive_idx][pair_idx].second->takeChild(childIndex);
			}
		}
	}
}