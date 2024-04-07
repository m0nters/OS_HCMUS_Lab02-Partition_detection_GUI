#include "Qt_GUI.h"
#include "Header.h"
#include <QDesktopServices>

Qt_GUI::Qt_GUI(QWidget* parent)
	: QMainWindow(parent)
{

	ui.setupUi(this);
	ui.treeWidget->setHeaderLabels({ "Name", "Date created\n(dd/mm/yyyy)", "Time created\n(hh:mm:ss:ms)", "Total Size" });
	
	// left click to .txt file to show its content
	connect(ui.treeWidget, &QTreeWidget::itemClicked, this, &Qt_GUI::onTreeItemClicked);
	// Connect signals to slots for auto resizing columns
	connect(ui.treeWidget, &QTreeWidget::itemExpanded, this, &Qt_GUI::resizeColumnsToContents);
	connect(ui.treeWidget, &QTreeWidget::itemCollapsed, this, &Qt_GUI::resizeColumnsToContents);
}

Qt_GUI::~Qt_GUI()
{}

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
	int ith_drive = 0;
	std::string name_file;
	if (selectedItem) {
		ith_drive = selectedItem->data(0, Qt::UserRole + 1).toInt();
		name_file = selectedItem->data(0, Qt::UserRole + 2).toString().toStdString();
	}
	Computer clone_pc;
	clone_pc.detectFormat();
	clone_pc.readDrives();
	clone_pc.FAT32_Remove_File(ith_drive, name_file);
}

void Qt_GUI::on_recover_button_clicked() {
	int ith_drive = 0;
	std::string name_file;
	if (selectedItem) {
		ith_drive = selectedItem->data(0, Qt::UserRole + 1).toInt();
		name_file = selectedItem->data(0, Qt::UserRole + 2).toString().toStdString();
	}
	Computer clone_pc;
	clone_pc.detectFormat();
	clone_pc.readDrives();
	clone_pc.FAT32_Recover_File(ith_drive, name_file);
}