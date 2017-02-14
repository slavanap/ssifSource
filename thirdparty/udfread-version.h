#pragma once

#define UDFREAD_VERSION_CODE(major, minor, micro) \
    (((major) * 10000) +                         \
     ((minor) *   100) +                         \
     ((micro) *     1))
#define UDFREAD_VERSION_MAJOR 1
#define UDFREAD_VERSION_MINOR 0
#define UDFREAD_VERSION_MICRO 0
#define UDFREAD_VERSION_STRING "1.0.0"
#define UDFREAD_VERSION \
    UDFREAD_VERSION_CODE(UDFREAD_VERSION_MAJOR, UDFREAD_VERSION_MINOR, UDFREAD_VERSION_MICRO)
void udfread_get_version(int *major, int *minor, int *micro);
