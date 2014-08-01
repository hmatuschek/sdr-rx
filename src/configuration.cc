#include "configuration.hh"

Configuration *Configuration::_instance = 0;


Configuration::Configuration() :
  QSettings("com.github.hmatuschek", "sdr-rx")
{
  // pass...
}

Configuration::~Configuration() {
  // pass...
}

Configuration &
Configuration::get() {
  if (0 == _instance) { _instance = new Configuration(); }
  return *_instance;
}
