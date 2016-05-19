# chart
add_library(cplot STATIC
    axisdefinition.cc
    areachart.cc
    barchart.cc
    linechart.cc
    pointchart.cc
    canvas.cc
    domain.cc
    domainprovider.cc
    drawable.cc
    griddefinition.cc
    legenddefinition.cc
    series.cc
    timedomain.cc)

add_executable(test-cplot chart_test.cc)
target_link_libraries(test-cplot cplot stx-base)
