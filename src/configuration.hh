#ifndef __SDR_RX_CONFIGURATION_HH__
#define __SDR_RX_CONFIGURATION_HH__

#include <QSettings>

/** A trivial class to provide a QSettings instance as a singleton. */
class Configuration : public QSettings
{
  Q_OBJECT

protected:
  /** Hidden constructor of the singleton instance. Use @c get to obtain that instance. */
  explicit Configuration();

public:
  /** Destructor. */
  virtual ~Configuration();

  /** Fatory method for the singleton instance. */
  static Configuration &get();

private:
  static Configuration *_instance;
};

#endif // __SDR_RX_CONFIGURATION_HH__
