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
    // pass...
  }
  _to_int16 = new Cast< std::complex<int8_t>, std::complex<int16_t> >(128);
  if (_device) {
    _device->connect(_to_int16, true);
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
  layout->addRow("Sample rate", _sampleRates);
  layout->addRow("Gain", _gain);
  layout->addRow("AGC", _agc);
  setLayout(layout);

  QObject::connect(_devices, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceSelected(int)));
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
RTLCtrlView::onSampleRateSelected(int idx) {

}

void
RTLCtrlView::onGainChanged(QString value) {

}

void
RTLCtrlView::onAGCToggled(bool enabled) {
  if (enabled) {
    _gain->setEnabled(false);
  } else {
    _gain->setEnabled(true);
  }
}
