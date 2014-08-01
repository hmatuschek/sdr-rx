#include "source.hh"
#include "receiver.hh"
#include "portaudiosource.hh"
#include "filesource.hh"
#include "rtldatasource.hh"
#include "queue.hh"


#include <QWidget>
#include <QComboBox>


/* ********************************************************************************************* *
 * Implementation of DataSource
 * ********************************************************************************************* */
DataSource::DataSource(QObject *parent)
  : QObject(parent)
{
  // pass...
}

DataSource::~DataSource() {
  // pass...
}

QWidget *
DataSource::createCtrlView() {
  return new QWidget();
}

void
DataSource::triggerNext() {
  // pass...
}

void DataSource::queueStarted() {
  // pass...
}

void DataSource::queueStopped() {
  // pass...
}


/* ********************************************************************************************* *
 * Implementation of DataSourceCtrl
 * ********************************************************************************************* */
DataSourceCtrl::DataSourceCtrl(Receiver *receiver)
  : QObject(receiver), Proxy(), _receiver(receiver), _source(SOURCE_PORT), _src_obj(0)
{
  // Initialize PortAudio
  sdr::PortAudio::init();

  // Instantiate data source
  setSource(SOURCE_PORT);

  sdr::Queue::get().addIdle(this, &DataSourceCtrl::_onQueueIdle);
  sdr::Queue::get().addStart(this, &DataSourceCtrl::_onQueueStart);
  sdr::Queue::get().addStop(this, &DataSourceCtrl::_onQueueStop);
}


DataSourceCtrl::~DataSourceCtrl() {
  // pass...
}


void
DataSourceCtrl::setSource(Src source) {
  bool was_running = _receiver->isRunning();
  if (was_running) { _receiver->stop(); }

  // Unlink current source
  _src_obj->disconnect(this);
  // Free current source late
  _src_obj->deleteLater();

  // Create and link new data source
  _source = source;
  switch (_source) {
  case SOURCE_PORT: _src_obj = new PortAudioSource(this); break;
  case SOURCE_PORT_IQ: _src_obj = new PortAudioIQSource(this); break;
  case SOURCE_FILE: _src_obj = new FileSource(this); break;
  case SOURCE_RTL: _src_obj = new RTLDataSource(100e6, 225001, this); break;
  }
  _src_obj->source()->connect(this, true);

  if (was_running) { _receiver->start(); }
}


QWidget *
DataSourceCtrl::createCtrlView() {
  return _src_obj->createCtrlView();
}

void
DataSourceCtrl::_onQueueIdle() {
  _src_obj->triggerNext();
}

void
DataSourceCtrl::_onQueueStart() {
  _src_obj->queueStarted();
}

void
DataSourceCtrl::_onQueueStop() {
  _src_obj->queueStopped();
}



/* ********************************************************************************************* *
 * Implementation of DataSourceCtrlView
 * ********************************************************************************************* */
DataSourceCtrlView::DataSourceCtrlView(DataSourceCtrl *src_ctrl, QWidget *parent)
  : QWidget(parent), _src_ctrl(src_ctrl)
{
  QComboBox *src_sel = new QComboBox();
  src_sel->addItem("Port Audio");
  src_sel->addItem("Port Audio I/Q");
  src_sel->addItem("WAV File");
  src_sel->addItem("RTL2832");

  // Get current source from receiver
  switch (_src_ctrl->source()) {
  case DataSourceCtrl::SOURCE_PORT: src_sel->setCurrentIndex(0); break;
  case DataSourceCtrl::SOURCE_PORT_IQ: src_sel->setCurrentIndex(1); break;
  case DataSourceCtrl::SOURCE_FILE: src_sel->setCurrentIndex(2); break;
  case DataSourceCtrl::SOURCE_RTL: src_sel->setCurrentIndex(3); break;
  }
  _currentSrcCtrl = _src_ctrl->createCtrlView();

  QObject::connect(src_sel, SIGNAL(currentIndexChanged(int)), this, SLOT(_onSourceSelected(int)));

  _layout = new QVBoxLayout();
  _layout->addWidget(src_sel, 0);
  _layout->addWidget(_currentSrcCtrl, 1);

  setLayout(_layout);
}

DataSourceCtrlView::~DataSourceCtrlView() {
  // pass...
}

void
DataSourceCtrlView::_onSourceSelected(int index) {
  // Update source of receiver
  switch (index) {
  case 0: _src_ctrl->setSource(DataSourceCtrl::SOURCE_PORT); break;
  case 1: _src_ctrl->setSource(DataSourceCtrl::SOURCE_PORT_IQ); break;
  case 2: _src_ctrl->setSource(DataSourceCtrl::SOURCE_FILE); break;
  case 3: _src_ctrl->setSource(DataSourceCtrl::SOURCE_RTL); break;
  default: return;
  }

  // Remove current source control:
  _currentSrcCtrl->deleteLater();
  _currentSrcCtrl = _src_ctrl->createCtrlView();
  _layout->addWidget(_currentSrcCtrl);
}
