#include "Header.h"
#include <QDesktopServices>
#include <regex>

std::unique_ptr<Computer> clone_pc = std::make_unique<Computer>(); // alter the GUI dynamically

Qt_GUI::Qt_GUI(QWidget* parent)
	: QMainWindow(parent)
{
	clone_pc->detectFormat();
	clone_pc->readDrives();
	ui.setupUi(this);
	ui.treeWidget->setHeaderLabels({ "Name", "Date created\n(dd/mm/yyyy)", "Time created\n(hh:mm:ss:ms)", "Total Size" });
	ui.splitter->setSizes({ 300, 25 });

	// left click to .txt file to show its content
	connect(ui.treeWidget, &QTreeWidget::itemClicked, this, &Qt_GUI::onTreeItemClicked);
	// Connect signals to slots for auto resizing columns
	connect(ui.treeWidget, &QTreeWidget::itemExpanded, this, &Qt_GUI::resizeColumnsToContents);
	connect(ui.treeWidget, &QTreeWidget::itemCollapsed, this, &Qt_GUI::resizeColumnsToContents);
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
		ui.plainTextEdit->setPlainText(data);
	else
		ui.plainTextEdit->setPlainText("");
}

void Qt_GUI::resizeColumnsToContents() {
	for (int i = 0; i < ui.treeWidget->columnCount(); ++i) {
		ui.treeWidget->resizeColumnToContents(i);
		ui.treeWidget->setColumnWidth(i, ui.treeWidget->columnWidth(i) + 10); // Add 10 pixels for spacing aesthetic
	}
}

void Qt_GUI::on_delete_button_clicked() {
	if (selectedItem) {
		int ith_drive = selectedItem->data(0, Qt::UserRole + 1).toInt();
		std::string name_file = selectedItem->data(0, Qt::UserRole + 2).toString().toStdString();

		// update the delete_list
		std::string str_to_add = ui.delete_list->toPlainText().toStdString();
		str_to_add += "Partion " + std::to_string(ith_drive) + ": " + name_file + "\n";
		ui.delete_list->setPlainText(QString::fromStdString(str_to_add));

		// change the real data
		clone_pc->FAT32_Remove_File(ith_drive, name_file);

		// change the GUI
		parent_selectedItem = selectedItem->parent();
		if (parent_selectedItem) {
			int childIndex = parent_selectedItem->indexOfChild(selectedItem);
			parent_selectedItem->takeChild(childIndex);
		}
	}
}

bool isNumber(const std::string& s) {
	static const std::regex num_regex("\\s*[+-]?\\d+(\\.\\d+)?\\s*");
	return std::regex_match(s, num_regex);
}

void Qt_GUI::on_restore_button_clicked() {
	int ith_drive = 0;
	if (isNumber(ui.drive_input->text().toStdString()))
		ith_drive = ui.drive_input->text().toInt();
	else
		ith_drive = clone_pc->getOrderDrive(ui.drive_input->text().toStdString());

	std::string name_file = ui.file_name_input->text().toStdString();

	// update the delete_list
	std::string str_to_add = ui.delete_list->toPlainText().toStdString();
	std::string sub_str = "Partion " + std::to_string(ith_drive) + ": " + name_file + "\n";
	size_t pos = str_to_add.find(sub_str);
	if (pos != std::string::npos) {
		str_to_add.erase(pos, sub_str.length());
	}
	ui.delete_list->setPlainText(QString::fromStdString(str_to_add));

	// change real data
	clone_pc->FAT32_Recover_File(ith_drive, name_file);

	// change GUI
	if (parent_selectedItem && selectedItem) // for the case the user try to restore file deleted in the previous sessions
		parent_selectedItem->addChild(selectedItem); 
}