#include <stdio.h>
#include <string.h>
#include "libnetwork.h"

int main(int argc, char **argv)
{
	uint32_t speed = 100;
	uint8_t duplex = 1;
	uint8_t autoneg = 0;
	uint32_t supported = 0;
	uint32_t advertising = 0;

#if 0

	int ret = get_ethtool_ecmd("enp0s31f6", &speed, &duplex, &autoneg, &supported, &advertising);
	if(ret < 0){
		printf("get_ethtool_ecmd failed");
		sleep(5);
	}
	printf("enp0s31f6 speed is %d, duplex is %d autoneg is %d supported is %d advertising is %d\n", speed, duplex, autoneg, supported, advertising);

	ret = set_ethtool_ecmd("enp0s31f6", speed, duplex, autoneg);
	if(ret < 0){
		printf("set_ethtool_ecmd failed\n");
	}

	while(true){
		ret = get_ethtool_ecmd("enp0s31f6", &speed, &duplex, &autoneg, &supported, &advertising);
		if(ret < 0){
			printf("get_ethtool_ecmd failed");
			sleep(5);
			continue;
		}
		printf("enp0s31f6 speed is %d, duplex is %d autoneg is %d supported is %d advertising is %d\n", speed, duplex, autoneg, supported, advertising);
		sleep(5);
	}
#endif

	short int if_flags;
	get_if_flags("wlp4s0", &if_flags);

	if_flags &= ~IFF_UP;
	set_if_flags("wlp4s0", if_flags);

	sleep(5);
	if_flags |= IFF_UP;
	set_if_flags("wlp4s0", if_flags);
	
	return 0;
}
