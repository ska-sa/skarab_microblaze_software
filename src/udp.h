#ifndef _UDP_H_
#define _UDP_H_

#define UDP_FRAME_BASE              (IP_FRAME_BASE + IP_FRAME_TOTAL_LEN) //34 
#define UDP_SRC_PORT_OFFSET         0
#define UDP_SRC_PORT_LEN            2
#define UDP_DST_PORT_OFFSET         (UDP_SRC_PORT_OFFSET + UDP_SRC_PORT_LEN) //2
#define UDP_DST_PORT_LEN            2
#define UDP_ULEN_OFFSET             (UDP_DST_PORT_OFFSET + UDP_DST_PORT_LEN) //4
#define UDP_ULEN_LEN                2
#define UDP_CHKSM_OFFSET            (UDP_ULEN_OFFSET + UDP_ULEN_LEN) //6
#define UDP_CHKSM_LEN               2
#define UDP_PAYLOAD_OFFSET          (UDP_CHKSM_OFFSET + UDP_CHKSM_LEN)

#define UDP_HEADER_TOTAL_LEN        (UDP_CHKSM_OFFSET + UDP_CHKSM_LEN)  /* udp length = 8 */

#endif
