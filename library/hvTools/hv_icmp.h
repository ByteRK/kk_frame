#ifndef HV_ICMP_H_
#define HV_ICMP_H_

#if defined(__cplusplus)
extern "C" {
#endif

// @param cnt: ping count
// @return: ok count
// @note: printd $CC -DPRINT_DEBUG
int ping_host_ip(const char* domain, int count, int* max_ms);

char* host_to_ip(const char *domain, char *ip, int len);

#if defined(__cplusplus)
}
#endif

#endif // HV_ICMP_H_
