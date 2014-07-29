#ifndef __SDR_RX_FILESOURCE_HH__
#define __SDR_RX_FILESOURCE_HH__

#include "wavfile.hh"
#include "source.hh"
#include "autocast.hh"
#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>


class FileSource : public DataSource, public sdr::Proxy
{
  Q_OBJECT

public:
  explicit FileSource(QObject *parent=0);
  virtual ~FileSource();

  bool isOpen() const;
  const QString &filepath() const;
  bool isReal() const;
  sdr::Config::Type format() const;

  void next();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source();
  virtual void triggerNext();

public slots:
  void open(const QString &filepath);

protected:
  sdr::WavSource *_src;
  sdr::AutoCast< std::complex<int16_t> > *_to_complex;
  QString _filename;
};


class FileSourceView: public QWidget
{
  Q_OBJECT

public:
  FileSourceView(FileSource *src, QWidget *parent=0);
  virtual ~FileSourceView();

protected slots:
  void _onSelectFile();
  void _onFileSelected();

protected:
  FileSource *_source;

  QLineEdit *_filename;
  QLabel *_error_message;
  QLabel *_iq;
  QLabel *_format;
  QLabel *_sample_rate;
};

#endif // __SDR_RX_FILESOURCE_HH__
