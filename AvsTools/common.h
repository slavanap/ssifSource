#pragma once

#define TEST(x, y) (((x) & (y)) != 0)
#define TESTALL(x, y) (((x) & (y)) == (y))
#define ARRAY_SIZE(a) (sizeof(a) / (sizeof((a)[0])))
#define DIV_AND_ROUND(x,y) (((x)+(y)-1) / (y))

#define strcasecmp _stricmp
