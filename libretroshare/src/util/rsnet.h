/*
 * libretroshare/src/util: rsnet.h
 *
 * Universal Networking Header for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef RS_UNIVERSAL_NETWORK_HEADER
#define RS_UNIVERSAL_NETWORK_HEADER

#include <inttypes.h>

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <errno.h>

#else

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h> /* for ssize_t */
//typedef uint32_t socklen_t;
typedef uint32_t in_addr_t;

int inet_aton(const char *name, struct in_addr *addr);

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

/* 64 bit conversions */
uint64_t ntohll(uint64_t x);
uint64_t htonll(uint64_t x);

/* blank a network address */
void sockaddr_clear(struct sockaddr_in *addr);

/* determine network type (moved from pqi/pqinetwork.cc) */
bool isValidNet(struct in_addr *addr);
bool isLoopbackNet(struct in_addr *addr);
bool isPrivateNet(struct in_addr *addr);
bool isExternalNet(struct in_addr *addr);


#endif /* RS_UNIVERSAL_NETWORK_HEADER */
