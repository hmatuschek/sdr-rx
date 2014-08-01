#include "receiver.hh"
#include "source.hh"

using namespace sdr;

Receiver::Receiver(QObject *parent)
  : QObject(parent), _queue(Queue::get())
{
  /// @todo Unify data sources...
  _src   = new DataSourceCtrl(this);
  _demod = new DemodulatorCtrl(this);
  _audio = new AudioPostProc(this);

  // Connect data source to demodulator
  _src->Source::connect(_demod, true); // spectrum
  _src->Source::connect(_demod->in(), true);

  // Connect demodulator to audio sink
  _demod->audioSource()->connect(_audio, true);

  // Connect to start signal of queue
  _queue.addStart(this, &Receiver::_onQueueStarted);
  // Connect to stop signal of queue
  _queue.addStop(this, &Receiver::_onQueueStopped);
}


Receiver::~Receiver() {
  stop();
}

bool
Receiver::isRunning() const {
  return _queue.isRunning();
}


QWidget *
Receiver::createSourceCtrlView() {
  return new DataSourceCtrlView(_src);
}

QWidget *
Receiver::createDemodCtrlView() {
  return _demod->createCtrlView();
}

QWidget *
Receiver::createDemodView() {
  return _demod->createSpectrumView();
}

QWidget *
Receiver::createAudioCtrlView() {
  return new AudioPostProcView(_audio);
}

double
Receiver::tunerFrequency() const {
  return _src->tunerFrequency();
}


void
Receiver::start() {
  _queue.start();
}

void
Receiver::stop() {
  _queue.stop();
  _queue.wait();
}


void
Receiver::_onQueueStarted() {
  emit started();
}

void
Receiver::_onQueueStopped() {
  emit stopped();
}
