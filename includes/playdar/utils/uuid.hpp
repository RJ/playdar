#ifndef _PLAYDAR_UTILS_UUID_H_
#define _PLAYDAR_UTILS_UUID_H_

namespace playdar {
namespace utils { 

inline std::string gen_uuid()
{
    // TODO use V3 instead, so uuids are anonymous?
    char *uuid_str;
    uuid_rc_t rc;
    uuid_t *uuid;
    void *vp;
    size_t len;

    if((rc = uuid_create(&uuid)) != UUID_RC_OK)
    {
        uuid_destroy(uuid);
        return "";
    }
    if((rc = uuid_make(uuid, UUID_MAKE_V1)) != UUID_RC_OK)
    {
        uuid_destroy(uuid);
        return "";
    }
    len = UUID_LEN_STR+1;
    if ((vp = uuid_str = (char *)malloc(len)) == NULL) {
        uuid_destroy(uuid);
        return "";
    }
    if ((rc = uuid_export(uuid, UUID_FMT_STR, &vp, &len)) != UUID_RC_OK) {
        uuid_destroy(uuid);
        return "";
    }
    uuid_destroy(uuid);
    string retval(uuid_str);
    delete(uuid_str);
    return retval;
}

}} // ns

#endif
