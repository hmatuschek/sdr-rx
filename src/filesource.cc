#include "filesource.hh"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QFileDialog>


using namespace sdr;


FileSource::FileSource(QObject *parent)
  : ::DataSource(parent), Proxy()
{
  // Assemble nodes
  _src = new WavSource(4*1024);
  _to_complex = new AutoCast< std::complex<int16_t> >();
  _src->connect(_to_complex, true);
  _to_complex->connect(this, true);
}

FileSource::~FileSource() {
  delete _src;
  delete _to_complex;
}

bool
FileSource::isOpen() const {
  return _src->isOpen();
}

void
FileSource::open(const QString &filepath) {
  // Open file and read header
  try { _src->open(filepath.toStdString()); }
  catch (RuntimeError &err) {
    LogMessage msg(LOG_WARNING);
    msg << __FILE__ << ": Can not open WAV file "
        << filepath.toStdString() << ": " << err.what();
    Logger::get().log(msg);
  }

  // Can not open file
  if (! _src->isOpen()) { return; }
  _filename = filepath;
}


const QString &
FileSource::filepath() const {
  return _filename;
}

bool
FileSource::isReal() const {
  return _src->isReal();
}

Config::Type
FileSource::format() const {
  return _src->type();
}

void
FileSource::next() {
  _src->next();
}

QWidget *
FileSource::createCtrlView() {
  return new FileSourceView(this);
}

Source *
FileSource::source() {
  return this;
}

void
FileSource::triggerNext() {
  next();
}



/* ******************************************************************************************** *
 * FileSourceView
 * ******************************************************************************************** */
FileSourceView::FileSourceView(FileSource *src, QWidget *parent)
  : QWidget(parent), _source(src)
{

  _filename = new QLineEdit(_source->filepath());
  QToolButton *fsel = new QToolButton();
  fsel->setText("...");

  _error_message = new QLabel();
  _error_message->setText("<b>Can not open file...</b>");

  _iq = new QLabel();
  _format = new QLabel();
  _sample_rate = new QLabel();

  if (_source->isOpen()) {
    _error_message->setVisible(false);
    _iq->setText(_source->isReal() ? "real" : "I/Q");
    _format->setText(typeName(_source->format()));
    _sample_rate->setText(QString("%1 Hz").arg(_source->sampleRate()));
  } else {
    _error_message->setVisible(true);
    _iq->setText("-");
    _format->setText("-");
    _sample_rate->setText("-");
  }

  QObject::connect(fsel, SIGNAL(clicked()), this, SLOT(_onSelectFile()));
  QObject::connect(_filename, SIGNAL(editingFinished()), this, SLOT(_onFileSelected()));

  QHBoxLayout *fbox = new QHBoxLayout();
  fbox->setContentsMargins(0,0,0,0);
  fbox->setMargin(0);
  fbox->addWidget(_filename);
  fbox->addWidget(fsel, 0);

  QFormLayout *layout = new QFormLayout();
  layout->addRow("File", fbox);
  layout->addWidget(_error_message);
  layout->addRow("Type", _iq);
  layout->addRow("Format", _format);
  layout->addRow("Sample rate", _sample_rate);
  setLayout(layout);
}

FileSourceView::~FileSourceView() {
  // pass...
}

void
FileSourceView::_onSelectFile() {
  QString filename = QFileDialog::getOpenFileName(0, "", "", "WAV (*.wav);; All files (*)");
  if (0 == filename.size()) { return; }
  _filename->setText(filename);
  _onFileSelected();
}

void
FileSourceView::_onFileSelected() {
  LogMessage msg(LOG_DEBUG);
  msg << "Try to open WAV file: " << _filename->text().toStdString();
  Logger::get().log(msg);

  _source->open(_filename->text());
  if (_source->isOpen()) {
    _error_message->setVisible(false);
    _iq->setText(_source->isReal() ? "real" : "I/Q");
    _format->setText(typeName(_source->format()));
    _sample_rate->setText(QString("%1 Hz").arg(_source->sampleRate()));
  } else {
    _error_message->setVisible(true);
    _iq->setText("-");
    _format->setText("-");
    _sample_rate->setText("-");
  }
}
