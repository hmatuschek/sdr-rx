#include "demodulator.hh"
#include "receiver.hh"

#include <QPainter>
#include <QPaintEvent>
#include <QComboBox>
#include <QTimer>
#include <QFormLayout>
#include <QTextEdit>


using namespace sdr;



/* ******************************************************************************************** *
 * Implementation of DemodulatorCtrl
 * ******************************************************************************************** */
DemodulatorCtrlConfig::DemodulatorCtrlConfig()
  : _config(Configuration::get())
{
  // pass...
}

DemodulatorCtrlConfig::~DemodulatorCtrlConfig() {
  // pass...
}

unsigned int
DemodulatorCtrlConfig::filterOrder() const {
  return _config.value("BaseBand/filterOrder", 15).toUInt();
}

void
DemodulatorCtrlConfig::storeFilterOrder(unsigned int order) {
  _config.setValue("BaseBand/fitlerOrder", order);
}

double
DemodulatorCtrlConfig::centerFrequency() const {
  return _config.value("BaseBand/centerFrequency", 0.0).toDouble();
}

void
DemodulatorCtrlConfig::storeCenterFrequency(double f) {
  _config.setValue("BaseBand/centerFrequency", f);
}

bool
DemodulatorCtrlConfig::agcEnabled() const {
  return _config.value("BaseBand/agcEnabled", false).toBool();
}

void
DemodulatorCtrlConfig::storeAgcEnabled(bool enabled) {
  _config.setValue("BaseBand/agcEnabled", enabled);
}

double
DemodulatorCtrlConfig::agcTau() const {
  return _config.value("BaseBand/agcTau", 0.1).toDouble();
}

void
DemodulatorCtrlConfig::storeAgcTau(double tau) {
  _config.setValue("BaseBand/agcTau", tau);
}

double
DemodulatorCtrlConfig::gain() const {
  return _config.value("BaseBand/gain", 1.0).toDouble();
}

void
DemodulatorCtrlConfig::storeGain(double gain) {
  _config.setValue("BaseBand/gain", gain);
}




/* ******************************************************************************************** *
 * Implementation of DemodulatorCtrl
 * ******************************************************************************************** */
DemodulatorCtrl::DemodulatorCtrl(Receiver *receiver) :
  gui::Spectrum(2, 1024, 5, receiver), _receiver(receiver), _demodObj(0), _config()
{
  // Assemble processing chain
  _agc = new AGC< std::complex<int16_t> >();
  _filter_node = new IQBaseBand<int16_t>(_config.centerFrequency(), 2000,
                                         _config.filterOrder(), 1, 16000.0);
  _audio_source = new sdr::Proxy();

  _agc->connect(_filter_node, true);
  _agc->connect(this);
  _agc->enable(_config.agcEnabled());
  _agc->setTau(_config.agcTau());
  _agc->setGain(_config.gain());

  setDemod(DEMOD_USB);
}


DemodulatorCtrl::~DemodulatorCtrl() {
  delete _agc;
  delete _filter_node;
  delete _audio_source;
  if (_demodObj) {
    delete _demodObj;
  }
}

Receiver *
DemodulatorCtrl::receiver() const {
  return _receiver;
}

bool
DemodulatorCtrl::isAGCEnabled() const {
  return _agc->enabled();
}

double
DemodulatorCtrl::gain() const {
  return 20*std::log10(_agc->gain());
}

void
DemodulatorCtrl::setGain(double gain) {
  _agc->setGain(pow(10, gain/20));
  _config.storeGain(_agc->gain());
}


double
DemodulatorCtrl::agcTime() const {
  return _agc->tau();
}

void
DemodulatorCtrl::setAGCTime(double tau) {
  _agc->setTau(tau);
  _config.storeAgcTau(tau);
}


void
DemodulatorCtrl::enableAGC(bool enable) {
  _agc->enable(enable);
  _config.storeAgcEnabled(enable);
}

void
DemodulatorCtrl::setCenterFreq(double f) {
  double dF = this->filterFrequency();
  _filter_node->setCenterFrequency(f);
  _config.storeCenterFrequency(f);
  this->setFilterFrequency(dF);
}

void
DemodulatorCtrl::setFilterFrequency(double f) {
  _filter_node->setFilterFrequency(_filter_node->centerFrequency() + f);
  emit filterChanged();
}

