#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

void ProtocolDestroyMessage(const char *p);
char *ProtoclCreateHeartBeat(int *size);
char *ProtoclCreatLogin(char *imei, int *size);
void ProtocolHandler(char *p);

#endif
