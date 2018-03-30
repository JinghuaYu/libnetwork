#ifndef __libnetwork_h__
#define __libnetwork_h__

#include <net/route.h>

#define NET_DEV_NAME          "/proc/net/dev"
#define ROUTE_TABLE_DEV_NAME  "/proc/net/route"
#define DEVICE_NAME_LEN_MAX   16
#define IF_DEV_NUM_MAX     	  128
#define ROUTE_TABLE_NUM_MAX   128

#ifndef SUCCESS
#define SUCCESS             (0)
#endif

#ifndef ERR_UNKNOWN
#define ERR_UNKNOWN         (-1)  /* Unknown error */
#endif

#ifndef ERR_WRONGVALS
#define ERR_WRONGVALS       (-2)  /* Wrong input arguments or return values */
#endif

#ifndef ERR_NOMEM
#define ERR_NOMEM           (-3)  /* Out of memory */
#endif

#ifndef ERR_WRONGSTATE
#define ERR_WRONGSTATE      (-4)  /* Wrong running state */
#endif

#ifndef ERR_EXIST
#define ERR_EXIST           (-5)  /* Target already exists */
#endif

#ifndef ERR_NOTEXIST
#define ERR_NOTEXIST        (-6)  /* Target not exist yet */
#endif

#ifndef ERR_NOBUFS
#define ERR_NOBUFS          (-7)  /* No buffer space left */
#endif

#ifndef ERR_NODATA
#define ERR_NODATA          (-8)  /* No data available or the buffer is empty */
#endif

#ifndef ERR_NOTIMPL
#define ERR_NOTIMPL         (-9)  /* Function not implemented yet */
#endif

#ifndef ERR_TIMEDOUT
#define ERR_TIMEDOUT        (-10) /* Waiting resource timed out */
#endif

#ifndef ERR_NOSWRSRC
#define ERR_NOSWRSRC        (-11) /* No software resource */
#endif

#ifndef ERR_NOHWRSRC
#define ERR_NOHWRSRC        (-12) /* No hardware resource */
#endif

#ifndef ERR_IOCTL
#define ERR_IOCTL          (-13) /*ioctl error*/
#endif


/*the struct to store the device name*/
typedef struct {
    char dev_name[DEVICE_NAME_LEN_MAX];
    bool used;
} ifr_name_t;

/*the struct to store the route table*/
typedef struct {
    char dev_name[DEVICE_NAME_LEN_MAX];
    bool used;
    struct rtentry route;
} libnet_rtentry_t;

typedef int (*ifinfomsg_fp)(struct nlmsghdr *, void *);
typedef int (*ifaddrmsg_fp)(struct nlmsghdr *, void *);
typedef int (*rtmsg_fp)(struct nlmsghdr *, void*);

/**
 ************************
 * The APIs of ifconfig  *
 ************************
*/
/**
 *get the net device name from the /proc/net/dev
 *@param ifr_names -out- the buffer to store the device name.
 *@param input_len -in- the buffer size of ifr_names should be MAX_IF_DEV_NUM * sizeof(ifr_name_t)
 *@param dev_num -out- the numbers of get device number
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_if_dev_name(ifr_name_t *ifr_names, unsigned int input_len, unsigned int *dev_nums);

/**
 *get the active ifreqs
 *@param ifrs -out- the buffer store the ifreqs
 *@param input_len -in- the buffer size of ifr_names should be MAX_IF_DEV_NUM * sizeof(struct ifreq)
 *@param ifreq_nums -out- the num of the active ifreqs
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_ifconfig(struct ifreq *ifrs, unsigned int input_len, unsigned int *ifreq_nums);
/**
 *get the ip address of specific device by name
 *@param dev_name -in- the device name point
 *@param address -out- the buffer to store ip address like "xxx.xxx.xxx.xxx"
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_if_ip_addr(char *dev_name, char *ip_address);

/**
 *set the ip address of specific device by name
 *@param dev_name -in- the device name point
 *@param address -in- the buffer to store ip address like "xxx.xxx.xxx.xxx"
 *@return SUCCESS - set successfully
 *@return ERR_xxx - failed to set
*/
int set_if_ip_addr(char *dev_name, char *ip_address);

/**
 *get the broadcast address of specific device by name
 *@param dev_name -in- the device name point
 *@param brd_address -out- the buffer to store broadcast address like "xxx.xxx.xxx.xxx"
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_if_brd_addr(char *dev_name, char *brd_address);

/**
 *set the broadcast address of specific device by name
 *@param dev_name -in- the device name point
 *@param brd_address -in- the buffer to store broadcast address like "xxx.xxx.xxx.xxx"
 *@return SUCCESS - set successfully
 *@return ERR_xxx - failed to set
*/
int set_if_brd_addr(char *dev_name, char *brd_address);

/**
 *get the netmask address of specific device by name
 *@param dev_name -in- the device name point
 *@param address -out- the buffer to store netmask address like "xxx.xxx.xxx.xxx"
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_if_netmask_addr(char *dev_name, char *netmask_address);

/**
 *set the netmask address of specific device by name
 *@param dev_name -in- the device name point
 *@param address -in- the buffer to store netmask address like "xxx.xxx.xxx.xxx"
 *@return SUCCESS - set successfully
 *@return ERR_xxx - failed to set
*/
int set_if_netmask_addr(char *dev_name, char *netmask_address);

