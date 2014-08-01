#ifndef __SDR_RX_RTLDATASOURCE_HH__
#define __SDR_RX_RTLDATASOURCE_HH__

#include "source.hh"
#include "rtlsource.hh"
#include "utils.hh"
#include "autocast.hh"

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>


class RTLDataSource : public DataSource
{
  Q_OBJECT

public:
  RTLDataSource(double frequency, double sample_rate, QObject *parent=0);
  virtual ~RTLDataSource();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source();

  virtual void queueStarted();
  virtual void queueStopped();

  virtual double tunerFrequency() const;

  bool isActive() const;

  double frequency() const;
  void setFrequency(double freq);

  double sampleRate() const;
  void setSampleRate(double rate);

  bool agcEnabled() const;
  void enableAGC(bool enable);

  double gain() const;
  void setGain(double gain);

  void setDevice(size_t idx);

  static size_t numDevices();
  static std::string deviceName(size_t idx);

protected:
  sdr::RTLSource *_device;
  sdr::AutoCast<std::complex<int16_t> > *_to_int16;
};


class RTLCtrlView: public QWidget
{
  Q_OBJECT

public:
  RTLCtrlView(RTLDataSource *source, QWidget *parent=0);
  virtual ~RTLCtrlView();

protected slots:
  void onDeviceSelected(int idx);
  void onFrequencyChanged(QString value);
  void onSampleRateSelected(int idx);
  void onGainChanged(QString value);
  void onAGCToggled(bool enabled);

protected:
  QLabel *_errorMessage;
  RTLDataSource *_source;

  QComboBox *_devices;
  QLineEdit *_freq;
  QComboBox *_sampleRates;
  QLineEdit *_gain;
  QCheckBox *_agc;
};

#endif // __SDR_RX_RTLDATASOURCE_HH__
