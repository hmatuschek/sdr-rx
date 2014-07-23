#ifndef __SDR_RX_DEMODULATOR_HH__
#define __SDR_RX_DEMODULATOR_HH__

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>

#include "gui/spectrum.hh"
#include "gui/spectrumview.hh"
#include "gui/waterfallview.hh"
#include "baseband.hh"

// Forward declaration
class Receiver;



class Demodulator : public sdr::gui::Spectrum
{
  Q_OBJECT

public:
  typedef enum {
    DEMOD_SSB, DEMOD_AM, DEMOD_FM
  } Demod;

public:
  explicit Demodulator(Receiver *receiver = 0);
  virtual ~Demodulator();

  bool isAGCEnabled() const;
  double gain() const;
  double agcTime() const;

  inline double centerFreq() const { return _Fc; }
  inline double filterLower() const { return _Fl; }
  inline double filterUpper() const { return _Fu; }

  inline Demod demod() const { return _demod; }

  inline sdr::SinkBase *in() const { return _agc; }
  inline sdr::Source *audioSource() const { return _audio_source; }

  QWidget *createCtrlView();
  QWidget *createSpectrumView();

signals:
  void filterChanged();

public slots:
  void enableAGC(bool enable);
  void setGain(double gain);
  void setAGCTime(double tau);

  void setCenterFreq(double f);
  void setFilter(double Fl, double Fu);

  void setDemod(Demod demod);

protected:
  Receiver *_receiver;

  double _Fc;
  double _Fl;
  double _Fu;

  /** The currently selected demodulator. */
  Demod _demod;

  // A AGC
  sdr::AGC< std::complex<int16_t> > *_agc;
  // The filter node
  sdr::IQBaseBand<int16_t> *_filter_node;
  // An USB demodulator, LSB will be handled by a BP below the center freq.
  sdr::USBDemod<int16_t> *_ssb_demod;
  // AM demodulator
  sdr::AMDemod<int16_t> *_am_demod;
  sdr::FMDemod<int16_t> *_fm_demod;

  sdr::Proxy *_audio_source;
};


class DemodulatorCtrlView : public QWidget
{
  Q_OBJECT

public:
  explicit DemodulatorCtrlView(Demodulator *demodulator, QWidget *parent = 0);

protected slots:
  void onModSelected(int idx);

  void onAGCToggled(bool enabled);
  void onAGCTauChanged(QString value);
  void onGainChanged(QString value);

  void onUpdateGain();

  void onCenterFreqChanged(QString value);
  void onLowerFreqChanged(QString value);
  void onUpperFreqChanged(QString value);

  void onUpdateFilter();


protected:
  Demodulator *_demodulator;

  QLineEdit *_gain;
  QCheckBox *_agc;
  QLineEdit *_agc_tau;
  QLineEdit *_centerFreq;
  QLineEdit *_lowerFreq;
  QLineEdit *_upperFreq;
};


class DemodulatorSpectrumView : public sdr::gui::SpectrumView
{
  Q_OBJECT

public:
  DemodulatorSpectrumView(Demodulator *demodulator);
  virtual ~DemodulatorSpectrumView();

protected:
  virtual void paintEvent(QPaintEvent *evt);

protected:
  Demodulator *_demodulator;
};


class DemodulatorWaterFallView : public sdr::gui::WaterFallView
{
  Q_OBJECT

public:
  DemodulatorWaterFallView(Demodulator *demodulator);
  virtual ~DemodulatorWaterFallView();

protected:
  virtual void paintEvent(QPaintEvent *evt);

protected:
  Demodulator *_demodulator;
};


#endif // __SDR_RX_DEMODULATOR_HH__
