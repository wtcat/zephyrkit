#include "stp/opc_proto.h"

#define OPC_CLASS(_class, _flags) \
    [_class] = {{STP_OPC_PROTO}, _class, _flags}

const struct opc_chan opc_chan_aggregation[] = {
    OPC_CLASS(OPC_CLASS_OTA,       0),
    OPC_CLASS(OPC_CLASS_INFO,      0),
    OPC_CLASS(OPC_CLASS_SETTING,   0),
    OPC_CLASS(OPC_CLASS_GPS,       0),
    OPC_CLASS(OPC_CLASS_REMIND,    0),
    OPC_CLASS(OPC_CLASS_CALL,      0),

    OPC_CLASS(OPC_CLASS_CAMERA,    0),
    OPC_CLASS(OPC_CLASS_MUSIC,    0),
};
