#include "portaudiosource.hh"
#include <QFormLayout>

using namespace sdr;



/* ********************************************************************************************* *
 * Implementation of PortAudioSource
 * ********************************************************************************************* */
PortAudioSource::PortAudioSource(QObject *parent)
  : ::DataSource(parent), sdr::Proxy(), _src(0), _to_complex(0)
{
  // Allocate and connect stuff:
  _src = new PortSource<int16_t>(8000, 1024);

  // Probe possible sample rates:
  if (_src->hasSampleRate(8000)) { _sampleRates.push_back(8000); }
  if (_src->hasSampleRate(16000)) { _sampleRates.push_back(16000); }
  if (_src->hasSampleRate(22050)) { _sampleRates.push_back(22050); }
  if (_src->hasSampleRate(44100)) { _sampleRates.push_back(44100); }
  if (_src->hasSampleRate(48000)) { _sampleRates.push_back(48000); }
  if (_src->hasSampleRate(96000)) { _sampleRates.push_back(96000); }
  if (_src->hasSampleRate(192000)) { _sampleRates.push_back(192000); }
  if (0 < _sampleRates.size())
    _src->setSampleRate(_sampleRates[0]);

  _to_complex = new ToComplex<int16_t, int16_t>();
  _src->connect(_to_complex, true);
  _to_complex->connect(this, true);
}

PortAudioSource::~PortAudioSource() {
  delete _src;
  delete _to_complex;
}

Config::Type
PortAudioSource::format() const {
  return _src->type();
}

void
PortAudioSource::next() {
  _src->next();
}

QWidget *
PortAudioSource::createCtrlView() {
  return new PortAudioSourceView(this);
}

Source *
PortAudioSource::source() {
  return this;
}

void
PortAudioSource::triggerNext() {
  this->next();
}

const std::vector<double> &
PortAudioSource::sampleRates() const {
  return _sampleRates;
}

void
PortAudioSource::setSampleRate(double rate) {
  bool is_running = sdr::Queue::get().isRunning();
  if (is_running) { sdr::Queue::get().stop(); }
  _src->setSampleRate(rate);
  if (is_running) { sdr::Queue::get().stop(); }
}



/* ********************************************************************************************* *
 * Implementation of PortAudioIQSource
 * ********************************************************************************************* */
PortAudioIQSource::PortAudioIQSource(QObject *parent)
  : ::DataSource(parent), sdr::Proxy(), _src(0)
{
  // Allocate and connect stuff:
  _src = new PortSource< std::complex<int16_t> >(8000, 1024);

  // Probe possible sample rates:
  if (_src->hasSampleRate(8000)) { _sampleRates.push_back(8000); }
  if (_src->hasSampleRate(16000)) { _sampleRates.push_back(16000); }
  if (_src->hasSampleRate(22050)) { _sampleRates.push_back(22050); }
  if (_src->hasSampleRate(44100)) { _sampleRates.push_back(44100); }
  if (_src->hasSampleRate(48000)) { _sampleRates.push_back(48000); }
  if (_src->hasSampleRate(96000)) { _sampleRates.push_back(96000); }
  if (_src->hasSampleRate(192000)) { _sampleRates.push_back(192000); }
  if (0 < _sampleRates.size())
    _src->setSampleRate(_sampleRates[0]);

  _src->connect(this, true);
}

PortAudioIQSource::~PortAudioIQSource() {
  delete _src;
}

Config::Type
PortAudioIQSource::format() const {
  return _src->type();
}

void
PortAudioIQSource::next() {
  _src->next();
}

QWidget *
PortAudioIQSource::createCtrlView() {
  return new PortAudioIQSourceView(this);
}

Source *
PortAudioIQSource::source() {
  return this;
}

void
PortAudioIQSource::triggerNext() {
  next();
}

const std::vector<double> &
PortAudioIQSource::sampleRates() const {
  return _sampleRates;
}

void
PortAudioIQSource::setSampleRate(double rate) {
  bool is_running = sdr::Queue::get().isRunning();
  if (is_running) { sdr::Queue::get().stop(); }
  _src->setSampleRate(rate);
  if (is_running) { sdr::Queue::get().start(); }
}



/* ********************************************************************************************* *
 * Implementation of PortAudioSourceView
 * ********************************************************************************************* */
PortAudioSourceView::PortAudioSourceView(PortAudioSource *src, QWidget *parent)
  : QWidget(parent), _src(src)
{
  _sample_rate = new QComboBox();
  const std::vector<double> &sample_rates = _src->sampleRates();
  for (size_t i=0; i<sample_rates.size(); i++) {
    _sample_rate->addItem(QString::number(sample_rates[i]), sample_rates[i]);
  }
  if (0 < sample_rates.size()) {
    _sample_rate->setCurrentIndex(0);
  }

  _format = new QLabel();
  _format->setText(typeName(_src->format()));

  QFormLayout *layout = new QFormLayout();
  layout->addRow("Sample rate", _sample_rate);
  layout->addRow("Format", _format);

  // Delete view on desctruction of source object
  QObject::connect(src, SIGNAL(destroyed()), this, SLOT(_onSourceDeleted()));
  QObject::connect(_sample_rate, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(_onSampleRateSelected(int)));

  setLayout(layout);
}

PortAudioSourceView::~PortAudioSourceView() {
  // pass...
}


void
PortAudioSourceView::_onSourceDeleted() {
  this->deleteLater();
}

void
PortAudioSourceView::_onSampleRateSelected(int idx) {
  double rate = _sample_rate->itemData(idx).toDouble();
  _src->setSampleRate(rate);
}


/* ********************************************************************************************* *
 * Implementation of PortAudioIQSourceView
 * ********************************************************************************************* */
PortAudioIQSourceView::PortAudioIQSourceView(PortAudioIQSource *src, QWidget *parent)
  : QWidget(parent), _src(src)
{
  _sample_rate = new QComboBox();
  const std::vector<double> &sample_rates = _src->sampleRates();
  for (size_t i=0; i<sample_rates.size(); i++) {
    _sample_rate->addItem(QString::number(sample_rates[i]), sample_rates[i]);
  }
  if (0 < sample_rates.size()) {
    _sample_rate->setCurrentIndex(0);
  }

  _format = new QLabel(typeName(_src->format()));

  QFormLayout *layout = new QFormLayout();
  layout->addRow("Sample rate", _sample_rate);
  layout->addRow("Format", _format);

  // Delete view on desctruction of source object
  QObject::connect(src, SIGNAL(destroyed()), this, SLOT(_onSourceDeleted()));
  QObject::connect(_sample_rate, SIGNAL(currentIndexChanged(int)),
                   this, SLOT(_onSampleRateSelected(int)));

  setLayout(layout);
}

PortAudioIQSourceView::~PortAudioIQSourceView() {
  // pass...
}


void
PortAudioIQSourceView::_onSourceDeleted() {
  this->deleteLater();
}

void
PortAudioIQSourceView::_onSampleRateSelected(int idx) {
  double rate = _sample_rate->itemData(idx).toDouble();
  _src->setSampleRate(rate);
}
