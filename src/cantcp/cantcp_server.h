#ifndef _CANTCP_SERVER_H_
#define _CANTCP_SERVER_H_

#include <kernel.h>

#include "cantcp.h"

/**
 * @brief Broadcast a message to all connected clients
 * 
 * @param msg 
 * @return int 
 */
int cantcp_server_broadcast(struct zcan_frame *msg);

/**
 * @brief Attach msgq to receive all clients messages
 * 
 * @param msgq 
 * @return int 
 */
int cantcp_server_attach_rx_msgq(struct k_msgq *msgq);

#endif /* _CANTCP_SERVER_H_ */