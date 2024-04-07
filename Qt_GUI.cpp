#include "Qt_GUI.h"

Qt_GUI::Qt_GUI(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.treeWidget->setColumnCount(2);
	ui.treeWidget->setHeaderLabels({ "Name", "Date created\n(dd/mm/yyyy)", "Time created\n(hh:mm:ss:ms)", "Total Size" });
	ui.treeWidget->expandAll();
	connect(ui.treeWidget, &QTreeWidget::itemClicked, this, &Qt_GUI::onTreeItemClicked);

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
	QVariant raw_data = item->data(0, Qt::UserRole);
	QString data = raw_data.toString();
	if (!data.isEmpty())
		ui.plainTextEdit->setPlainText(data);
	else
		ui.plainTextEdit->setPlainText("");
}