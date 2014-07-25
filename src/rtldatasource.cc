#include "rtldatasource.hh"
#include "receiver.hh"

#include <QComboBox>
#include <QFormLayout>

using namespace sdr;


/* ******************************************************************************************** *
 * Implementation of RTLDataSource
 * ******************************************************************************************** */
RTLDataSource::RTLDataSource(double frequency, double sample_rate, QObject *parent)
  : DataSource(parent), _device(0)
{
  try {
    _device = new RTLSource(frequency, sample_rate);
  } catch (sdr::SDRError &err) {
    sdr::LogMessage msg(sdr::LOG_WARNING);
    msg << "Can not open RTL2832 device: " << err.what();
    sdr::Logger::get().log(msg);
  }

  _to_int16 = new AutoCast< std::complex<int16_t> >();
  if (0 != _device) {
    _device->connect(_to_int16, true);
    sdr::Queue::get().addStart(_device, &RTLSource::start);
    sdr::Queue::get().addStop(_device, &RTLSource::stop);
  }
}

RTLDataSource::~RTLDataSource() {
  if (_device) { delete _device; }
  delete _to_int16;
}

QWidget *
RTLDataSource::createCtrlView() {
  return new RTLCtrlView(this);
}

Source *
RTLDataSource::source() {
  return _to_int16;
}

void
RTLDataSource::triggerNext() {
  // RTL data source runs in parallel
}

bool
RTLDataSource::isActive() const {
  return 0 != _device;
}

double
RTLDataSource::frequency() const {
  return _device->frequency();
}

void
RTLDataSource::setFrequency(double freq) {
  _device->setFrequency(freq);
}

bool
RTLDataSource::agcEnabled() const {
  return _device->agcEnabled();
}

void
RTLDataSource::enableAGC(bool enable) {
  _device->enableAGC(enable);
}

double
RTLDataSource::gain() const {
  return _device->gain();
}

void
RTLDataSource::setGain(double gain) {
  return _device->setGain(gain);
}

size_t
RTLDataSource::numDevices() {
  return sdr::RTLSource::numDevices();
}

std::string
RTLDataSource::deviceName(size_t idx) {
  return sdr::RTLSource::deviceName(idx);
}

void
RTLDataSource::setDevice(size_t idx) {
  if (idx >= numDevices()) { return; }
  if (_device) {
    _device->disconnect(_to_int16);
    delete _device; _device=0;
  }
  try {
    _device = new RTLSource(100e6);
    _device->connect(_to_int16, true);
  } catch (sdr::SDRError &err) {
    sdr::LogMessage msg(sdr::LOG_WARNING);
    msg << "Can not open RTL2832 device: " << err.what();
    sdr::Logger::get().log(msg);
  }
}


/* ******************************************************************************************** *
 * Implementation of RTLDataSource
 * ******************************************************************************************** */
RTLCtrlView::RTLCtrlView(RTLDataSource *source, QWidget *parent)
  : QWidget(parent), _source(source)
{
  _errorMessage = new QLabel("Can not open RTL device.");
  if (_source->isActive()) {
    _errorMessage->setVisible(false);
  }

  // Populate device list:
  _devices = new QComboBox();
  for (size_t i=0; i<RTLDataSource::numDevices(); i++) {
    _devices->addItem(RTLDataSource::deviceName(i).c_str());
  }
  // Frequency
  _freq = new QLineEdit();
  QDoubleValidator *freq_val = new QDoubleValidator();
  freq_val->setBottom(0);
  _freq->setValidator(freq_val);
  if(_source->isActive()) {
    _freq->setText(QString::number(_source->frequency()));
  }

  // Sample rate:
  _sampleRates = new QComboBox();
  _sampleRates->addItem("2 MS/s", 2e6);
  _sampleRates->addItem("1 MS/s", 1e6);
  _sampleRates->addItem("800 kS/s", 800e3);

  _gain = new QLineEdit("0");
  QDoubleValidator *gain_val = new QDoubleValidator();
  gain_val->setBottom(0); _gain->setValidator(gain_val);
  if (_source->isActive()) {
    _gain->setText(QString::number(_source->gain()));
  }
  _agc = new QCheckBox();
  if (_source->isActive()) {
    _agc->setChecked(_source->agcEnabled());
    if (_source->agcEnabled()) { _gain->setEnabled(false); }
  }

  QFormLayout *layout = new QFormLayout();
  QVBoxLayout *deviceLayout = new QVBoxLayout();
  deviceLayout->addWidget(_devices);
  deviceLayout->addWidget(_errorMessage);
  layout->addRow("Device", deviceLayout);
  layout->addRow("Frequency", _freq);
  layout->addRow("Sample rate", _sampleRates);
  layout->addRow("Gain", _gain);
  layout->addRow("AGC", _agc);
  setLayout(layout);

  QObject::connect(_devices, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceSelected(int)));
  QObject::connect(_freq, SIGNAL(textEdited(QString)), this, SLOT(onFrequencyChanged(QString)));
  QObject::connect(_sampleRates, SIGNAL(currentIndexChanged(int)), this, SLOT(onSampleRateSelected(int)));
  QObject::connect(_gain, SIGNAL(textEdited(QString)), this, SLOT(onGainChanged(QString)));
  QObject::connect(_agc, SIGNAL(toggled(bool)), this, SLOT(onAGCToggled(bool)));
}

RTLCtrlView::~RTLCtrlView() {
  // pass...
}

void
RTLCtrlView::onDeviceSelected(int idx) {
}

void
RTLCtrlView::onFrequencyChanged(QString value) {
  _source->setFrequency(value.toDouble());
}

void
RTLCtrlView::onSampleRateSelected(int idx) {
  // pass...
}

void
RTLCtrlView::onGainChanged(QString value) {
  if (_source->isActive() && !_source->agcEnabled()) {
    _source->setGain(value.toDouble());
  }
}

void
RTLCtrlView::onAGCToggled(bool enabled) {
  if (enabled) {
    _gain->setEnabled(false);
    _source->enableAGC(true);
  } else {
    _gain->setEnabled(true);
    _source->enableAGC(false);
  }
}
