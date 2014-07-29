#ifndef __SDR_RX_RECEIVER_HH__
#define __SDR_RX_RECEIVER_HH__

#include <QObject>
#include "sdr.hh"

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
