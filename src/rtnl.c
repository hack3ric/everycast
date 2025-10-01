#include <linux/veth.h>

#include "nl.h"
#include "try.h"

int rtnl_create_veth_pair(int rtnl, const char* ifname, const char* peername) {
  struct nl_req* req = nl_req_alloca(1024);
  req->n.nlmsg_type = RTM_NEWLINK;
  req->n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
  req->i.ifi_family = AF_UNSPEC;

  nl_str(req, IFLA_IFNAME, ifname);
  nl_nest(req, IFLA_LINKINFO, {
    nl_str(req, IFLA_INFO_KIND, "veth");
    nl_nest(req, IFLA_INFO_DATA, {
      nl_nest(req, VETH_INFO_PEER, {
        struct ifinfomsg* peer_msg = RTA_DATA(it);
        req->n.nlmsg_len += sizeof(*peer_msg);
        peer_msg->ifi_family = AF_UNSPEC;
        peer_msg->ifi_change = 0xffffffff;
        nl_str(req, IFLA_IFNAME, peername);
      });
    });
  });

  return try(nl_send(rtnl, req));
}
