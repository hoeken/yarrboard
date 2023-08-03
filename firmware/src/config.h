#ifndef YARR_CONFIG_H
#define YARR_CONFIG_H

// enable one of these
//#define CONFIG_8CH_MOSFET_REVA
//#define CONFIG_8CH_MOSFET_REVB

#if defined CONFIG_8CH_MOSFET_REVA
  #include "./configs/config.8ch-mosfet-reva.h"
#elif defined CONFIG_8CH_MOSFET_REVB
  #include "./configs/config.8ch-mosfet-revb.h"
#endif

#endif // YARR_CONFIG_H