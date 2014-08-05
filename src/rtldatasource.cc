#include "rtldatasource.hh"
#include "receiver.hh"

#include <QComboBox>
#include <QFormLayout>
#include <QToolButton>
#include <QPushButton>

using namespace sdr;


/* ******************************************************************************************** *
 * Implementation of RTLDataSourceConfig
 * ******************************************************************************************** */
RTLDataSourceConfig::RTLDataSourceConfig()
  : _config(Configuration::get())
{
  // pass...
}

RTLDataSourceConfig::~RTLDataSourceConfig() {
  // pass...
}

double
RTLDataSourceConfig::frequency() const {
  return _config.value("RTLDataSource/frequency", 100.0e6).toDouble();
}

void
RTLDataSourceConfig::storeFrequency(double f) {
  _config.setValue("RTLDataSource/frequency", f);
}

double
RTLDataSourceConfig::sampleRate() const {
  return _config.value("RTLDataSource/sampleRate", 1.0e6).toDouble();
}

void
RTLDataSourceConfig::storeSampleRate(double f) {
  _config.setValue("RTLDataSource/sampleRate", f);
}



/* ******************************************************************************************** *
 * Implementation of RTLDataSource
 * ******************************************************************************************** */
RTLDataSource::RTLDataSource(QObject *parent)
  : DataSource(parent), _device(0), _balance(), _config()
{
  try {
    _device = new RTLSource(_config.frequency(), _config.sampleRate());
  } catch (sdr::SDRError &err) {
    sdr::LogMessage msg(sdr::LOG_WARNING);
    msg << "Can not open RTL2832 device: " << err.what();
    sdr::Logger::get().log(msg);
  }

  _to_int16 = new AutoCast< std::complex<int16_t> >();
  if (0 != _device) {
    _device->connect(_to_int16, true);
  }
  _to_int16->connect(&_balance, true);
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
  return &_balance;
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
  // Set frequency of the device
  _device->setFrequency(freq);
  // and store it in the config
  _config.storeFrequency(freq);
}

double
RTLDataSource::sampleRate() const {
  return _device->sampleRate();
}

