#include "audiopostproc.hh"
#include <QLineEdit>
#include <QDoubleValidator>
#include <QCheckBox>
#include <QFormLayout>

using namespace sdr;

AudioPostProc::AudioPostProc(QObject *parent)
  : QObject(parent), Sink<int16_t>()
{
  // Assemble processing chain
  _sub_sample = new SubSample<int16_t>(16000.0);
  _low_pass   = new FIRLowPass<int16_t>(31, 3e3);
  _low_pass->enable(false);
  _sink       = new PortSink();
  _audio_spectrum = new gui::Spectrum(2, 256, 5, this);

  // Connect all
  _sub_sample->connect(_low_pass, true);
  _low_pass->connect(_sink);
  _low_pass->connect(_audio_spectrum);
}

AudioPostProc::~AudioPostProc() {
  delete _low_pass;
  delete _sink;
}


void
AudioPostProc::config(const Config &src_cfg) {
  // Forward to low pass
  _sub_sample->config(src_cfg);
}

void
AudioPostProc::process(const Buffer<int16_t> &buffer, bool allow_overwrite) {
  // Forward to low pass
  _sub_sample->process(buffer, allow_overwrite);
}

bool
AudioPostProc::lowPassEnabled() const {
  return _low_pass->enabled();
}

void
AudioPostProc::enableLowPass(bool enable) {
  _low_pass->enable(enable);
}

double
AudioPostProc::lowPassFreq() const {
  return _low_pass->freq();
}

void
AudioPostProc::setLowPassFreq(double freq) {
  _low_pass->setFreq(freq);
}

size_t
AudioPostProc::lowPassOrder() const {
  return _low_pass->order();
}

void
AudioPostProc::setLowPassOrder(size_t order) {
  _low_pass->setOrder(order);
}

gui::Spectrum *
AudioPostProc::spectrum() const {
  return _audio_spectrum;
}



/* ******************************************************************************************** *
 * Implementation of View
 * ******************************************************************************************** */
AudioPostProcView::AudioPostProcView(AudioPostProc *proc, QWidget *parent)
  : QWidget(parent), _proc(proc)
{
  QCheckBox *lp_enable = new QCheckBox("enable");
  lp_enable->setChecked(proc->lowPassEnabled());

  _lp_freq = new QLineEdit(QString("%1").arg(proc->lowPassFreq()));
  QDoubleValidator *lpf_val = new QDoubleValidator();
  lpf_val->setBottom(0); _lp_freq->setValidator(lpf_val);

  _lp_order = new QSpinBox();
  _lp_order->setRange(0, 1024); _lp_order->setValue(_proc->lowPassOrder());
  _lp_order->setSingleStep(5);

  if (! proc->lowPassEnabled()) {
    _lp_freq->setEnabled(false);
    _lp_order->setEnabled(false);
  }

  // Create spectrum view:
  _spectrum = new gui::SpectrumView(_proc->spectrum());
  _spectrum->setNumXTicks(5);

  QObject::connect(_lp_freq, SIGNAL(textEdited(QString)), this, SLOT(onSetLowPassFreq(QString)));
  QObject::connect(_lp_order, SIGNAL(valueChanged(int)), this, SLOT(onSetLowPassOrder(int)));
  QObject::connect(lp_enable, SIGNAL(toggled(bool)), this, SLOT(onLowPassToggled(bool)));

  // Layout
  QVBoxLayout *layout = new QVBoxLayout();
  QFormLayout *table = new QFormLayout();
  table->addRow("Low Pass (Hz)", _lp_freq);
  table->addRow("order", _lp_order);
  table->addWidget(lp_enable);
  layout->addLayout(table, 0);

  layout->addWidget(_spectrum, 1);

  this->setLayout(layout);
}

AudioPostProcView::~AudioPostProcView() {
  // pass...
}

void
AudioPostProcView::onLowPassToggled(bool enable) {
  _proc->enableLowPass(enable);
  _lp_freq->setEnabled(enable);
  _lp_order->setEnabled(enable);
}

void
AudioPostProcView::onSetLowPassFreq(QString value) {
  bool ok; double freq = value.toDouble(&ok);
  if (ok) {
    _proc->setLowPassFreq(freq);
  }
}

void
AudioPostProcView::onSetLowPassOrder(int value) {
  if (value < 1) { _lp_order->setValue(1); }
  _proc->setLowPassOrder((size_t) value);
}
