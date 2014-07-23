#ifndef __SDR_RX_RTLDATASOURCE_HH__
#define __SDR_RX_RTLDATASOURCE_HH__

#include "source.hh"
#include "rtlsource.hh"
#include "utils.hh"

class RTLDataSource : public DataSource
{
  Q_OBJECT

public:
  RTLDataSource(double frequency, double sample_rate, QObject *parent=0);
  virtual ~RTLDataSource();

  virtual QWidget *createCtrlView();
  virtual sdr::Source *source();
  virtual void triggerNext();

protected:
  sdr::RTLSource *_device;
  sdr::Cast< std::complex<int8_t>, std::complex<int16_t> > *_to_int16;
};

#endif // __SDR_RX_RTLDATASOURCE_HH__