void
RTLDataSource::setSampleRate(double rate) {
  bool is_running = sdr::Queue::get().isRunning();
  if (is_running) { sdr::Queue::get().stop(); }
  _device->setSampleRate(rate);
  if (is_running) { sdr::Queue::get().start(); }
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

const std::vector<double> &
RTLDataSource::gainFactors() const {
  return _device->gainFactors();
}

double
RTLDataSource::IQBalance() const {
  return _balance.balance();
}

void
RTLDataSource::setIQBalance(double balance) {
  _balance.setBalance(balance);
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
  // Check for correct idx
  if (idx >= numDevices()) { return; }
  // Stop queue if running
  bool is_running = sdr::Queue::get().isRunning();
  if (is_running) { sdr::Queue::get().stop(); }
  // Delete device (if it exists)
  if (_device) {
    _device->disconnect(_to_int16);
    delete _device; _device=0;
  }
  // Try to start device
  try {
    _device = new RTLSource(100e6, 1e6, idx);
    _device->connect(_to_int16, true);
    // restart queue if it was running
    if (is_running) { sdr::Queue::get().start(); }
  } catch (sdr::SDRError &err) {
    sdr::LogMessage msg(sdr::LOG_WARNING);
    msg << "Can not open RTL2832 device: " << err.what();
    sdr::Logger::get().log(msg);
  }
}


void
RTLDataSource::queueStarted() {
  if (_device) { _device->start(); }
}

void
RTLDataSource::queueStopped() {
  if (_device) { _device->stop(); }
}

double
RTLDataSource::tunerFrequency() const {
  if (isActive()) {
    return frequency();
  }
  return 0.0;
}


/* ******************************************************************************************** *
 * Implementation of RTLDataSource
 * ******************************************************************************************** */
RTLCtrlView::RTLCtrlView(RTLDataSource *source, QWidget *parent)
  : QWidget(parent), _source(source)
{
  _errorMessage = new QLabel("Cannot open RTL2832 device.");
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
    double f = _source->frequency();
    _freq->setText(QString::number(f));
  }

  // save frequencies
  QAction *saveFreqAction = new QAction(QIcon::fromTheme("document-save"), "Save",this);
  QObject::connect(saveFreqAction, SIGNAL(triggered()), this, SLOT(onSaveFrequency()));
  QToolButton *saveFreqButton = new QToolButton();
  saveFreqButton->setDefaultAction(saveFreqAction);

  // load frequencies
  _freqMenu = new QMenu();
  QAction *loadFreqAction = new QAction(QIcon::fromTheme("document-open"), "Load", this);
  QToolButton *loadFreqButton = new QToolButton();
  loadFreqButton->setDefaultAction(loadFreqAction);
  loadFreqButton->setMenu(_freqMenu);

  // Sample rate:
  _sampleRates = new QComboBox();
  _sampleRates->addItem("2 MS/s", 2e6);
  _sampleRates->addItem("1 MS/s", 1e6);
  _sampleRates->addItem("900 kS/s", 900e3);
  _sampleRates->addItem("300 kS/s", 300e3);
  _sampleRates->setCurrentIndex(0);
  this->onSampleRateSelected(0);

  _gain = new QComboBox();
  if (_source->isActive()) {
    for (size_t i=0; i<_source->gainFactors().size(); i++) {
      _gain->addItem(QString("%1 dB").arg(_source->gainFactors()[i]/10), _source->gainFactors()[i]);
    }
  }

  _agc = new QCheckBox();
  if (_source->isActive()) {
    _agc->setChecked(_source->agcEnabled());
    if (_source->agcEnabled()) { _gain->setEnabled(false); }
  }

  _balance = new QLineEdit();
  QDoubleValidator *balance_val = new QDoubleValidator();
  balance_val->setRange(-1., 1.);
  _balance->setValidator(balance_val);
  if (_source->isActive()) {
    _balance->setText(QString::number(_source->IQBalance()));
  }

  if (! _source->isActive()) {
    _freq->setEnabled(false);
    _sampleRates->setEnabled(false);
    _gain->setEnabled(false);
    _agc->setEnabled(false);
    _balance->setEnabled(false);
  }

  QFormLayout *layout = new QFormLayout();

  QVBoxLayout *deviceLayout = new QVBoxLayout();
  deviceLayout->addWidget(_devices);
  deviceLayout->addWidget(_errorMessage);
  layout->addRow("Device", deviceLayout);

  QHBoxLayout *freqLayout = new QHBoxLayout();
  freqLayout->addWidget(_freq, 1); freqLayout->addWidget(saveFreqButton, 0);
  freqLayout->addWidget(loadFreqButton, 0);
  layout->addRow("Frequency", freqLayout);

  layout->addRow("Sample rate", _sampleRates);
  layout->addRow("Gain", _gain);
  layout->addRow("AGC", _agc);
  layout->addRow("IQ Balance", _balance);
  setLayout(layout);

  QObject::connect(_devices, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceSelected(int)));
  QObject::connect(_freq, SIGNAL(returnPressed()), this, SLOT(onFrequencyChanged()));
  QObject::connect(_sampleRates, SIGNAL(currentIndexChanged(int)), this, SLOT(onSampleRateSelected(int)));
  QObject::connect(_gain, SIGNAL(currentIndexChanged(int)), this, SLOT(onGainChanged(int)));
  QObject::connect(_agc, SIGNAL(toggled(bool)), this, SLOT(onAGCToggled(bool)));
  QObject::connect(_balance, SIGNAL(returnPressed()), this, SLOT(onBalanceChanged()));
}

RTLCtrlView::~RTLCtrlView() {
  // pass...
}

void
RTLCtrlView::onDeviceSelected(int idx) {
  _source->setDevice(idx);
  if (_source->isActive()) {
    // Enable all controlls:
    _freq->setEnabled(true);
    _sampleRates->setEnabled(true);
    _gain->setEnabled(true);
    _agc->setEnabled(true);
  } else {
    // Disable all controlls:
    _freq->setEnabled(false);
    _sampleRates->setEnabled(false);
    _gain->setEnabled(false);
    _agc->setEnabled(false);
  }

}

void
RTLCtrlView::onFrequencyChanged() {
  double freq = _freq->text().toDouble();
  if (! _source->isActive()) { return; }
  _source->setFrequency(freq);
}

void
RTLCtrlView::onSaveFrequency() {
  std::cerr << "Save frequency " << _freq->text().toDouble() << std::endl;
}

void
RTLCtrlView::onSampleRateSelected(int idx) {
  double rate = _sampleRates->itemData(idx).toDouble();
  if (_source->isActive()) {
    _source->setSampleRate(rate);
  }
}

void
RTLCtrlView::onGainChanged(int idx) {
  if (_source->isActive() && !_source->agcEnabled()) {
    double gain = _gain->itemData(idx).toDouble();
    _source->setGain(gain);
  }
}

void
RTLCtrlView::onAGCToggled(bool enabled) {
  if (! _source->isActive()) { return; }
  if (enabled) {
    _gain->setEnabled(false);
    _source->enableAGC(true);
  } else {
    _gain->setEnabled(true);
    _source->enableAGC(false);
  }
}

void
RTLCtrlView::onBalanceChanged() {
  double value = _balance->text().toDouble();
  if ( _source->isActive()) {
    _source->setIQBalance(value);
  }
}
