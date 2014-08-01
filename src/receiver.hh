#ifndef __SDR_RX_RECEIVER_HH__
#define __SDR_RX_RECEIVER_HH__

#include <QObject>

#include "source.hh"
#include "demodulator.hh"
#include "audiopostproc.hh"


class Receiver: public QObject
{
  Q_OBJECT

public:
  explicit Receiver(QObject *parent = 0);
  virtual ~Receiver();

  bool isRunning() const;


  QWidget *createSourceCtrlView();
  QWidget *createDemodCtrlView();
  QWidget *createDemodView();
  QWidget *createAudioCtrlView();

  /** Returns the tuner frequency of the source or 0 if the source does not have a tuner. */
  double tunerFrequency() const;

signals:
  void started();
  void stopped();

public slots:
  void start();
  void stop();

protected:
  void _onQueueStarted();
  void _onQueueStopped();

protected:
  sdr::Queue &_queue;

  DataSourceCtrl  *_src;
  DemodulatorCtrl *_demod;
  AudioPostProc *_audio;
};


#endif // RECEIVER_HH
