#include "Header.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    Qt_GUI main_screen(NULL);

    std::unique_ptr<Computer> MyPC = std::make_unique<Computer>();
    MyPC->detectFormat();
    MyPC->readDrives();
    MyPC->make_GUI(main_screen);

    main_screen.show();
    return a.exec();
}