void
DemodulatorCtrl::setFilterWidth(double w) {
  _filter_node->setFilterWidth(w);

  bool was_running = _receiver->isRunning();
  if (was_running) { _receiver->stop(); }
  // Update resampling of IQBaseBand node. Ensures that the output sample-rate is at least
  // filter width and >= 8000 Hz
  double oFs = std::max(w, 8000.0);
  _filter_node->setOutputSampleRate(oFs);
  if (was_running) { _receiver->start(); }

  emit filterChanged();
}


void
DemodulatorCtrl::setDemod(Demod demod) {
  bool was_running = _receiver->isRunning();
  if (was_running) { _receiver->stop(); }

  // Unlink current demodulator
  if (_demodObj) {
    _filter_node->disconnect(_demodObj->sink());
    _demodObj->audioSource()->disconnect(_audio_source);
    delete _demodObj; _demodObj = 0;
  }

  switch (demod) {
  case DEMOD_AM:     _demodObj = new AMDemodulator(this); break;
  case DEMOD_WFM:    _demodObj = new WFMDemodulator(this); break;
  case DEMOD_NFM:    _demodObj = new NFMDemodulator(this); break;
  case DEMOD_USB:    _demodObj = new USBDemodulator(this); break;
  case DEMOD_LSB:    _demodObj = new LSBDemodulator(this); break;
  case DEMOD_CW:     _demodObj = new CWDemodulator(this); break;
  case DEMOD_BPSK31: _demodObj = new BPSK31Demodulator(this); break;
  }

  // Link new demodulator
  _filter_node->connect(_demodObj->sink());
  _demodObj->audioSource()->connect(_audio_source);

  // Restart queue if it was running...
  if (was_running) { _receiver->start(); }
}


QWidget *
DemodulatorCtrl::createCtrlView() {
  return new DemodulatorCtrlView(this);
}


QWidget *
DemodulatorCtrl::createSpectrumView() {
  //DemodulatorSpectrumView *view = new DemodulatorSpectrumView(this);
  DemodulatorWaterFallView *view = new DemodulatorWaterFallView(this);
  QObject::connect(view, SIGNAL(click(double)), this, SLOT(setCenterFreq(double)));
  return view;
}



/* ******************************************************************************************** *
 * Implementation of DemodulatorSpectrumView
 * ******************************************************************************************** */
DemodulatorSpectrumView::DemodulatorSpectrumView(DemodulatorCtrl *demodulator)
  : gui::SpectrumView(demodulator), _demodulator(demodulator)
{
  setMinimumWidth(640);
  setMinimumHeight(200);
  QObject::connect(demodulator, SIGNAL(filterChanged()), this, SLOT(update()));
}

DemodulatorSpectrumView::~DemodulatorSpectrumView() {
  // pass...
}


void
DemodulatorSpectrumView::paintEvent(QPaintEvent *evt) {
  gui::SpectrumView::paintEvent(evt);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.save();

  // Clip region to update
  painter.setClipRect(evt->rect());
  // Clip plot area
  painter.setClipRect(_plotArea);

  // Draw a thin line at the center frequency
  QPen pen(QBrush(Qt::black), 1);
  if (_demodulator->isInputReal()) {
    double dfdx = _demodulator->sampleRate()/(2*_plotArea.width());
    double x = _plotArea.left()+_demodulator->centerFreq()/dfdx;
    painter.drawLine(x, _plotArea.top(), x, _plotArea.bottom());
  } else {
    double dfdx = _demodulator->sampleRate()/(_plotArea.width());
    double x = _plotArea.left()+(_demodulator->centerFreq()+_demodulator->sampleRate()/2)/dfdx;
    painter.drawLine(x, _plotArea.top(), x, _plotArea.bottom());
  }

  // Draw filter area:
  QRect filter; QColor color(0,0,255, 64);
  if (_demodulator->isInputReal()) {
    double dfdx = _demodulator->sampleRate()/(2*_plotArea.width());
    double x1 = _plotArea.left()+_demodulator->filterLower()/dfdx;
    double x2 = _plotArea.left()+_demodulator->filterUpper()/dfdx;
    filter = QRect(x1, _plotArea.top(), x2-x1, _plotArea.height());
  } else {
    double dfdx = _demodulator->sampleRate()/(_plotArea.width());
    double x1 = _plotArea.left()+(_demodulator->filterLower()+_demodulator->sampleRate()/2)/dfdx;
    double x2 = _plotArea.left()+(_demodulator->filterUpper()+_demodulator->sampleRate()/2)/dfdx;
    filter = QRect(x1, _plotArea.top(), x2-x1, _plotArea.height());
  }
  painter.fillRect(filter, color);

  painter.restore();
}


