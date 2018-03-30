#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include "libnetwork.h"

//#define ENABLE_NET_WORK_DBG

#ifdef ENABLE_NET_WORK_DBG
#define NET_ERR(...)	printf(__VA_ARGS__)
#define NET_WAR(...)	printf(__VA_ARGS__)
#define NET_DBG(...) 	printf(__VA_ARGS__)
#else
#define NET_ERR(...)	printf(__VA_ARGS__)
#define NET_WAR(...)	printf(__VA_ARGS__)
#define NET_DBG(...)
#endif

#if 0
/**
 *data dump
 *@param buf_name  -in- the dumped buffer name
 *@param buffer -in- the buffer need be dumped
 *@input_len -in- the length need be dumped
*/
static void data_dump(char *buf_name, char *buffer, int input_len)
{
	int i = 0;

	if(!buf_name || !buffer){
		NET_ERR("the buf_name or buffer is NULL\n");
		return;
	}
	NET_DBG("The %s: dump as below:\n", buf_name);
	for(i=0; i<input_len; i++){
		NET_DBG("%02x", buffer[i]);
		if((i+1)%16 == 0){
			NET_DBG("\n");
		}
	}
}
#endif
/**
 * chage the char to sockaddr
 *@param buffer -in- the buffer wanted to change
 *@param sockaddr_buf -out- the sockaddr store the address
*/
static int change_char_to_sockaddr(char *buffer, struct sockaddr *sockaddr_buf)
{
    char *sp = buffer, *bp;
    unsigned int i;
    unsigned val;
    struct sockaddr_in *sin;

    sin = (struct sockaddr_in *) sockaddr_buf;
    sin->sin_family = AF_INET;
    sin->sin_port = 0;

    val = 0;
    bp = (char *) &val;
    for (i = 0; i < sizeof(sin->sin_addr.s_addr); i++) {
		*sp = toupper(*sp);

		if ((*sp >= 'A') && (*sp <= 'F'))
	    	bp[i] |= (int) (*sp - 'A') + 10;
		else if ((*sp >= '0') && (*sp <= '9'))
	    	bp[i] |= (int) (*sp - '0');
		else
	    	return (-1);

		bp[i] <<= 4;
		sp++;
		*sp = toupper(*sp);

		if ((*sp >= 'A') && (*sp <= 'F'))
	    	bp[i] |= (int) (*sp - 'A') + 10;
		else if ((*sp >= '0') && (*sp <= '9'))
	    	bp[i] |= (int) (*sp - '0');
		else
	    	return (-1);

		sp++;
    }

    sin->sin_addr.s_addr = htonl(val);

    return (sp - buffer);
}

