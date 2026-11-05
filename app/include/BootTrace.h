#ifndef REMD_BOOT_TRACE_H
#define REMD_BOOT_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

void bootTrace(const char *line);
void bootTraceStep(const char *step);

#ifdef __cplusplus
}
#endif

#endif
