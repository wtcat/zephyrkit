#include <string.h>

#include <device.h>
#include <net/buf.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(stp_app, CONFIG_STP_APP_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"

#define OBSERVER_CLASS_DEFINE
#include "stp/app/music_service.h"

#define MUSIC_REQ_SYNC_VOLUME  0x06


int remote_music_player_request(uint32_t req)
{
    stp_packet_declare(1) p;
    struct stp_bufv v[1];
    opc_bufv_init(&v[0], &p.hdr, req, 0);
    return stp_sendmsgs(opc_chan_get(OPC_CLASS_CAMERA), v, 1);
}

int remote_music_volume_sync_request(int volume)
{
    Music__Player player = MUSIC__PLAYER__INIT;
    stp_packet_declare(32) m;
    struct stp_bufv v[1];
    size_t size;

    player.has_volume = true;
    player.volume = volume;
    size = music__player__pack(&player, m.hdr.data);
    __ASSERT(size < sizeof(m) - sizeof(*hdr), "");
    opc_bufv_init(&v[0], &m.hdr, MUSIC_REQ_SYNC_VOLUME, size);
    return stp_sendmsgs(opc_chan_get(OPC_CLASS_MUSIC), v, 1);
}

static void music_player_control(const void *buf, size_t size,
    struct net_buf *obuf, uint32_t req)
{
    Music__Player *player;
    stp_packet_declare(1) m;
    uint8_t result = 0;

    player = music__player__unpack(NULL, size, buf);
    if (player == NULL) {
        LOG_ERR("%s(): Music player receive data failed\n", __func__);
        result = 0xF0;
        goto _exit;
    }
    // if (player->song == NULL) {
    //     LOG_ERR("%s(): Music player logic error\n", __func__);
    //     result = 0xF1;
    //     goto _exit;
    // }

    /* Notify GUI appliction */
    music_notify(req, player);
    music__player__free_unpacked(player, NULL);

_exit:
    if (obuf) {
        m.hdr.minor = MUSIC_REQ_PREV;
        m.hdr.len = ltons(OPC_SLEN(1));
        m.hdr.data[0] = result;
        net_buf_add_mem(obuf, &m.hdr, OPC_SUBHDR_SIZE + 1);
    }
}

static void music_switch_to_preivous(const void *buf, size_t size,
    struct net_buf *obuf)
{
    music_player_control(buf, OPC_LEN(size), obuf, MUSIC_REQ_PREV);
}

static void music_switch_to_next(const void *buf, size_t size,
    struct net_buf *obuf)
{
    music_player_control(buf, OPC_LEN(size), obuf, MUSIC_REQ_NEXT);
}

static void music_player_pause(const void *buf, size_t size,
    struct net_buf *obuf)
{
    music_player_control(buf, OPC_LEN(size), obuf, MUSIC_REQ_PAUSE);
}

static void music_player_start(const void *buf, size_t size,
    struct net_buf *obuf)
{
    music_player_control(buf, OPC_LEN(size), obuf, MUSIC_REQ_PLAY);
}

static void music_player_sync_volume(const void *buf, size_t size,
    struct net_buf *obuf)
{
    music_player_control(buf, OPC_LEN(size), obuf, MUSIC_REQ_PLAY);
}

STP_SERVICE(music, OPC_CLASS_MUSIC, MUSIC_REQ_PREV, music_switch_to_preivous);
STP_SERVICE(music, OPC_CLASS_MUSIC, MUSIC_REQ_NEXT, music_switch_to_next);
STP_SERVICE(music, OPC_CLASS_MUSIC, MUSIC_REQ_PLAY, music_player_start);
STP_SERVICE(music, OPC_CLASS_MUSIC, MUSIC_REQ_PAUSE, music_player_pause);
STP_SERVICE(music, OPC_CLASS_MUSIC, MUSIC_REQ_SYNC_VOLUME, music_player_sync_volume);