/* ******************************************************************************************** *
 * Implementation of DemodulatorWaterFallView
 * ******************************************************************************************** */
DemodulatorWaterFallView::DemodulatorWaterFallView(DemodulatorCtrl *demodulator)
  : gui::WaterFallView(demodulator, 200), _demodulator(demodulator)
{
  setMinimumWidth(640);
  QObject::connect(demodulator, SIGNAL(filterChanged()), this, SLOT(update()));
}

DemodulatorWaterFallView::~DemodulatorWaterFallView() {
  // pass...
}


void
DemodulatorWaterFallView::paintEvent(QPaintEvent *evt) {
  gui::WaterFallView::paintEvent(evt);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.save();

  // Clip region to update
  painter.setClipRect(evt->rect());

  // Draw a thin line at the center frequency
  QPen pen(QColor(0,0,255));
  painter.setPen(pen);
  double dfdx = _demodulator->sampleRate()/(this->width());
  double x = (_demodulator->centerFreq()+_demodulator->sampleRate()/2)/dfdx;
  painter.drawLine(x, 0, x, this->height());

  // Get tuner frequency (if possible):
  double tunerF = _demodulator->centerFreq();
  if (_demodulator->receiver()) {
    tunerF += _demodulator->receiver()->tunerFrequency();
  }
  // Layout frequency lable
  QString freqText = "%1 %2";
  if (std::abs(tunerF) < 10e3) {
    freqText = freqText.arg(QString::number(tunerF, 'f', 1), "Hz");
  } else if (std::abs(tunerF) < 10e6) {
    freqText = freqText.arg(QString::number(tunerF/1e3, 'f', 3), "kHz");
  } else {
    freqText = freqText.arg(QString::number(tunerF/1e6, 'f', 4), "MHz");
  }
  QFont font = painter.font(); font.setBold(true);
  painter.setFont(font);
  // Draw label
  QRect rect = painter.fontMetrics().boundingRect(freqText);
  painter.drawText(QPoint(int(x-rect.width()/2), 10+rect.height()/2), freqText);

  // Draw filter area:
  QRect filter; QColor color(0,0,255, 64);
  double x1 = (_demodulator->filterLower()+_demodulator->sampleRate()/2)/dfdx;
  double x2 = (_demodulator->filterUpper()+_demodulator->sampleRate()/2)/dfdx;
  filter = QRect(x1, 0, x2-x1, this->height());
  painter.fillRect(filter, color);

  painter.restore();
}


/* ******************************************************************************************** *
 * Implementation of DemodulatorCtrlView
 * ******************************************************************************************** */
