# Devices & Events model

A physical device can be described by the following attributes:
- Type
- Medium
- MAC Address

A device should at least have (one of):
- A unique type
- A unique MAC address within a medium

## Glossary

- `ha`: Home Automation
- `dev`: Device
- `ev`: Event
- `ep`: Endpoint
- `ds`: Dataset
- `dt`: Datatype
- `subs`: Subscription

## TODO

### Generic device API

Add generic API for new devices types:

Singly linked list of device types with their specific MAC functions:

```c
struct ha_dev_mac_api {
	const char *type_str;
	const char *medium_str;
    ha_dev_type_t type;
	addr_cmp_func_t cmp;
	addr_str_func_t str;
};
```

### Lookup table feature

- Garantee minimum interval between notifications for a given device/endpoint pair
- Or notify only an event every N events.

```c
struct ha_ev_subs_lookup_table_entry
{
	sys_dnode_t _handle;

	ha_dev_addr_t device_mac;
	ha_endpoint_id_t endpoint_id;
};

typedef struct ha_ev_subs
{
	sys_dnode_t _handle;

	/* Queue of events to be notified to the waiter */
	struct k_fifo _evq;
	
	/* and more ... */
}


#if HA_DEV_SUBS_FILTER_LOOKUP_TABLE_ENABLED
	/* Reference device/endpoint pairs which have been previously 
	 * notified. This can be used to garantee minimum interval between
	 * notifications for a given device/endpoint pair. Or to filter out 
	 * duplicate, if we are only interested in the first event.
	 * 
	 * Note: Will serve future use cases.
	 */
	sys_dlist_t _lookup_table;
#endif
```