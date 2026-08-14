#ifndef HAVE_TS2PHC_NMEA_PPS_SOURCE_H
#define HAVE_TS2PHC_NMEA_PPS_SOURCE_H

#include "ts2phc.h"
#include "ts2phc_pps_source.h"

struct ts2phc_pps_source *ts2phc_nmea_pps_source_create(struct ts2phc_private *priv,
							const char *dev);
#endif