/**
 ************************
 * The APIs of ifconfig  *
 ************************
*/
int get_if_dev_name(ifr_name_t *ifr_names, unsigned int input_len, unsigned int *dev_nums)
{
	FILE *fp = NULL;
	char buffer[4096] = {0};
	char *dev_name = NULL;
	char *tmp = NULL;
	int ret = SUCCESS;
	int i = 0;

	if(!ifr_names || input_len < (IF_DEV_NUM_MAX * sizeof(ifr_name_t)) || !dev_nums){
		NET_ERR("%s: Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	fp = fopen(NET_DEV_NAME, "r");
	if(!fp){
		NET_ERR("%s: open the net device failed. errno: %s\n", __func__, strerror(errno));
		ret = ERR_NOTEXIST;
		goto ERROR0;
	}

	for (i = 0; fgets(buffer, 4096, fp); i++){
		NET_DBG("buffer is: %s\n", buffer);
		/*drop the first and second line*/
		if(i<2) continue;

		tmp = buffer;
		/*drop the space*/
		while(isspace(*tmp)) tmp++;
		/*cut out the device name from the buffer*/
		dev_name = strsep(&tmp, ":");
		NET_DBG("dev_name is:%s\n", dev_name);

		strncpy(ifr_names[i-2].dev_name, dev_name, strlen(dev_name));
		ifr_names[i-2].used = true;
	}
	fclose(fp);

	*dev_nums = i - 2;

ERROR0:
	return ret;
}

int get_ifconfig(struct ifreq *ifrs, unsigned int input_len, unsigned int *ifreq_nums)
{
	int sockfd;
	struct ifconf ifc;
	int count = 0;
	int ret = SUCCESS;

	if(!ifrs || input_len < (IF_DEV_NUM_MAX * sizeof(struct ifreq)) || !ifreq_nums){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	ifc.ifc_len = input_len;
	ifc.ifc_buf = (void *)ifrs;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0){
		NET_DBG("ioctl SIOCGIFCONF error\n");
		ret = ERR_IOCTL;
		goto ERROR1;
	}
	count = ifc.ifc_len / sizeof(struct ifreq);
	*ifreq_nums = count;

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int get_if_ip_addr(char *dev_name, char *ip_address)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !ip_address){
		NET_ERR("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}
	if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

	memset(ip_address, 0, INET_ADDRSTRLEN);
	if(!inet_ntop(AF_INET, &(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr), ip_address, INET_ADDRSTRLEN)){
		NET_ERR("%s: change sockaddr_in to ip_address failed\n", __func__);
		ret = ERR_UNKNOWN;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}


int set_if_ip_addr(char *dev_name, char *ip_address)
{
	int sockfd;
	int ret = SUCCESS;
	struct ifreq ifr;
	struct sockaddr_in addr;

	if(!dev_name || !ip_address){
		NET_ERR("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, ip_address, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input ip_address is not valied\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the ip_address to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(ifr.ifr_addr), &addr, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCSIFADDR, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int get_if_brd_addr(char *dev_name, char *brd_address)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !brd_address){
		NET_ERR("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCGIFBRDADDR, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

	memset(brd_address, 0, INET_ADDRSTRLEN);
	if(!inet_ntop(AF_INET, &(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr), brd_address, INET_ADDRSTRLEN)){
		NET_ERR("%s: change sockaddr_in to brd_address failed\n", __func__);
		ret = ERR_UNKNOWN;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int set_if_brd_addr(char *dev_name, char *brd_address)
{
	int sockfd;
	int ret = SUCCESS;
	struct ifreq ifr;
	struct sockaddr_in addr;

	if(!dev_name || !brd_address){
		NET_ERR("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, brd_address, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input brd_address is not valied\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the brd_address to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(ifr.ifr_addr), &addr, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCSIFBRDADDR, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int get_if_netmask_addr(char *dev_name, char *netmask_address)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !netmask_address){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

	memset(netmask_address, 0, INET_ADDRSTRLEN);
	if(!inet_ntop(AF_INET, &(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr), netmask_address, INET_ADDRSTRLEN)){
		NET_ERR("%s: change sockaddr_in to netmask_address failed\n", __func__);
		ret = ERR_UNKNOWN;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int set_if_netmask_addr(char *dev_name, char *netmask_address)
{
	int sockfd;
	int ret = SUCCESS;
	struct ifreq ifr;
	struct sockaddr_in addr;

	if(!dev_name || !netmask_address){
		NET_ERR("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, netmask_address, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input netmask_address is not valied\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the netmask_address to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(ifr.ifr_addr), &addr, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int get_if_mac_addr(char *dev_name, unsigned char *mac_address, unsigned int input_len)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !mac_address || input_len < ETH_ALEN){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}
	memcpy(mac_address, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int set_if_mac_addr(char *dev_name, unsigned char *mac_address)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !mac_address){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}
	/*get the device flags first*/
	if(ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

	/*if the device is up, down it first*/
	if(ifr.ifr_flags & IFF_UP){
		ifr.ifr_flags &= ~IFF_UP;
		if(ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0){
			NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
			ret = ERR_IOCTL;
			goto ERROR1;
		}
	}

	ifr.ifr_addr.sa_family = ARPHRD_ETHER;

	memcpy(ifr.ifr_hwaddr.sa_data, mac_address, ETH_ALEN);
	if(ioctl(sockfd, SIOCSIFHWADDR, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int get_if_flags(char *dev_name, short int *if_flags)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !if_flags){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}
	*if_flags = ifr.ifr_flags;

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int set_if_flags(char *dev_name, short int if_flags)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	ifr.ifr_flags = if_flags;

	if(ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int get_if_mtu(char *dev_name, int *if_mtu)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !if_mtu){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCGIFMTU, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}
	*if_mtu = ifr.ifr_mtu;

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int set_if_mtu(char *dev_name, int if_mtu)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	NET_DBG("the device name is %s\n", ifr.ifr_name);

	if(if_mtu != -1)
		ifr.ifr_mtu = if_mtu;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCSIFMTU, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int change_if_name(char *old_dev_name, char *new_dev_name)
{
	int sockfd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!old_dev_name || !old_dev_name){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, old_dev_name, IFNAMSIZ-1);
	NET_DBG("the old_dev_name is %s\n", ifr.ifr_name);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}
	/*get the device flags first*/
	if(ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

	/*if the device is up, down it first*/
	if(ifr.ifr_flags & IFF_UP){
		ifr.ifr_flags &= ~IFF_UP;
		if(ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0){
			NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
			ret = ERR_IOCTL;
			goto ERROR1;
		}
	}

	strncpy(ifr.ifr_newname, new_dev_name, IFNAMSIZ-1);
	NET_DBG("the new_dev_name is %s\n", ifr.ifr_name);

	if(ioctl(sockfd, SIOCSIFNAME, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

/**
 *********************
 * The APIs of route  *
 *********************
*/

int get_route_table(libnet_rtentry_t *route_tables, unsigned int input_len, unsigned int *rt_table_nums)
{
	FILE *fp;
	char buffer[1024] = {0};
	int num = 0;
	char dev_name[16] = {0};
	char dst[128] = {0};
	struct sockaddr dst_addr;
	char gateway[128] = {0};
	struct sockaddr gateway_addr;
	char netmask[128] = {0};
	struct sockaddr netmask_addr;
	int iflags, metric, refcnt, use, mss, window, irtt;
	int ret = SUCCESS;
	int i = 0;

	if(!route_tables || input_len < (ROUTE_TABLE_NUM_MAX * sizeof(libnet_rtentry_t)) || !rt_table_nums){
		NET_DBG("%s: Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	fp = fopen(ROUTE_TABLE_DEV_NAME, "r");
	if(!fp){
		NET_DBG("%s: open the net device failed\n", __func__);
		ret = ERR_NOTEXIST;
		goto ERROR0;
	}

	for (i=0; fgets(buffer, 1024, fp); ++i){
		NET_DBG("the buffer is %s\n", buffer);
		/*drop the first line*/
		if(i < 1) continue;

		num = sscanf(buffer, "%16s\t%128s\t%128s\t%X\t%d\t%d\t%d\t%128s\t%d\t%d\t%d\t",
			    	dev_name, dst, gateway, &iflags, &refcnt, &use, &metric, netmask,
		    		&mss, &window, &irtt);
		if(num < 10 || !(iflags & RTF_UP)){
			continue;
		}

		NET_DBG("dev_name is:%s\n", dev_name);
		strncpy(route_tables[i-1].dev_name, dev_name, strlen(dev_name));
		route_tables[i-1].used = true;
		route_tables[i-1].route.rt_dev = route_tables[i-1].dev_name;

		change_char_to_sockaddr(dst, &dst_addr);
		memcpy(&(route_tables[i-1].route.rt_dst), &dst_addr, sizeof(struct sockaddr));

		change_char_to_sockaddr(gateway, &gateway_addr);
		memcpy(&(route_tables[i-1].route.rt_gateway), &gateway_addr, sizeof(struct sockaddr));

		change_char_to_sockaddr(netmask, &netmask_addr);
		memcpy(&(route_tables[i-1].route.rt_genmask), &netmask_addr, sizeof(struct sockaddr));

		route_tables[i-1].route.rt_flags = iflags;
		NET_DBG("%x\n", route_tables[i-1].route.rt_flags);
	}
	fclose(fp);

	*rt_table_nums = i -1;

ERROR0:
	return ret;
}

int add_route_rule(char *dst_type, char *dst_addr, char *netmask_addr, char *gw_addr, char *dev_name)
{
	int sockfd;
	int ret = SUCCESS;
	struct rtentry route;
	struct sockaddr_in addr;

	if(!dst_addr || !(dev_name || gw_addr)){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&route, 0, sizeof(struct rtentry));
	/*the route rule is net*/
	if(!strncmp("net", dst_type, 3)){
		route.rt_flags = RTF_UP;
		if(!netmask_addr){
			NET_DBG("%s:Invalided input parameter\n", __func__);
			ret = ERR_WRONGVALS;
			goto ERROR0;
		}
	}
	/*the route rule is host*/
	if(!strncmp("host", dst_type, 4)){
		route.rt_flags = RTF_UP | RTF_HOST;
	}

	/*dst address setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, dst_addr, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input dst_addr is Invalided\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the dst_addr to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(route.rt_dst), &addr, sizeof(struct sockaddr_in));

	/*net mask setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, netmask_addr, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input netmask_addr is Invalided\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the netmask_addr to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(route.rt_genmask), &addr, sizeof(struct sockaddr_in));

	/*device setting*/
	if(dev_name)
		route.rt_dev = dev_name;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	if(gw_addr && strncmp(gw_addr, "0.0.0.0", strlen("0.0.0.0"))){
		route.rt_flags = RTF_UP | RTF_GATEWAY;
		/*gw address setting*/
		ret = inet_pton(AF_INET, gw_addr, &(addr.sin_addr));
		if(0 == ret){
			NET_ERR("%s: the input gw_addr is Invalided\n", __func__);
			ret = ERR_WRONGVALS;
			goto ERROR0;
		}else if(-1 == ret){
			NET_ERR("%s: change the gw_addr to sockaddr_in failed\n", __func__);
			ret = ERR_WRONGVALS;
			goto ERROR0;
		}
	}else{
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	memcpy(&(route.rt_gateway), &addr, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCADDRT, &route) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int del_route_rule(char *dst_type, char *dst_addr, char *netmask_addr, char *gw_addr, char *dev_name)
{
	int sockfd;
	int ret = SUCCESS;
	struct rtentry route;
	struct sockaddr_in addr;

	if(!dst_addr){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&route, 0, sizeof(struct rtentry));
	/*the route rule is net*/
	if(!strncmp("net", dst_type, 3))
		route.rt_flags = RTF_UP;
	/*the route rule is host*/
	if(!strncmp("host", dst_type, 4))
		route.rt_flags = RTF_UP | RTF_HOST;

	/*dst address setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, dst_addr, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input dst_addr is Invalided\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the dst_addr to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(route.rt_dst), &addr, sizeof(struct sockaddr_in));

	/*net mask setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, netmask_addr, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input netmask_addr is Invalided\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the netmask_addr to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(route.rt_genmask), &addr, sizeof(struct sockaddr_in));

	/*device setting*/
	if(dev_name)
		route.rt_dev = dev_name;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	if(gw_addr && strncmp(gw_addr, "0.0.0.0", strlen("0.0.0.0"))){
		route.rt_flags = RTF_UP | RTF_GATEWAY;
		/*gw address setting*/
		ret = inet_pton(AF_INET, gw_addr, &(addr.sin_addr));
		if(0 == ret){
			NET_ERR("%s: the input gw_addr is Invalided\n", __func__);
			ret = ERR_WRONGVALS;
			goto ERROR0;
		}else if(-1 == ret){
			NET_ERR("%s: change the gw_addr to sockaddr_in failed\n", __func__);
			ret = ERR_WRONGVALS;
			goto ERROR0;
		}
	}else{
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	memcpy(&(route.rt_gateway), &addr, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCDELRT, &route) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int add_route_rule_def_gw(char *def_gw, char *dev_name)
{
	int sockfd;
	int ret = SUCCESS;
	struct rtentry route;
	struct sockaddr_in addr;

	if(!def_gw){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&route, 0, sizeof(struct rtentry));

	/*dst address setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memcpy(&(route.rt_dst), &addr, sizeof(struct sockaddr_in));

	/*net mask setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memcpy(&(route.rt_genmask), &addr, sizeof(struct sockaddr_in));

	/*device setting*/
	if(dev_name){
		route.rt_dev = dev_name;
	}

	route.rt_flags = RTF_UP | RTF_GATEWAY;

	/*gw address setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, def_gw, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input def_gw is Invalided\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the def_gw to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(route.rt_gateway), &addr, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCADDRT, &route) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int del_route_rule_def_gw(char *def_gw, char *dev_name)
{
	int sockfd;
	int ret = SUCCESS;
	struct rtentry route;
	struct sockaddr_in addr;

	if(!def_gw){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&route, 0, sizeof(struct rtentry));

	/*dst address setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memcpy(&(route.rt_dst), &addr, sizeof(struct sockaddr_in));

	/*net mask setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memcpy(&(route.rt_genmask), &addr, sizeof(struct sockaddr_in));

	/*dev_name settings*/
	if(dev_name)
		route.rt_dev = dev_name;

	route.rt_flags = RTF_UP | RTF_GATEWAY;

	/*gw address setting*/
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, def_gw, &(addr.sin_addr));
	if(0 == ret){
		NET_ERR("%s: the input def_gw is Invalided\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}else if(-1 == ret){
		NET_ERR("%s: change the def_gw to sockaddr_in failed\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}
	memcpy(&(route.rt_gateway), &addr, sizeof(struct sockaddr_in));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCADDRT, &route) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

/**
 ***********************
 *The APIs of ethtool  *
 ***********************
*/
int get_ethtool_ecmd(char *dev_name, uint32_t *speed, uint8_t *duplex, uint8_t *autoneg,
				uint32_t *supported, uint32_t *advertising)
{
	int sockfd;
	struct ethtool_cmd ecmd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !speed || !duplex || !autoneg || !supported || !advertising){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&ecmd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCETHTOOL, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}

	*speed = ethtool_cmd_speed(&ecmd);
	*duplex = ecmd.duplex;
	*autoneg = ecmd.autoneg;
	*supported = ecmd.supported;
	*advertising = ecmd.advertising;

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}


int set_ethtool_ecmd(char *dev_name, uint32_t speed, uint8_t duplex, uint8_t autoneg)
{
	int sockfd;
	struct ethtool_cmd ecmd;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t)&ecmd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		NET_ERR("%s: create sockfd failed. errno: %s", __func__, strerror(errno));
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	if(ioctl(sockfd, SIOCETHTOOL, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}else{
		ethtool_cmd_speed_set(&ecmd, speed);
		ecmd.duplex = duplex;
		ecmd.autoneg = autoneg;
		if(ecmd.autoneg && ecmd.advertising == 0){
			NET_DBG("Cannot advertise");
			NET_DBG("speed  %d\n", speed);
			NET_DBG("duplex %s\n", duplex? "full" : "half");
			ret = ERR_WRONGVALS;
			goto ERROR1;
		}
		ecmd.cmd = ETHTOOL_SSET;
		ifr.ifr_data = (caddr_t)&ecmd;

		if(ioctl(sockfd, SIOCETHTOOL, &ifr) < 0){
			NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
			ret = ERR_IOCTL;
			goto ERROR1;
		}
	}
ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

int get_ethtool_link(char *dev_name, uint32_t *eth_data)
{
	int sockfd;
	struct ethtool_value edata;
	struct ifreq ifr;
	int ret = SUCCESS;

	if(!dev_name || !eth_data){
		NET_DBG("%s:Invalided input parameter\n", __func__);
		ret = ERR_WRONGVALS;
		goto ERROR0;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, dev_name, IFNAMSIZ-1);
	edata.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (caddr_t)&edata;

	if(ioctl(sockfd, SIOCETHTOOL, &ifr) < 0){
		NET_ERR("%s: ioctl error errno:%s\n", __func__, strerror(errno));
		ret = ERR_IOCTL;
		goto ERROR1;
	}
	*eth_data = edata.data;

ERROR1:
	close(sockfd);
ERROR0:
	return ret;
}

/**
 *****************************************
 *The APIs of to monitor network status *
 *****************************************
*/
void parse_rtattr(struct rtattr **tb, int max, struct rtattr *attr, int len)
{
	for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
		if (attr->rta_type <= max) {
			tb[attr->rta_type] = attr;
		}
	}
}

void print_ifinfomsg(struct nlmsghdr *nh)
{
	int len;
	struct rtattr *tb[IFLA_MAX + 1];
	struct ifinfomsg *ifinfo;

	memset(tb, 0, sizeof(tb));
	ifinfo = NLMSG_DATA(nh);
	len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));
	parse_rtattr(tb, IFLA_MAX, IFLA_RTA (ifinfo), len);
	NET_DBG("%s: %s ", (nh->nlmsg_type==RTM_NEWLINK)?"NEWLINK":"DELLINK", (ifinfo->ifi_flags & IFF_RUNNING) ? "up" : "down");
	if(tb[IFLA_IFNAME]) {
		NET_ERR("%s", (char *)RTA_DATA(tb[IFLA_IFNAME]));
	}
	NET_DBG("\n");
}

void print_ifaddrmsg(struct nlmsghdr *nh)
{
	int len;
	struct rtattr *tb[IFA_MAX + 1];
	struct ifaddrmsg *ifaddr;
	char tmp[256];

	memset(tb, 0, sizeof(tb));
	ifaddr = NLMSG_DATA(nh);
	len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*ifaddr));
	parse_rtattr(tb, IFA_MAX, IFA_RTA (ifaddr), len);

	NET_DBG("%s ", (nh->nlmsg_type==RTM_NEWADDR)?"NEWADDR":"DELADDR");
	if (tb[IFA_LABEL] != NULL) {
		NET_DBG("%s ", (char *)RTA_DATA(tb[IFA_LABEL]));
	}
	if (tb[IFA_ADDRESS] != NULL) {
		inet_ntop(ifaddr->ifa_family, RTA_DATA(tb[IFA_ADDRESS]), tmp, sizeof(tmp));
		NET_DBG("%s ", tmp);
	}
	NET_DBG("\n");
}

void print_rtmsg(struct nlmsghdr *nh)
{
	int len;
	struct rtattr *tb[RTA_MAX + 1];
	struct rtmsg *rt;
	char tmp[256];

	memset(tb, 0, sizeof(tb));
	rt = NLMSG_DATA(nh);
	len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*rt));
	parse_rtattr(tb, RTA_MAX, RTM_RTA(rt), len);
	NET_DBG("%s: ", (nh->nlmsg_type==RTM_NEWROUTE)?"NEWROUT":"DELROUT");
	if (tb[RTA_DST] != NULL) {
		inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_DST]), tmp, sizeof(tmp));
		NET_DBG("RTA_DST %s ", tmp);
	}
	if (tb[RTA_SRC] != NULL) {
		inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_SRC]), tmp, sizeof(tmp));
		NET_DBG("RTA_SRC %s ", tmp);
	}
	if (tb[RTA_GATEWAY] != NULL) {
		inet_ntop(rt->rtm_family, RTA_DATA(tb[RTA_GATEWAY]), tmp, sizeof(tmp));
		NET_DBG("RTA_GATEWAY %s ", tmp);
	}

	NET_DBG("\n");
}

int monitor_network_status(ifinfomsg_fp ifinfomsg_cb, ifaddrmsg_fp ifaddrmsg_cb, rtmsg_fp rtmsg_cb, void *p_ctx)
{
	int socketfd;
	struct sockaddr_nl sa;
	struct timeval timeout;
	fd_set rd_set;
	int select_r;
	unsigned int read_r;
	struct nlmsghdr *nh;
	char buffer[2048] = {0};
	int ret = SUCCESS;

	socketfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if(socketfd < 0){
		NET_ERR("%s: create socketfd failed\n", __func__);
		ret = ERR_UNKNOWN;
		goto ERROR0;
	}

	memset(&sa, 0, sizeof(struct sockaddr_nl));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE;
	ret = bind(socketfd, (struct sockaddr *)&sa, sizeof(struct sockaddr));
	if(ret == -1 ){
		NET_ERR("%s: bind socketfd failed\n", __func__);
		ret = ERR_UNKNOWN;
		goto ERROR1;
	}

	while(true){
		FD_ZERO(&rd_set);
		FD_SET(socketfd, &rd_set);
		timeout.tv_sec = 15;
		timeout.tv_usec = 0;
		select_r = select(socketfd + 1, &rd_set, NULL, NULL, &timeout);
		if (select_r < 0) {
			NET_ERR("%s: select error\n", __func__);
		} else if (select_r > 0) {
		if (FD_ISSET(socketfd, &rd_set)) {
			read_r = read(socketfd, buffer, 2048);
			for (nh = (struct nlmsghdr *)buffer; NLMSG_OK(nh, read_r); nh = NLMSG_NEXT(nh, read_r)){
				switch (nh->nlmsg_type){
						case NLMSG_DONE:
						case NLMSG_ERROR:
							break;
						case RTM_NEWLINK:
						case RTM_DELLINK:
							ifinfomsg_cb(nh, p_ctx);
							break;
						case RTM_NEWADDR:
						case RTM_DELADDR:
							ifaddrmsg_cb(nh, p_ctx);
							break;
						case RTM_NEWROUTE:
						case RTM_DELROUTE:
							rtmsg_cb(nh, p_ctx);
							break;
						default:
							NET_WAR("nh->nlmsg_type = %d\n", nh->nlmsg_type);
							break;
					}
				}
			}
		}
	}

ERROR1:
	close(socketfd);
ERROR0:

	return ret;
}