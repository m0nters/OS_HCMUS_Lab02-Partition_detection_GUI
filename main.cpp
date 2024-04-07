#include "Header.h"

int main(int argc, char* argv[])
{
    Computer MyPC;
    MyPC.detectFormat();
    MyPC.readDrives();
    QApplication a(argc, argv);
    Qt_GUI main_screen(NULL);
    MyPC.make_GUI(main_screen); 
    main_screen.show();
    return a.exec();
}