DemodulatorCtrlView::DemodulatorCtrlView(DemodulatorCtrl *demodulator, QWidget *parent)
  : QWidget(parent), _demodulator(demodulator)
{
  _demodList = new QComboBox();
  _demodList->addItem("AM", DemodulatorCtrl::DEMOD_AM);
  _demodList->addItem("WFM", DemodulatorCtrl::DEMOD_WFM);
  _demodList->addItem("NFM", DemodulatorCtrl::DEMOD_NFM);
  _demodList->addItem("USB", DemodulatorCtrl::DEMOD_USB);
  _demodList->addItem("LSB", DemodulatorCtrl::DEMOD_LSB);
  _demodList->addItem("CW", DemodulatorCtrl::DEMOD_CW);
  _demodList->addItem("BPSK31", DemodulatorCtrl::DEMOD_BPSK31);
  _demodList->setCurrentIndex(3);

  _gain  = new QLineEdit();
  QDoubleValidator *gain_val = new QDoubleValidator();
  _gain->setValidator(gain_val);
  _agc   = new QCheckBox("AGC");
  if (_demodulator->isAGCEnabled()) {
    _agc->setChecked(true); _gain->setEnabled(false);
    _gain->setText(QString("%1").arg(_demodulator->gain()));
  } else {
    _agc->setChecked(false); _gain->setEnabled(true);
    _gain->setText(QString("%1").arg(_demodulator->gain()));
  }

  _agc_tau = new QLineEdit();
  QDoubleValidator *tau_val = new QDoubleValidator();
  tau_val->setBottom(0); _agc_tau->setValidator(tau_val);
  _agc_tau->setText(QString("%1").arg(_demodulator->agcTime()));

  _centerFreq = new QLineEdit();
  QDoubleValidator *fc_val = new QDoubleValidator();
  if (_demodulator->isInputReal()) { fc_val->setBottom(0); fc_val->setTop(_demodulator->sampleRate()/2); }
  else { fc_val->setBottom(-_demodulator->sampleRate()/2); fc_val->setTop(_demodulator->sampleRate()/2); }
  _centerFreq->setValidator(fc_val);
  _centerFreq->setText(QString("%1").arg(_demodulator->centerFreq()));

  QTimer *gainUpdate = new QTimer(this);
  gainUpdate->setInterval(500);
  gainUpdate->setSingleShot(false);
  gainUpdate->start();

  QObject::connect(_demodList, SIGNAL(activated(int)), this, SLOT(onDemodSelected(int)));
  QObject::connect(_agc, SIGNAL(toggled(bool)), SLOT(onAGCToggled(bool)));
  QObject::connect(_agc_tau, SIGNAL(textEdited(QString)), this, SLOT(onAGCTauChanged(QString)));
  QObject::connect(_gain, SIGNAL(textEdited(QString)), SLOT(onGainChanged(QString)));
  QObject::connect(gainUpdate, SIGNAL(timeout()), SLOT(onUpdateGain()));
  QObject::connect(_centerFreq, SIGNAL(textEdited(QString)), this, SLOT(onCenterFreqChanged(QString)));
  QObject::connect(_demodulator, SIGNAL(filterChanged()), this, SLOT(onFilterChanged()));

  QFormLayout *side = new QFormLayout();
  side->addRow("Modulation", _demodList);
  side->addRow("Gain", _gain);
  side->addWidget(_agc);
  side->addRow("AGC time", _agc_tau);
  side->addRow("Center freq.", _centerFreq);

  _layout = new QVBoxLayout();
  _layout->addLayout(side, 0);
  // If there is a demodulator selected already:
  if (_demodulator->demod()) {
    _layout->addWidget(_demodulator->demod()->createView(), 1, Qt::AlignTop);
  }
  this->setLayout(_layout);
}


void
DemodulatorCtrlView::onDemodSelected(int idx) {
  // First of all remove the current demodulator view from the _layout
  if (_demodulator->demod()) {
    // Get the current demod view:
    QWidget *view = _demodulator->demod()->createView();
    _layout->removeWidget(view);
  }
  // set the demodulator
  _demodulator->setDemod(DemodulatorCtrl::Demod(_demodList->itemData(idx).toUInt()));
  QWidget *view = _demodulator->demod()->createView();
  // And append the demodulator view
  _layout->addWidget(view, 1, Qt::AlignTop);
}

void
DemodulatorCtrlView::onAGCToggled(bool enabled) {
  _demodulator->enableAGC(enabled);
  if (_demodulator->isAGCEnabled()) {
    _gain->setEnabled(false);
    _gain->setText(QString("%1").arg(_demodulator->gain()));
  } else {
    _gain->setEnabled(true);
    _gain->setText(QString("%1").arg(_demodulator->gain()));
  }
}

void
DemodulatorCtrlView::onAGCTauChanged(QString value) {
  bool ok; double tau = value.toDouble(&ok);
  if (ok) { _demodulator->setAGCTime(tau); }
}

void
DemodulatorCtrlView::onGainChanged(QString value) {
  if (! _gain->isEnabled()) { return; }
  bool ok;
  double gain = value.toDouble(&ok);
  if (ok) { _demodulator->setGain(gain); }
}


void
DemodulatorCtrlView::onUpdateGain() {
  if (!_gain->isEnabled()) {
    _gain->setText(QString("%1").arg(_demodulator->gain()));
  }
}

void
DemodulatorCtrlView::onCenterFreqChanged(QString value) {
  bool ok; double f = value.toDouble(&ok);
  if (ok) { _demodulator->setCenterFreq(f); }
}

void
DemodulatorCtrlView::onFilterChanged() {
  _centerFreq->setText(QString::number(_demodulator->centerFreq()));
}

/* ******************************************************************************************** *
 * Implementation of DemodInterface
 * ******************************************************************************************** */
DemodInterface::DemodInterface() {
  // pass...
}

DemodInterface::~DemodInterface() {
  // pass...
}


