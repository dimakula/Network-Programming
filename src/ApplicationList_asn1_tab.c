#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <libtasn1.h>

const asn1_static_node ApplicationList_asn1_tab[] = {
  { "ApplicationList", 536875024, NULL },
  { NULL, 1073741836, NULL },
  { "Gossip", 1610620933, NULL },
  { NULL, 1073744904, "1"},
  { "sha256hash", 1073741831, NULL },
  { "timestamp", 1073741861, NULL },
  { "message", 34, NULL },
  { "Peer", 1610620933, NULL },
  { NULL, 1073746952, "2"},
  { "name", 1073741858, NULL },
  { "port", 1073741827, NULL },
  { "ip", 31, NULL },
  { "PeersQuery", 1610620948, NULL },
  { NULL, 5128, "3"},
  { "PeersAnswer", 1610620939, NULL },
  { NULL, 1073743880, "1"},
  { NULL, 2, "Peer"},
  { "UTF8String", 1610620935, NULL },
  { NULL, 4360, "12"},
  { "PrintableString", 536879111, NULL },
  { NULL, 4360, "19"},
  { NULL, 0, NULL }
};
