#ifndef __SDR_RX_RTLDATASOURCE_HH__
#define __SDR_RX_RTLDATASOURCE_HH__

#include "source.hh"
#include "rtlsource.hh"
#include "utils.hh"
#include "autocast.hh"
#include "configuration.hh"

#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QMenu>

/** Persistent configuration of the RTL device. */
class RTLDataSourceConfig
{
public:
  RTLDataSourceConfig();
  virtual ~RTLDataSourceConfig();

  double frequency() const;
  void storeFrequency(double f);

  double sampleRate() const;
  void storeSampleRate(double rate);

protected:
  /** The global config instance. */
  Configuration &_config;
};


class RTLDataSource : public DataSource
{
  Q_OBJECT

public:
  RTLDataSource(QObject *parent=0);
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
  const std::vector<double> &gainFactors() const;

  double IQBalance() const;
  void setIQBalance(double balance);

  void setDevice(size_t idx);

  static size_t numDevices();
  static std::string deviceName(size_t idx);

protected:
  sdr::RTLSource *_device;
  sdr::AutoCast< std::complex<int16_t> > *_to_int16;
  sdr::IQBalance< std::complex<int16_t> > _balance;
  RTLDataSourceConfig _config;
};


class RTLCtrlView: public QWidget
{
  Q_OBJECT

public:
  RTLCtrlView(RTLDataSource *source, QWidget *parent=0);
  virtual ~RTLCtrlView();

protected slots:
  void onDeviceSelected(int idx);
  void onFrequencyChanged();
  void onSaveFrequency();
  void onSampleRateSelected(int idx);
  void onGainChanged(int idx);
  void onAGCToggled(bool enabled);
  void onBalanceChanged();

protected:
  QLabel *_errorMessage;
  RTLDataSource *_source;

  QComboBox *_devices;
  QLineEdit *_freq;
  QMenu     *_freqMenu;
  QAction   *_saveFreqAction;
  QComboBox *_sampleRates;
  QComboBox *_gain;
  QCheckBox *_agc;
  QLineEdit *_balance;
};

#endif // __SDR_RX_RTLDATASOURCE_HH__
