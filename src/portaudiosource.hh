#ifndef __SDR_RX_PORTAUDIOSOURCE_HH__
#define __SDR_RX_PORTAUDIOSOURCE_HH__

#include "portaudio.hh"
#include "source.hh"
#include "utils.hh"
#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>


class PortAudioSource: public DataSource, public sdr::Proxy
{
  Q_OBJECT

public:
  explicit PortAudioSource(QObject *parent=0);
  virtual ~PortAudioSource();

  sdr::Config::Type format() const;

  void next();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source();
  virtual void triggerNext();

  const std::vector<double> &sampleRates() const;
  void setSampleRate(double rate);

protected:
  sdr::PortSource<int16_t> *_src;
  sdr::ToComplex<int16_t, int16_t>  *_to_complex;
  std::vector<double> _sampleRates;
};


class PortAudioIQSource: public DataSource, public sdr::Proxy
{
  Q_OBJECT

public:
  explicit PortAudioIQSource(QObject *parent=0);
  virtual ~PortAudioIQSource();

  sdr::Config::Type format() const;

  void next();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source();
  virtual void triggerNext();

  const std::vector<double> &sampleRates() const;
  void setSampleRate(double rate);

protected:
  sdr::PortSource< std::complex<int16_t> > *_src;
  std::vector<double> _sampleRates;
};


class PortAudioSourceView: public QWidget
{
  Q_OBJECT

public:
  PortAudioSourceView(PortAudioSource *src, QWidget *parent=0);
  virtual ~PortAudioSourceView();

protected slots:
  void _onSourceDeleted();
  void _onSampleRateSelected(int idx);

protected:
  // A weak reference to the source object
  PortAudioSource *_src;
  // The sample-rate
  QComboBox *_sample_rate;
  QLabel *_format;
};


class PortAudioIQSourceView: public QWidget
{
  Q_OBJECT

public:
  PortAudioIQSourceView(PortAudioIQSource *src, QWidget *parent=0);
  virtual ~PortAudioIQSourceView();

protected slots:
  void _onSourceDeleted();
  void _onSampleRateSelected(int idx);

protected:
  // A weak reference to the source object
  PortAudioIQSource *_src;
  // The sample-rate
  QComboBox *_sample_rate;
  QLabel *_format;
};

#endif // __SDR_RX_PORTAUDIOSOURCE_HH__
