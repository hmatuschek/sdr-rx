#ifndef __SDR_DATA_SOURCE_HH__
#define __SDR_DATA_SOURCE_HH__

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>

#include "node.hh"

// Forward Declaration
class Receiver;



class DataSource : public QObject
{
  Q_OBJECT

public:
  explicit DataSource(QObject *parent = 0);
  virtual ~DataSource();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source() = 0;
  virtual void triggerNext() = 0;
};



class DataSourceCtrl: public QObject, public sdr::Proxy
{
  Q_OBJECT

public:
  typedef enum {
    SOURCE_PORT, SOURCE_PORT_IQ, SOURCE_FILE, SOURCE_RTL
  } Src;

public:
  DataSourceCtrl(Receiver *receiver);
  virtual ~DataSourceCtrl();

  inline Src source() const { return _source; }
  void setSource(Src source);

  QWidget *createCtrlView();

protected:
  void _onQueueIdle();

protected:
  Receiver *_receiver;
  /** Currently selected source. */
  Src _source;
  /** Currently selected source object. */
  DataSource *_src_obj;
};



class DataSourceCtrlView: public QWidget
{
  Q_OBJECT

public:
  DataSourceCtrlView(DataSourceCtrl *src_ctrl, QWidget *parent=0);
  virtual ~DataSourceCtrlView();

protected slots:
  void _onSourceSelected(int index);

protected:
  DataSourceCtrl *_src_ctrl;
  QVBoxLayout *_layout;
  QWidget *_currentSrcCtrl;
};

#endif // __SDR_DATA_SOURCE_HH__
