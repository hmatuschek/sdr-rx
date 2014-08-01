#ifndef __SDR_RX_DEMODULATOR_HH__
#define __SDR_RX_DEMODULATOR_HH__

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QComboBox>
#include <QGroupBox>

#include "gui/spectrum.hh"
#include "gui/spectrumview.hh"
#include "gui/waterfallview.hh"
#include "baseband.hh"
#include "psk31.hh"
#include "demod.hh"
#include "firfilter.hh"


// Forward declaration
class Receiver;
class DemodInterface;

class DemodulatorCtrl : public sdr::gui::Spectrum
{
  Q_OBJECT

public:
  typedef enum {
    DEMOD_AM,
    DEMOD_WFM,
    DEMOD_NFM,
    DEMOD_USB,
    DEMOD_LSB,
    DEMOD_CW,
    DEMOD_BPSK31
  } Demod;

public:
  explicit DemodulatorCtrl(Receiver *receiver = 0);
  virtual ~DemodulatorCtrl();

  /** Simply returns the receiver instance passed to the constructor. */
  Receiver *receiver() const;

  bool isAGCEnabled() const;
  double gain() const;
  double agcTime() const;

  inline double centerFreq() const { return _filter_node->centerFrequency(); }
  inline double filterFrequency() const { return _filter_node->filterFrequency()-_filter_node->centerFrequency(); }
  inline double filterLower() const { return _filter_node->filterFrequency()-_filter_node->filterWidth()/2; }
  inline double filterUpper() const { return _filter_node->filterFrequency()+_filter_node->filterWidth()/2; }
  inline double filterWidth() const { return _filter_node->filterWidth(); }

  inline DemodInterface *demod() const { return _demodObj; }

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
  void setFilterFrequency(double f);
  void setFilterWidth(double w);

  void setDemod(Demod demod);

protected:
  Receiver *_receiver;

  /** The currently selected demodulator. */
  DemodInterface *_demodObj;

  // A AGC
  sdr::AGC< std::complex<int16_t> > *_agc;
  // The filter node
  sdr::IQBaseBand<int16_t> *_filter_node;

  sdr::Proxy *_audio_source;
};


class DemodulatorCtrlView : public QWidget
{
  Q_OBJECT

public:
  explicit DemodulatorCtrlView(DemodulatorCtrl *demodulator, QWidget *parent = 0);

protected slots:
  void onDemodSelected(int idx);

  void onAGCToggled(bool enabled);
  void onAGCTauChanged(QString value);
  void onGainChanged(QString value);

  void onUpdateGain();

  void onCenterFreqChanged(QString value);
  void onFilterChanged();

protected:
  DemodulatorCtrl *_demodulator;
  QComboBox *_demodList;
  QLineEdit *_gain;
  QCheckBox *_agc;
  QLineEdit *_agc_tau;
  QLineEdit *_centerFreq;

  QVBoxLayout *_layout;
};


class DemodulatorSpectrumView : public sdr::gui::SpectrumView
{
  Q_OBJECT

public:
  DemodulatorSpectrumView(DemodulatorCtrl *demodulator);
  virtual ~DemodulatorSpectrumView();

protected:
  virtual void paintEvent(QPaintEvent *evt);

protected:
  DemodulatorCtrl *_demodulator;
};


class DemodulatorWaterFallView : public sdr::gui::WaterFallView
{
  Q_OBJECT

public:
  DemodulatorWaterFallView(DemodulatorCtrl *demodulator);
  virtual ~DemodulatorWaterFallView();

protected:
  virtual void paintEvent(QPaintEvent *evt);

protected:
  DemodulatorCtrl *_demodulator;
};


class DemodInterface
{
public:
  DemodInterface();
  virtual ~DemodInterface();

  /** Should return the sink of the demodulator. */
  virtual sdr::SinkBase *sink() = 0;
  /** Should return the audio source of the demodulator. */
  virtual sdr::Source *audioSource() = 0;
  /** Should create a control view for the demodulator. */
  virtual QWidget *createView() = 0;
};


class AMDemodulatorView;
class AMDemodulator: public QObject, public DemodInterface
{
  Q_OBJECT

public:
  AMDemodulator(DemodulatorCtrl *ctrl);
  virtual ~AMDemodulator();

  double filterWidth() const;
  void setFilterWidth(double width);

  virtual sdr::SinkBase *sink();
  virtual sdr::Source *audioSource();

  virtual QWidget *createView();

protected slots:
  void _onViewDeleted();

protected:
  DemodulatorCtrl *_ctrl;
  sdr::AMDemod<int16_t> _demod;
  AMDemodulatorView *_view;
};


class AMDemodulatorView: public QGroupBox
{
  Q_OBJECT

public:
  AMDemodulatorView(AMDemodulator *demod, QWidget *parent=0);
  virtual ~AMDemodulatorView();

protected slots:
  void _onFilterWidthChanged(QString value);

protected:
  AMDemodulator *_demod;
  QLineEdit *_filterWidth;
};


