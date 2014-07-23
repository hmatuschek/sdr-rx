set(sdr_rx_SOURCES main.cc
    receiver.cc mainwindow.cc source.cc portaudiosource.cc filesource.cc
    demodulator.cc audiopostproc.cc rtldatasource.cc)
set(sdr_rx_MOC_HEADERS
    receiver.hh mainwindow.hh source.hh portaudiosource.hh filesource.hh
    demodulator.hh audiopostproc.hh rtldatasource.hh)
qt5_wrap_cpp(sdr_rx_MOC_SOURCES ${sdr_rx_MOC_HEADERS})

set(sdr_rx_HEADERS ${sdr_rx_MOC_HEADERS})
add_executable(sdr_rx ${sdr_rx_SOURCES} ${sdr_rx_MOC_SOURCES})
target_link_libraries(sdr_rx ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${LIBS} libsdr libsdr-gui)
