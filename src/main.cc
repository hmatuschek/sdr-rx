#include "gui/gui.hh"

#include <QApplication>

#include "receiver.hh"
#include "mainwindow.hh"


using namespace sdr;

int main(int argc, char *argv[]) {

  QApplication application(argc, argv);

  // Install log message handler:
  sdr::Logger::get().addHandler(new sdr::StreamLogHandler(std::cerr, sdr::LOG_DEBUG));

  // Instantiate Receiver
  Receiver receiver;
  // Receiver view
  MainWindow win(&receiver);
  win.show();

  // Start...
  application.exec();
  // Stop...
  receiver.stop();
  // done...
  return 0;
}