class FMDemodulatorView;
class FMDemodulator: public QObject, public DemodInterface
{
  Q_OBJECT

protected:
  FMDemodulator(DemodulatorCtrl *demod);

public:
  virtual ~FMDemodulator();

  double filterWidth() const;
  void setFilterWidth(double width);

  virtual sdr::SinkBase *sink();
  virtual sdr::Source *audioSource();

  virtual QWidget *createView();

protected slots:
  void _onViewDeleted();

protected:
  DemodulatorCtrl *_ctrl;
  sdr::FMDemod<int16_t> _demod;
  FMDemodulatorView *_view;
};

class WFMDemodulator: public FMDemodulator
{
  Q_OBJECT

public:
  WFMDemodulator(DemodulatorCtrl *demod);
  virtual ~WFMDemodulator();
};

class NFMDemodulator: public FMDemodulator
{
  Q_OBJECT

public:
  NFMDemodulator(DemodulatorCtrl *demod);
  virtual ~NFMDemodulator();
};


class FMDemodulatorView: public QGroupBox
{
  Q_OBJECT

public:
  FMDemodulatorView(FMDemodulator *demod, QWidget *parent=0);
  virtual ~FMDemodulatorView();

protected slots:
  void _onFilterWidthChanged(QString value);

protected:
  FMDemodulator *_demod;
  QLineEdit *_filterWidth;
};


class SSBDemodulatorView;
class SSBDemodulator: public QObject, public DemodInterface
{
  Q_OBJECT

protected:
  SSBDemodulator(DemodulatorCtrl *ctrl, QObject *parent=0);

public:
  virtual ~SSBDemodulator();

  double filterWidth() const;
  void setFilterWidth(double width);

  virtual sdr::SinkBase *sink();
  virtual sdr::Source *audioSource();

  virtual QWidget *createView();

protected slots:
  void _onViewDeleted();

protected:
  DemodulatorCtrl *_ctrl;
  sdr::USBDemod<int16_t> _demod;
  SSBDemodulatorView *_view;
};


class USBDemodulator: public SSBDemodulator
{
  Q_OBJECT

public:
  USBDemodulator(DemodulatorCtrl *ctrl, QObject *parent=0);
  virtual ~USBDemodulator();
};


class LSBDemodulator: public SSBDemodulator
{
  Q_OBJECT

public:
  LSBDemodulator(DemodulatorCtrl *ctrl, QObject *parent=0);
  virtual ~LSBDemodulator();
};


class CWDemodulator: public SSBDemodulator
{
  Q_OBJECT

public:
  CWDemodulator(DemodulatorCtrl *ctrl, QObject *parent=0);
  virtual ~CWDemodulator();
};


class SSBDemodulatorView: public QGroupBox
{
  Q_OBJECT

public:
  SSBDemodulatorView(SSBDemodulator *demod, QWidget *parent=0);
  virtual ~SSBDemodulatorView();

protected slots:
  void _onFilterWidthChanged(QString value);

protected:
  SSBDemodulator *_demod;
  QLineEdit *_filterWidth;
};


class BPSK31DemodulatorView;
class BPSK31Demodulator: public QObject, public DemodInterface, public sdr::Sink<uint8_t>
{
  Q_OBJECT

public:
  BPSK31Demodulator(DemodulatorCtrl *ctrl, QObject *parent=0);
  virtual ~BPSK31Demodulator();

  double filterWidth() const;
  void setFilterWidth(double width);

  virtual sdr::SinkBase *sink();
  virtual sdr::Source *audioSource();

  virtual QWidget *createView();

  virtual void config(const sdr::Config &src_cfg);
  virtual void process(const sdr::Buffer<uint8_t> &buffer, bool allow_overwrite);

  const QString & text() const;
  void clearText();

signals:
  void textReceived();

protected slots:
  void _onViewDeleted();

protected:
  DemodulatorCtrl *_ctrl;
  sdr::Proxy _input_proxy;
  sdr::FreqShift<int16_t> _freq_shift;
  sdr::USBDemod<int16_t> _audio_demod;
  sdr::FIRLowPass< std::complex<int16_t> > _bpsk_filter;
  sdr::BPSK31<int16_t>   _bpsk;
  sdr::Varicode          _decode;
  QString _text_buffer;
  BPSK31DemodulatorView *_view;
};


class BPSK31DemodulatorView: public QGroupBox
{
Q_OBJECT

public:
  BPSK31DemodulatorView(BPSK31Demodulator *demod, QWidget *parent=0);
  virtual ~BPSK31DemodulatorView();

protected slots:
  void _onTextReceived();
  void _onFilterWidthChanged(QString width);

protected:
  BPSK31Demodulator *_demod;
  QLineEdit *_filterWidth;
  QPlainTextEdit *_text;
};

#endif // __SDR_RX_DEMODULATOR_HH__
