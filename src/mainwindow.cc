#include "mainwindow.hh"
#include "logger.hh"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QValidator>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QSplitter>


using namespace sdr;


MainWindow::MainWindow(Receiver *receiver, QWidget *parent)
  : QMainWindow(parent), _receiver(receiver)
{
  setWindowTitle("SDR-RX");

  _play = new QPushButton("play");
  _play->setCheckable(true);
  if (_receiver->isRunning()) { _play->setChecked(true); _play->setText("Stop"); }
  else { _play->setChecked(false); _play->setText("Start"); }

  QTabWidget *ctrls = new QTabWidget();
  ctrls->addTab(_receiver->createSourceCtrlView(), "Source");
  ctrls->addTab(_receiver->createDemodCtrlView(), "Demodulator");
  ctrls->addTab(_receiver->createAudioCtrlView(), "Audio");

  QObject::connect(_play, SIGNAL(clicked()), SLOT(onPlayClicked()));
  QObject::connect(_receiver, SIGNAL(started()), SLOT(onReceiverStarted()));
  QObject::connect(_receiver, SIGNAL(stopped()), SLOT(onReceiverStopped()));

  QSplitter *splitter = new QSplitter();
  splitter->addWidget(_receiver->createDemodView());
  splitter->setCollapsible(0, false);

  QVBoxLayout *side = new QVBoxLayout();
  side->addWidget(_play, 0);
  side->addWidget(ctrls, 1);

  QWidget *sidepanel = new QWidget();
  sidepanel->setLayout(side);
  splitter->addWidget(sidepanel);
  splitter->setCollapsible(1, true);

  setCentralWidget(splitter);

  resize(1024, 400);
}


MainWindow::~MainWindow() {
  // pass...
}



void
MainWindow::onPlayClicked() {
  if (_play->isChecked()) {
    if (_receiver->isRunning()) { return; }
    _receiver->start();
    // Disable button, gets re-enabled once the state of the reciver has changed
    _play->setEnabled(false);
  } else {
    if (!_receiver->isRunning()) { return; }
    _receiver->stop();
    // Disable button, gets re-enabled once the state of the reciver has changed
    _play->setEnabled(false);
  }
}

void
MainWindow::onReceiverStarted() {
  sdr::Logger::get().log(sdr::LogMessage(sdr::LOG_INFO, "Receiver started."));
  _play->setChecked(true); _play->setText("Stop"); _play->setEnabled(true);
}

void
MainWindow::onReceiverStopped() {
  sdr::Logger::get().log(sdr::LogMessage(sdr::LOG_INFO, "Receiver stopped."));
  _play->setChecked(false); _play->setText("Start"); _play->setEnabled(true);
}



