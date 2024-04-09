#include "Header.h"
#include <QDesktopServices>
#include <regex>
#include <qmessagebox.h>

std::unique_ptr<Computer> clone_pc = std::make_unique<Computer>(); // mimic the current Computer object in main() to alter the GUI

Qt_GUI::Qt_GUI(QWidget* parent)
	: QMainWindow(parent)
{
	clone_pc->detectFormat();
	clone_pc->readDrives();
	ui.setupUi(this);
	ui.treeWidget->setHeaderLabels({ "Name", "Date created\n(dd/mm/yyyy)", "Time created\n(hh:mm:ss:ms)", "Total Size" });
	ui.splitter->setSizes({ 300,25 });

	// left click to .txt file to show its content
	connect(ui.treeWidget, &QTreeWidget::itemClicked, this, &Qt_GUI::onTreeItemClicked);

	// Connect signals to slots for auto resizing columns
	connect(ui.treeWidget, &QTreeWidget::itemExpanded, this, &Qt_GUI::resizeColumnsToContents);
	connect(ui.treeWidget, &QTreeWidget::itemCollapsed, this, &Qt_GUI::resizeColumnsToContents);

	// Connect textChanged signals of drive_input and name_input to onDriveOrFileNameChanged slot
	connect(ui.drive_input, &QLineEdit::textChanged, this, &Qt_GUI::onDriveOrFileNameChanged);
	connect(ui.name_input, &QLineEdit::textChanged, this, &Qt_GUI::onDriveOrFileNameChanged);
}

Qt_GUI::~Qt_GUI()
{
	delete selectedItem;
	delete parent_selectedItem;
}

void Qt_GUI::QTreeWidgetItem_populate_info(QTreeWidgetItem*& node, QString name, QString date_created, QString time_created, QString total_size) {
	node->setText(0, name);
	node->setText(1, date_created);
	node->setText(2, time_created);
	node->setText(3, total_size);
}

void Qt_GUI::onTreeItemClicked(QTreeWidgetItem* item, int column) {
	selectedItem = item; // Store the selected item
	QVariant raw_data = item->data(0, Qt::UserRole);
	QString data = raw_data.toString();
	if (!data.isEmpty())
		ui.file_content_box->setPlainText(data);
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
	if (!selectedItem) return;

	parent_selectedItem = selectedItem->parent();

	int ith_drive = selectedItem->data(0, Qt::UserRole + 1).toInt();
	std::string name_file = selectedItem->data(0, Qt::UserRole + 2).toString().toStdString();
	if (deleted_map.find({ ith_drive, name_file }) != deleted_map.end()) // IN CASE OF SPAMMING DELETE BUTTON FOR 1 ITEM
		return;
	deleted_map[{ith_drive, name_file}] = { selectedItem, parent_selectedItem }; // create an identity mapping for each deleted node in case mass recovery in the future

	// update the deleted_list_box in GUI
	std::string previous_str = ui.deleted_list_box->toPlainText().toStdString();
	std::string str_to_add = "Drive " + std::to_string(ith_drive) + ": " + name_file + "\n";
	std::string current_str = previous_str + str_to_add;

	ui.deleted_list_box->setPlainText(QString::fromStdString(current_str));
	// change the real data
	bool success = clone_pc->FAT32_Remove_File(ith_drive, name_file);
	if (!success) { // this should never fail, but put it here just in case
		QMessageBox::critical(this, "Không thể xóa file đã chọn", "Đã xảy ra lỗi, vui lòng thử lại hoặc đóng chương trình và thử lại.");
		return;
	}

	// change the GUI
	if (parent_selectedItem) {
		int childIndex = parent_selectedItem->indexOfChild(selectedItem);
		parent_selectedItem->takeChild(childIndex);
	}
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
	else // if it's name (string), convert to the corresponding number
		ith_drive = clone_pc->getOrderDrive(ui.drive_input->text().toStdString());
	std::string name_file = ui.name_input->text().toStdString();

	// if the node to restore exists in the deleted_map, remove it from the map and restore it
	QTreeWidgetItem* node = nullptr;
	QTreeWidgetItem* parent = nullptr;
	if (deleted_map.find({ ith_drive, name_file }) != deleted_map.end()) {
		node = deleted_map[{ith_drive, name_file}].first;
		parent = deleted_map[{ith_drive, name_file}].second;
		deleted_map.erase({ ith_drive, name_file }); // not only to look up for the next time, but also make the map lighter
	}
	else {
		QMessageBox::critical(this, "Không thể restore file đã chọn", "Thông tin file chọn restore không khớp danh sách các thông tin tương ứng của các file đã xóa ở phiên hiện tại.\nHoặc có thể là file đã được restore trước đó rồi, hoặc vui lòng kiểm tra kĩ lại nhập liệu tên (hoặc số thứ tự) drive cần restore hoặc tên file cần restore");
		return; // IN CASE OF SPAMMING RESTORE BUTTON FOR 1 ITEM
	}

	// update the delete_list_box in GUI
	std::string current_str = ui.deleted_list_box->toPlainText().toStdString();
	std::string deleted_file_str = "Drive " + std::to_string(ith_drive) + ": " + name_file + "\n";
	size_t pos = current_str.find(deleted_file_str); // this should never fail since we have checked the existence condtion above
	current_str.erase(pos, deleted_file_str.length());
	ui.deleted_list_box->setPlainText(QString::fromStdString(current_str));

	// change real data
	bool success = clone_pc->FAT32_Recover_File(ith_drive, name_file);
	if (!success) { // this should never fail, again
		QMessageBox::critical(this, "Không thể restore file đã chọn", "Đã xảy ra lỗi, vui lòng thử lại hoặc đóng chương trình và thử lại.");
		return;
	}

	// change GUI
	parent->addChild(node);
}

void Qt_GUI::onDriveOrFileNameChanged()
{
	if (!ui.drive_input->text().isEmpty() && !ui.name_input->text().isEmpty())
		ui.restore_button->setEnabled(true);
	else
		ui.restore_button->setEnabled(false);
}