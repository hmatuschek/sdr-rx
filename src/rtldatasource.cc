#include "rtldatasource.hh"
#include "receiver.hh"

using namespace sdr;


RTLDataSource::RTLDataSource(double frequency, double sample_rate, QObject *parent)
  : DataSource(parent)
{
  _device = new RTLSource(frequency, sample_rate);
  _to_int16 = new Cast< std::complex<int8_t>, std::complex<int16_t> >(128);

  _device->connect(_to_int16, true);
}

RTLDataSource::~RTLDataSource() {
  delete _device;
  delete _to_int16;
}

QWidget *
RTLDataSource::createCtrlView() {
  return new QWidget();
}

Source *
RTLDataSource::source() {
  return _to_int16;
}

void
RTLDataSource::triggerNext() {
  // RTL data source runs in parallel
}


