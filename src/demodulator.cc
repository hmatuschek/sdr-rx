#include "demodulator.hh"
#include "receiver.hh"

#include <QPainter>
#include <QPaintEvent>
#include <QComboBox>
#include <QTimer>
#include <QFormLayout>

using namespace sdr;


Demodulator::Demodulator(Receiver *receiver) :
  gui::Spectrum(2, 1024, 5, receiver), _receiver(receiver),
  _Fc(0), _Fl(-2000), _Fu(2000), _demod(DEMOD_SSB)
{
  // Assemble processing chain
  _agc = new AGC< std::complex<int16_t> >();
  _filter_node = new IQBaseBand<int16_t>(_Fc, _Fu-_Fl, 15, 1, 8000.0);
  _ssb_demod = new sdr::USBDemod<int16_t>();
  _am_demod  = new sdr::AMDemod<int16_t>();
  _fm_demod  = new sdr::FMDemod<int16_t>();
  _audio_source = new sdr::Proxy();

  _agc->connect(_filter_node, true);
  _agc->connect(this);
  _agc->enable(false);
  _agc->setGain(1);

  setDemod(_demod);
}


Demodulator::~Demodulator() {
  delete _agc;
  delete _filter_node;
  delete _ssb_demod;
  delete _am_demod;
  delete _audio_source;
}


bool
Demodulator::isAGCEnabled() const {
  return _agc->enabled();
}

double
Demodulator::gain() const {
  return 20*std::log10(_agc->gain());
}

void
Demodulator::setGain(double gain) {
  _agc->setGain(pow(10, gain/20));
}


double
Demodulator::agcTime() const {
  return _agc->tau();
}

void
Demodulator::setAGCTime(double tau) {
  _agc->setTau(tau);
}


void
Demodulator::enableAGC(bool enable) {
  _agc->enable(enable);
}

void
Demodulator::setCenterFreq(double f) {
  // Update filter boundaries
  double Fl = _Fl-_Fc+f;
  double Fu = _Fu-_Fc+f;
  _Fc = f;
  _filter_node->setCenterFrequency(_Fc);
  setFilter(Fl,Fu);
}

void
Demodulator::setFilter(double Fl, double Fu) {
  _Fl = Fl; _Fu = Fu; double width = _Fu-_Fl;
  _filter_node->setFilterFrequency(_Fl+width/2);
  _filter_node->setFilterWidth(width);

  bool was_running = _receiver->isRunning();
  if (was_running) { _receiver->stop(); }
  // Update resampling of IQBaseBand node. Ensures that the output sample-rate is at least
  // filter width and >= 16000 Hz
  double oFs = std::max(width, 16000.0);
  _filter_node->setOutputSampleRate(oFs);
  if (was_running) { _receiver->start(); }

  emit filterChanged();
}


void
Demodulator::setDemod(Demod demod) {
  bool was_running = _receiver->isRunning();
  if (was_running) { _receiver->stop(); }
  // Unlink current demodulator
  switch (_demod) {
  case DEMOD_SSB:
    _filter_node->disconnect(_ssb_demod);
    _ssb_demod->disconnect(_audio_source);
    break;
  case DEMOD_AM:
    _filter_node->disconnect(_am_demod);
    _am_demod->disconnect(_audio_source);
    break;
  case DEMOD_FM:
    _filter_node->disconnect(_fm_demod);
    _fm_demod->disconnect(_audio_source);
    break;
  }
  _demod = demod;
  // Link new demodulator
  switch (_demod) {
  case DEMOD_SSB:
    _filter_node->connect(_ssb_demod, true);
    _ssb_demod->connect(_audio_source, true);
    break;
  case DEMOD_AM:
    _filter_node->connect(_am_demod, true);
    _am_demod->connect(_audio_source, true);
    break;
  case DEMOD_FM:
    _filter_node->connect(_fm_demod, true);
    _fm_demod->connect(_audio_source, true);
    break;

  }
  if (was_running) { _receiver->start(); }
}


QWidget *
Demodulator::createCtrlView() {
  return new DemodulatorCtrlView(this);
}


QWidget *
Demodulator::createSpectrumView() {
  //DemodulatorSpectrumView *view = new DemodulatorSpectrumView(this);
  DemodulatorWaterFallView *view = new DemodulatorWaterFallView(this);
  QObject::connect(view, SIGNAL(click(double)), this, SLOT(setCenterFreq(double)));
  return view;
}



/* ******************************************************************************************** *
 * Implementation of DemodulatorSpectrumView
 * ******************************************************************************************** */
