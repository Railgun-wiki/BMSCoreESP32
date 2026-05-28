#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void taskLvgl(void* param);
void taskSensor(void* param);
void taskSensorInit(void);
void taskMqtt(void* param);

#ifdef __cplusplus
}
#endif
