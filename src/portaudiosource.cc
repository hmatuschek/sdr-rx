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
  _src = new PortSource<int16_t>(44100, 1024);
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



/* ********************************************************************************************* *
 * Implementation of PortAudioIQSource
 * ********************************************************************************************* */
PortAudioIQSource::PortAudioIQSource(QObject *parent)
  : ::DataSource(parent), sdr::Proxy(), _src(0)
{
  // Allocate and connect stuff:
  _src = new PortSource< std::complex<int16_t> >(44100, 1024);
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


/* ********************************************************************************************* *
 * Implementation of PortAudioSourceView
 * ********************************************************************************************* */
PortAudioSourceView::PortAudioSourceView(PortAudioSource *src, QWidget *parent)
  : QWidget(parent), _src(src)
{
  _sample_rate = new QLabel();
  _sample_rate->setText(QString::number(_src->sampleRate()));

  _format = new QLabel();
  _format->setText(typeName(_src->format()));

  QFormLayout *layout = new QFormLayout();
  layout->addRow("Sample rate", _sample_rate);
  layout->addRow("Format", _format);

  // Delete view on desctruction of source object
  QObject::connect(src, SIGNAL(destroyed()), this, SLOT(_onSourceDeleted()));

  setLayout(layout);
}

PortAudioSourceView::~PortAudioSourceView() {
  // pass...
}


void
PortAudioSourceView::_onSourceDeleted() {
  this->deleteLater();
}



/* ********************************************************************************************* *
 * Implementation of PortAudioIQSourceView
 * ********************************************************************************************* */
PortAudioIQSourceView::PortAudioIQSourceView(PortAudioIQSource *src, QWidget *parent)
  : QWidget(parent), _src(src)
{
  _sample_rate = new QLabel();
  _sample_rate->setText(QString::number(_src->sampleRate()));

  _format = new QLabel(typeName(_src->format()));

  QFormLayout *layout = new QFormLayout();
  layout->addRow("Sample rate", _sample_rate);
  layout->addRow("Format", _format);

  // Delete view on desctruction of source object
  QObject::connect(src, SIGNAL(destroyed()), this, SLOT(_onSourceDeleted()));

  setLayout(layout);
}

PortAudioIQSourceView::~PortAudioIQSourceView() {
  // pass...
}


void
PortAudioIQSourceView::_onSourceDeleted() {
  this->deleteLater();
}
