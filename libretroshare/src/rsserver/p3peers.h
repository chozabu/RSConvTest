#ifndef RETROSHARE_P3_PEER_INTERFACE_H
#define RETROSHARE_P3_PEER_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3peers.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#include "retroshare/rspeers.h"
class p3LinkMgr;
class p3PeerMgr;
class p3NetMgr;


class p3Peers: public RsPeers 
{
	public:

        p3Peers(p3LinkMgr *lm, p3PeerMgr *pm, p3NetMgr *nm);
virtual ~p3Peers() { return; }

	/* Updates ... */
virtual bool FriendsChanged();
virtual bool OthersChanged();

	/* Peer Details (Net & Auth) */
virtual const RsPeerId& getOwnId();

		virtual bool   haveSecretKey(const PGPIdType& gpg_id) ;


virtual bool	getOnlineList(std::list<RsPeerId> &ids);
virtual bool	getFriendList(std::list<RsPeerId> &ids);
//virtual bool	getOthersList(std::list<std::string> &ids);
virtual bool    getPeerCount (unsigned int *friendCount, unsigned int *onlineCount, bool ssl);

virtual bool    isOnline(const RsPeerId &id);
virtual bool    isFriend(const RsPeerId &id);
virtual bool    isGPGAccepted(const PGPIdType &gpg_id_is_friend); //
virtual std::string getGPGName(const PGPIdType &gpg_id);
virtual std::string getPeerName(const RsPeerId& ssl_or_gpg_id);
virtual bool	getPeerDetails(const RsPeerId& ssl_or_gpg_id, RsPeerDetails &d);

                /* Using PGP Ids */
virtual const PGPIdType& getGPGOwnId();
virtual PGPIdType getGPGId(const RsPeerId &ssl_id);
virtual bool    isKeySupported(const PGPIdType& ids);
virtual bool    getGPGAcceptedList(std::list<PGPIdType> &ids);
virtual bool    getGPGSignedList(std::list<PGPIdType> &ids);
virtual bool    getGPGValidList(std::list<PGPIdType> &ids);
virtual bool    getGPGAllList(std::list<PGPIdType> &ids);
virtual bool	getGPGDetails(const PGPIdType &id, RsPeerDetails &d);
virtual bool	getAssociatedSSLIds(const PGPIdType& gpg_id, std::list<RsPeerId> &ids);
virtual bool    gpgSignData(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen) ;

	/* Add/Remove Friends */
virtual	bool addFriend(const RsPeerId &ssl_id, const PGPIdType &gpg_id,ServicePermissionFlags flags = RS_SERVICE_PERM_ALL);
virtual	bool removeFriend(const PGPIdType& gpgid);
virtual	bool removeFriend(const RsPeerId& sslid);
virtual bool removeFriendLocation(const RsPeerId& sslId);

		/* keyring management */
		virtual bool removeKeysFromPGPKeyring(const std::list<PGPIdType>& pgp_ids,std::string& backup_file,uint32_t& error_code);

	/* Network Stuff */
virtual	bool connectAttempt(const RsPeerId &id);
virtual bool setLocation(const RsPeerId &ssl_id, const std::string &location);//location is shown in the gui to differentiate ssl certs
virtual bool setHiddenNode(const RsPeerId &id, const std::string &hidden_node_address);
virtual bool setHiddenNode(const RsPeerId &id, const std::string &address, uint16_t port);

virtual	bool setLocalAddress(const RsPeerId &id, const std::string &addr, uint16_t port);
virtual	bool setExtAddress(const RsPeerId &id, const std::string &addr, uint16_t port);
virtual	bool setDynDNS(const RsPeerId &id, const std::string &dyndns);
virtual	bool setNetworkMode(const RsPeerId &id, uint32_t netMode);
virtual bool setVisState(const RsPeerId &id, uint16_t vs_disc, uint16_t vs_dht);

virtual bool getProxyServer(std::string &addr, uint16_t &port);
virtual bool setProxyServer(const std::string &addr, const uint16_t port);

virtual void getIPServersList(std::list<std::string>& ip_servers) ;
virtual void allowServerIPDetermination(bool) ;
virtual bool getAllowServerIPDetermination() ;

	/* Auth Stuff */
// Get the invitation (GPG cert + local/ext address + SSL id for the given peer)
virtual	std::string GetRetroshareInvite(const RsPeerId& ssl_id,bool include_signatures);
// same but for own id
virtual	std::string GetRetroshareInvite(bool include_signatures);
virtual bool GetPGPBase64StringAndCheckSum(const PGPIdType& gpg_id,std::string& gpg_base64_string,std::string& gpg_base64_checksum) ;

virtual bool hasExportMinimal() ;

virtual	bool loadCertificateFromString(const std::string& cert, RsPeerId& ssl_id,PGPIdType& pgp_id, std::string& error_string);
virtual	bool loadDetailsFromStringCert(const std::string &cert, RsPeerDetails &pd, uint32_t& error_code);

virtual	bool cleanCertificate(const std::string &certstr, std::string &cleanCert,int& error_code);
virtual	bool saveCertificateToFile(const RsPeerId &id, const std::string &fname);
virtual	std::string saveCertificateToString(const RsPeerId &id);

virtual	bool signGPGCertificate(const PGPIdType &id);
virtual	bool trustGPGCertificate(const PGPIdType &id, uint32_t trustlvl);

	/* Group Stuff */
virtual bool addGroup(RsGroupInfo &groupInfo);
virtual bool editGroup(const std::string &groupId, RsGroupInfo &groupInfo);
virtual bool removeGroup(const std::string &groupId);
virtual bool getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo);
virtual bool getGroupInfoList(std::list<RsGroupInfo> &groupInfoList);
virtual bool assignPeerToGroup(const std::string &groupId, const PGPIdType &peerId, bool assign);
virtual bool assignPeersToGroup(const std::string &groupId, const std::list<PGPIdType>& peerIds, bool assign);

virtual FileSearchFlags computePeerPermissionFlags(const RsPeerId& peer_id,FileStorageFlags share_flags,const std::list<std::string>& parent_groups) ;

// service permission stuff

	virtual ServicePermissionFlags servicePermissionFlags(const PGPIdType& gpg_id) ;
	virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId & ssl_id) ;
	virtual void setServicePermissionFlags(const PGPIdType& gpg_id,const ServicePermissionFlags& flags) ;

	private:

	p3LinkMgr *mLinkMgr;
	p3PeerMgr *mPeerMgr;
	p3NetMgr *mNetMgr;
	
};

#endif
