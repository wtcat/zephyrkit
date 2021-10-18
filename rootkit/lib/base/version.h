#ifndef FIRMWARE_VERSION_H_
#define FIRMWARE_VERSION_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

struct internal_version {
    union {
        uint16_t v16;
        uint8_t v8[2];
    };
    const char *vstr;
};

#define DEFINE_VERSION(_name, _major, _minor) \
    const struct internal_version _name##__##version = { \
	    .v8 = {(uint8_t)_minor, (uint8_t)_major}, \
		.vstr = "V" #_major "." #_minor \
	}
	

extern const struct internal_version firmware__version;
extern const struct internal_version hardware__version;
 
#define FIRMWARE_VER &firmware__version
#define HARDWARE_VER &hardware__version

static inline uint32_t get_version_number(
    const struct internal_version *self)
{
	return self->v16;
}

static inline const char *get_version_string(
    const struct internal_version *self)
{
	return self->vstr;
}

#ifdef __cplusplus
}
#endif
#endif /* FIRMWARE_VERSION_H_ */
