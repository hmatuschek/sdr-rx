#ifndef __SDR_RX_MAINWINDOW_HH__
#define __SDR_RX_MAINWINDOW_HH__

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>

#include "receiver.hh"


class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(Receiver *receiver, QWidget *parent = 0);
  virtual ~MainWindow();

protected slots:
  void onPlayClicked();
  void onReceiverStarted();
  void onReceiverStopped();

protected:
  Receiver *_receiver;
  QPushButton *_play;
};

#endif // __SDR_RX_MAINWINDOW_HH__