/**
 *get the if flags of specific device by name
 *@param dev_name -in- the device name point
 *@param if_flags -out- the buffer to store if_flags
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_if_flags(char *dev_name, short int *if_flags);

/**
 *get the mac address of specific device by name
 *@param dev_name -in- the device name point
 *@param mac_addr -out- the buffer to store mac address like
 *@param input_len -in- the length of mac_addr
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_if_mac_addr(char *dev_name, unsigned char *mac_address, unsigned int input_len);
/**
 *set the mac address of specific device by name
 *@param dev_name -in- the device name point
 *@param mac_addr -in- the buffer to store mac address
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int set_if_mac_addr(char *dev_name, unsigned char *mac_address);
/**
 *get the mac address of specific device by name
 *@param dev_name -in- the device name point
 *@param if_mtu -out- the buffer to store if_mtu
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_if_mtu(char *dev_name, int *if_mtu);

/**
 *set the if_mtu of specific device by name
 *@param dev_name -in- the device name point
 *@param if_mtu -in- the buffer to store if_mtu
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int set_if_mtu(char *dev_name, int if_mtu);

/**
 *change the device name
 *@param old_dev_name -in- the old device name point
 *@param new_dev_name -in- the new device name point
 *@return SUCCESS - change successfully
 *@return ERR_xxx - failed to change
*/
int change_if_name(char *old_dev_name, char *new_dev_name);

/**
 *********************
 * The APIs of route  *
 *********************
*/
/**
 *get the route tables from the /proc/net/route
 *@param route_tables -in- the buffer to store the route tables
 *@param int_len -in- the buffer len of route_tables should be MAX_ROUTE_TABLE_NUM * sizeof(libnet_rtentry_t)
 *@param rt_table_nums -out- the buffer to store the route rule numbers
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_route_table(libnet_rtentry_t *route_tables, unsigned int input_len, unsigned int *rt_table_nums);

/**
 *add the net route rule
 *@param dst_type -in- the buffer to store the route rule type like "net" or "host"
 *@param dst_addr -in- the buffer to store the destination address like "xxx.xxx.xxx.xxx"
 *@param net_mask -in- the buffer to store the netmask address like "xxx.xxx.xxx.xxx"
 *@param gw_addr -in- the buffer to store the gateway address like "xxx.xxx.xxx.xxx" or NULL
 *@param dev_name -in- the buffer to store the device name
 *@return SUCCESS - add_route_rule_net successfully
 *@return ERR_xxx - failed to add_route_rule_net
*/
int add_route_rule(char *dst_type, char *dst_addr, char *net_mask, char *gw_addr, char *dev_name);

/**
 *del the net route rule
 *@param dst_type -in- the buffer to store the route rule type like "net" or "host"
 *@param dst_addr -in- the buffer to store the destination address like "xxx.xxx.xxx.xxx"
 *@param net_mask -in- the buffer to store the netmask address like "xxx.xxx.xxx.xxx"
 *@param gw_addr -in- the buffer to store the gateway address like "xxx.xxx.xxx.xxx" or NULL
 *@param dev_name -in- the buffer to store the device name
 *@return SUCCESS - del_route_rule_net successfully
 *@return ERR_xxx - failed to del_route_rule_net
*/
int del_route_rule(char *dst_type, char *dst_addr, char *net_mask, char *gw_addr, char *dev_name);

/**
 *add the defult gateway route rule
 *@param dst_addr -in- the buffer to store the default gateway address like "xxx.xxx.xxx.xxx"
 *@param dev_name -in- the buffer to store the device name, you can ignore it by set it as NULL
 *@return SUCCESS - add_route_rule_def_gw successfully
 *@return ERR_xxx - failed to add_route_rule_def_gw
*/
int add_route_rule_def_gw(char *def_gw, char *dev_name);

/**
 *del the defult gateway route rule
 *@param dst_addr -in- the buffer to store the default gateway address like "xxx.xxx.xxx.xxx"
 *@param dev_name -in- the buffer to store the device name, you can ignore it by set it as NULL
 *@return SUCCESS - del_route_rule_def_gw successfully
 *@return ERR_xxx - failed to del_route_rule_def_gw
*/
int del_route_rule_def_gw(char *def_gw, char *dev_name);

/**
 ***********************
 *The APIs of ethtool  *
 ***********************
*/

/**
 *get the ethtool cmd data
 *@param dev_name -in- the buffer to store the device name
 *@param speed -out- the buffer to store the speed
 *@param duplex -out- the buffer to store the duplex
 *@param autoneg -out- the buffer to store the autoneg
 *@param supported -out- the buffer to store the supported
 *@param advertising -out- the buffer to store the advertising
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_ethtool_ecmd(char *dev_name, uint32_t *speed, uint8_t *duplex, uint8_t *autoneg,
                uint32_t *supported, uint32_t *advertising);

/**
 *get the ethtool cmd data
 *@param dev_name -in- the buffer to store the device name
 *@param speed -in- the buffer to store the speed
 *@param duplex -in- the buffer to store the duplex
 *@param autoneg -in- the buffer to store the autoneg
 *@return SUCCESS - set successfully
 *@return ERR_xxx - failed to set
*/
int set_ethtool_ecmd(char *dev_name, uint32_t speed, uint8_t duplex, uint8_t autoneg);


/**
 *del the defult gateway route rule
 *@param dev_name -in- the buffer to store the device name
 *@param eth_data -out- the buffer to store the eth_data 0 means no connected; 1 means connected
 *@return SUCCESS - get successfully
 *@return ERR_xxx - failed to get
*/
int get_ethtool_link(char *dev_name, uint32_t *eth_data);


#endif /* #ifndef __error_codes_h__ */