DemodulatorSpectrumView::DemodulatorSpectrumView(Demodulator *demodulator)
  : gui::SpectrumView(demodulator), _demodulator(demodulator)
{
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
DemodulatorWaterFallView::DemodulatorWaterFallView(Demodulator *demodulator)
  : gui::WaterFallView(demodulator, 200), _demodulator(demodulator)
{
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
DemodulatorCtrlView::DemodulatorCtrlView(Demodulator *demodulator, QWidget *parent)
  : QWidget(parent), _demodulator(demodulator)
{
  QComboBox *mod = new QComboBox();
  mod->addItem("AM");
  mod->addItem("WFM");
  mod->addItem("NFM");
  mod->addItem("DSB");
  mod->addItem("LSB");
  mod->addItem("USB");
  mod->addItem("CW");
  this->onModSelected(0);

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

  _lowerFreq = new QLineEdit();
  QDoubleValidator *fl_val = new QDoubleValidator();
  if (_demodulator->isInputReal()) { fl_val->setBottom(0); fl_val->setTop(_demodulator->sampleRate()/2); }
  else { fl_val->setBottom(-_demodulator->sampleRate()/2); fl_val->setTop(_demodulator->sampleRate()/2); }
  _lowerFreq->setValidator(fl_val);
  _lowerFreq->setText(QString("%1").arg(_demodulator->filterLower()));

  _upperFreq = new QLineEdit();
  QDoubleValidator *fu_val = new QDoubleValidator();
  if (_demodulator->isInputReal()) { fu_val->setBottom(0); fu_val->setTop(_demodulator->sampleRate()/2); }
  else { fu_val->setBottom(-_demodulator->sampleRate()/2); fu_val->setTop(_demodulator->sampleRate()/2); }
  _upperFreq->setValidator(fu_val);
  _upperFreq->setText(QString("%1").arg(_demodulator->filterUpper()));

  QTimer *gainUpdate = new QTimer(this);
  gainUpdate->setInterval(500);
  gainUpdate->setSingleShot(false);
  gainUpdate->start();

  QObject::connect(mod, SIGNAL(activated(int)), this, SLOT(onModSelected(int)));
  QObject::connect(_agc, SIGNAL(toggled(bool)), SLOT(onAGCToggled(bool)));
  QObject::connect(_agc_tau, SIGNAL(textEdited(QString)), this, SLOT(onAGCTauChanged(QString)));
  QObject::connect(_gain, SIGNAL(textEdited(QString)), SLOT(onGainChanged(QString)));
  QObject::connect(gainUpdate, SIGNAL(timeout()), SLOT(onUpdateGain()));
  QObject::connect(_centerFreq, SIGNAL(textEdited(QString)), this, SLOT(onCenterFreqChanged(QString)));
  QObject::connect(_lowerFreq, SIGNAL(textEdited(QString)), this, SLOT(onLowerFreqChanged(QString)));
  QObject::connect(_upperFreq, SIGNAL(textEdited(QString)), this, SLOT(onUpperFreqChanged(QString)));
  QObject::connect(_demodulator, SIGNAL(filterChanged()), this, SLOT(onUpdateFilter()));

  QFormLayout *side = new QFormLayout();
  side->addRow("Modulation", mod);
  side->addRow("Gain", _gain);
  side->addWidget(_agc);
  side->addRow("AGC time", _agc_tau);
  side->addRow("Center freq.", _centerFreq);
  side->addRow("Lower freq.", _lowerFreq);
  side->addRow("Upper freq.", _upperFreq);

  this->setLayout(side);
}


void
DemodulatorCtrlView::onModSelected(int idx) {
  if (0 == idx) { // AM
      double fc   = _demodulator->centerFreq();
      double fmin = fc-5000, fmax = fc+5000;
      _demodulator->setFilter(fmin, fmax);
      _demodulator->setDemod(Demodulator::DEMOD_AM);
  } else if (1 == idx) { // WFM
    double fc   = _demodulator->centerFreq();
    double fmin = fc-100000, fmax = fc+100000;
    _demodulator->setFilter(fmin, fmax);
    _demodulator->setDemod(Demodulator::DEMOD_FM);
  } else if (2 == idx) { // NFM
    double fc   = _demodulator->centerFreq();
    double fmin = fc-6000, fmax = fc+6000;
    _demodulator->setFilter(fmin, fmax);
    _demodulator->setDemod(Demodulator::DEMOD_FM);
  } else if (3 == idx) { // DSB
    double fc   = _demodulator->centerFreq();
    double fmin = fc-2000, fmax = fc+2000;
    _demodulator->setFilter(fmin, fmax);
    _demodulator->setDemod(Demodulator::DEMOD_SSB);
  } else if (4 == idx) { // LSB
    double fc   = _demodulator->centerFreq();
    double fmin = fc-2000, fmax = fc-100;
    _demodulator->setFilter(fmin, fmax);
    _demodulator->setDemod(Demodulator::DEMOD_SSB);
  } else if (5 == idx) { // USB
    double fc   = _demodulator->centerFreq();
    double fmin = fc+100, fmax = fc+2000;
    _demodulator->setFilter(fmin, fmax);
    _demodulator->setDemod(Demodulator::DEMOD_SSB);
  } else if (6 == idx) { // CW
    double fc   = _demodulator->centerFreq();
    double fmin = fc+600, fmax = fc+900;
    _demodulator->setFilter(fmin, fmax);
    _demodulator->setDemod(Demodulator::DEMOD_SSB);
  } else {
    LogMessage msg(LOG_WARNING);
    msg << "Unkwon demodulator index: " << idx
        << " not in range [0,6]";
    Logger::get().log(msg);
  }
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
DemodulatorCtrlView::onUpdateFilter() {
  _centerFreq->setText(QString("%1").arg(_demodulator->centerFreq()));
  _lowerFreq->setText(QString("%1").arg(_demodulator->filterLower()));
  _upperFreq->setText(QString("%1").arg(_demodulator->filterUpper()));
}


void
DemodulatorCtrlView::onCenterFreqChanged(QString value) {
  double fc   = _demodulator->centerFreq();
  double fmin = _demodulator->filterLower();
  double fmax = _demodulator->filterUpper();
  bool ok; double f = value.toDouble(&ok);
  if (ok) {
    _demodulator->setCenterFreq(f);
    _demodulator->setFilter(fmin-fc+f, fmax-fc+f);
  }
}

void
DemodulatorCtrlView::onLowerFreqChanged(QString value) {
  double fmax = _demodulator->filterUpper();
  bool ok; double f = value.toDouble(&ok);
  if (ok) { _demodulator->setFilter(f,fmax); }
}

void
DemodulatorCtrlView::onUpperFreqChanged(QString value) {
  double fmin = _demodulator->filterLower();
  bool ok; double f = value.toDouble(&ok);
  if (ok) { _demodulator->setFilter(fmin,f); }
}
