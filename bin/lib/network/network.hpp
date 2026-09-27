#ifndef NETLIB
#define NETLIB

#ifdef __cplusplus
extern "C" {
#endif
void DownloadUrl(const wchar_t* host, const wchar_t* path);
void RServer(int port);
void ServerBody(const char ui[512]);
void ServerTitle(char temp_title[32]);
#ifdef __cplusplus
}
#endif
#endif
