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
#define GOOGLE_CLOUD_DEVICE "stm32f769-demo"
#endif
#ifndef GOOGLE_CLOUD_TOKEN
#define GOOGLE_CLOUD_TOKEN "91d483513b0a48849c922f5c67e35b4aefa7d5b743c143e2a89cfc3d8cd11c03"
#endif
#ifndef GOOGLE_CLOUD_PATH
#define GOOGLE_CLOUD_PATH "/macros/s/AKfycbwUpJU2Md_8Vqw2h_UDqqmo0VnpHt47-PdBddD9ZGdeS83A6bBtGGVBOCPGIvJ68DulDQ/exec"
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
