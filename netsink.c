/*
 * Copyright 2013-2019 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>


#include "srslte/phy/io/netsink.h"

int srslte_netsink_init(srslte_netsink_t *q, const char *address, uint16_t port, srslte_netsink_type_t type) {
  bzero(q, sizeof(srslte_netsink_t));

  q->sockfd=socket(AF_INET, type==SRSLTE_NETSINK_TCP?SOCK_STREAM:SOCK_DGRAM,0);  
  if (q->sockfd < 0) {
    perror("socket");
    return -1; 
  }

  int enable = 1;
#if defined (SO_REUSEADDR)
  if (setsockopt(q->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
      perror("setsockopt(SO_REUSEADDR) failed");
#endif
#if defined (SO_REUSEPORT)
  if (setsockopt(q->sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
      perror("setsockopt(SO_REUSEPORT) failed");
#endif

  q->servaddr.sin_family = AF_INET;
  q->servaddr.sin_addr.s_addr=inet_addr(address);
  q->servaddr.sin_port=htons(port);
  q->connected = false; 
  q->type = type; 
  
  return 0;
}

void srslte_netsink_free(srslte_netsink_t *q) {
  if (q->sockfd) {
    close(q->sockfd);
  }
  bzero(q, sizeof(srslte_netsink_t));
}

int srslte_netsink_set_nonblocking(srslte_netsink_t *q) {
  if (fcntl(q->sockfd, F_SETFL, O_NONBLOCK)) {
    perror("fcntl");
    return -1; 
  }
  return 0; 
}

int srslte_netsink_write(srslte_netsink_t *q, void *buffer, int nof_bytes) {
  if (!q->connected) {
    if (connect(q->sockfd,&q->servaddr,sizeof(q->servaddr)) < 0) {
      if (errno == ECONNREFUSED || errno == EINPROGRESS) {
        return 0; 
      } else {
        perror("connect");
        exit(-1);
        return -1;        
      }
    } else {
      q->connected = true; 
    }
  } 

  int n = 0; 
  if (q->connected) {
    n = write(q->sockfd, buffer, nof_bytes);  
    if (n < 0) {
      if (errno == ECONNRESET) {
        close(q->sockfd);
        q->sockfd=socket(AF_INET, q->type==SRSLTE_NETSINK_TCP?SOCK_STREAM:SOCK_DGRAM,0);  
        if (q->sockfd < 0) {
          perror("socket");
          return -1; 
        }
        q->connected = false; 
        return 0; 
      }
    }    
  } 
  return n;
}

int srslte_netsink_write_pcap(MAC_Context_Info_t *capInfo,srslte_netsink_t *q, void *buffer, int nof_bytes) {
  if (!q->connected) {
    if (connect(q->sockfd,&q->servaddr,sizeof(q->servaddr)) < 0) {
      if (errno == ECONNREFUSED || errno == EINPROGRESS) {
        return 0; 
      } else {
        perror("connect");
        exit(-1);
        return -1;        
      }
    } else {
      q->connected = true; 
    }
  } 

    pcaprec_hdr_t packet_header;
    char context_header[16000];
    int offset = 0;
    uint16_t tmp16;
    /*****************************************************************/
    /* Context information (same as written by UDP heuristic clients */
	memcpy(context_header, MAC_LTE_START_STRING,
           strlen(MAC_LTE_START_STRING));
    offset += strlen(MAC_LTE_START_STRING);
    context_header[offset++] = capInfo->radioType;
    context_header[offset++] = capInfo->direction;
    context_header[offset++] = capInfo->rntiType;

    /* RNTI */
    context_header[offset++] = MAC_LTE_RNTI_TAG;
    tmp16 = htons(capInfo->rnti);
    memcpy(context_header+offset, &tmp16, 2);
    offset += 2;

    /* UEId */
    context_header[offset++] = MAC_LTE_UEID_TAG;
    tmp16 = htons(capInfo->ueid);
    memcpy(context_header+offset, &tmp16, 2);
    offset += 2;

    /* Subframe Number and System Frame Number */
    /* SFN is stored in 12 MSB and SF in 4 LSB */
    context_header[offset++] = MAC_LTE_FRAME_SUBFRAME_TAG;
    tmp16 = (capInfo->sysFrameNumber << 4) | capInfo->subFrameNumber;
    tmp16 = htons(tmp16);
    memcpy(context_header+offset, &tmp16, 2);
    offset += 2;

    /* CRC Status */
    context_header[offset++] = MAC_LTE_CRC_STATUS_TAG;
    context_header[offset++] = capInfo->crcStatusOK;

    /* Data tag immediately preceding PDU */
    context_header[offset++] = MAC_LTE_PAYLOAD_TAG;

  int n = 0; 
  if (q->connected) {
    //n = write(q->sockfd, buffer, nof_bytes); 
    memcpy(&context_header[offset],buffer,nof_bytes);
	n = write(q->sockfd, context_header, nof_bytes + offset);  
    if (n < 0) {
      if (errno == ECONNRESET) {
        close(q->sockfd);
        q->sockfd=socket(AF_INET, q->type==SRSLTE_NETSINK_TCP?SOCK_STREAM:SOCK_DGRAM,0);  
        if (q->sockfd < 0) {
          perror("socket");
          return -1; 
        }
        q->connected = false; 
        return 0; 
      }
    }    
  } 
  return n;
}


