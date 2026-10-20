#ifndef GOOGLE_CLOUD_CONFIG_H
#define GOOGLE_CLOUD_CONFIG_H
#if defined(__has_include)
#if __has_include("google_cloud_config.local.h")
#include "google_cloud_config.local.h"
#endif
#endif
#ifndef GOOGLE_CLOUD_ENABLED
#define GOOGLE_CLOUD_ENABLED 0
#endif
#ifndef GOOGLE_CLOUD_DEVICE
#define GOOGLE_CLOUD_DEVICE "stm32h753"
#endif
#ifndef GOOGLE_CLOUD_TOKEN
#define GOOGLE_CLOUD_TOKEN ""
#endif
#ifndef GOOGLE_CLOUD_PATH
#define GOOGLE_CLOUD_PATH ""
#endif
#ifndef GOOGLE_CLOUD_PERIOD_MS
#define GOOGLE_CLOUD_PERIOD_MS 60000U
#endif
#ifndef GOOGLE_CLOUD_TIMEOUT_MS
#define GOOGLE_CLOUD_TIMEOUT_MS 25000U
#endif
#ifndef GOOGLE_CLOUD_CA_INDEX
#define GOOGLE_CLOUD_CA_INDEX 1U
#endif
#ifndef GOOGLE_CLOUD_INSTALL_CA
#define GOOGLE_CLOUD_INSTALL_CA 1
#endif
#ifndef GOOGLE_CLOUD_TEMPERATURE_PORT
#define GOOGLE_CLOUD_TEMPERATURE_PORT 1U
#endif
#endif