/* ******************************************************************************************** *
 * Implementation of AMDemodulator and view
 * ******************************************************************************************** */
AMDemodulator::AMDemodulator(DemodulatorCtrl *ctrl)
  : QObject(), DemodInterface(), _ctrl(ctrl), _demod(), _view(0)
{
  // First configure base-band for AM reception
  _ctrl->setFilterFrequency(0.0);
  _ctrl->setFilterWidth(6000);
}

AMDemodulator::~AMDemodulator() {
  if (0 != _view) { _view->deleteLater(); _view = 0; }
}


double
AMDemodulator::filterWidth() const {
  return _ctrl->filterWidth();
}

void
AMDemodulator::setFilterWidth(double width) {
  if (width > 0) {
    _ctrl->setFilterWidth(width);
  }
}

sdr::SinkBase *
AMDemodulator::sink() {
  return &_demod;
}

sdr::Source *
AMDemodulator::audioSource() {
  return &_demod;
}

QWidget *
AMDemodulator::createView() {
  if (0 == _view) {
    _view = new AMDemodulatorView(this);
    QObject::connect(_view, SIGNAL(destroyed()), this, SLOT(_onViewDeleted()));
  }
  return _view;
}

void
AMDemodulator::_onViewDeleted() {
  _view = 0;
}


AMDemodulatorView::AMDemodulatorView(AMDemodulator *demod, QWidget *parent)
  : QGroupBox("AM Demodulator", parent), _demod(demod)
{
  _filterWidth = new QLineEdit(QString::number(_demod->filterWidth()));
  QDoubleValidator *validator = new QDoubleValidator();
  validator->setBottom(0);
  _filterWidth->setValidator(validator);

  QFormLayout *layout = new QFormLayout();
  layout->addRow("Filter width", _filterWidth);
  setLayout(layout);

  QObject::connect(_filterWidth, SIGNAL(textEdited(QString)),
                   this, SLOT(_onFilterWidthChanged(QString)));
}

AMDemodulatorView::~AMDemodulatorView() {
  // pass...
}

void
AMDemodulatorView::_onFilterWidthChanged(QString value) {
  _demod->setFilterWidth(value.toDouble());
}


/* ******************************************************************************************** *
 * Implementation of FMDemodulator and view
 * ******************************************************************************************** */
FMDemodulator::FMDemodulator(DemodulatorCtrl *demod)
  : QObject(), DemodInterface(), _ctrl(demod), _demod(), _deemph(), _view(0)
{
  // connect stuff...
  _demod.connect(&_deemph, true);
}

FMDemodulator::~FMDemodulator() {
  if (0 != _view) { _view->deleteLater(); _view = 0; }
}

double
FMDemodulator::filterWidth() const {
  return _ctrl->filterWidth();
}

void
FMDemodulator::setFilterWidth(double width) {
  _ctrl->setFilterWidth(width);
}

bool
FMDemodulator::deemphEnabled() const {
  return _deemph.isEnabled();
}

void
FMDemodulator::enableDeemph(bool enable) {
  _deemph.enable(enable);
}

sdr::SinkBase *
FMDemodulator::sink() {
  return &_demod;
}

sdr::Source *
FMDemodulator::audioSource() {
  return &_deemph;
}

QWidget *
FMDemodulator::createView() {
  if (0 == _view) {
    _view = new FMDemodulatorView(this);
    QObject::connect(_view, SIGNAL(destroyed()), this, SLOT(_onViewDeleted()));
  }
  return _view;
}

void
FMDemodulator::_onViewDeleted() {
  _view = 0;
}

WFMDemodulator::WFMDemodulator(DemodulatorCtrl *demod)
  : FMDemodulator(demod)
{
  // Configure BaseBand for WFM RX:
  _ctrl->setFilterFrequency(0.0);
  _ctrl->setFilterWidth(200000.0);
}

WFMDemodulator::~WFMDemodulator() {
  // pass...
}

NFMDemodulator::NFMDemodulator(DemodulatorCtrl *demod)
  : FMDemodulator(demod)
{
  // Configure BaseBand for NFM RX:
  _ctrl->setFilterFrequency(0.0);
  _ctrl->setFilterWidth(12500.0);
}

NFMDemodulator::~NFMDemodulator() {
  // pass...
}


