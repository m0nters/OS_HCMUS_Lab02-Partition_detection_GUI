#ifndef QT_GUI_H
#define QT_GUI_H
#pragma once

#include "ui_Qt_GUI.h"
#include <QtWidgets/QMainWindow>
#include <QFileSystemModel> 

class Qt_GUI : public QMainWindow
{
    Q_OBJECT

public:
    Qt_GUI(QWidget *parent = nullptr);
    ~Qt_GUI();
    Ui::Qt_GUIClass* getUi() { return &ui; }

    QTreeWidget* get_current_treeWidget() {
        return ui.treeWidget;
    }

    void QTreeWidgetItem_populate_info(QTreeWidgetItem*& node, QString name, QString date_created, QString time_created, QString total_size);

public slots:
    void onTreeItemClicked(QTreeWidgetItem* item, int column); // left click to .txt file to show its content
    void resizeColumnsToContents(); // auto resize columns width when expand/collapse folders
private:
    Ui::Qt_GUIClass ui;
};
#endif