FMDemodulatorView::FMDemodulatorView(FMDemodulator *demod, QWidget *parent)
  : QGroupBox("FM Demodulator", parent), _demod(demod)
{
  _filterWidth = new QLineEdit(QString::number(_demod->filterWidth()));
  QDoubleValidator *validator = new QDoubleValidator();
  validator->setBottom(0);
  _filterWidth->setValidator(validator);

  QCheckBox *deemph = new QCheckBox();
  deemph->setChecked(_demod->deemphEnabled());

  QFormLayout *layout = new QFormLayout();
  layout->addRow("Filter width", _filterWidth);
  layout->addRow("De-emph.", deemph);
  setLayout(layout);

  QObject::connect(_filterWidth, SIGNAL(textEdited(QString)),
                   this, SLOT(_onFilterWidthChanged(QString)));
  QObject::connect(deemph, SIGNAL(toggled(bool)), this, SLOT(_onDeemphToggled(bool)));
}

FMDemodulatorView::~FMDemodulatorView() {
  // pass...
}

void
FMDemodulatorView::_onFilterWidthChanged(QString value) {
  _demod->setFilterWidth(value.toDouble());
}

void
FMDemodulatorView::_onDeemphToggled(bool enabled) {
  _demod->enableDeemph(enabled);
}


/* ******************************************************************************************** *
 * Implementation of SSBDemodulator and view
 * ******************************************************************************************** */
SSBDemodulator::SSBDemodulator(DemodulatorCtrl *ctrl, QObject *parent)
  : QObject(parent), _ctrl(ctrl), _demod(), _view(0)
{
  // pass...
}

SSBDemodulator::~SSBDemodulator() {
  if (_view) { _view->deleteLater(); _view = 0; }
}


double
SSBDemodulator::filterWidth() const {
  return _ctrl->filterWidth();
}

void
SSBDemodulator::setFilterWidth(double width) {
  _ctrl->setFilterWidth(width);
}

double
SSBDemodulator::filterFrequency() const {
  return _ctrl->filterFrequency();
}

void
SSBDemodulator::setFilterFrequency(double f) {
  _ctrl->setFilterFrequency(f);
}


sdr::SinkBase *
SSBDemodulator::sink() {
  return &_demod;
}

sdr::Source *
SSBDemodulator::audioSource() {
  return &_demod;
}

QWidget *
SSBDemodulator::createView() {
  if (0 == _view) {
    _view = new SSBDemodulatorView(this);
    QObject::connect(_view, SIGNAL(destroyed()), this, SLOT(_onViewDeleted()));
  }
  return _view;
}

void
SSBDemodulator::_onViewDeleted() {
  _view = 0;
}

USBDemodulator::USBDemodulator(DemodulatorCtrl *ctrl, QObject *parent)
  : SSBDemodulator(ctrl, parent)
{
  // Configure BaseBand for SSB reception
  _ctrl->setFilterWidth(3000);
  _ctrl->setFilterFrequency(1700);
}

USBDemodulator::~USBDemodulator() {
  // pass...
}

LSBDemodulator::LSBDemodulator(DemodulatorCtrl *ctrl, QObject *parent)
  : SSBDemodulator(ctrl, parent)
{
  // Configure BaseBand for SSB reception
  _ctrl->setFilterWidth(3000);
  _ctrl->setFilterFrequency(-1700);
}

LSBDemodulator::~LSBDemodulator() {
  // pass...
}

CWDemodulator::CWDemodulator(DemodulatorCtrl *ctrl, QObject *parent)
  : SSBDemodulator(ctrl, parent)
{
  // Configure BaseBand for CW reception
  _ctrl->setFilterWidth(200);
  _ctrl->setFilterFrequency(700);
}

CWDemodulator::~CWDemodulator() {
  // pass...
}


SSBDemodulatorView::SSBDemodulatorView(SSBDemodulator *demod, QWidget *parent)
  : QGroupBox("SSB Demodulator", parent), _demod(demod)
{
  _filterWidth = new QLineEdit(QString::number(_demod->filterWidth()));
  QDoubleValidator *validator = new QDoubleValidator();
  validator->setBottom(0);
  _filterWidth->setValidator(validator);

  QFormLayout *layout = new QFormLayout();
  layout->addRow("Filter width", _filterWidth);
  setLayout(layout);

  QObject::connect(_filterWidth, SIGNAL(textEdited(QString)),
                   this, SLOT(_onFilterWidthChanged(QString)));
}

SSBDemodulatorView::~SSBDemodulatorView() {
  // pass...
}

void
SSBDemodulatorView::_onFilterWidthChanged(QString value) {
  double new_width = value.toDouble();
  // Get change in center frequency to maintain lower cut-off frequency of the filter
  double dF = (new_width - _demod->filterWidth())/2;
  _demod->setFilterWidth(new_width);
  _demod->setFilterFrequency(_demod->filterFrequency()+dF);
}



/* ******************************************************************************************** *
 * Implementation of BPSKDemodulator and view
 * ******************************************************************************************** */
BPSK31Demodulator::BPSK31Demodulator(DemodulatorCtrl *ctrl, QObject *parent)
  : QObject(parent), sdr::Sink<uint8_t>(),
    _ctrl(ctrl), _input_proxy(), _freq_shift(700.0), _audio_demod(), _bpsk_filter(31, 200), _bpsk(),
    _decode(), _text_buffer(""), _view(0)
{
  // Configure BaseBand to RX BPSK31 stuff
  _ctrl->setFilterFrequency(0);
  _ctrl->setFilterWidth(400);

  // Connect decoder and audio path
  _input_proxy.connect(&_freq_shift);
  _input_proxy.connect(&_bpsk_filter);
  _freq_shift.connect(&_audio_demod, true);
  _bpsk_filter.connect(&_bpsk, true);
  _bpsk.connect(&_decode, true);
  _decode.connect(this, true);
}

BPSK31Demodulator::~BPSK31Demodulator() {
  if (_view) { _view->deleteLater(); _view = 0; }
}

double
BPSK31Demodulator::filterWidth() const {
  return _ctrl->filterWidth();
}

void
BPSK31Demodulator::setFilterWidth(double width) {
  _ctrl->setFilterWidth(width);
  _bpsk_filter.setFreq(width/2);
}

sdr::SinkBase *
BPSK31Demodulator::sink() {
  return &_input_proxy;
}

sdr::Source *
BPSK31Demodulator::audioSource() {
  return &_audio_demod;
}

QWidget *
BPSK31Demodulator::createView() {
  if (0 == _view) {
    _view = new BPSK31DemodulatorView(this);
    QObject::connect(_view, SIGNAL(destroyed()), this, SLOT(_onViewDeleted()));
  }
  return _view;
}

void
BPSK31Demodulator::config(const Config &src_cfg) {
  // pass...
}

void
BPSK31Demodulator::process(const sdr::Buffer<uint8_t> &buffer, bool allow_overwrite) {
  for (size_t i=0; i<buffer.size(); i++) {
    _text_buffer.append(char(buffer[i]));
  }
  emit textReceived();
}

const QString &
BPSK31Demodulator::text() const {
  return _text_buffer;
}

void
BPSK31Demodulator::clearText() {
  _text_buffer = "";
}

void
BPSK31Demodulator::_onViewDeleted() {
  _view = 0;
}


BPSK31DemodulatorView::BPSK31DemodulatorView(BPSK31Demodulator *demod, QWidget *parent)
  : QGroupBox("BPSK31 Demodulator", parent), _demod(demod)
{
  _filterWidth = new QLineEdit(QString::number(_demod->filterWidth()));
  QDoubleValidator *validator = new QDoubleValidator();
  validator->setBottom(0);
  _filterWidth->setValidator(validator);

  _text = new QPlainTextEdit();
  _text->setReadOnly(true);
  _text->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setContentsMargins(0, 5, 0, 0);
  QFormLayout *form_layout = new QFormLayout();
  form_layout->addRow("Filter width", _filterWidth);
  layout->addLayout(form_layout, 0);
  layout->addWidget(_text, 1);

  setLayout(layout);

  QObject::connect(_filterWidth, SIGNAL(textEdited(QString)),
                   this, SLOT(_onFilterWidthChanged(QString)));
  QObject::connect(_demod, SIGNAL(textReceived()), this, SLOT(_onTextReceived()));
}

BPSK31DemodulatorView::~BPSK31DemodulatorView() {
// pass...
}

void
BPSK31DemodulatorView::_onTextReceived() {
  QString text = _demod->text(); _demod->clearText();
  _text->insertPlainText(text);
  _text->ensureCursorVisible();
}

void
BPSK31DemodulatorView::_onFilterWidthChanged(QString value) {
  _demod->setFilterWidth(value.toDouble());
}
